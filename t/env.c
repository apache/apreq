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

#include "apreq.h"
#include "apreq_env.h"
#include "apreq_params.h"
#include "apreq_parsers.h"
#include "apreq_cookie.h"

#include "apr_lib.h"
#include "apr_strings.h"

#include <stdlib.h>
#include <stdio.h>

#define dENV struct env_ctx *env = (struct env_ctx *)ctx
/* the "warehouse" */

struct env_ctx {
    apr_pool_t         *pool;
    apreq_request_t    *req;
    apreq_jar_t        *jar;
    apr_bucket_brigade *bb;
};

static const char env_name[] = "CGI";
#define CRLF "\015\012"

static apr_pool_t *env_pool(void *ctx)
{
    dENV;
    return env->pool;
}

static const char *env_in(void *ctx, const char *name)
{
    return getenv(name);
}

static apr_status_t env_out(void *ctx, const char *name, char *value)
{    
    return printf("%s: %s" CRLF, name, value) > 0 ? APR_SUCCESS : APR_EGENERAL;
}

static const char *env_args(void *ctx)
{
    return getenv("QUERY_STRING");
}

static void *env_jar(void *ctx, void *jar)
{
    dENV;
    if (jar != NULL) {
        apreq_jar_t *old_jar = env->jar;
        env->jar = jar;
        return old_jar;
    }

    return env->jar;
}

static void *env_request(void *ctx, void *req)
{
    dENV;
    if (req != NULL) {
        apreq_request_t *old_req = env->req;
        env->req = req;
        return old_req;
    }
    return env->req;
}

static apreq_cfg_t *env_cfg(void *ctx)
{
    /* XXX: not implemented */
    return NULL;
}


static int dump_table(void *ctx, const char *key, const char *value)
{
    dENV;
    dAPREQ_LOG;
    apreq_log(APREQ_DEBUG 0, env, "%s => %s", key, value);
    return 1;
}


APREQ_LOG(env_log)
{

}

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
    env_log
 };

