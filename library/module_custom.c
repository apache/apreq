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

#include "apr_strings.h"
#include "apreq_module.h"
#include "apreq_error.h"

#define READ_BYTES (64 * 1024)

struct custom_handle {
    struct apreq_handle_t    env;
    const char                  *cookie_header, *cookie2_header;

    apr_table_t                 *jar, *args, *body;
    apr_status_t                 jar_status,
                                 args_status,
                                 body_status;

    apreq_parser_t              *parser;

    apr_uint64_t                 read_limit;
    apr_uint64_t                 bytes_read;
    apr_bucket_brigade          *in;
};


static apr_status_t custom_parse_brigade(apreq_handle_t *env, apr_uint64_t bytes)
{       
    struct custom_handle *handle = (struct custom_handle*)env;
    apr_status_t s;
    apr_bucket *e;

    if (handle->body_status != APR_INCOMPLETE)
        return handle->body_status;

    switch (s = apr_brigade_partition(handle->in, bytes, &e)) {
        apr_bucket_brigade *bb;
        apr_uint64_t len;

    case APR_SUCCESS:
        bb = apr_brigade_split(handle->in, e);
        handle->bytes_read += bytes;

        if (handle->bytes_read > handle->read_limit) {
            handle->body_status = APREQ_ERROR_OVERLIMIT;
            break;
        }

        handle->body_status = 
            apreq_parser_run(handle->parser, handle->body, handle->in);

        apr_brigade_cleanup(handle->in);
        APR_BRIGADE_CONCAT(handle->in, bb);
        break;

    case APR_INCOMPLETE:
        bb = apr_brigade_split(handle->in, e);
        s = apr_brigade_length(handle->in, 1, &len);
        if (s != APR_SUCCESS) {
            handle->body_status = s;
            break;
        }
        handle->bytes_read += len;

        if (handle->bytes_read > handle->read_limit) {
            handle->body_status = APREQ_ERROR_OVERLIMIT;
            break;
        }
        handle->body_status = 
            apreq_parser_run(handle->parser, handle->body, handle->in);

        apr_brigade_cleanup(handle->in);
        APR_BRIGADE_CONCAT(handle->in, bb);
        break;

    default:
        handle->body_status = s;
    }

    return handle->body_status;
}



static apr_status_t custom_jar(apreq_handle_t *env, const apr_table_t **t)
{
    struct custom_handle *handle = (struct custom_handle*)env;
    *t = handle->jar;
    return handle->jar_status;
}

static apr_status_t custom_args(apreq_handle_t *env, const apr_table_t **t)
{
    struct custom_handle *handle = (struct custom_handle*)env;
    *t = handle->args;
    return handle->args_status;
}

static apr_status_t custom_body(apreq_handle_t *env, const apr_table_t **t)
{
    struct custom_handle *handle = (struct custom_handle*)env;
    while (handle->body_status == APR_INCOMPLETE)
        custom_parse_brigade(env, READ_BYTES);
    *t = handle->body;
    return handle->body_status;
}



static apreq_cookie_t *custom_jar_get(apreq_handle_t *env, const char *name)
{
    struct custom_handle *handle = (struct custom_handle*)env;
    const char *val;

    if (handle->jar == NULL || name == NULL)
        return NULL;

    val = apr_table_get(handle->jar, name);

    if (val == NULL)
        return NULL;

    return apreq_value_to_cookie(val);
}

static apreq_param_t *custom_args_get(apreq_handle_t *env, const char *name)
{
    struct custom_handle *handle = (struct custom_handle*)env;
    const char *val;

    if (handle->args == NULL || name == NULL)
        return NULL;

    val = apr_table_get(handle->args, name);

    if (val == NULL)
        return NULL;

    return apreq_value_to_param(val);
}

static apreq_param_t *custom_body_get(apreq_handle_t *env, const char *name)
{
    struct custom_handle *handle = (struct custom_handle*)env;
    const char *val;

    if (handle->body == NULL || name == NULL)
        return NULL;

 get_body_value:

    *(const char **)&val = apr_table_get(handle->body, name);
    if (val == NULL) {
        if (handle->body_status == APR_INCOMPLETE) {
            custom_parse_brigade(env, READ_BYTES);
            goto get_body_value;
        }
        else
            return NULL;
    }

    return apreq_value_to_param(val);
}



static apr_status_t custom_parser_get(apreq_handle_t *env,
                                      const apreq_parser_t **parser)
{
    struct custom_handle *handle = (struct custom_handle*)env;
    *parser = handle->parser;

    return APR_SUCCESS;
}

static apr_status_t custom_parser_set(apreq_handle_t *env,
                                      apreq_parser_t *parser)
{
    (void)env;
    (void)parser;
    return APR_ENOTIMPL;
}

static apr_status_t custom_hook_add(apreq_handle_t *env,
                                    apreq_hook_t *hook)
{
    struct custom_handle *handle = (struct custom_handle*)env;
    apreq_parser_add_hook(handle->parser, hook);
    return APR_SUCCESS;
}

static apr_status_t custom_brigade_limit_get(apreq_handle_t *env,
                                             apr_size_t *bytes)
{
    struct custom_handle *handle = (struct custom_handle*)env;
    *bytes = handle->parser->brigade_limit;
    return APR_SUCCESS;
}

static apr_status_t custom_brigade_limit_set(apreq_handle_t *env,
                                             apr_size_t bytes)
{
    (void)env;
    (void)bytes;
    return APR_ENOTIMPL;
}

static apr_status_t custom_read_limit_get(apreq_handle_t *env,
                                          apr_uint64_t *bytes)
{
    struct custom_handle *handle = (struct custom_handle*)env;
    *bytes = handle->read_limit;
    return APR_SUCCESS;
}

static apr_status_t custom_read_limit_set(apreq_handle_t *env,
                                          apr_uint64_t bytes)
{
    (void)env;
    (void)bytes;
    return APR_ENOTIMPL;
}

static apr_status_t custom_temp_dir_get(apreq_handle_t *env,
                                        const char **path)
{
    struct custom_handle *handle = (struct custom_handle*)env;

    *path = handle->parser->temp_dir;
    return APR_SUCCESS;
}

static apr_status_t custom_temp_dir_set(apreq_handle_t *env,
                                        const char *path)
{
    (void)env;
    (void)path;
    return APR_ENOTIMPL;
}




static const char *custom_header_in(apreq_handle_t *env,
                                    const char *name)
{
    struct custom_handle *handle = (struct custom_handle*)env;

    if (strcasecmp(name, "Content-Type") == 0)
        return handle->parser->content_type;
    else if (strcasecmp(name, "Cookie") == 0)
        return handle->cookie_header;
    else if (strcasecmp(name, "Cookie2") == 0)
        return handle->cookie2_header;
    else
        return NULL;
}

static apr_status_t custom_header_out(apreq_handle_t *env, const char *name,
                                      char *value)
{
    (void)env;
    (void)name;
    (void)value;

    return APR_ENOTIMPL;
}


static APREQ_MODULE(custom, 20050130);

APREQ_DECLARE(apreq_handle_t*) apreq_handle_custom(apr_pool_t *pool,
                                                       const char *query_string,
                                                       const char *cookie,
                                                       const char *cookie2,
                                                       apreq_parser_t *parser,
                                                       apr_uint64_t read_limit,
                                                       apr_bucket_brigade *in)
{
    struct custom_handle *handle;
    handle = apr_palloc(pool, sizeof(*handle));
    handle->env.module = &custom_module;
    handle->cookie_header = cookie;
    handle->cookie2_header = cookie2;
    handle->read_limit = read_limit;
    handle->parser = parser;
    handle->in = in;

    if (cookie != NULL) {
        handle->jar = apr_table_make(pool, APREQ_DEFAULT_NELTS);
        handle->jar_status = 
            apreq_parse_cookie_header(pool, handle->args, query_string);
    }
    else {
        handle->jar = NULL;
        handle->jar_status = APREQ_ERROR_NODATA;
    }


    if (query_string != NULL) {
        handle->args = apr_table_make(pool, APREQ_DEFAULT_NELTS);
        handle->args_status = 
            apreq_parse_query_string(pool, handle->args, query_string);
    }
    else {
        handle->args = NULL;
        handle->args_status = APREQ_ERROR_NODATA;
    }

    if (in != NULL) {
        handle->body = apr_table_make(pool, APREQ_DEFAULT_NELTS);
        handle->body_status = APR_INCOMPLETE;
    }
    else {
        handle->body = NULL;
        handle->body_status = APREQ_ERROR_NODATA;
    }

    return &handle->env;
}

