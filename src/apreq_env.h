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

#ifndef APREQ_ENV_H
#define APREQ_ENV_H

#include "apreq_params.h"
#include "apreq_cookie.h"
#include "apreq_parsers.h"

#ifdef  __cplusplus
 extern "C" {
#endif 

/**
 * An apreq environment, associated with an env module. The structure
 * may have variable size, because the module may append its own data
 * structures after it.
 */
typedef struct apreq_env_handle_t {
    const struct apreq_env_module_t *module;
} apreq_env_handle_t;

/**
 * This must be fully defined for libapreq2 to operate properly 
 * in a given environment. Normally it is set once, with an apreq_env_module() 
 * call during process initialization, and should remain fixed thereafter.
 * @brief Vtable describing the necessary environment functions.
 */


typedef struct apreq_env_module_t {
    const char *name;
    apr_uint32_t magic_number;

    apr_status_t (*jar)(apreq_env_handle_t *, const apr_table_t **);
    apr_status_t (*args)(apreq_env_handle_t *, const apr_table_t **);
    apr_status_t (*body)(apreq_env_handle_t *, const apr_table_t **);

    apreq_cookie_t *(*jar_get)(apreq_env_handle_t *, const char *);
    apreq_param_t *(*args_get)(apreq_env_handle_t *, const char *);
    apreq_param_t *(*body_get)(apreq_env_handle_t *, const char *);

    apr_status_t (*parser_get)(apreq_env_handle_t *, const apreq_parser_t **);
    apr_status_t (*parser_set)(apreq_env_handle_t *, apreq_parser_t *);
    apr_status_t (*hook_add)(apreq_env_handle_t *, apreq_hook_t *);

    apr_status_t (*brigade_limit_get)(apreq_env_handle_t *, apr_size_t *);
    apr_status_t (*brigade_limit_set)(apreq_env_handle_t *, apr_size_t);

    apr_status_t (*read_limit_get)(apreq_env_handle_t *, apr_uint64_t *);
    apr_status_t (*read_limit_set)(apreq_env_handle_t *, apr_uint64_t);

    apr_status_t (*temp_dir_get)(apreq_env_handle_t *, const char **);
    apr_status_t (*temp_dir_set)(apreq_env_handle_t *, const char *);

    const char *(*header_in)(apreq_env_handle_t *,const char *);
    apr_status_t (*header_out)(apreq_env_handle_t *, const char *,char *);

} apreq_env_module_t;



static APR_INLINE
apr_status_t apreq_jar(apreq_env_handle_t *env, const apr_table_t **t)
{
    return env->module->jar(env,t);
}

static APR_INLINE
apr_status_t apreq_args(apreq_env_handle_t *env, const apr_table_t **t)
{
    return env->module->args(env,t);
}

static APR_INLINE
apr_status_t apreq_body(apreq_env_handle_t *env, const apr_table_t **t)
{
    return env->module->body(env,t);
}

static APR_INLINE
apreq_cookie_t *apreq_jar_get(apreq_env_handle_t *env, const char *name)
{
    return env->module->jar_get(env, name);
}

static APR_INLINE
apreq_param_t *apreq_args_get(apreq_env_handle_t *env, const char *name)
{
    return env->module->args_get(env, name);
}

static APR_INLINE
apreq_param_t *apreq_body_get(apreq_env_handle_t *env, const char *name)
{
    return env->module->body_get(env, name);
}

static APR_INLINE
apr_status_t apreq_parser_get(apreq_env_handle_t *env,
                              const apreq_parser_t **parser)
{
    return env->module->parser_get(env, parser);
}

static APR_INLINE
apr_status_t apreq_parser_set(apreq_env_handle_t *env,
                              apreq_parser_t *parser)
{
    return env->module->parser_set(env, parser);
}

static APR_INLINE
apr_status_t apreq_hook_add(apreq_env_handle_t *env, apreq_hook_t *hook)
{
    return env->module->hook_add(env, hook);
}

static APR_INLINE
const char *apreq_header_in(apreq_env_handle_t *env, const char *name)
{
    return env->module->header_in(env, name);
}

static APR_INLINE
apr_status_t apreq_header_out(apreq_env_handle_t *env,
                              const char *name, char *val)
{
    return env->module->header_out(env, name, val);
}

static APR_INLINE
apr_status_t apreq_brigade_limit_set(apreq_env_handle_t *env,
                                     apr_size_t bytes)
{
    return env->module->brigade_limit_set(env, bytes);
}

static APR_INLINE
apr_status_t apreq_brigade_limit_get(apreq_env_handle_t *env,
                                     apr_size_t *bytes)
{
    return env->module->brigade_limit_get(env, bytes);
}

static APR_INLINE
apr_status_t apreq_read_limit_set(apreq_env_handle_t *env,
                                  apr_uint64_t bytes)
{
    return env->module->read_limit_set(env, bytes);
}

static APR_INLINE
apr_status_t apreq_read_limit_get(apreq_env_handle_t *env,
                                  apr_uint64_t *bytes)
{
    return env->module->read_limit_get(env, bytes);
}

static APR_INLINE
apr_status_t apreq_temp_dir_set(apreq_env_handle_t *env, const char *path)
{
    return env->module->temp_dir_set(env, path);
}

static APR_INLINE
apr_status_t apreq_temp_dir_get(apreq_env_handle_t *env, const char **path)
{
    return env->module->temp_dir_get(env, path);
}






/**
 * @file apreq_env.h
 * @brief Logging and environment (module) declarations.
 * @ingroup libapreq2
 */


/**
 * Fetch the header value (joined by ", " if there are multiple headers)
 * for a given header name.
 * @param env The current environment.
 * @param name The header name.
 * @return The value of the header, NULL if not found.
 */


/**
 * Add a header field to the environment's outgoing response headers
 * @param env The current environment.
 * @param name The name of the outgoing header.
 * @param val Value of the outgoing header.
 * @return APR_SUCCESS on success, error code otherwise.
 */


/**
 * Convenience macro for defining an environment module by mapping
 * a function prefix to an associated environment structure.
 * @param pre Prefix to define new environment.  All attributes of
 * the apreq_env_module_t struct are defined with this as their prefix. The
 * generated struct is named by appending "_module" to the prefix.
 * @param name Name of this environment.
 * @param mmn Magic number (i.e. version number) of this environment.
 */
#define APREQ_MODULE(pre, mmn) const apreq_env_module_t \
  pre##_module = { #pre, mmn,                           \
  pre##_jar,        pre##_args,       pre##_body,       \
  pre##_jar_get,    pre##_args_get,   pre##_body_get,   \
  pre##_parser_get, pre##_parser_set, pre##_hook_add,   \
  pre##_brigade_limit_get, pre##_brigade_limit_set,     \
  pre##_read_limit_get,    pre##_read_limit_set,        \
  pre##_temp_dir_get,      pre##_temp_dir_set,          \
  pre##_header_in,         pre##_header_out }

/**
 * Create an apreq handle which is suitable for a CGI program. It
 * reads input from stdin and writes output to stdout.
 */
APREQ_DECLARE(apreq_env_handle_t*) apreq_handle_cgi(apr_pool_t *pool);

/**
 * Create a custom apreq handle which knows only some static
 * values. Useful if you want to test the parser code or if you have
 * got data from a custom source (neither Apache 2 nor CGI).
 * @param pool the APR pool
 * @param query_string the query string
 * @param cookie value of the request "Cookie" header
 * @param cookie2 value of the request "Cookie2" header
 * @param content_type content type of the request body
 * @param in a bucket brigade containing the request body
 */
APREQ_DECLARE(apreq_env_handle_t*) apreq_handle_custom(apr_pool_t *pool,
                                                       const char *query_string,
                                                       const char *cookie,
                                                       const char *cookie2,
                                                       apreq_parser_t *parser,
                                                       apr_uint64_t read_limit,
                                                       apr_bucket_brigade *in);

/**
 * Add the cookie to the outgoing "Set-Cookie" headers.
 *
 * @param c The cookie.
 * @param env Environment.
 */
APREQ_DECLARE(apr_status_t) apreq_cookie_bake(const apreq_cookie_t *c,
                                              apreq_env_handle_t *env);

/**
 * Add the cookie to the outgoing "Set-Cookie2" headers.
 *
 * @param c The cookie.
 * @param env Environment.
 */
APREQ_DECLARE(apr_status_t) apreq_cookie_bake2(const apreq_cookie_t *c,
                                               apreq_env_handle_t *env);

/**
 * Looks for the presence of a "Cookie2" header to determine whether
 * or not the current User-Agent supports rfc2965.
 * @param env The current environment.
 * @return APREQ_COOKIE_VERSION_RFC if rfc2965 is supported, 
 *         APREQ_COOKIE_VERSION_NETSCAPE otherwise.
 */
APREQ_DECLARE(apreq_cookie_version_t)
    apreq_ua_cookie_version(apreq_env_handle_t *env);


APREQ_DECLARE(apreq_param_t *)apreq_param(apreq_env_handle_t *env, 
                                          const char *name);

#define apreq_cookie(env, name) apreq_jar_get(env, name)

/**
 * Returns a table containing key-value pairs for the full request
 * (args + body).
 * @param p Allocates the returned table.
 * @param req The current apreq_request_t object.
 * @remark Also parses the request if necessary.
 */
APREQ_DECLARE(apr_table_t *) apreq_params(apr_pool_t *p,
                                          apreq_env_handle_t *env);


APREQ_DECLARE(apr_table_t *)apreq_cookies(apr_pool_t *p,
                                          apreq_env_handle_t *env);

/**
 * Force a complete parse.
 * @param req Current request handle.
 * @return APR_SUCCESS on an error-free parse of the request data.
 *         Any other status code indicates a problem somewhere.
 *
 */

static APR_INLINE
apr_status_t apreq_parse_request(apreq_env_handle_t *req)
{
    const apr_table_t *dummy;
    apr_status_t jar_status, args_status, body_status;

    jar_status = apreq_jar(req, &dummy);
    args_status = apreq_args(req, &dummy);
    body_status = apreq_body(req, &dummy);

    /* XXX: punt to APR_EGENERAL; need to improve this
     * for valid requests where certain data/headers are 
     * unavailable.
     */
    if (jar_status || args_status || body_status)
        return APR_EGENERAL;

    return APR_SUCCESS;
}




#ifdef __cplusplus
 }
#endif

#endif
