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

#define dR  request_rec *r = (request_rec *)ctx

/* the "warehouse" */

struct env_ctx {
    apreq_request_t    *req;
    apreq_jar_t        *jar;
    apr_bucket_brigade *bb_in;
    apr_bucket_brigade *bb_parse;
};

static const char env_name[] = "APACHE2";
static const char filter_name[] = "APREQ";
module AP_MODULE_DECLARE_DATA apreq_module;

static apr_pool_t *env_pool(void *ctx)
{
    dR;
    return r->pool;
}

static const char *env_in(void *ctx, const char *name)
{
    dR;
    return apr_table_get(r->headers_in, name);
}

static apr_status_t env_out(void *ctx, const char *name, char *value)
{
    dR;
    apr_table_addn(r->headers_out, name, value);
    return APR_SUCCESS;
}

static const char *env_args(void *ctx)
{
    dR;
    return r->args;
}

static APR_INLINE struct env_ctx *apreq_note(request_rec *r)
{
    struct env_ctx *ctx = (struct env_ctx *)
        apr_table_get(r->notes, filter_name);

    if (ctx == NULL) {
        ctx = apr_palloc(r->pool, sizeof *ctx);
        ctx->req = NULL;
        ctx->jar = NULL;
        ctx->bb_in = ctx->bb_parse = NULL;
        apr_table_add(r->notes, filter_name, (char *)ctx);
    }
    return ctx;
}

static void *env_jar(void *ctx, void *jar)
{
    dR;
    struct env_ctx *c = apreq_note(r);

    if (jar != NULL) {
        apreq_jar_t *oldjar = c->jar;
        c->jar = (apreq_jar_t *)jar;
        return oldjar;
    }        
    return c->jar;
}

static void *env_request(void *ctx, void *req)
{
    dR;
    struct env_ctx *c = apreq_note(r);

    if (req != NULL) {
        apreq_request_t *oldreq = c->req;
        c->req = (apreq_request_t *)req;
        return oldreq;
    }
    return c->req;
}

static apreq_cfg_t *env_cfg(void *ctx)
{
    /* XXX: not implemented */
    return NULL;
}


static int dump_table(void *ctx, const char *key, const char *value)
{
    request_rec *r = ctx;
    dAPREQ_LOG;
    apreq_log(APREQ_DEBUG 0, r, "%s => %s", key, value);
    return 1;
}


static apr_status_t apreq_filter(ap_filter_t *f,
                                 apr_bucket_brigade *bb,
                                 ap_input_mode_t mode,
                                 apr_read_type_e block,
                                 apr_off_t readbytes)
{
    request_rec *r = f->r;
    struct env_ctx *ctx;
    apr_status_t rv;
    dAPREQ_LOG;

    /* just get out of the way of things we don't want. */
    if (mode != AP_MODE_READBYTES) {
        return ap_get_brigade(f->next, bb, mode, block, readbytes);
    }

    if (f->ctx == NULL) {
        ctx = apreq_note(r);
        f->ctx = ctx;
        if (ctx->bb_in == NULL)
            ctx->bb_in = apr_brigade_create(r->pool, f->c->bucket_alloc);
        if (ctx->bb_parse == NULL)
            ctx->bb_parse = apr_brigade_create(r->pool, f->c->bucket_alloc);
        if (ctx->req == NULL) {
            apreq_parser_t *parser;
            ctx->req = apreq_request(r);
            parser = apreq_make_parser(r->pool, APREQ_URL_ENCTYPE,
                                       apreq_parse_urlencoded, NULL, ctx->req);
            apreq_register_parser(ctx->req, parser);
            parser = apreq_make_parser(r->pool, APREQ_MFD_ENCTYPE,
                                       apreq_parse_multipart, NULL, ctx->req);
            apreq_register_parser(ctx->req, parser);
        }
    }
    else
        ctx = (struct env_ctx *)f->ctx;

    /* XXX configure filter & parser here */

    rv = ap_get_brigade(f->next, ctx->bb_in, AP_MODE_READBYTES,
                        block, readbytes);

    apreq_log(APREQ_DEBUG rv, r, "dump args:");
    apreq_table_do(dump_table, r, ctx->req->args, NULL);


    if (ctx->req->v.status == APR_INCOMPLETE) {
        int saw_eos = 0;

        while (! APR_BRIGADE_EMPTY(ctx->bb_in)) {
            apr_bucket *e, *f = APR_BRIGADE_FIRST(ctx->bb_in);
            apr_bucket_copy(f, &e);
            apr_bucket_setaside(e, r->pool);

            APR_BUCKET_REMOVE(f);
            APR_BRIGADE_INSERT_TAIL(ctx->bb_parse, e);
            APR_BRIGADE_INSERT_TAIL(bb, f);

            if (APR_BUCKET_IS_EOS(e)) {
                saw_eos = 1;
                break;
            }
        }

        rv = apreq_parse(ctx->req, ctx->bb_parse);

        if (rv == APR_INCOMPLETE && saw_eos == 1)
            return APR_EGENERAL;        /* parser choked on EOS bucket */
    }

    else {
        APR_BRIGADE_CONCAT(bb, ctx->bb_in);
    }

    if (ctx->req->body) {
        apreq_log(APREQ_DEBUG rv, r, "dump body:");
        apreq_table_do(dump_table, r, ctx->req->body, NULL);
    }

    return rv;
}



static void register_hooks (apr_pool_t *p)
{
    ap_register_input_filter(filter_name, apreq_filter, NULL, 
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

const struct apreq_env APREQ_ENV =
{
    env_name,
    env_pool,
    env_in,
    env_out,
    env_args,
    env_jar,
    env_request,
    env_cfg,
    (APREQ_LOG(*))ap_log_rerror
 };

