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

#include <apr_strings.h>

#include "apreq.h"
#include "apreq_env.h"

struct custom_handle {
    struct apreq_env_handle_t env;
    apr_pool_t *pool;
    const char *query_string;
    apreq_request_t *request;
    const char *content_type;
    const char *cookie_string, *cookie2_string;
    apreq_jar_t *jar;
    const char *temp_dir;
    apr_off_t max_body;
    apr_ssize_t max_brigade;

    /* body state */
    apr_status_t status;
    apr_off_t bytes_read;
    apr_bucket_brigade *in;
};

static apr_pool_t *custom_pool(apreq_env_handle_t *env) {
    struct custom_handle *handle = (struct custom_handle*)env;

    return handle->pool;
}

static apr_status_t bucket_alloc_cleanup(void *data)
{
    apr_bucket_alloc_t *ba = data;
    apr_bucket_alloc_destroy(ba);
    return APR_SUCCESS;
}

static apr_bucket_alloc_t *custom_bucket_alloc(apreq_env_handle_t *env)
{
    struct custom_handle *handle = (struct custom_handle*)env;
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(handle->pool);

    apr_pool_cleanup_register(handle->pool, ba,
                              bucket_alloc_cleanup,
                              bucket_alloc_cleanup);
    return ba;
}

static const char *custom_query_string(apreq_env_handle_t *env)
{
    struct custom_handle *handle = (struct custom_handle*)env;

    return handle->query_string;
}

static const char *custom_header_in(apreq_env_handle_t *env,
                                    const char *name)
{
    struct custom_handle *handle = (struct custom_handle*)env;

    if (strcasecmp(name, "Content-Type") == 0)
        return handle->content_type;
    else if (strcasecmp(name, "Cookie") == 0)
        return handle->cookie_string;
    else if (strcasecmp(name, "Cookie2") == 0)
        return handle->cookie2_string;
    else
        return NULL;
}

static apr_status_t custom_header_out(apreq_env_handle_t *env, const char *name,
                                      char *value)
{
    (void)env;
    (void)name;
    (void)value;

    return APR_SUCCESS;
}

static apreq_jar_t *custom_jar(apreq_env_handle_t *env, apreq_jar_t *jar)
{
    struct custom_handle *handle = (struct custom_handle*)env;
    apreq_jar_t *value = handle->jar;

    if (jar != NULL)
        handle->jar = jar;

    return value;
}

static apreq_request_t *custom_request(apreq_env_handle_t *env,
                                       apreq_request_t *req)
{
    struct custom_handle *handle = (struct custom_handle*)env;
    apreq_request_t *value = handle->request;

    if (req != NULL)
        handle->request = req;

    return value;
}

static void custom_log(const char *file, int line, int level,
                       apr_status_t status, apreq_env_handle_t *env,
                       const char *fmt, va_list vp)
{
    (void)file;
    (void)line;
    (void)level;
    (void)status;
    (void)env;
    (void)fmt;
    (void)vp;
}

static apr_status_t custom_read(apreq_env_handle_t *env,
                                apr_read_type_e block,
                                apr_off_t bytes)
{
    struct custom_handle *handle = (struct custom_handle*)env;
    apr_bucket *e;

    (void)block;

    if (handle->status != APR_INCOMPLETE)
        return handle->status;

    handle->status = apr_brigade_partition(handle->in, bytes, &e);
    switch (handle->status) {
        apr_bucket_brigade *bb;
        apr_off_t len;
        apreq_request_t *req;

    case APR_SUCCESS:
        bb = apr_brigade_split(handle->in, e);
        req = apreq_request(env, NULL);
        if (handle->max_body >= 0 &&
            handle->bytes_read + bytes > handle->max_body) {
            apreq_log(APREQ_ERROR APR_EGENERAL, env,
                      "Bytes read (%" APR_OFF_T_FMT
                      ") exceeds configured limit (%" APR_OFF_T_FMT ")",
                      handle->bytes_read, handle->max_body);
            req->body_status = APR_EGENERAL;
            return handle->status = APR_EGENERAL;
        }
        handle->bytes_read += bytes;
        handle->status = apreq_parse_request(req, handle->in);
        apr_brigade_cleanup(handle->in);
        APR_BRIGADE_CONCAT(handle->in, bb);
        break;

    case APR_INCOMPLETE:
        bb = apr_brigade_split(handle->in, e);
        handle->status = apr_brigade_length(handle->in,1,&len);
        if (handle->status != APR_SUCCESS)
            return handle->status;
        req = apreq_request(env, NULL);
        if (handle->max_body >= 0 &&
            handle->bytes_read + len > handle->max_body) {
            apreq_log(APREQ_ERROR APR_EGENERAL, env,
                      "Bytes read (%" APR_OFF_T_FMT
                      ") exceeds configured limit (%" APR_OFF_T_FMT ")",
                      handle->bytes_read, handle->max_body);
            req->body_status = APR_EGENERAL;
            return handle->status = APR_EGENERAL;
        }
        handle->bytes_read += len;
        handle->status = apreq_parse_request(req, handle->in);
        apr_brigade_cleanup(handle->in);
        APR_BRIGADE_CONCAT(handle->in, bb);
        break;
    }

    return handle->status;
}

static const char *custom_temp_dir(apreq_env_handle_t *env, const char *path)
{
    struct custom_handle *handle = (struct custom_handle*)env;
    const char *value = handle->temp_dir;

    if (path != NULL)
        handle->temp_dir = apr_pstrdup(handle->pool, path);

    if (handle->temp_dir == NULL) {
        apr_status_t status;

        status = apr_temp_dir_get(&value, handle->pool);
        if (status != APR_SUCCESS)
            value = NULL;

        handle->temp_dir = value;
    }

    return value;
}

static apr_off_t custom_max_body(apreq_env_handle_t *env, apr_off_t bytes)
{
    struct custom_handle *handle = (struct custom_handle*)env;
    apr_off_t value = handle->max_body;

    if (bytes >= 0)
        handle->max_body = bytes;

    return value;
}

static apr_ssize_t custom_max_brigade(apreq_env_handle_t *env, apr_ssize_t bytes)
{
    struct custom_handle *handle = (struct custom_handle*)env;
    apr_off_t value = handle->max_brigade;

    if (bytes >= 0)
        handle->max_brigade = bytes;

    return value;
}

static APREQ_ENV_MODULE(custom, "Custom", 20050110);

APREQ_DECLARE(apreq_env_handle_t*) apreq_env_make_custom(apr_pool_t *pool,
                                                         const char *query_string,
                                                         const char *cookie,
                                                         const char *cookie2,
                                                         const char *content_type,
                                                         apr_bucket_brigade *in) {
    struct custom_handle *handle;

    handle = apr_pcalloc(pool, sizeof(*handle));
    handle->env.module = &custom_module;
    handle->pool = pool;
    handle->max_body = -1;
    handle->max_brigade = -1;

    if (query_string != NULL)
        handle->query_string = apr_pstrdup(pool, query_string);
    if (content_type != NULL)
        handle->content_type = apr_pstrdup(pool, content_type);
    if (cookie != NULL)
        handle->cookie_string = apr_pstrdup(pool, cookie);
    if (cookie2 != NULL)
        handle->cookie2_string = apr_pstrdup(pool, cookie2);

    handle->in = in;
    handle->status = handle->in == NULL ? APR_ENOTIMPL : APR_INCOMPLETE;

    return &handle->env;
}
