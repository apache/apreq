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

#include "apreq.h"
#include "apreq_env.h"
#include "apreq_params.h"
#include "apreq_parsers.h"
#include "apreq_cookie.h"

#define dR  request_rec *r = (request_rec *)env

/*      the "warehouse"
 *   The parser body is attached to the filter-selected parser.
 * 
 *  request & jar are in r->request_config;
 *  parser config data inside parser_t
 *     (may need to be merged with server/request cfg)
 *
 */

struct env_config {
    apreq_jar_t        *jar;
    apreq_request_t    *req;
    ap_filter_t        *f;
};

struct filter_ctx {
    apr_bucket_brigade *bb_in;
    apr_bucket_brigade *bb_out;
    apr_status_t        status;
};


const char apreq_env[] = "APACHE2";
static const char filter_name[] = "APREQ";
module AP_MODULE_DECLARE_DATA apreq_module;

APREQ_DECLARE_LOG(apreq_log)
{
    dR;
    va_list args;
    va_start(args,fmt);
    ap_log_rerror(file, line, level, status, r, "%s",
                  apr_pvsprintf(r->pool, fmt, args));
    va_end(args);
}


APREQ_DECLARE(const char*)apreq_env_args(void *env)
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

    if (cfg->f != NULL)
        return cfg->f;

    for (cfg->f  = r->input_filters; 
         cfg->f != NULL && cfg->f->frec->ftype == AP_FTYPE_CONTENT_SET;  
         cfg->f  = cfg->f->next)
    {
        if (strcmp(cfg->f->frec->name, filter_name) == 0)
            return cfg->f;
    }
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
    return c->req;
}

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

static int apreq_filter_init(ap_filter_t *f)
{
    request_rec *r = f->r;
    apr_bucket_alloc_t *alloc = apr_bucket_alloc_create(r->pool);
    struct env_config *cfg = get_cfg(r);
    struct filter_ctx *ctx;

    if (f->ctx) {
        apreq_log(APREQ_ERROR 500, r, "apreq filter is already initialized!");
        return 500; /* filter already initialized */
    }
    ctx = apr_palloc(r->pool, sizeof *ctx);
    f->ctx      = ctx;
    ctx->bb_in  = apr_brigade_create(r->pool, alloc);
    ctx->bb_out = apr_brigade_create(r->pool, alloc);
    ctx->status = APR_INCOMPLETE;

    if (cfg->req == NULL)
        cfg->req = apreq_request(r, r->args);

    if (cfg->req->body == NULL)
        cfg->req->body = apreq_table_make(r->pool, APREQ_NELTS);

    apreq_log(APREQ_DEBUG 0, r, "filter initialized successfully");
    return 0;
}

static apr_status_t apreq_filter(ap_filter_t *f,
                                 apr_bucket_brigade *bb,
                                 ap_input_mode_t mode,
                                 apr_read_type_e block,
                                 apr_off_t readbytes)
{
    request_rec *r = f->r;
    struct apreq_request_t *req = apreq_request(r, NULL);
    struct filter_ctx *ctx;
    apr_status_t rv;
    apr_bucket *e;
    int saw_eos = 0;

    if (f->ctx == NULL)
        apreq_filter_init(f);

    ctx = f->ctx;

    switch (mode) {

    case AP_MODE_SPECULATIVE: 
        if (bb != NULL) { /* not a prefetch read  */
            if (APR_BRIGADE_EMPTY(ctx->bb_out))
                return ap_get_brigade(f->next, bb, mode, block, readbytes);

            APR_BRIGADE_FOREACH(e,ctx->bb_out) {
                apr_bucket *b;
                apr_bucket_copy(e,&b);
                APR_BRIGADE_INSERT_TAIL(bb,b);
            }
        }
        mode = AP_MODE_READBYTES;
        bb = ctx->bb_out;
        break;

    case AP_MODE_EXHAUSTIVE:
    case AP_MODE_READBYTES:
        APR_BRIGADE_CONCAT(bb, ctx->bb_out);
        break;

    case AP_MODE_EATCRLF:
    case AP_MODE_GETLINE:
        if (!APR_BRIGADE_EMPTY(ctx->bb_out))
            return APR_ENOTIMPL;

    default: 
        return ap_get_brigade(f->next, bb, mode, block, readbytes);
    }

    e = APR_BRIGADE_LAST(bb);
    rv = ap_get_brigade(f->next, bb, mode, block, readbytes);
    if (rv != APR_SUCCESS || ctx->status != APR_INCOMPLETE)
        return rv;

    e = APR_BUCKET_NEXT(e);

    for ( ;e != APR_BRIGADE_SENTINEL(bb); e = APR_BUCKET_NEXT(e)) {
        apr_bucket *b;
        apr_bucket_copy(e, &b);
        apr_bucket_setaside(b, r->pool);
        APR_BRIGADE_INSERT_TAIL(ctx->bb_in, b);

        if (APR_BUCKET_IS_EOS(e)) {
            saw_eos = 1;
            break;
        }
    }

    ctx->status = apreq_parse_request(req, ctx->bb_in);

    if (ctx->status == APR_INCOMPLETE && saw_eos == 1)
        ctx->status = APR_EOF; /* parser choked on EOS bucket */

    apreq_log(APREQ_DEBUG rv, r, "filter parser returned(%d)", ctx->status);
    return rv;
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
