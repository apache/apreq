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
#include "apr_strings.h"
#include "apr_lib.h"
#include "apr_env.h"
#include "apr_file_io.h"


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

APREQ_DECLARE(const char *) apreq_env_temp_dir(void *env, const char *path)
{
    if (path != NULL)
        /* ensure path is a valid pointer during the entire request */
        path = apr_pstrdup(apreq_env_pool(env),path);

    return apreq_env->temp_dir(env,path);
}

APREQ_DECLARE(apr_off_t) apreq_env_max_body(void *env, apr_off_t bytes)
{
    return apreq_env->max_body(env,bytes);
}

APREQ_DECLARE(apr_ssize_t) apreq_env_max_brigade(void *env, apr_ssize_t bytes)
{
    return apreq_env->max_brigade(env,bytes);
}


#define dP apr_pool_t *p = (apr_pool_t *)env

static struct {
    apreq_request_t    *req;
    apreq_jar_t        *jar;
    apr_status_t        status;
    const char         *temp_dir;
    apr_off_t           max_body;
    apr_ssize_t         max_brigade;
    apr_bucket_brigade *in;
    apr_off_t           bytes_read;
} ctx =  {NULL, NULL, APR_SUCCESS, NULL, -1, APREQ_MAX_BRIGADE_LEN, NULL, 0};

#define CRLF "\015\012"

#define APREQ_ENV_STATUS(rc_run, k) do {                                \
         apr_status_t rc = rc_run;                                      \
         if (rc != APR_SUCCESS) {                                       \
             apreq_log(APREQ_DEBUG 0, p,                                \
                       "Lookup of %s failed: status=%d", k, rc);        \
         }                                                              \
     } while (0)


/**
 * @defgroup apreq_cgi Common Gateway Interface
 * @ingroup apreq_env
 * @brief CGI module included in the libapreq2 library.
 *
 * CGI is the default environment module included in libapreq2...
 * XXX add more info here XXX
 *
 * @{
 */


#define APREQ_MODULE_NAME         "CGI"
#define APREQ_MODULE_MAGIC_NUMBER 20040707

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
    apr_status_t s = apr_file_open_stdout(&out, p);
    apreq_log(APREQ_DEBUG s, p, "Setting header: %s => %s", name, value);
    bytes = apr_file_printf(out, "%s: %s" CRLF, name, value);
    apr_file_flush(out);
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
    char buf[256];
    apr_file_t *err;
    apr_file_open_stderr(&err, p);
    apr_file_printf(err, "[%s(%d): %s] %s\n", file, line, 
            apr_strerror(status,buf,255),apr_pvsprintf(p,fmt,vp));
    apr_file_flush(err);
}

static apr_status_t cgi_read(void *env, 
                             apr_read_type_e block,
                             apr_off_t bytes)
{
    dP;
    apreq_request_t *req = apreq_request(env, NULL);
    apr_bucket *e;
    apr_status_t s;

    if (ctx.in == NULL) {
        apr_bucket_alloc_t *alloc = apr_bucket_alloc_create(p);
        apr_bucket *stdin_pipe, *eos = apr_bucket_eos_create(alloc);
        apr_file_t *in;
        apr_file_open_stdin(&in, p);
        stdin_pipe = apr_bucket_pipe_create(in,alloc);
        ctx.in = apr_brigade_create(p, alloc);
        APR_BRIGADE_INSERT_HEAD(ctx.in, stdin_pipe);
        APR_BRIGADE_INSERT_TAIL(ctx.in, eos);
        ctx.status = APR_INCOMPLETE;
    }


    if (ctx.status != APR_INCOMPLETE)
        return ctx.status;

    switch (s = apr_brigade_partition(ctx.in, bytes, &e)) {
        apr_bucket_brigade *bb;
        apr_off_t len;

    case APR_SUCCESS:
        bb = ctx.in;
        ctx.in = apr_brigade_split(bb, e);
        ctx.bytes_read += bytes;
        if (ctx.max_body >= 0) {
            if (ctx.bytes_read > ctx.max_body) {
                apreq_log(APREQ_ERROR APR_EGENERAL, env,
                          "Bytes read (%" APR_OFF_T_FMT 
                          ") exceeds configured limit (%" APR_OFF_T_FMT ")",
                          ctx.bytes_read, ctx.max_body);
                return ctx.status = APR_EGENERAL;
            }
        }
        ctx.status = apreq_parse_request(req, bb);
        apr_brigade_cleanup(bb);
        break;

    case APR_INCOMPLETE:
        bb = ctx.in;
        ctx.in = apr_brigade_split(bb, e);
        s = apr_brigade_length(bb,1,&len);
        if (s != APR_SUCCESS)
            return ctx.status = s;
        ctx.bytes_read += len;
        if (ctx.max_body >= 0) {
            if (ctx.bytes_read > ctx.max_body) {
                apreq_log(APREQ_ERROR APR_EGENERAL, env,
                          "Bytes read (%" APR_OFF_T_FMT 
                          ") exceeds configured limit (%" APR_OFF_T_FMT ")",
                          ctx.bytes_read, ctx.max_body);
                return ctx.status = APR_EGENERAL;
            }
        }
        ctx.status = apreq_parse_request(req, bb);
        apr_brigade_cleanup(bb);
        break;

    default:
        ctx.status = s;
    }

    return ctx.status;
}


static const char *cgi_temp_dir(void *env, const char *path)
{
    if (path != NULL) {
        dP;
        const char *rv = ctx.temp_dir;
        ctx.temp_dir = apr_pstrdup(p, path);
        return rv;
    }
    if (ctx.temp_dir == NULL) {
        dP;
        if (apr_temp_dir_get(&ctx.temp_dir, p) != APR_SUCCESS)
            ctx.temp_dir = NULL;
    }

    return ctx.temp_dir;
}


static apr_off_t cgi_max_body(void *env, apr_off_t bytes)
{
    if (bytes >= 0) {
        apr_off_t rv = ctx.max_body;
        ctx.max_body = bytes;
        return rv;
    }
    return ctx.max_body;
}


static apr_ssize_t cgi_max_brigade(void *env, apr_ssize_t bytes)
{
    if (bytes >= 0) {
        apr_ssize_t rv = ctx.max_brigade;
        ctx.max_brigade = bytes;
        return rv;
    }
    return ctx.max_brigade;
}



static APREQ_ENV_MODULE(cgi, APREQ_MODULE_NAME, 
                        APREQ_MODULE_MAGIC_NUMBER);

static const apreq_env_t *apreq_env = &cgi_module;

/** @} */
