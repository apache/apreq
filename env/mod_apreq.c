/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
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
#include "apreq_parsers.h"
#include "apreq_cookie.h"

#define dR  request_rec *r = (request_rec *)env

/**
 * @file mod_apreq.c
 * @brief Source for mod_apreq.so.
 */

/**
 * Uses apxs to compile and install.
 * @defgroup mod_apreq mod_apreq.so
 * @ingroup MODULES
 * @brief Apache-2 filter module.
 * @{
 */

/** The warehouse. */
struct env_config {
    apreq_jar_t        *jar;
    apreq_request_t    *req;
    ap_filter_t        *f;
};

/** Tracks the filter state */
struct filter_ctx {
    apr_bucket_brigade *bb;
    apr_off_t           bytes_seen;
    apr_status_t        status;
};


const char apreq_env[] = "APACHE2"; /**< internal name of module */
static const char filter_name[] = "APREQ";
module AP_MODULE_DECLARE_DATA apreq_module;

/** request logger */
APREQ_DECLARE_LOG(apreq_log)
{
    dR;
    va_list args;
    va_start(args,fmt);
    ap_log_rerror(file, line, level, status, r, "%s",
                  apr_pvsprintf(r->pool, fmt, args));
    va_end(args);
}


APREQ_DECLARE(const char*)apreq_env_query_string(void *env)
{
    dR;
    return r->args;
}

APREQ_DECLARE(apr_pool_t *)apreq_env_pool(void *env)
{
    dR;
    return r->pool;
}

APREQ_DECLARE(const char *)apreq_env_header_in(void *env, const char *name)
{
    dR;
    return apr_table_get(r->headers_in, name);
}

/**
 * r->headers_out.
 * @bug Sending a Set-Cookie header on a 304
 * requires a different header table.
 */
APREQ_DECLARE(apr_status_t)apreq_env_header_out(void *env, const char *name, 
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
    }
    return cfg;
}

APREQ_DECLARE(apreq_jar_t *) apreq_env_jar(void *env, apreq_jar_t *jar)
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

static ap_filter_t *get_apreq_filter(request_rec *r)
{
    struct env_config *cfg = get_cfg(r);
    ap_filter_t *f;
    if (cfg->f != NULL)
        return cfg->f;

    for (f  = r->input_filters; 
         f != NULL && f->frec->ftype == AP_FTYPE_CONTENT_SET;  
         f  = f->next)
    {
        if (strcmp(f->frec->name, filter_name) == 0)
            return cfg->f = f;
    }
    apreq_log(APREQ_DEBUG 0, r, 
              "apreq filter is now added" );
    return cfg->f = ap_add_input_filter(filter_name, NULL, r, r->connection);
}

APREQ_DECLARE(apreq_request_t *) apreq_env_request(void *env, 
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

    apreq_log(APREQ_DEBUG 0, r, 
              "apreq request is now initialized" );
    return c->req;
}

/**
 * Reads data directly into the parser.
 *@bug  This function is badly broken.  It needs to use
 *      SPECULATIVE mode when doing readsin pre-content-handler
 *      phase, but should use a normal (READBYTES) read otherwise.
 */

APREQ_DECLARE(apr_status_t) apreq_env_read(void *env, 
                                           apr_read_type_e block,
                                           apr_off_t bytes)
{
    dR;
    ap_filter_t *f = get_apreq_filter(r);
    if (f == NULL)
        return APR_NOTFOUND;

    return ap_get_brigade(f, NULL, AP_MODE_SPECULATIVE, block, bytes);
}

APR_INLINE
static void apreq_filter_make_context(ap_filter_t *f)
{
    request_rec *r = f->r;
    apr_bucket_alloc_t *alloc = apr_bucket_alloc_create(r->pool);
    struct filter_ctx *ctx = apr_palloc(r->pool, sizeof *ctx);
    f->ctx      = ctx;
    ctx->bb     = apr_brigade_create(r->pool, alloc);
    ctx->bytes_seen = 0;
    ctx->status = APR_INCOMPLETE;

    apreq_log(APREQ_DEBUG 0, r, 
              "apreq filter context created." );    
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

static apr_status_t apreq_filter_init(ap_filter_t *f)
{
    request_rec *r = f->r;
    struct filter_ctx *ctx;

    /* We must be inside config.c:ap_invoke_handler -> 
     * ap_invoke_filter_init (r->input_filters), and
     * we have to deal with the possibility that apreq may have
     * prefetched data prior to apache running the ap_invoke_filter_init
     * hook.
     */

    if (f->ctx) {
        ctx = f->ctx;
        /* first we relocate the filter to the top of the chain. */
        apreq_filter_relocate(f);

        /* We may have already prefetched some data, perhaps
         * at the behest of an aaa handler, or during a speculative
         * read (via apreq_env_read) prior to an internal redirect.
         * We need to drop whatever data we may already have parsed.
         */

        if (ctx->bytes_seen) {
            apreq_request_t *req = apreq_env_request(r, NULL);

            /* must dump the current parser because 
             * its existing state is now incorrect.
             * NOTE:
             *   req->parser != NULL && req->body != NULL, since 
             *   apreq_parse_request was called at least once already.
             * 
             */

             req->parser = NULL;
             req->body = NULL;

             ctx->bytes_seen = 0;
             apr_brigade_cleanup(ctx->bb);
             ctx->status = APR_INCOMPLETE;
        }
    } 
    else {
        apr_bucket_alloc_t *alloc = apr_bucket_alloc_create(r->pool);
        ctx = apr_palloc(r->pool, sizeof *ctx);
        f->ctx      = ctx;
        ctx->bb     = apr_brigade_create(r->pool, alloc);
        ctx->bytes_seen = 0;
        ctx->status = APR_INCOMPLETE;

        apreq_log(APREQ_DEBUG 0, r, 
                  "apreq filter is initialized" );
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
    apr_bucket *e;
    int saw_eos = 0;
    apr_status_t rv;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;

    switch (ctx->status) {
    case APR_INCOMPLETE:
        break;
    case APR_EOF:
        return APR_EOF;

    case APR_SUCCESS:
    default:
        return ap_get_brigade(f->next, bb, mode, block, readbytes);
    }

    switch (mode) {

    case AP_MODE_SPECULATIVE: 
        if (bb == NULL) {

            /* prefetch read! */

            apr_off_t skip_bytes = ctx->bytes_seen;

            bb = ctx->bb;
            e = APR_BRIGADE_LAST(bb);
            rv = ap_get_brigade(f->next, bb, mode, block,
                                readbytes + skip_bytes);
            if (rv != APR_SUCCESS)
                return rv;
            e = APR_BUCKET_NEXT(e);

            /* throw away buckets we've already seen */

            while (skip_bytes > 0 && e != APR_BRIGADE_SENTINEL(bb)) {
                apr_bucket *f = e;
                apr_size_t len;
                const char *dummy;

                if (APR_BUCKET_IS_EOS(e)) {
                    ctx->status = APR_EOF;
                    break;
                }
                rv = apr_bucket_read(e, &dummy, &len, APR_BLOCK_READ);
                if (rv != APR_SUCCESS)
                    return rv;

                if (skip_bytes < len) {
                    apr_bucket_split(e, skip_bytes);
                    len = skip_bytes;
                }
                e = APR_BUCKET_NEXT(e);
                apr_bucket_delete(f);
                skip_bytes -= len;
            }

            /* add the new buckets to the ctx->bytes_seen count */

            while (e != APR_BRIGADE_SENTINEL(bb)) {
                apr_bucket *f = e;
                apr_size_t len;
                const char *dummy;

                if (APR_BUCKET_IS_EOS(e)) {
                    ctx->status = APR_EOF;
                    break;
                }
                rv = apr_bucket_read(e, &dummy, &len, APR_BLOCK_READ);
                if (rv != APR_SUCCESS)
                    return rv;

                ctx->bytes_seen += len;
            }
        }
        else {
            /* not a prefetch read; can simply get out of the way */
            return ap_get_brigade(f->next, bb, mode, block, readbytes);
        }
        break;

    case AP_MODE_EXHAUSTIVE:
    case AP_MODE_READBYTES:
        e = APR_BRIGADE_LAST(bb);

        rv = ap_get_brigade(f->next, bb, mode, block, readbytes);
            if (rv != APR_SUCCESS)
                return rv;

        for (e  = APR_BUCKET_NEXT(e);
             e != APR_BRIGADE_SENTINEL(bb); 
             e  = APR_BUCKET_NEXT(e)) 
        {
            apr_bucket *b;

            if (ctx->bytes_seen) {
                const char *dummy;
                apr_size_t len;
                rv = apr_bucket_read(e, &dummy, &len, APR_BLOCK_READ);
                if (rv != APR_SUCCESS)
                    return rv;
                if (ctx->bytes_seen < len) {
                    apr_bucket_split(e, ctx->bytes_seen);
                    len = ctx->bytes_seen;
                }
                ctx->bytes_seen -= len;
                continue;
            }

            apr_bucket_copy(e, &b);
            apr_bucket_setaside(b, r->pool);
            APR_BRIGADE_INSERT_TAIL(ctx->bb, b);

            if (APR_BUCKET_IS_EOS(e)) {
                ctx->status = APR_EOF;
                break;
            }
        }
        break;

    case AP_MODE_EATCRLF:
    case AP_MODE_GETLINE:
    default:
        return APR_ENOTIMPL;
    }

    rv = apreq_parse_request(apreq_request(r, NULL), ctx->bb);
    return rv == APR_INCOMPLETE ? APR_SUCCESS : rv;
}


static void register_hooks (apr_pool_t *p)
{
    ap_register_input_filter(filter_name, apreq_filter, apreq_filter_init,
                             AP_FTYPE_CONTENT_SET);
}

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

/** @} */
