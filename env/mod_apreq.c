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
 * XXX describe normal operation, effects of apreq_config_t settings, etc. 
 *
 * @defgroup mod_apreq Apache-2 Filter Module
 * @ingroup MODULES
 * @brief mod_apreq.c: Apache-2 filter module
 * @{
 */


#define APREQ_MODULE_NAME "APACHE2"
#define APREQ_MODULE_MAGIC_NUMBER 20040324


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
        cfg = apr_pcalloc(r->pool, sizeof *cfg);
        ap_set_module_config(r->request_config, &apreq_module, cfg);
        cfg->max_body = -1;
        cfg->max_brigade = APREQ_MAX_BRIGADE_LEN;
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
    ap_filter_t *f;
    if (cfg->f != NULL)
        return cfg->f;

    for (f  = r->input_filters; 
         f != r->proto_input_filters;
         f  = f->next)
    {
        if (strcmp(f->frec->name, filter_name) == 0)
            return cfg->f = f;
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
                          "max_body limit (" APR_OFF_T_FMT ")", 
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

    apreq_log(APREQ_DEBUG 0, r, "prefetching %ld bytes", bytes);
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


static apr_status_t apreq_filter_init(ap_filter_t *f)
{
    request_rec *r = f->r;
    struct filter_ctx *ctx;
    struct env_config *cfg = get_cfg(r);

    /* We must be inside config.c:ap_invoke_handler -> 
     * ap_invoke_filter_init (r->input_filters), and
     * we have to deal with the possibility that apreq may have
     * prefetched data prior to apache running the ap_invoke_filter_init
     * hook.
     */

    if (f->ctx) {
        ctx = f->ctx;

        /* We may have already prefetched some data.
         * If "f" is no longer at the top of the filter chain,
         * we need to add a new apreq filter to the top and start over.
         * XXX: How safe would the following (unimplemented) optimization 
         * be on redirects????: Leave the filter intact if
         * it's still at the end of the input filter chain (this would
         * imply that no other input filters were added after libapreq
         * started parsing).
         */

        if (!APR_BRIGADE_EMPTY(ctx->spool)) {
            apreq_request_t *req = cfg->req;

            /* Adding "f" to the protocol filter chain ensures the 
             * spooled data is preserved across internal redirects.
             */

            r->proto_input_filters = f;            

            /* We MUST dump the current parser (and filter) because 
             * its existing state is now incorrect.
             * NOTE:
             *   req->parser != NULL && req->body != NULL, since 
             *   apreq_parse_request was called at least once already.
             * 
             */

            apreq_log(APREQ_DEBUG 0, r, "dropping stale apreq filter (%p)", f);

            if (req) {
                req->parser = NULL;
                req->body = NULL;
            }
            if (cfg->f == f) {
                ctx->status = APR_SUCCESS;
                cfg->f = NULL;
            }
        }
        else {
            /* No data was parsed/prefetched, so it's safe to move the filter
             * up to the top of the chain.
             */
            apreq_filter_relocate(f);
        }
    }

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

    if (bb != NULL) {

        if (!ctx->saw_eos) {

            if (ctx->status == APR_INCOMPLETE) {
                apr_bucket_brigade *tmp;
                apr_off_t len;
                rv = ap_get_brigade(f->next, bb, mode, block, readbytes);
            
                if (rv != APR_SUCCESS) {
                    apreq_log(APREQ_ERROR rv, r, "get_brigade failed");
                    return rv;
                }

                tmp = apreq_brigade_copy(bb);
                apr_brigade_length(tmp,0,&len);
                ctx->bytes_read += len;
                APR_BRIGADE_CONCAT(ctx->bb, tmp);

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

        if (req == NULL)
            req = apreq_request(r, NULL);

    }
    else if (!ctx->saw_eos) {

        /* prefetch read! */

        apr_bucket_brigade *tmp = apr_brigade_create(r->pool, 
                                      apr_bucket_alloc_create(r->pool));
        apr_bucket *last = APR_BRIGADE_LAST(ctx->spool);
        apr_size_t total_read = 0;

        if (req == NULL)
            req = apreq_request(r, NULL);

        while (total_read < readbytes) {
            apr_off_t len;

            if (APR_BUCKET_IS_EOS(last)) {
                ctx->saw_eos = 1;
                break;
            }

            rv = ap_get_brigade(f->next, tmp, mode, block, readbytes);
            if (rv != APR_SUCCESS)
                return rv;

            bb = apreq_brigade_copy(tmp);
            apr_brigade_length(bb,0,&len);
            total_read += len;
            apreq_brigade_concat(r, ctx->spool, tmp);
            APR_BRIGADE_CONCAT(ctx->bb, bb);
            last = APR_BRIGADE_LAST(ctx->spool);
        }
        ctx->bytes_read += total_read;

        if (cfg->max_body >= 0 && ctx->bytes_read > cfg->max_body) {
            ctx->status = APR_ENOSPC;
            apreq_log(APREQ_ERROR ctx->status, r, "Bytes read (" APR_OFF_T_FMT
                      ") exceeds configured max_body limit (" APR_OFF_T_FMT ")",
                      ctx->bytes_read, cfg->max_body);
        }
    }
    else
        return APR_SUCCESS;

    if (ctx->status == APR_INCOMPLETE)
        ctx->status = apreq_parse_request(req, ctx->bb);

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

/** @} */

module AP_MODULE_DECLARE_DATA apreq_module =
{
	STANDARD20_MODULE_STUFF,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	register_hooks,			/* callback for registering hooks */
};
