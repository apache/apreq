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
#include "apreq_cookie.h"

#define dR  request_rec *r = (request_rec *)ctx

/* the "warehouse" */

struct env_note {
    apreq_request_t *req;
    apreq_jar_t *jar;
    apr_bucket_brigade *bb;
};

static const char env_name[] = "APACHE2";
static const char filter_name[] = "APREQ";

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
    apr_table_add(r->headers_in, name, value);
    return APR_SUCCESS;
}

static const char *env_args(void *ctx)
{
    dR;
    return r->args;
}

static APR_INLINE struct env_note *apreq_note(request_rec *r)
{
    struct env_note *n = (struct env_note *)
        apr_table_get(r->notes, filter_name);

    if (n == NULL) {
        n = apr_palloc(r->pool, sizeof *n);
        n->req = NULL;
        n->jar = NULL;
        n->bb = NULL;
        apr_table_add(r->notes, filter_name, (char *)n);
    }
    return n;
}

static void *env_jar(void *ctx, void *jar)
{
    dR;
    struct env_note *n = apreq_note(r);

    if (jar != NULL) {
        apreq_jar_t *oldjar = n->jar;
        n->jar = (apreq_jar_t *)jar;
        return oldjar;
    }        
    return n->jar;
}

static void *env_request(void *ctx, void *req)
{
    dR;
    struct env_note *n = apreq_note(r);

    if (req != NULL) {
        apreq_request_t *oldreq = n->req;
        n->req = (apreq_request_t *)req;
        return oldreq;
    }
    return n->req;
}

static apreq_cfg_t *env_cfg(void *ctx)
{
    /* XXX: not implemented */
    return NULL;
}

static apr_status_t env_get_brigade(void *ctx, apr_bucket_brigade **bb)
{
    dR;
    struct env_note *n = apreq_note(r);

    if (n->bb == NULL)
        n->bb = apr_brigade_create(r->pool, 
                                   apr_bucket_alloc_create(r->pool));

    /* XXX: do something here to populate n->bb */

    *bb = n->bb;
    return APR_SUCCESS;
}

static apr_status_t apreq_filter(ap_filter_t *f,
                                 apr_bucket_brigade *bb,
                                 ap_input_mode_t mode,
                                 apr_read_type_e block,
                                 apr_off_t readbytes)
{
    /* XXX ... */

	return APR_SUCCESS;
}



static void register_hooks (apr_pool_t *p)
{
    ap_register_input_filter(filter_name, apreq_filter, NULL, 
                             AP_FTYPE_CONTENT_SET);
}


module AP_MODULE_DECLARE_DATA apreq_module =
{
	// Only one callback function is provided.  Real
	// modules will need to declare callback functions for
	// server/directory configuration, configuration merging
	// and other tasks.
	STANDARD20_MODULE_STUFF,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	register_hooks,			/* callback for registering hooks */
};

const struct apreq_env APREQ_ENV = (struct apreq_env)
{
    env_name,
    env_pool,
    env_in,
    env_out,
    env_args,
    env_jar,
    env_request,
    env_cfg,
    env_get_brigade,
    ap_log_rerror
 };

