/*
**  Copyright 2003-2004  The Apache Software Foundation
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**      http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

#include "httpd.h"
#include "http_config.h"
#include "http_log.h"
#include "util_filter.h"
#include "apr_tables.h"
#include "apr_buckets.h"
#include "http_request.h"
#include "apr_strings.h"

#include "apreq.h"
#include "apreq_env.h"
#include "apreq_params.h"
#include "apreq_cookie.h"

#define dR  request_rec *r = (request_rec *)env

struct dir_config {
    const char         *temp_dir;
    apr_off_t           max_body;
    apr_ssize_t         max_brigade;
};


static void *apreq_create_dir_config(apr_pool_t *p, char *d)
{
    /* d == OR_ALL */
    struct dir_config *dc = apr_palloc(p, sizeof *dc);
    dc->temp_dir    = NULL;
    dc->max_body    = -1;
    dc->max_brigade = APREQ_MAX_BRIGADE_LEN;
    return dc;
}

static void *apreq_merge_dir_config(apr_pool_t *p, void *a_, void *b_)
{
    struct dir_config *a = a_, *b = b_, *c = apr_palloc(p, sizeof *c);
    c->temp_dir    = (b->temp_dir != NULL)    ? b->temp_dir    : a->temp_dir;
    c->max_body    = (b->max_body >= 0)       ? b->max_body    : a->max_body;
    c->max_brigade = (b->max_brigade >= 0)    ? b->max_brigade : a->max_brigade;
    return c;
}


/* The "warehouse", stored in r->request_config */
struct env_config {
    apreq_jar_t        *jar;    /* Active jar for the current request_rec */
    apreq_request_t    *req;    /* Active request for current request_rec */
    ap_filter_t        *f;      /* Active apreq filter for this request_rec */
    const char         *temp_dir; /* Temporary directory for spool files */
    apr_off_t           max_body; /* Maximum bytes the parser may see */
    apr_ssize_t         max_brigade; /* Maximum heap space for brigades */
};

/* Tracks the apreq filter state */
struct filter_ctx {
    request_rec        *r;     /* request that originally created this filter */
    apr_bucket_brigade *bb;    /* input brigade that's passed to the parser */
    apr_bucket_brigade *spool; /* copied prefetch data for downstream filters */
    apr_status_t        status;/* APR_SUCCESS, APR_INCOMPLETE, or parse error */
    unsigned            saw_eos;      /* Has EOS bucket appeared in filter? */
    apr_off_t           bytes_read;   /* Total bytes read into this filter. */
};

static const char filter_name[] = "APREQ";
module AP_MODULE_DECLARE_DATA apreq_module;

/**
 * @defgroup mod_apreq Apache 2.X Filter Module
 * @ingroup apreq_env
 * @brief mod_apreq - DSO that ties libapreq2 to Apache 2.X.
 *
 * mod_apreq provides the "APREQ" input filter for using libapreq2
 * (and allow its parsed data structures to be shared) within
 * the Apache 2.X webserver.  Using it, libapreq2 works properly
 * in every phase of the HTTP request, from translation handlers 
 * to output filters, and even for subrequests / internal redirects.
 *
 * <hr>
 *
 * <h2>Activating mod_apreq in Apache 2.X</h2>
 *
 * Normally the installation process triggered by
 * <code>% make install</code>
 * will make the necessary changes to httpd.conf for you. In any case,
 * after installing the mod_apreq.so module, be sure your webserver's
 * httpd.conf activates it on startup with a LoadModule directive, e.g.
 * @code
 *
 *     LoadModule    modules/mod_apreq.so
 *
 * @endcode
 *
 * The mod_apreq filter is named "APREQ", and may be used in Apache's
 * input filter directives, e.g.
 * @code
 *
 *     AddInputFilter APREQ         # or
 *     SetInputFilter APREQ
 *
 * @endcode
 *
 * However, this is not required because libapreq2 will add the filter (only)
 * if it's necessary.  You just need to ensure that your module instantiates
 * an apreq_request_t using apreq_request() <em>before the content handler
 * ultimately reads from the input filter chain</em>.  It is important to
 * recognize that no matter how the input filters are initially arranged,
 * the APREQ filter will always reposition itself to be the last input filter
 * to read the data.  In other words, <code>r->input_filters</code> will
 * always point to the active APREQ filter for the request.
 *
 * <h2>Server Configuration Directives</h2>
 *
 * <TABLE class="qref"><CAPTION>Per-directory commands for mod_apreq</CAPTION>
 * <TR><TH>Directive</TH><TH>Context</TH><TH>Default</TH><TH>Description</TH></TR>
 * <TR class="odd"><TD>APREQ_MaxBody</TD><TD>directory</TD><TD>-1 (Unlimited)</TD><TD>
 * Maximum number of bytes mod_apreq will send off to libapreq for parsing.  
 * mod_apreq will log this event and remove itself from the filter chain.
 * The APR_EGENERAL error will be reported to libapreq2 users via the return 
 * value of apreq_env_read().
 * </TD></TR>
 * <TR><TD>APREQ_MaxBrigade</TD><TD>directory</TD><TD> #APREQ_MAX_BRIGADE_LEN </TD><TD>
 * Maximum number of bytes apreq will allow to accumulate
 * within a brigade.  Excess data will be spooled to a
 * file bucket appended to the brigade.
 * </TD></TR>
 * <TR class="odd"><TD>APREQ_TempDir</TD><TD>directory</TD><TD>NULL</TD><TD>
 * Sets the location of the temporary directory apreq will use to spool
 * overflow brigade data (based on the APREQ_MaxBrigade setting).
 * If left unset, libapreq2 will select a platform-specific location via apr_temp_dir_get().
 * </TD></TR>
 * </TABLE>
 *
 * <h2>Implementation Details</h2>
 * <pre>
 * XXX apreq as a normal input filter
 * XXX apreq as a "virtual" content handler.
 * XXX apreq as a transparent "tee".
 * </pre>
 * @{
 */


#define APREQ_MODULE_NAME               "APACHE2"
#define APREQ_MODULE_MAGIC_NUMBER       20040802

static void apache2_log(const char *file, int line, int level, 
                        apr_status_t status, void *env, const char *fmt,
                        va_list vp)
{
    dR;
    ap_log_rerror(file, line, level, status, r, 
                  "%s", apr_pvsprintf(r->pool, fmt, vp));
}


static const char *apache2_query_string(void *env)
{
    dR;
    return r->args;
}

static apr_pool_t *apache2_pool(void *env)
{
    dR;
    return r->pool;
}

static const char *apache2_header_in(void *env, const char *name)
{
    dR;
    return apr_table_get(r->headers_in, name);
}

/*
 * r->headers_out ~> r->err_headers_out ?
 * @bug Sending a Set-Cookie header on a 304
 * requires err_headers_out table.
 */
static apr_status_t apache2_header_out(void *env, const char *name, 
                                       char *value)
{
    dR;
    apr_table_addn(r->headers_out, name, value);
    return APR_SUCCESS;
}


APR_INLINE
static struct env_config *get_cfg(request_rec *r)
{
    struct env_config *cfg = 
        ap_get_module_config(r->request_config, &apreq_module);
    if (cfg == NULL) {
        struct dir_config *d = ap_get_module_config(r->per_dir_config, 
                                                    &apreq_module);
        cfg = apr_pcalloc(r->pool, sizeof *cfg);
        ap_set_module_config(r->request_config, &apreq_module, cfg);

        if (d) {
            cfg->temp_dir    = d->temp_dir;
            cfg->max_body    = d->max_body;
            cfg->max_brigade = d->max_brigade;
        } 
        else {
            cfg->max_body    = -1;
            cfg->max_brigade = APREQ_MAX_BRIGADE_LEN;
        }
    }
    return cfg;
}

static apreq_jar_t *apache2_jar(void *env, apreq_jar_t *jar)
{
    dR;
    struct env_config *c = get_cfg(r);
    if (jar != NULL) {
        apreq_jar_t *old = c->jar;
        c->jar = jar;
        return old;
    }
    return c->jar;
}

APR_INLINE
static void apreq_filter_relocate(ap_filter_t *f)
{
    request_rec *r = f->r;
    if (f != r->input_filters) {
        ap_filter_t *top = r->input_filters;
        ap_remove_input_filter(f);
        r->input_filters = f;
        f->next = top;
    }
}

static ap_filter_t *get_apreq_filter(request_rec *r)
{
    struct env_config *cfg = get_cfg(r);

    if (cfg->f == r->input_filters)
       return cfg->f;

    if (strcmp(r->input_filters->frec->name, filter_name) == 0)
        return cfg->f = r->input_filters;

    cfg->f = ap_add_input_filter(filter_name, NULL, r, r->connection);

/* ap_add_input_filter does not guarantee cfg->f == r->input_filters,
 * so we reposition the new filter there as necessary.
 */

    apreq_filter_relocate(cfg->f); 
    return cfg->f;
}

static apreq_request_t *apache2_request(void *env, 
                                        apreq_request_t *req)
{
    dR;
    struct env_config *c = get_cfg(r);

    if (c->f == NULL)
        get_apreq_filter(r);

    if (req != NULL) {
        apreq_request_t *old = c->req;
        c->req = req;
        return old;
    }

    return c->req;
}

APR_INLINE
static void apreq_filter_make_context(ap_filter_t *f)
{
    request_rec *r = f->r;
    apr_bucket_alloc_t *alloc = apr_bucket_alloc_create(r->pool);
    struct filter_ctx *ctx = apr_palloc(r->pool, sizeof *ctx);
    struct env_config *cfg = get_cfg(r);

    f->ctx       = ctx;
    ctx->r       = r;
    ctx->bb      = apr_brigade_create(r->pool, alloc);
    ctx->spool   = apr_brigade_create(r->pool, alloc);
    ctx->status  = APR_INCOMPLETE;
    ctx->saw_eos = 0;
    ctx->bytes_read = 0;

    if (cfg->max_body >= 0) {
        const char *cl = apr_table_get(r->headers_in, "Content-Length");
        if (cl != NULL) {
            char *dummy;
            apr_int64_t content_length = apr_strtoi64(cl,&dummy,0);

            if (dummy == NULL || *dummy != 0) {
                apreq_log(APREQ_ERROR APR_EGENERAL, r, 
                      "Invalid Content-Length header (%s)", cl);
                ctx->status = APR_EGENERAL;
                apreq_request(r, NULL)->body_status = APR_EGENERAL;
            }
            else if (content_length > (apr_int64_t)cfg->max_body) {
                apreq_log(APREQ_ERROR APR_EGENERAL, r,
                          "Content-Length header (%s) exceeds configured "
                          "max_body limit (%" APR_OFF_T_FMT ")", 
                          cl, cfg->max_body);
                ctx->status = APR_EGENERAL;
                apreq_request(r, NULL)->body_status = APR_EGENERAL;
            }
        }
    }
}

/*
 * Reads data directly into the parser.
 */

static apr_status_t apache2_read(void *env, 
                                 apr_read_type_e block,
                                 apr_off_t bytes)
{
    dR;
    ap_filter_t *f = get_apreq_filter(r); /*ensures correct filter for prefetch */
    struct filter_ctx *ctx;
    apr_status_t s;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);
    ctx = f->ctx;
    if (ctx->status != APR_INCOMPLETE || bytes == 0)
        return ctx->status;

    apreq_log(APREQ_DEBUG 0, r, "prefetching %" APR_OFF_T_FMT " bytes", bytes);
    s = ap_get_brigade(f, NULL, AP_MODE_READBYTES, block, bytes);
    if (s != APR_SUCCESS)
        return s;
    return ctx->status;
}


static const char *apache2_temp_dir(void *env, const char *path)
{
    dR;
    struct env_config *c = get_cfg(r);

    if (path != NULL) {
        const char *rv = c->temp_dir;
        c->temp_dir = apr_pstrdup(r->pool, path);
        return rv;
    }
    if (c->temp_dir == NULL) {
        if (apr_temp_dir_get(&c->temp_dir, r->pool) != APR_SUCCESS)
            c->temp_dir = NULL;
    }

    return c->temp_dir;
}


static apr_off_t apache2_max_body(void *env, apr_off_t bytes)
{
    dR;
    struct env_config *c = get_cfg(r);

    if (bytes >= 0) {
        apr_off_t rv = c->max_body;
        c->max_body = bytes;
        return rv;
    }
    return c->max_body;
}


static apr_ssize_t apache2_max_brigade(void *env, apr_ssize_t bytes)
{
    dR;
    struct env_config *c = get_cfg(r);

    if (bytes >= 0) {
        apr_ssize_t rv = c->max_brigade;
        c->max_brigade = bytes;
        return rv;
    }
    return c->max_brigade;
}


/*
 * Situations to contend with:
 *
 * 1) Often the filter will be added by the content handler itself,
 *    so the apreq_filter_init hook will not be run.
 * 2) If an auth handler uses apreq, the apreq_filter will ensure
 *    it's part of the protocol filters.  apreq_filter_init does NOT need
 *    to notify the protocol filter that it must not continue parsing,
 *    the apreq filter can perform this check itself.  apreq_filter_init
 *    just needs to ensure cfg->f does not point at it.
 * 3) If req->proto_input_filters and req->input_filters are apreq
 *    filters, and req->input_filters->next == req->proto_input_filters,
 *    it is safe for apreq_filter to "steal" the proto filter's context 
 *    and subsequently drop it from the chain.
 */


/* Examines the input_filter chain and moves the apreq filter(s) around
 * before the filter chain is stacked by ap_get_brigade.
 */

static apr_status_t apreq_filter_init(ap_filter_t *f)
{
    request_rec *r = f->r;
    struct env_config *cfg = get_cfg(r);
    ap_filter_t *in;

    if (f != r->proto_input_filters) {
        if (f == r->input_filters) {
            cfg->f = f;
            return APR_SUCCESS;
        }
        for (in = r->input_filters; in != r->proto_input_filters; 
             in = in->next)
        {
            if (f == in) {
                if (strcmp(r->input_filters->frec->name, filter_name) == 0) {
                    /* this intermediate apreq filter is superfluous- remove it */
                    if (cfg->f == f)
                        cfg->f = r->input_filters;
                    ap_remove_input_filter(f);
                }
                else {
                    /* move to top and register it */
                    apreq_filter_relocate(f);
                    cfg->f = f;
                }
                return APR_SUCCESS;
            }
        }
    }

    /* else this is a protocol filter which may still be active.
     * if it is, we must deregister it now.
     */
    if (cfg->f == f)
        cfg->f = NULL;

    return APR_SUCCESS;
}

static apr_status_t apreq_filter(ap_filter_t *f,
                                 apr_bucket_brigade *bb,
                                 ap_input_mode_t mode,
                                 apr_read_type_e block,
                                 apr_off_t readbytes)
{
    request_rec *r = f->r;
    struct filter_ctx *ctx;
    struct env_config *cfg;
    apreq_request_t *req;
    apr_status_t rv;

    switch (mode) {
    case AP_MODE_READBYTES:
    case AP_MODE_EXHAUSTIVE:
        /* only the modes above are supported */
        break;
    default:
        return APR_ENOTIMPL;
    }

    cfg = get_cfg(r);
    req = cfg->req;

    if (f->ctx == NULL) {
        if (f == r->input_filters 
            && r->proto_input_filters == f->next
            && strcmp(f->next->frec->name, filter_name) == 0) 
        {
            /* Try to steal the context and parse data of the 
               upstream apreq filter. */
            ctx = f->next->ctx;

            switch (ctx->status) {
            case APR_SUCCESS:
            case APR_INCOMPLETE:
                break;
            default:
                /* bad filter state, don't steal anything */
                goto make_new_context;
            }

            if (ctx->r != r) {
                /* r is a new request (subrequest or internal redirect) */
                apreq_request_t *old_req;

                if (req != NULL) {
                    if (req->parser != NULL)
                        /* new parser on this request: cannot steal context */
                        goto make_new_context;
                }
                else {
                    req = apreq_request(r, NULL);
                }

                /* steal the parser output */
                old_req = apreq_request(ctx->r, NULL);
                req->parser = old_req->parser;
                req->body = old_req->body;
                req->body_status = old_req->body_status;
                ctx->r = r;
            }

            /* steal the filter context */
            f->ctx = f->next->ctx;
            r->proto_input_filters = f;
            ap_remove_input_filter(f->next);
            goto context_initialized;
        }

    make_new_context:
        apreq_filter_make_context(f);
        if (req != NULL && f == r->input_filters) {
            if (req->body_status != APR_EINIT) {
                req->body = NULL;
                req->parser = NULL;
                req->body_status = APR_EINIT;
            }
        }
    }

 context_initialized:
    ctx = f->ctx;

    if (cfg->f != f)
        ctx->status = APR_SUCCESS;

    if (bb != NULL) {

        if (!ctx->saw_eos) {
 
            if (ctx->status == APR_INCOMPLETE) {
                apr_off_t len;
                rv = ap_get_brigade(f->next, bb, mode, block, readbytes);
            
                if (rv != APR_SUCCESS) {
                    apreq_log(APREQ_ERROR rv, r, "ap_get_brigade failed");
                    return rv;
                }

                APREQ_BRIGADE_COPY(ctx->bb, bb);
                apr_brigade_length(bb, 1, &len);
                ctx->bytes_read += len;

                if (cfg->max_body >= 0 && ctx->bytes_read > cfg->max_body) {
                    ctx->status = APR_EGENERAL;
                    apreq_request(r, NULL)->body_status = APR_EGENERAL;
                    apreq_log(APREQ_ERROR ctx->status, r, "Bytes read (" APR_OFF_T_FMT
                              ") exceeds configured max_body limit (" APR_OFF_T_FMT ")",
                              ctx->bytes_read, cfg->max_body);
                }

            }
            if (APR_BUCKET_IS_EOS(APR_BRIGADE_LAST(ctx->bb)))
                ctx->saw_eos = 1;
        }

        if (!APR_BRIGADE_EMPTY(ctx->spool)) {
            APR_BRIGADE_PREPEND(bb, ctx->spool);
            if (mode == AP_MODE_READBYTES) {
                apr_bucket *e;
                rv = apr_brigade_partition(bb, readbytes, &e);
                if (rv != APR_SUCCESS && rv != APR_INCOMPLETE) {
                    apreq_log(APREQ_ERROR rv, r, "partition failed");
                    return rv;
                }
                if (APR_BUCKET_IS_EOS(e))
                    e = APR_BUCKET_NEXT(e);
                ctx->spool = apr_brigade_split(bb, e);
                APREQ_BRIGADE_SETASIDE(ctx->spool,r->pool);
            }
        }

        if (ctx->status != APR_INCOMPLETE) {
            if (APR_BRIGADE_EMPTY(ctx->spool)) {
                ap_filter_t *next = f->next;

                if (cfg->f != f) {
                    apreq_log(APREQ_DEBUG ctx->status, r,
                              "removing inactive filter (%d)",
                              r->input_filters == f);

                    ap_remove_input_filter(f);
                }
                if (APR_BRIGADE_EMPTY(bb))
                    return ap_get_brigade(next, bb, mode, block, readbytes);
            }
            return APR_SUCCESS;
        }
    }
    else if (!ctx->saw_eos) {
        /* bb == NULL, so this is a prefetch read! */
        apr_off_t total_read = 0;

        bb = apr_brigade_create(ctx->bb->p, ctx->bb->bucket_alloc);

        while (total_read < readbytes) {
            apr_off_t len;
            apr_bucket *last = APR_BRIGADE_LAST(ctx->spool);

            if (APR_BUCKET_IS_EOS(last)) {
                ctx->saw_eos = 1;
                break;
            }

            rv = ap_get_brigade(f->next, bb, mode, block, readbytes);
            if (rv != APR_SUCCESS) {
                apreq_log(APREQ_ERROR rv, r, "ap_get_brigade failed");
                return rv;
            }
            APREQ_BRIGADE_SETASIDE(bb, r->pool);
            APREQ_BRIGADE_COPY(ctx->bb, bb);

            apr_brigade_length(bb, 1, &len);
            total_read += len;
            apreq_brigade_concat(r, ctx->spool, bb);
        }

        ctx->bytes_read += total_read;

        if (cfg->max_body >= 0 && ctx->bytes_read > cfg->max_body) {
            ctx->status = APR_EGENERAL;
            apreq_request(r, NULL)->body_status = APR_EGENERAL;
            apreq_log(APREQ_ERROR ctx->status, r, "Bytes read (%" APR_OFF_T_FMT
                      ") exceeds configured max_body limit (%" APR_OFF_T_FMT ")",
                      ctx->bytes_read, cfg->max_body);
        }

        /* Adding "f" to the protocol filter chain ensures the 
         * spooled data is preserved across internal redirects.
         */
        if (f != r->proto_input_filters) {
            ap_filter_t *in;
            for (in = r->input_filters; in != r->proto_input_filters; 
                 in = in->next)
            {
                if (f == in) {
                    r->proto_input_filters = f;
                    break;
                }
            }
        }
    }
    else
        return APR_SUCCESS;

    if (ctx->status == APR_INCOMPLETE) {
        if (req == NULL)
            req = apreq_request(r, NULL);

        ctx->status = apreq_parse_request(req, ctx->bb);
        apr_brigade_cleanup(ctx->bb);
    }

    return APR_SUCCESS;
}

static APREQ_ENV_MODULE(apache2, APREQ_MODULE_NAME,
                        APREQ_MODULE_MAGIC_NUMBER);

static void register_hooks (apr_pool_t *p)
{
    const apreq_env_t *old_env;
    old_env = apreq_env_module(&apache2_module);
    ap_register_input_filter(filter_name, apreq_filter, apreq_filter_init,
                             AP_FTYPE_CONTENT_SET);
}


/* Configuration directives */


static const char *apreq_set_temp_dir(cmd_parms *cmd, void *data,
                                      const char *arg)
{
    struct dir_config *conf = data;
    const char *err = ap_check_cmd_context(cmd, NOT_IN_LIMIT);

    if (err != NULL)
        return err;

    conf->temp_dir = arg;
    return NULL;
}

static const char *apreq_set_max_body(cmd_parms *cmd, void *data,
                                      const char *arg)
{
    struct dir_config *conf = data;
    const char *err = ap_check_cmd_context(cmd, NOT_IN_LIMIT);

    if (err != NULL)
        return err;

    conf->max_body = apreq_atoi64f(arg);

    if (conf->max_body < 0)
        return "APREQ_MaxBody requires a non-negative integer.";

    return NULL;
}

static const char *apreq_set_max_brigade(cmd_parms *cmd, void *data,
                                          const char *arg)
{
    struct dir_config *conf = data;
    const char *err = ap_check_cmd_context(cmd, NOT_IN_LIMIT);

    if (err != NULL)
        return err;

    conf->max_brigade = apreq_atoi64f(arg);

    if (conf->max_brigade < 0)
        return "APREQ_MaxBrigade requires a non-negative integer.";

    return NULL;
}

static const command_rec apreq_cmds[] =
{
    AP_INIT_TAKE1("APREQ_TempDir", apreq_set_temp_dir, NULL, OR_ALL,
                  "Default location of temporary directory"),
    AP_INIT_TAKE1("APREQ_MaxBody", apreq_set_max_body, NULL, OR_ALL,
                  "Maximum amount of data that will be fed into a parser."),
    AP_INIT_TAKE1("APREQ_MaxBrigade", apreq_set_max_brigade, NULL, OR_ALL,
                  "Maximum in-memory bytes a brigade may use."),
    {NULL}
};

/** @} */

module AP_MODULE_DECLARE_DATA apreq_module =
{
	STANDARD20_MODULE_STUFF,
	apreq_create_dir_config,
	apreq_merge_dir_config,
	NULL,
	NULL,
	apreq_cmds,
	register_hooks,
};
