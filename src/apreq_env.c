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

static const apreq_env_t *apreq_env;

APREQ_DECLARE(const apreq_env_t *) apreq_env_module(const apreq_env_t *mod)
{
    if (mod != NULL) {
        const apreq_env_t *old_mod = apreq_env;
        apreq_env = mod;
        return old_mod;
    }
    return apreq_env;
}


APREQ_DECLARE_NONSTD(void) apreq_log(const char *file, int line,
                                     int level, apr_status_t status,
                                     void *env, const char *fmt, ...)
{
    va_list vp;
    va_start(vp, fmt);
    apreq_env->log(file,line,level,status,env,fmt,vp);
    va_end(vp);
}

APREQ_DECLARE(apr_pool_t *) apreq_env_pool(void *env)
{
    return apreq_env->pool(env);
}

APREQ_DECLARE(apreq_jar_t *) apreq_env_jar(void *env, apreq_jar_t *jar)
{
    return apreq_env->jar(env,jar);
}

APREQ_DECLARE(apreq_request_t *) apreq_env_request(void *env,
                                                   apreq_request_t *req)
{
    return apreq_env->request(env,req);
}

APREQ_DECLARE(const char *) apreq_env_query_string(void *env)
{
    return apreq_env->query_string(env);
}

APREQ_DECLARE(const char *) apreq_env_header_in(void *env, const char *name)
{
    return apreq_env->header_in(env, name);
}

APREQ_DECLARE(apr_status_t)apreq_env_header_out(void *env, 
                                                const char *name,
                                                char *val)
{
    return apreq_env->header_out(env,name,val);
}

APREQ_DECLARE(apr_status_t) apreq_env_read(void *env,
                                           apr_read_type_e block,
                                           apr_off_t bytes)
{
    return apreq_env->read(env,block,bytes);
}

#define dP apr_pool_t *p = (apr_pool_t *)env

static struct {
    apreq_request_t    *req;
    apreq_jar_t        *jar;
    apr_status_t        status;
} ctx;

#define CRLF "\015\012"

#define APREQ_ENV_STATUS(rc_run, k) do {                                \
         apr_status_t rc = rc_run;                                      \
         if (rc != APR_SUCCESS) {                                       \
             apreq_log(APREQ_DEBUG 0, p,                                \
                       "Lookup of %s failed: status=%d", k, rc);        \
         }                                                              \
     } while (0)


/**
 * CGI is the default environment module included in libapreq2...
 * XXX add more info here XXX
 *
 * @defgroup CGI Common Gateway Interface
 * @ingroup MODULES
 * @brief apreq_env.c: libapreq2's default CGI module
 * @{
 */


#define APREQ_MODULE_NAME         "CGI"
#define APREQ_MODULE_MAGIC_NUMBER 20031025

static apr_pool_t *cgi_pool(void *env)
{
    return (apr_pool_t *)env;
}

static const char *cgi_query_string(void *env)
{
    dP;
    char *value = NULL, qs[] = "QUERY_STRING";
    APREQ_ENV_STATUS(apr_env_get(&value, qs, p), qs);
    return value;
}

static const char *cgi_header_in(void *env, 
                                 const char *name)
{
    dP;
    char *key = apr_pstrcat(p, "HTTP_", name, NULL);
    char *k, *value = NULL;
    for (k = key; *k; ++k) {
        if (*k == '-')
            *k = '_';
        else
            *k = apr_toupper(*k);
    }

    if (!strcmp(key, "HTTP_CONTENT_TYPE") 
        || !strcmp(key, "HTTP_CONTENT_LENGTH"))
    {
        key += 5; /* strlen("HTTP_") */
    }

    APREQ_ENV_STATUS(apr_env_get(&value, key, p), key);

    return value;
}

static apr_status_t cgi_header_out(void *env, const char *name, 
                                   char *value)
{
    dP;
    apr_file_t *out;
    int bytes;
    apr_file_open_stdout(&out, p);
    bytes = apr_file_printf(out, "%s: %s" CRLF, name, value);
    apreq_log(APREQ_DEBUG 0, p, "Setting header: %s => %s", name, value);
    return bytes > 0 ? APR_SUCCESS : APR_EGENERAL;
}


static apreq_jar_t *cgi_jar(void *env, apreq_jar_t *jar)
{

    if (jar != NULL) {
        apreq_jar_t *old_jar = ctx.jar;
        ctx.jar = jar;
        return old_jar;
    }

    return ctx.jar;
}

static apreq_request_t *cgi_request(void *env,
                                    apreq_request_t *req)
{

    if (req != NULL) {
        apreq_request_t *old_req = ctx.req;
        ctx.req = req;
        return old_req;
    }
    return ctx.req;
}

static void cgi_log(const char *file, int line, int level, 
                    apr_status_t status, void *env, const char *fmt,
                    va_list vp)
{
    dP;
    fprintf(stderr, "[%s(%d)] %s\n", file, line, apr_pvsprintf(p,fmt,vp));
}

static apr_status_t cgi_read(void *env, 
                             apr_read_type_e block,
                             apr_off_t bytes)
{
    dP;
    apreq_request_t *req = apreq_request(env, NULL);

    if (req->body == NULL) {
        apr_bucket_alloc_t *alloc = apr_bucket_alloc_create(p);
        apr_bucket_brigade *bb = apr_brigade_create(p, alloc);
        apr_bucket *stdin_pipe, *eos = apr_bucket_eos_create(alloc);
        apr_file_t *in;
        apr_file_open_stdin(&in, p);
        stdin_pipe = apr_bucket_pipe_create(in,alloc);
        APR_BRIGADE_INSERT_HEAD(bb, stdin_pipe);
        APR_BRIGADE_INSERT_TAIL(bb, eos);
        ctx.status = apreq_parse_request(req, bb);
    }
    return ctx.status;
}

static APREQ_ENV_MODULE(cgi, APREQ_MODULE_NAME, 
                        APREQ_MODULE_MAGIC_NUMBER);

static const apreq_env_t *apreq_env = &cgi_module;

/** @} */
