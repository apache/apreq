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

#ifndef APREQ_MODULE_H
#define APREQ_MODULE_H

#include "apreq_cookie.h"
#include "apreq_parser.h"
#include "apreq_error.h"

#ifdef  __cplusplus
 extern "C" {
#endif 

/**
 * An apreq handle associated with a module. The structure
 * may have variable size, because the module may append its own data
 * structures after it.
 */
typedef struct apreq_handle_t {
    const struct apreq_module_t *module;
} apreq_handle_t;

/**
 * @brief Vtable describing the necessary environment functions.
 */


typedef struct apreq_module_t {
    const char *name;
    apr_uint32_t magic_number;

    apr_status_t (*jar)(apreq_handle_t *, const apr_table_t **);
    apr_status_t (*args)(apreq_handle_t *, const apr_table_t **);
    apr_status_t (*body)(apreq_handle_t *, const apr_table_t **);

    apreq_cookie_t *(*jar_get)(apreq_handle_t *, const char *);
    apreq_param_t *(*args_get)(apreq_handle_t *, const char *);
    apreq_param_t *(*body_get)(apreq_handle_t *, const char *);

    apr_status_t (*parser_get)(apreq_handle_t *, const apreq_parser_t **);
    apr_status_t (*parser_set)(apreq_handle_t *, apreq_parser_t *);
    apr_status_t (*hook_add)(apreq_handle_t *, apreq_hook_t *);

    apr_status_t (*brigade_limit_get)(apreq_handle_t *, apr_size_t *);
    apr_status_t (*brigade_limit_set)(apreq_handle_t *, apr_size_t);

    apr_status_t (*read_limit_get)(apreq_handle_t *, apr_uint64_t *);
    apr_status_t (*read_limit_set)(apreq_handle_t *, apr_uint64_t);

    apr_status_t (*temp_dir_get)(apreq_handle_t *, const char **);
    apr_status_t (*temp_dir_set)(apreq_handle_t *, const char *);

    const char *(*header_in)(apreq_handle_t *,const char *);
    apr_status_t (*header_out)(apreq_handle_t *, const char *,char *);

} apreq_module_t;


static APR_INLINE
unsigned char apreq_module_status_is_error(apr_status_t s) {
    switch (s) {
    case APR_SUCCESS:
    case APR_INCOMPLETE:
    case APR_EINIT:
    case APREQ_ERROR_NODATA:
    case APREQ_ERROR_NOPARSER:
    case APREQ_ERROR_NOHEADER:
        return 0;
    default:
        return 1;
    }
}


/**
 * Expose the parsed "cookie" header associated to this handle. 
 * @req
 * @arg t  The resulting table, or which may point to NULL 
 *          when the return value is not APR_SUCCESS. Otherwise 
 *         it must point to a valid table object.
 * @return APR_SUCCESS on success.
 * @return APREQ_ERROR_NODATA if no "Cookie" header data is available.
 */
static APR_INLINE
apr_status_t apreq_jar(apreq_handle_t *req, const apr_table_t **t)
{
    return req->module->jar(req,t);
}

/**
 * Expose the parsed "query string" associated to this handle. 
 * @req
 * @arg t  The resulting table, or which may point to NULL 
 *          when the return value is not APR_SUCCESS. Otherwise 
 *         it must point to a valid table object.
 * @return APR_SUCCESS on success.
 * @return APREQ_ERROR_NODATA if no query string is available.
 */
static APR_INLINE
apr_status_t apreq_args(apreq_handle_t *req, const apr_table_t **t)
{
    return req->module->args(req,t);
}

/**
 * Expose the parsed "request body" associated to this handle. 
 * @req
 * @arg t  The resulting table, or which may point to NULL 
 *          when the return value is not APR_SUCCESS. Otherwise 
 *         it must point to a valid table object.
 * @return APR_SUCCESS on success.
 * @return APREQ_ERROR_NODATA if no request content is available.
 */
static APR_INLINE
apr_status_t apreq_body(apreq_handle_t *req, const apr_table_t **t)
{
    return req->module->body(req, t);
}


/**
 * Fetch the first cookie with the given name. 
 * @req
 * @arg name Case-insensitive cookie name.
 * @return Desired cookie, or NULL if none match.
 */
static APR_INLINE
apreq_cookie_t *apreq_jar_get(apreq_handle_t *req, const char *name)
{
    return req->module->jar_get(req, name);
}

/**
 * Fetch the first query string param with the given name. 
 * @req
 * @arg name Case-insensitive param name.
 * @return Desired param, or NULL if none match.
 */
static APR_INLINE
apreq_param_t *apreq_args_get(apreq_handle_t *req, const char *name)
{
    return req->module->args_get(req, name);
}

/**
 * Fetch the first cookie with the given name. 
 * @req
 * @arg name Case-insensitive cookie name.
 * @return Desired cookie, or NULL if none match.
 */
static APR_INLINE
apreq_param_t *apreq_body_get(apreq_handle_t *req, const char *name)
{
    return req->module->body_get(req, name);
}

/**
 * Fetch the active body parser.
 * @req
 * @arg parser Points to the active parser on return.
 * @return Parser's current status.  Use apreq_body
 * if you need its final status (the return values
 * will be identical once the parser has finished).
 *
 */
static APR_INLINE
apr_status_t apreq_parser_get(apreq_handle_t *req,
                              const apreq_parser_t **parser)
{
    return req->module->parser_get(req, parser);
}


/**
 * Set the body parser for this request.
 * @req
 * @arg parser New parser to use.
 * @return APR_SUCCESS if the action was succesful, error otherwise.
 */
static APR_INLINE
apr_status_t apreq_parser_set(apreq_handle_t *req,
                              apreq_parser_t *parser)
{
    return req->module->parser_set(req, parser);
}

/**
 * Add a parser hook for this request.
 * @req
 * @arg hook Hook to add.
 * @return APR_SUCCESS if the action was succesful, error otherwise.
 */
static APR_INLINE
apr_status_t apreq_hook_add(apreq_handle_t *req, apreq_hook_t *hook)
{
    return req->module->hook_add(req, hook);
}


/**
 * Fetch the header value (joined by ", " if there are multiple headers)
 * for a given header name.
 * @req
 * @param name The header name.
 * @return The value of the header, NULL if not found.
 */
static APR_INLINE
const char *apreq_header_in(apreq_handle_t *req, const char *name)
{
    return req->module->header_in(req, name);
}


/**
 * Add a header field to the environment's outgoing response headers
 * @req.
 * @param name The name of the outgoing header.
 * @param val Value of the outgoing header.
 * @return APR_SUCCESS on success, error code otherwise.
 */

static APR_INLINE
apr_status_t apreq_header_out(apreq_handle_t *req,
                              const char *name, char *val)
{
    return req->module->header_out(req, name, val);
}


static APR_INLINE
apr_status_t apreq_brigade_limit_set(apreq_handle_t *req,
                                     apr_size_t bytes)
{
    return req->module->brigade_limit_set(req, bytes);
}

static APR_INLINE
apr_status_t apreq_brigade_limit_get(apreq_handle_t *req,
                                     apr_size_t *bytes)
{
    return req->module->brigade_limit_get(req, bytes);
}

static APR_INLINE
apr_status_t apreq_read_limit_set(apreq_handle_t *req,
                                  apr_uint64_t bytes)
{
    return req->module->read_limit_set(req, bytes);
}

static APR_INLINE
apr_status_t apreq_read_limit_get(apreq_handle_t *req,
                                  apr_uint64_t *bytes)
{
    return req->module->read_limit_get(req, bytes);
}

static APR_INLINE
apr_status_t apreq_temp_dir_set(apreq_handle_t *req, const char *path)
{
    return req->module->temp_dir_set(req, path);
}

static APR_INLINE
apr_status_t apreq_temp_dir_get(apreq_handle_t *req, const char **path)
{
    return req->module->temp_dir_get(req, path);
}



/**
 * Convenience macro for defining a module by mapping
 * a function prefix to an associated apreq_module_t structure.
 * @param pre Prefix to define new environment.  All attributes of
 * the apreq_env_module_t struct are defined with this as their prefix. The
 * generated struct is named by appending "_module" to the prefix.
 * @param name Name of this environment.
 * @param mmn Magic number (i.e. version number) of this environment.
 */
#define APREQ_MODULE(pre, mmn) const apreq_module_t     \
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
APREQ_DECLARE(apreq_handle_t*) apreq_handle_cgi(apr_pool_t *pool);

/**
 * Create a custom apreq handle which knows only some static
 * values. Useful if you want to test the parser code or if you have
 * got data from a custom source (neither Apache 2 nor CGI).
 * @param pool the APR pool
 * @param query_string the query string
 * @param cookie value of the request "Cookie" header
 * @param cookie2 value of the request "Cookie2" header
 * @param parser parser for handling the request body
 * @param in a bucket brigade containing the request body
 */
APREQ_DECLARE(apreq_handle_t*) apreq_handle_custom(apr_pool_t *pool,
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
 * @req
 */
APREQ_DECLARE(apr_status_t) apreq_cookie_bake(const apreq_cookie_t *c,
                                              apreq_handle_t *req);

/**
 * Add the cookie to the outgoing "Set-Cookie2" headers.
 *
 * @param c The cookie.
 */
APREQ_DECLARE(apr_status_t) apreq_cookie_bake2(const apreq_cookie_t *c,
                                               apreq_handle_t *req);

/**
 * Looks for the presence of a "Cookie2" header to determine whether
 * or not the current User-Agent supports rfc2965.
 * @req
 * @return APREQ_COOKIE_VERSION_RFC if rfc2965 is supported, 
 *         APREQ_COOKIE_VERSION_NETSCAPE otherwise.
 */
APREQ_DECLARE(unsigned)apreq_ua_cookie_version(apreq_handle_t *req);


APREQ_DECLARE(apreq_param_t *)apreq_param(apreq_handle_t *req, const char *key);

#define apreq_cookie(req, name) apreq_jar_get(req, name)

/**
 * Returns a table containing key-value pairs for the full request
 * (args + body).
 * @param p Allocates the returned table.
 * @req
 * @remark Also parses the request if necessary.
 */
APREQ_DECLARE(apr_table_t *) apreq_params(apreq_handle_t *req, apr_pool_t *p);


APREQ_DECLARE(apr_table_t *)apreq_cookies(apreq_handle_t *req, apr_pool_t *p);

/**
 * Force a complete parse.
 * @req
 * @return APR_SUCCESS on an error-free parse of the request data.
 *         Any other status code indicates a problem somewhere.
 *
 */

static APR_INLINE
apr_status_t apreq_parse(apreq_handle_t *req)
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

#endif /* APREQ_MODULE_H */
