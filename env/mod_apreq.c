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


/** The warehouse. */
struct env_config {
    apreq_jar_t        *jar;
    apreq_request_t    *req;
    ap_filter_t        *f;
    const char         *temp_dir;
    apr_off_t           max_body;
    apr_ssize_t         max_brigade;
};

/** Tracks the filter state */
struct filter_ctx {
    apr_bucket_brigade *bb;
    apr_bucket_brigade *spool;
    apr_status_t        status;
    unsigned            saw_eos;
    apr_off_t           bytes_read;
};

static const char filter_name[] = "APREQ";
module AP_MODULE_DECLARE_DATA apreq_module;

/**
 * mod_apreq.c provides an input filter for using libapreq2
 * (and allow its parsed data structures to be shared) within
 * the Apache-2 webserver.  Using it, libapreq2 works properly
 * in every phase of the HTTP request, from translation handlers 
 * to output filters, and even for subrequests / internal redirects.
 *
 * After installing mod_apreq, be sure your webserver's
 * httpd.conf activates it on startup with a LoadModule directive:
 * <pre><code>
 *
 *     LoadModule modules/mod_apreq.so
 *
 * </code></pre>
 * Normally the installation process triggered by '% make install'
 * will make the necessary changes to httpd.conf for you.
 * 
 * XXX describe normal operation, effects of config settings, etc. 
 *
 * @defgroup mod_apreq Apache-2 Filter Module
 * @ingroup MODULES
 * @brief mod_apreq.c: Apache-2 filter module
 * @{
 */


#define APREQ_MODULE_NAME "APACHE2"
#define APREQ_MODULE_MAGIC_NUMBER 20040615


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
    ap_filter_t *f = r->input_filters;

    if (cfg->f != NULL)
        return cfg->f;

    for (;;) {
        if (strcmp(f->frec->name, filter_name) == 0)
            return cfg->f = f;
        if (f == r->proto_input_filters)
            break;
        f = f->next;
    }

    cfg->f = ap_add_input_filter(filter_name, NULL, r, r->connection);
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
                apreq_log(APREQ_ERROR APR_BADARG, r, 
                      "invalid Content-Length header (%s)", cl);
                ctx->status = APR_BADARG;
            }
            else if (content_length > (apr_int64_t)cfg->max_body) {
                apreq_log(APREQ_ERROR APR_EINIT, r,
                          "Content-Length header (%s) exceeds configured "
                          "max_body limit (%" APR_OFF_T_FMT ")", 
                          cl, cfg->max_body);
                ctx->status = APR_EINIT;
            }
        }
    }
}

/**
 * Reads data directly into the parser.
 */

static apr_status_t apache2_read(void *env, 
                                 apr_read_type_e block,
                                 apr_off_t bytes)
{
    dR;
    ap_filter_t *f = get_apreq_filter(r);
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

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    switch (mode) {
    case AP_MODE_READBYTES:
    case AP_MODE_EXHAUSTIVE:
        /* only the modes above are supported */
        break;
    default:
        return APR_ENOTIMPL;
    }

    ctx = f->ctx;
    cfg = get_cfg(r);
    req = cfg->req;

    if (f != r->input_filters) {
        ctx->status = APR_SUCCESS;
        if (cfg->f == f) {
            cfg->f = NULL;
            if (req) {
                req->parser = NULL;
                req->body = NULL;
            }
            get_apreq_filter(r);
        }
    }

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
                    ctx->status = APR_ENOSPC;
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
                apreq_log(APREQ_DEBUG ctx->status,r,"removing filter (%d)",
                          r->input_filters == f);
                ap_remove_input_filter(f);
            }
            return APR_SUCCESS;
        }
    }
    else if (!ctx->saw_eos) {
        ap_filter_t *in;
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
            ctx->status = APR_ENOSPC;
            apreq_log(APREQ_ERROR ctx->status, r, "Bytes read (%" APR_OFF_T_FMT
                      ") exceeds configured max_body limit (%" APR_OFF_T_FMT ")",
                      ctx->bytes_read, cfg->max_body);
        }

        /* Adding "f" to the protocol filter chain ensures the 
         * spooled data is preserved across internal redirects.
         */
        for (in = r->input_filters; in != r->proto_input_filters; 
             in = in->next)
        {
            if (f == in) {
                r->proto_input_filters = f;
                break;
            }
        }

    }
    else
        return APR_SUCCESS;

    if (ctx->status == APR_INCOMPLETE) {
        if (req == NULL)
            req = apreq_request(r, NULL);

        assert(req->env == r);
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
    ap_register_input_filter(filter_name, apreq_filter, NULL,
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
        return "ApReqMaxBody requires a non-negative integer.";

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
        return "ApReqMaxBody requires a non-negative integer.";

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
