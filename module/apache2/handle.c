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

#include "assert.h"

#include "httpd.h"
#include "http_config.h"
#include "http_log.h"
#include "util_filter.h"
#include "apr_tables.h"
#include "apr_buckets.h"
#include "http_request.h"
#include "apr_strings.h"

#include "apreq_module_apache2.h"
#include "apreq_private_apache2.h"
#include "apreq_error.h"


APR_INLINE
static ap_filter_t *get_apreq_filter(apreq_handle_t *env)
{
    struct apache2_handle *handle = (struct apache2_handle *)env;

    if (handle->f == NULL) {
        handle->f = ap_add_input_filter(APREQ_FILTER_NAME, NULL, 
                                        handle->r, 
                                        handle->r->connection);
        /* ap_add_input_filter does not guarantee cfg->f == r->input_filters,
         * so we reposition the new filter there as necessary.
         */
        apreq_filter_relocate(handle->f); 
    }

    return handle->f;
}


static apr_status_t apache2_jar(apreq_handle_t *env, const apr_table_t **t)
{
    struct apache2_handle *handle = (struct apache2_handle*)env;
    request_rec *r = handle->r;

    if (handle->jar_status == APR_EINIT) {
        const char *cookies = apr_table_get(r->headers_in, "Cookie");
        if (cookies != NULL) {
            handle->jar = apr_table_make(handle->r->pool, APREQ_DEFAULT_NELTS);
            handle->jar_status = 
                apreq_parse_cookie_header(r->pool, handle->jar, cookies);
        }
        else
            handle->jar_status = APREQ_ERROR_NODATA;
    }

    *t = handle->jar;
    return handle->jar_status;
}

static apr_status_t apache2_args(apreq_handle_t *env, const apr_table_t **t)
{
    struct apache2_handle *handle = (struct apache2_handle*)env;
    request_rec *r = handle->r;

    if (handle->args_status == APR_EINIT) {
        if (r->args != NULL) {
            handle->args = apr_table_make(handle->r->pool, APREQ_DEFAULT_NELTS);
            handle->args_status = 
                apreq_parse_query_string(r->pool, handle->args, r->args);
        }
        else
            handle->args_status = APREQ_ERROR_NODATA;
    }

    *t = handle->args;
    return handle->args_status;
}




static apreq_cookie_t *apache2_jar_get(apreq_handle_t *env, const char *name)
{
    struct apache2_handle *handle = (struct apache2_handle *)env;
    const apr_table_t *t;
    const char *val;

    if (handle->jar_status == APR_EINIT)
        apache2_jar(env, &t);
    else
        t = handle->jar;

    if (t == NULL)
        return NULL;

    val = apr_table_get(t, name);
    if (val == NULL)
        return NULL;

    return apreq_value_to_cookie(val);
}

static apreq_param_t *apache2_args_get(apreq_handle_t *env, const char *name)
{
    struct apache2_handle *handle = (struct apache2_handle *)env;
    const apr_table_t *t;
    const char *val;

    if (handle->args_status == APR_EINIT)
        apache2_args(env, &t);
    else
        t = handle->args;

    if (t == NULL)
        return NULL;

    val = apr_table_get(t, name);
    if (val == NULL)
        return NULL;

    return apreq_value_to_param(val);
}


static const char *apache2_header_in(apreq_handle_t *env, const char *name)
{
    struct apache2_handle *handle = (struct apache2_handle *)env;
    return apr_table_get(handle->r->headers_in, name);
}

static apr_status_t apache2_header_out(apreq_handle_t *env,
                                       const char *name, char *value)
{
    struct apache2_handle *handle = (struct apache2_handle *)env;
    apr_table_add(handle->r->err_headers_out, name, value);
    return APR_SUCCESS;
}

static apr_status_t apache2_body(apreq_handle_t *req, const apr_table_t **t)
{
    ap_filter_t *f = get_apreq_filter(req);
    struct filter_ctx *ctx;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;

    switch (ctx->body_status) {

    case APR_EINIT:
        apreq_filter_init_context(f);
        if (ctx->body_status != APR_INCOMPLETE)
            break;

    case APR_INCOMPLETE:
        while (apreq_filter_prefetch(f, APREQ_DEFAULT_READ_BLOCK_SIZE) == APR_INCOMPLETE)
            ;   /*loop*/
    }

    *t = ctx->body;
    return ctx->body_status;
}

static apreq_param_t *apache2_body_get(apreq_handle_t *env, const char *name)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;
    const char *val;
    apreq_hook_t *h;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;

    switch (ctx->body_status) {

    case APR_EINIT:

        apreq_filter_init_context(f);
        if (ctx->body_status != APR_INCOMPLETE)
            return NULL;
        apreq_filter_prefetch(f, APREQ_DEFAULT_READ_BLOCK_SIZE);


    case APR_INCOMPLETE:

        val = apr_table_get(ctx->body, name);
        if (val != NULL)
            return apreq_value_to_param(val);

        /* Not seen yet, so we need to scan for 
           param while prefetching the body */

        if (ctx->find_param == NULL)
            ctx->find_param = apreq_hook_make(f->r->pool, 
                                              apreq_hook_find_param, 
                                              NULL, NULL);
        h = ctx->find_param;
        h->next = ctx->parser->hook;
        ctx->parser->hook = h;
        *(const char **)&h->ctx = name;

        do {
            apreq_filter_prefetch(f, APREQ_DEFAULT_READ_BLOCK_SIZE);
            if (h->ctx != name) {
                ctx->parser->hook = h->next;
                return h->ctx;
            }
        } while (ctx->body_status == APR_INCOMPLETE);

        ctx->parser->hook = h->next;
        return NULL;


    case APR_SUCCESS:

        val = apr_table_get(ctx->body, name);
        if (val != NULL)
            return apreq_value_to_param(val);
        return NULL;

    default:

        if (ctx->body == NULL)
            return NULL;

        val = apr_table_get(ctx->body, name);
        if (val != NULL)
            return apreq_value_to_param(val);
        return NULL;

    }

    /* not reached */
    return NULL;
}

static
apr_status_t apache2_parser_get(apreq_handle_t *env, 
                                  const apreq_parser_t **parser)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx = f->ctx;

    if (ctx == NULL) {
        *parser = NULL;
        return APR_EINIT;
    }
    *parser = ctx->parser;
    return APR_SUCCESS;
}

static
apr_status_t apache2_parser_set(apreq_handle_t *env, 
                                apreq_parser_t *parser)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;

    if (ctx->parser == NULL) {
        ctx->parser = parser;
        return APR_SUCCESS;
    }
    else
        return APREQ_ERROR_NOTEMPTY;
}



static
apr_status_t apache2_hook_add(apreq_handle_t *env,
                              apreq_hook_t *hook)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;

    if (ctx->parser != NULL) {
        return apreq_parser_add_hook(ctx->parser, hook);
    }
    else if (ctx->hook_queue != NULL) {
        apreq_hook_t *h = ctx->hook_queue;
        while (h->next != NULL)
            h = h->next;
        h->next = hook;
    }
    else {
        ctx->hook_queue = hook;
    }
    return APR_SUCCESS;

}

static
apr_status_t apache2_brigade_limit_set(apreq_handle_t *env,
                                       apr_size_t bytes)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;

    if (ctx->body_status == APR_EINIT || ctx->brigade_limit > bytes) {
        ctx->brigade_limit = bytes;
        return APR_SUCCESS;
    }

    return APREQ_ERROR_MISMATCH;
}

static
apr_status_t apache2_brigade_limit_get(apreq_handle_t *env,
                                       apr_size_t *bytes)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;
    *bytes = ctx->brigade_limit;
    return APR_SUCCESS;
}

static
apr_status_t apache2_read_limit_set(apreq_handle_t *env,
                                    apr_uint64_t bytes)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;

    if (ctx->read_limit > bytes && ctx->bytes_read < bytes) {
        ctx->read_limit = bytes;
        return APR_SUCCESS;
    }

    return APREQ_ERROR_MISMATCH;
}

static
apr_status_t apache2_read_limit_get(apreq_handle_t *env,
                                    apr_uint64_t *bytes)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;
    *bytes = ctx->read_limit;
    return APR_SUCCESS;
}

static
apr_status_t apache2_temp_dir_set(apreq_handle_t *env,
                                  const char *path)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;
    // init vs incomplete state?
    if (ctx->temp_dir == NULL && ctx->bytes_read == 0) {
        if (path != NULL)
            ctx->temp_dir = apr_pstrdup(f->r->pool, path);
        return APR_SUCCESS;
    }

    return APREQ_ERROR_NOTEMPTY;
}

static
apr_status_t apache2_temp_dir_get(apreq_handle_t *env,
                                  const char **path)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;
    *path = ctx->parser ? ctx->parser->temp_dir : ctx->temp_dir;
    return APR_SUCCESS;
}

static APREQ_MODULE(apache2, 20050315);

APREQ_DECLARE(apreq_handle_t *) apreq_handle_apache2(request_rec *r)
{
    struct apache2_handle *handle =
        ap_get_module_config(r->request_config, &apreq_module);

    if (handle != NULL) {
        get_apreq_filter(&handle->env);
        return &handle->env;
    }

    handle = apr_palloc(r->pool, sizeof *handle);
    ap_set_module_config(r->request_config, &apreq_module, handle);

    handle->env.module = &apache2_module;
    handle->r = r;

    handle->args_status = handle->jar_status = APR_EINIT;
    handle->args = handle->jar = NULL;

    handle->f = NULL;

    get_apreq_filter(&handle->env);
    return &handle->env;

}
