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

#define dP struct cgi_env *cgi_env = (struct cgi_env*)env; \
    apr_pool_t *p = cgi_env->pool

static struct {
    apreq_request_t    *req;
    apreq_jar_t        *jar;
    apr_status_t        status;
    const char         *temp_dir;
    apr_off_t           max_body;
    apr_ssize_t         max_brigade;
    apr_bucket_brigade *in;
    apr_off_t           bytes_read;
} ctx = {NULL, NULL, APR_SUCCESS, NULL, -1, APREQ_MAX_BRIGADE_LEN, NULL, 0};

struct cgi_env {
    struct apreq_env_handle_t env;
    apr_pool_t *pool;
};

#define CRLF "\015\012"

#define APREQ_ENV_STATUS(rc_run, k) do {                                \
         apr_status_t rc = rc_run;                                      \
         if (rc != APR_SUCCESS) {                                       \
             apreq_log(APREQ_DEBUG APR_EGENERAL, env,                   \
                       "Lookup of %s failed: status=%d", k, rc);        \
         }                                                              \
     } while (0)

#define APREQ_MODULE_NAME         "CGI"
#define APREQ_MODULE_MAGIC_NUMBER 20041130

static apr_pool_t *cgi_pool(apreq_env_handle_t *env)
{
    struct cgi_env *cgi_env = (struct cgi_env*)env;

    return cgi_env->pool;
}

static apr_status_t bucket_alloc_cleanup(void *data)
{
    apr_bucket_alloc_t *ba = data;
    apr_bucket_alloc_destroy(ba);
    return APR_SUCCESS;
}


static apr_bucket_alloc_t *cgi_bucket_alloc(apreq_env_handle_t *env)
{
    struct cgi_env *cgi_env = (struct cgi_env*)env;
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(cgi_env->pool);

    apr_pool_cleanup_register(cgi_env->pool, ba,
                              bucket_alloc_cleanup,
                              bucket_alloc_cleanup);
    return ba;
}

static const char *cgi_query_string(apreq_env_handle_t *env)
{
    dP;
    char *value = NULL, qs[] = "QUERY_STRING";
    APREQ_ENV_STATUS(apr_env_get(&value, qs, p), qs);
    return value;
}

static const char *cgi_header_in(apreq_env_handle_t *env,
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

static apr_status_t cgi_header_out(apreq_env_handle_t *env, const char *name,
                                   char *value)
{
    dP;
    apr_file_t *out;
    int bytes;
    apr_status_t s = apr_file_open_stdout(&out, p);
    apreq_log(APREQ_DEBUG s, env, "Setting header: %s => %s", name, value);
    bytes = apr_file_printf(out, "%s: %s" CRLF, name, value);
    apr_file_flush(out);
    return bytes > 0 ? APR_SUCCESS : APR_EGENERAL;
}


static apreq_jar_t *cgi_jar(apreq_env_handle_t *env, apreq_jar_t *jar)
{
    (void)env;

    if (jar != NULL) {
        apreq_jar_t *old_jar = ctx.jar;
        ctx.jar = jar;
        return old_jar;
    }

    return ctx.jar;
}

static apreq_request_t *cgi_request(apreq_env_handle_t *env,
                                    apreq_request_t *req)
{
    (void)env;

    if (req != NULL) {
        apreq_request_t *old_req = ctx.req;
        ctx.req = req;
        return old_req;
    }
    return ctx.req;
}


typedef struct {
    const char *t_name;
    int      t_val;
} TRANS;

static const TRANS priorities[] = {
    {"emerg",   APREQ_LOG_EMERG},
    {"alert",   APREQ_LOG_ALERT},
    {"crit",    APREQ_LOG_CRIT},
    {"error",   APREQ_LOG_ERR},
    {"warn",    APREQ_LOG_WARNING},
    {"notice",  APREQ_LOG_NOTICE},
    {"info",    APREQ_LOG_INFO},
    {"debug",   APREQ_LOG_DEBUG},
    {NULL,      -1},
};


static void cgi_log(const char *file, int line, int level,
                    apr_status_t status, apreq_env_handle_t *env,
                    const char *fmt, va_list vp)
{
    dP;
    char buf[256];
    char *log_level_string, *ra;
    const char *remote_addr;
    unsigned log_level = APREQ_LOG_WARNING;
    char date[APR_CTIME_LEN];
#ifndef WIN32
    apr_file_t *err;
#endif


    if (apr_env_get(&log_level_string, "LOG_LEVEL", p) == APR_SUCCESS)
        log_level = (log_level_string[0] - '0');

    level &= APREQ_LOG_LEVELMASK;

    if (level > (int)log_level)
        return;

    if (apr_env_get(&ra, "REMOTE_ADDR", p) == APR_SUCCESS)
        remote_addr = ra;
    else
        remote_addr = "address unavailable";

    apr_ctime(date, apr_time_now());

#ifndef WIN32

    apr_file_open_stderr(&err, p);
    apr_file_printf(err, "[%s] [%s] [%s] %s(%d): %s: %s\n",
                    date, priorities[level].t_name, remote_addr, file, line,
                    apr_strerror(status,buf,255),apr_pvsprintf(p,fmt,vp));
    apr_file_flush(err);

#else
    fprintf(stderr, "[%s] [%s] [%s] %s(%d): %s: %s\n",
            date, priorities[level].t_name, remote_addr, file, line,
            apr_strerror(status,buf,255),apr_pvsprintf(p,fmt,vp));
#endif

}

static apr_status_t cgi_read(apreq_env_handle_t *env,
                             apr_read_type_e block,
                             apr_off_t bytes)
{
    dP;
    apreq_request_t *req = apreq_request(env, NULL);
    apr_bucket *e;
    apr_status_t s;

    (void)block;

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

        if (ctx.max_body >= 0) {
            const char *cl = apreq_env_header_in(env, "Content-Length");
            if (cl != NULL) {
                char *dummy;
                apr_int64_t content_length = apr_strtoi64(cl,&dummy,0);

                if (dummy == NULL || *dummy != 0) {
                    apreq_log(APREQ_ERROR APR_EGENERAL, env,
                              "Invalid Content-Length header (%s)", cl);
                    ctx.status = APR_EGENERAL;
                    req->body_status = APR_EGENERAL;
                }
                else if (content_length > (apr_int64_t)ctx.max_body) {
                    apreq_log(APREQ_ERROR APR_EGENERAL, env,
                              "Content-Length header (%s) exceeds configured "
                              "max_body limit (%" APR_OFF_T_FMT ")",
                              cl, ctx.max_body);
                    ctx.status = APR_EGENERAL;
                    req->body_status = APR_EGENERAL;
                }
            }
        }
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
                req->body_status = APR_EGENERAL;
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
                req->body_status = APR_EGENERAL;
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


static const char *cgi_temp_dir(apreq_env_handle_t *env, const char *path)
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


static apr_off_t cgi_max_body(apreq_env_handle_t *env, apr_off_t bytes)
{
    (void)env;

    if (bytes >= 0) {
        apr_off_t rv = ctx.max_body;
        ctx.max_body = bytes;
        return rv;
    }
    return ctx.max_body;
}


static apr_ssize_t cgi_max_brigade(apreq_env_handle_t *env, apr_ssize_t bytes)
{
    (void)env;

    if (bytes >= 0) {
        apr_ssize_t rv = ctx.max_brigade;
        ctx.max_brigade = bytes;
        return rv;
    }
    return ctx.max_brigade;
}

APREQ_ENV_MODULE(cgi, APREQ_MODULE_NAME,
                 APREQ_MODULE_MAGIC_NUMBER);

APREQ_DECLARE(apreq_env_handle_t*) apreq_env_make_cgi(apr_pool_t *pool) {
    struct cgi_env *handle;

    handle = apr_pcalloc(pool, sizeof(*handle));
    handle->env.module = &cgi_module;
    handle->pool = pool;

    return &handle->env;
}