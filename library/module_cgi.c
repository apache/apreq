/*
**  Copyright 2003-2005  The Apache Software Foundation
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
#include <assert.h>

#include "apreq_module.h"
#include "apreq_error.h"
#include "apr_strings.h"
#include "apr_lib.h"
#include "apr_env.h"
#include "apreq_util.h"

#define USER_DATA_KEY "apreq"

/* Parroting APLOG_* ... */

#define	CGILOG_EMERG	0	/* system is unusable */
#define	CGILOG_ALERT	1	/* action must be taken immediately */
#define	CGILOG_CRIT	2	/* critical conditions */
#define	CGILOG_ERR	3	/* error conditions */
#define	CGILOG_WARNING	4	/* warning conditions */
#define	CGILOG_NOTICE	5	/* normal but significant condition */
#define	CGILOG_INFO	6	/* informational */
#define	CGILOG_DEBUG	7	/* debug-level messages */

#define CGILOG_LEVELMASK 7
#define CGILOG_MARK     __FILE__, __LINE__




struct cgi_handle {
    struct apreq_handle_t    env;

    apr_pool_t                  *pool;
    apr_bucket_alloc_t          *bucket_alloc;

    apr_table_t                 *jar, *args, *body;
    apr_status_t                 jar_status,
                                 args_status,
                                 body_status;

    apreq_parser_t              *parser;
    apreq_hook_t                *hook_queue;
    apreq_hook_t                *find_param;

    const char                  *temp_dir;
    apr_size_t                   brigade_limit;
    apr_uint64_t                 read_limit;
    apr_uint64_t                 bytes_read;

    apr_bucket_brigade          *in;
    apr_bucket_brigade          *tmpbb;

};

#define CRLF "\015\012"

typedef struct {
    const char *t_name;
    int      t_val;
} TRANS;
 
static const TRANS priorities[] = {
    {"emerg",   CGILOG_EMERG},
    {"alert",   CGILOG_ALERT},
    {"crit",    CGILOG_CRIT},
    {"error",   CGILOG_ERR},
    {"warn",    CGILOG_WARNING},
    {"notice",  CGILOG_NOTICE},
    {"info",    CGILOG_INFO},
    {"debug",   CGILOG_DEBUG},
    {NULL,      -1},
};
 
static const char *cgi_header_in(apreq_handle_t *env,
                                 const char *name)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;
    apr_pool_t *p = handle->pool;
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

    apr_env_get(&value, key, p);

    return value;
}




static void cgi_log_error(const char *file, int line, int level,
                          apr_status_t status, apreq_handle_t *env,
                          const char *fmt, ...)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;
    apr_pool_t *p = handle->pool;
    char buf[256];
    char *log_level_string, *ra;
    const char *remote_addr;
    unsigned log_level = CGILOG_WARNING;
    char date[APR_CTIME_LEN];
    va_list vp;
#ifndef WIN32
    apr_file_t *err;
#endif

    va_start(vp, fmt);

    if (apr_env_get(&log_level_string, "LOG_LEVEL", p) == APR_SUCCESS)
        log_level = (log_level_string[0] - '0');

    level &= CGILOG_LEVELMASK;

    if (level < (int)log_level) {

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

    va_end(vp);

}

static apr_status_t cgi_header_out(apreq_handle_t *env, const char *name,
                                   char *value)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;
    apr_pool_t *p = handle->pool;
    apr_file_t *out;
    int bytes;
    apr_status_t s = apr_file_open_stdout(&out, p);
    cgi_log_error(CGILOG_MARK, CGILOG_DEBUG, s, env, 
                  "Setting header: %s => %s", name, value);
    bytes = apr_file_printf(out, "%s: %s" CRLF, name, value);
    apr_file_flush(out);
    return bytes > 0 ? APR_SUCCESS : APREQ_ERROR_GENERAL;
}


APR_INLINE
static const char *cgi_query_string(apreq_handle_t *env)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;
    char *value = NULL, qs[] = "QUERY_STRING";
    apr_env_get(&value, qs, handle->pool);
    return value;
}


static void init_body(apreq_handle_t *env)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;
    const char *cl_header = cgi_header_in(env, "Content-Length");
    apr_bucket_alloc_t *ba = handle->bucket_alloc;
    apr_pool_t *pool = handle->pool;
    apr_file_t *file;
    apr_bucket *eos, *pipe;

    handle->body  = apr_table_make(pool, APREQ_DEFAULT_NELTS);

    if (cl_header != NULL) {
        char *dummy;
        apr_int64_t content_length = apr_strtoi64(cl_header, &dummy, 0);

        if (dummy == NULL || *dummy != 0) {
            handle->body_status = APREQ_ERROR_BADHEADER;
            cgi_log_error(CGILOG_MARK, CGILOG_ERR, handle->body_status, env,
                          "Invalid Content-Length header (%s)", cl_header);
            return;
        }
        else if ((apr_uint64_t)content_length > handle->read_limit) {
            handle->body_status = APREQ_ERROR_OVERLIMIT;
            cgi_log_error(CGILOG_MARK, CGILOG_ERR, handle->body_status, env,
                          "Content-Length header (%s) exceeds configured "
                          "max_body limit (%" APR_UINT64_T_FMT ")", 
                          cl_header, handle->read_limit);
            return;
        }
    }

    if (handle->parser == NULL) {
        const char *ct_header = cgi_header_in(env, "Content-Type");

        if (ct_header != NULL) {
            apreq_parser_function_t pf = apreq_parser(ct_header);

            if (pf != NULL) {
                handle->parser = apreq_parser_make(handle->pool,
                                                   ba,
                                                   ct_header, 
                                                   pf,
                                                   handle->brigade_limit,
                                                   handle->temp_dir,
                                                   handle->hook_queue,
                                                   NULL);
            }
            else {
                handle->body_status = APREQ_ERROR_NOPARSER;
                return;
            }
        }
        else {
            handle->body_status = APREQ_ERROR_NOHEADER;
            return;
        }
    }
    else {
        if (handle->parser->brigade_limit > handle->brigade_limit)
            handle->parser->brigade_limit = handle->brigade_limit;
        if (handle->temp_dir != NULL)
            handle->parser->temp_dir = handle->temp_dir;
        if (handle->hook_queue != NULL)
            apreq_parser_add_hook(handle->parser, handle->hook_queue);
    }

    handle->hook_queue = NULL;
    handle->in         = apr_brigade_create(pool, ba);
    handle->tmpbb      = apr_brigade_create(pool, ba);

    apr_file_open_stdin(&file, pool); // error status?    
    pipe = apr_bucket_pipe_create(file, ba);
    eos = apr_bucket_eos_create(ba);
    APR_BRIGADE_INSERT_HEAD(handle->in, pipe);
    APR_BRIGADE_INSERT_TAIL(handle->in, eos);

    handle->body_status = APR_INCOMPLETE;

}

static apr_status_t cgi_read(apreq_handle_t *env,
                             apr_off_t bytes)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;
    apr_bucket *e;
    apr_status_t s;

    if (handle->body_status == APR_EINIT)
        init_body(env);

    if (handle->body_status != APR_INCOMPLETE)
        return handle->body_status;


    switch (s = apr_brigade_partition(handle->in, bytes, &e)) {
        apr_off_t len;

    case APR_SUCCESS:

        apreq_brigade_move(handle->tmpbb, handle->in, e);
        handle->bytes_read += bytes;

        if (handle->bytes_read > handle->read_limit) {
            handle->body_status = APREQ_ERROR_OVERLIMIT;
            cgi_log_error(CGILOG_MARK, CGILOG_ERR, handle->body_status,
                          env, "Bytes read (%" APR_UINT64_T_FMT
                          ") exceeds configured limit (%" APR_UINT64_T_FMT ")",
                          handle->bytes_read, handle->read_limit);
            break;
        }

        handle->body_status =
            apreq_parser_run(handle->parser, handle->body, handle->tmpbb);
        apr_brigade_cleanup(handle->tmpbb);
        break;


    case APR_INCOMPLETE:

        apreq_brigade_move(handle->tmpbb, handle->in, e);
        s = apr_brigade_length(handle->tmpbb, 1, &len);

        if (s != APR_SUCCESS) {
            handle->body_status = s;
            break;
        }
        handle->bytes_read += len;

        if (handle->bytes_read > handle->read_limit) {
            handle->body_status = APREQ_ERROR_OVERLIMIT;
            cgi_log_error(CGILOG_MARK, CGILOG_ERR, handle->body_status, env,
                          "Bytes read (%" APR_UINT64_T_FMT
                          ") exceeds configured limit (%" APR_UINT64_T_FMT ")",
                          handle->bytes_read, handle->read_limit);
            
            break;
        }

        handle->body_status =
            apreq_parser_run(handle->parser, handle->body, handle->tmpbb);
        apr_brigade_cleanup(handle->tmpbb);
        break;

    default:
        handle->body_status = s;
    }

    return handle->body_status;
}



static apr_status_t cgi_jar(apreq_handle_t *env,
                            const apr_table_t **t)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;

    if (handle->jar_status == APR_EINIT) {
        const char *cookies = cgi_header_in(env, "Cookie");
        if (cookies != NULL) {
            handle->jar = apr_table_make(handle->pool, APREQ_DEFAULT_NELTS);
            handle->jar_status = 
                apreq_parse_cookie_header(handle->pool, handle->jar, cookies);
        }
        else
            handle->jar_status = APREQ_ERROR_NODATA;
    }

    *t = handle->jar;
    return handle->jar_status;
}

static apr_status_t cgi_args(apreq_handle_t *env,
                             const apr_table_t **t)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;

    if (handle->args_status == APR_EINIT) {
        const char *query_string = cgi_query_string(env);
        if (query_string != NULL) {
            handle->args = apr_table_make(handle->pool, APREQ_DEFAULT_NELTS);
            handle->args_status = 
                apreq_parse_query_string(handle->pool, handle->args, query_string);
        }
        else
            handle->args_status = APREQ_ERROR_NODATA;
    }

    *t = handle->args;
    return handle->args_status;
}




static apreq_cookie_t *cgi_jar_get(apreq_handle_t *env,
                                   const char *name)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;
    const apr_table_t *t;
    const char *val;

    if (handle->jar_status == APR_EINIT)
        cgi_jar(env, &t);
    else
        t = handle->jar;

    if (t == NULL)
        return NULL;

    val = apr_table_get(t, name);
    if (val == NULL)
        return NULL;

    return apreq_value_to_cookie(val);
}

static apreq_param_t *cgi_args_get(apreq_handle_t *env,
                                   const char *name)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;
    const apr_table_t *t;
    const char *val;

    if (handle->args_status == APR_EINIT)
        cgi_args(env, &t);
    else
        t = handle->args;

    if (t == NULL)
        return NULL;

    val = apr_table_get(t, name);
    if (val == NULL)
        return NULL;

    return apreq_value_to_param(val);
}



static apr_status_t cgi_body(apreq_handle_t *env,
                             const apr_table_t **t)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;

    switch (handle->body_status) {

    case APR_EINIT:
        init_body(env);
        if (handle->body_status != APR_INCOMPLETE)
            break;

    case APR_INCOMPLETE:
        while (cgi_read(env, APREQ_DEFAULT_READ_BLOCK_SIZE) == APR_INCOMPLETE)
            ;   /*loop*/
    }

    *t = handle->body;
    return handle->body_status;
}

static apreq_param_t *cgi_body_get(apreq_handle_t *env, 
                                   const char *name)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;
    const char *val;
    apreq_hook_t *h;

    switch (handle->body_status) {

    case APR_SUCCESS:

        val = apr_table_get(handle->body, name);
        if (val != NULL)
            return apreq_value_to_param(val);
        return NULL;


    case APR_EINIT:

        init_body(env);
        if (handle->body_status != APR_INCOMPLETE)
            return NULL;
        cgi_read(env, APREQ_DEFAULT_READ_BLOCK_SIZE);


    case APR_INCOMPLETE:

        val = apr_table_get(handle->body, name);
        if (val != NULL)
            return apreq_value_to_param(val);

        /* Not seen yet, so we need to scan for 
           param while prefetching the body */

        if (handle->find_param == NULL)
            handle->find_param = apreq_hook_make(handle->pool, 
                                                 apreq_hook_find_param, 
                                                 NULL, NULL);
        h = handle->find_param;
        h->next = handle->parser->hook;
        handle->parser->hook = h;
        *(const char **)&h->ctx = name;

        do {
            cgi_read(env, APREQ_DEFAULT_READ_BLOCK_SIZE);
            if (h->ctx != name) {
                handle->parser->hook = h->next;
                return h->ctx;
            }
        } while (handle->body_status == APR_INCOMPLETE);

        handle->parser->hook = h->next;
        return NULL;


    default:

        if (handle->body == NULL)
            return NULL;

        val = apr_table_get(handle->body, name);
        if (val != NULL)
            return apreq_value_to_param(val);
        return NULL;
    }

    /* not reached */
    return NULL;
}

static apr_status_t cgi_parser_get(apreq_handle_t *env, 
                                   const apreq_parser_t **parser)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;

    *parser = handle->parser;
    return APR_SUCCESS;
}

static apr_status_t cgi_parser_set(apreq_handle_t *env, 
                                   apreq_parser_t *parser)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;

    if (handle->parser == NULL) {

        if (handle->hook_queue != NULL) {
            apr_status_t s = apreq_parser_add_hook(parser, handle->hook_queue);
            if (s != APR_SUCCESS)
                return s;
        }
        if (handle->temp_dir != NULL) {
            parser->temp_dir = handle->temp_dir;
        }
        if (handle->brigade_limit < parser->brigade_limit) {
            parser->brigade_limit = handle->brigade_limit;
        }

        handle->hook_queue = NULL;
        handle->parser = parser;
        return APR_SUCCESS;
    }
    else
        return APREQ_ERROR_MISMATCH;
}


static apr_status_t cgi_hook_add(apreq_handle_t *env,
                                     apreq_hook_t *hook)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;

    if (handle->parser != NULL) {
        return apreq_parser_add_hook(handle->parser, hook);
    }
    else if (handle->hook_queue != NULL) {
        apreq_hook_t *h = handle->hook_queue;
        while (h->next != NULL)
            h = h->next;
        h->next = hook;
    }
    else {
        handle->hook_queue = hook;
    }
    return APR_SUCCESS;

}

static apr_status_t cgi_brigade_limit_set(apreq_handle_t *env,
                                          apr_size_t bytes)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;
    apr_size_t *limit = (handle->parser == NULL) 
                      ? &handle->brigade_limit 
                      : &handle->parser->brigade_limit;

    if (*limit > bytes) {
        *limit = bytes;
        return APR_SUCCESS;
    }

    return APREQ_ERROR_MISMATCH;
}

static apr_status_t cgi_brigade_limit_get(apreq_handle_t *env,
                                          apr_size_t *bytes)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;
    *bytes = (handle->parser == NULL) 
           ?  handle->brigade_limit 
           :  handle->parser->brigade_limit;

    return APR_SUCCESS;
}

static apr_status_t cgi_read_limit_set(apreq_handle_t *env,
                                       apr_uint64_t bytes)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;

    if (handle->read_limit > bytes && handle->bytes_read < bytes) {
        handle->read_limit = bytes;
        return APR_SUCCESS;
    }

    return APREQ_ERROR_MISMATCH;
}


static apr_status_t cgi_read_limit_get(apreq_handle_t *env,
                                       apr_uint64_t *bytes)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;
    *bytes = handle->read_limit;
    return APR_SUCCESS;
}


static apr_status_t cgi_temp_dir_set(apreq_handle_t *env,
                                     const char *path)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;
    const char **temp_dir = (handle->parser == NULL) 
                          ? &handle->temp_dir
                          : &handle->parser->temp_dir;


    if (*temp_dir == NULL && handle->bytes_read == 0) {
        if (path != NULL)
            *temp_dir = apr_pstrdup(handle->pool, path);
        return APR_SUCCESS;
    }

    return APREQ_ERROR_MISMATCH;
}


static apr_status_t cgi_temp_dir_get(apreq_handle_t *env,
                                     const char **path)
{
    struct cgi_handle *handle = (struct cgi_handle *)env;
    *path = (handle->parser == NULL)
           ?  handle->temp_dir 
           :  handle->parser->temp_dir;
    return APR_SUCCESS;
}



#ifdef APR_POOL_DEBUG
static apr_status_t ba_cleanup(void *data)
{
    apr_bucket_alloc_t *ba = data;
    apr_bucket_alloc_destroy(ba);
    return APR_SUCCESS;
}
#endif

static APREQ_MODULE(cgi, 20050312);

APREQ_DECLARE(apreq_handle_t *)apreq_handle_cgi(apr_pool_t *pool)
{
    apr_bucket_alloc_t *ba;
    struct cgi_handle *handle;
    void *data;
    
    apr_pool_userdata_get(&data, USER_DATA_KEY, pool);

    if (data != NULL)
        return data;

    handle = apr_pcalloc(pool, sizeof *handle);
    ba = apr_bucket_alloc_create(pool);

    /* check pool's userdata first. */

    handle->env.module    = &cgi_module;
    handle->pool          = pool;
    handle->bucket_alloc  = ba;
    handle->read_limit    = (apr_uint64_t) -1;
    handle->brigade_limit = APREQ_DEFAULT_BRIGADE_LIMIT;

    handle->args_status = 
        handle->jar_status = 
            handle->body_status = APR_EINIT;

    apr_pool_userdata_setn(&handle->env, USER_DATA_KEY, NULL, pool);

#ifdef APR_POOL_DEBUG
    apr_pool_cleanup_register(pool, ba, ba_cleanup, ba_cleanup);
#endif

    return &handle->env;
}
