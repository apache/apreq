/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2003 The Apache Software Foundation.  All rights
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
#include "apr_strings.h"
#include "apr_lib.h"
#include "apr_env.h"

#include <stdlib.h>
#include <stdio.h>

#define dCTX struct env_ctx *ctx = (struct env_ctx *)env

/**
 * @file libapreq_cgi.c
 * @brief Source for libapreq_cgi.a.
 */

/**
 * libapreq_cgi is a static library that CGI "scripts" (written
 * in C) can link against to to have libapreq work in a CGI environment.
 *
 * @defgroup libapreq_cgi libapreq_cgi.a
 * @ingroup MODULES
 * @brief Static library for linking libapreq to a CGI C-script.
 * @{
 */

struct env_ctx {
    apr_pool_t         *pool;
    apreq_request_t    *req;
    apreq_jar_t        *jar;
    apr_bucket_brigade *bb;
    int                 loglevel;
    apr_status_t        status;
};

const char apreq_env_name[] = "CGI";
const unsigned int apreq_env_magic_number = 20031014;

#define CRLF "\015\012"

#define APREQ_ENV_STATUS(rc_run, k) do { \
         apr_status_t rc = rc_run; \
         if (rc != APR_SUCCESS) { \
             apreq_log(APREQ_DEBUG 0, ctx, \
                       "Lookup of %s failed: status=%d", k, rc); \
         } \
     } while (0)

APREQ_DECLARE(apr_pool_t *)apreq_env_pool(void *env)
{
    dCTX;
    return ctx->pool;
}

APREQ_DECLARE(const char *)apreq_env_query_string(void *env)
{
    dCTX;
    char *value, qs[] = "QUERY_STRING";
    APREQ_ENV_STATUS(apr_env_get(&value, qs, ctx->pool), qs);
    return value;
}

APREQ_DECLARE(const char *)apreq_env_header_in(void *env, 
                                               const char *name)
{
    dCTX;
    char *key = apr_pstrdup(ctx->pool, name);
    char *k, *value=NULL, *http_key, http[] = "HTTP_";
    for (k = key; *k; ++k) {
        if (*k == '-')
            *k = '_';
        else
            *k = apr_toupper(*k);
    }

    if (!strcmp(key, "CONTENT_TYPE") || !strcmp(key, "CONTENT_LENGTH")) {
        APREQ_ENV_STATUS(apr_env_get(&value, key, ctx->pool), key);
    }
    else {
        http_key = (char *) apr_palloc(ctx->pool, sizeof(http) + strlen(key));
        http_key = strcat(strcpy(http_key, http), key);
        APREQ_ENV_STATUS(apr_env_get(&value, http_key, ctx->pool), http_key);
    }

    return value;
}

APREQ_DECLARE(apr_status_t)apreq_env_header_out(void *env, const char *name, 
                                                char *value)
{
    dCTX;
    apr_file_t *out;
    int bytes;
    apr_file_open_stdout(&out, ctx->pool);
    bytes = apr_file_printf(out, "%s: %s" CRLF, name, value);
    apreq_log(APREQ_DEBUG 0, ctx, "Setting header: %s => %s", name, value);
    return bytes > 0 ? APR_SUCCESS : APR_EGENERAL;
}


APREQ_DECLARE(apreq_jar_t *) apreq_env_jar(void *env, apreq_jar_t *jar)
{
    dCTX;
    if (jar != NULL) {
        apreq_jar_t *old_jar = ctx->jar;
        ctx->jar = jar;
        return old_jar;
    }

    return ctx->jar;
}

APREQ_DECLARE(apreq_request_t *)apreq_env_request(void *env,
                                                  apreq_request_t *req)
{
    dCTX;

    if (req != NULL) {
        apreq_request_t *old_req = ctx->req;
        ctx->req = req;
        return old_req;
    }
    return ctx->req;
}


APREQ_DECLARE_LOG(apreq_log)
{
    dCTX;
    va_list vp;
    if (level < ctx->loglevel)
        return;

    va_start(vp, fmt);
    fprintf(stderr, "[%s(%d)] %s\n", file, line, 
            apr_pvsprintf(ctx->pool,fmt,vp));
    va_end(vp);
}

APREQ_DECLARE(apr_status_t) apreq_env_read(void *env, 
                                           apr_read_type_e block,
                                           apr_off_t bytes)
{
    dCTX;
    if (ctx->bb == NULL) {
        apr_bucket_alloc_t *alloc = apr_bucket_alloc_create(ctx->pool);
        apr_file_t *in;
        apr_bucket *stdin_pipe, *b;

        ctx->bb = apr_brigade_create(ctx->pool, alloc);
        apr_file_open_stdin(&in, ctx->pool);
        stdin_pipe = apr_bucket_pipe_create(in,alloc);
        APR_BRIGADE_INSERT_HEAD(ctx->bb, stdin_pipe);
        b = apr_bucket_eos_create(alloc);
        APR_BRIGADE_INSERT_TAIL(ctx->bb, b);
    }
    return apreq_parse_request(apreq_request(env, NULL), ctx->bb);
}

/** @} */
