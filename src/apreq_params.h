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

#ifndef APREQ_PARAM_H
#define APREQ_PARAM_H

#include "apreq.h"
#include "apreq_parsers.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 * @file apreq_params.h
 * @brief Request and param stuff.
 */
/**
 * @defgroup params Request params
 * @ingroup LIBRARY
 * @{
 */


/** Common data structure for params and file uploads */
typedef struct apreq_param_t {
    apr_table_t         *info; /**< header table associated with the param */
    apr_bucket_brigade  *bb;   /**< brigade to spool upload files */
    apreq_value_t        v;    /**< underlying name/value/status info */
} apreq_param_t;

/** accessor macros */
#define apreq_value_to_param(ptr) apreq_attr_to_type(apreq_param_t, v, ptr)
#define apreq_param_name(p)      ((p)->v.name)
#define apreq_param_value(p)     ((p)->v.data)
#define apreq_param_info(p)      ((p)->info)
#define apreq_param_status(p)    ((p)->v.status)
#define apreq_param_brigade(p) ((p)->bb ? apreq_copy_brigade((p)->bb) : NULL)

/** creates a param from name/value information */
APREQ_DECLARE(apreq_param_t *) apreq_make_param(apr_pool_t *p, 
                                                const char *name, 
                                                const apr_size_t nlen, 
                                                const char *val, 
                                                const apr_size_t vlen);

/** Structure which manages the request data. */
typedef struct apreq_request_t {
    apr_table_t        *args;         /**< parsed query_string */
    apr_table_t        *body;         /**< parsed post data */
    apreq_parser_t     *parser;       /**< active parser for this request */
    apreq_cfg_t        *cfg;          /**< parser configuration */
    void               *env;          /**< request environment */
} apreq_request_t;


/**
 * Creates an apreq_request_t object.
 * @param env The current request environment.
 * @param qs  The query string.
 * @remark "qs = NULL" has special behavior.  In this case,
 * apreq_request(env,NULL) will attempt to fetch a cached object
 * from the environment via apreq_env_request.  Failing that, it will
 * replace "qs" with the result of apreq_env_query_string(env), 
 * parse that, and store the resulting apreq_request_t object back 
 * within the environment.  This maneuver is designed to both mimimize
 * parsing work and allow the environent to place the parsed POST data in
 * req->body (otherwise the caller may need to do this manually).
 * For details on this, see the environment's documentation for
 * the apreq_env_read function.
 */

APREQ_DECLARE(apreq_request_t *)apreq_request(void *env, const char *qs);


/**
 * Returns the first parameter value for the requested key,
 * NULL if none found. The key is case-insensitive.
 * @param req The current apreq_request_t object.
 * @param key Nul-terminated search key.  Returns the first table value 
 * if NULL.
 * @remark Also parses the request as necessary.
 */

APREQ_DECLARE(apreq_param_t *) apreq_param(const apreq_request_t *req, 
                                           const char *name); 

/**
 * Returns all parameters for the requested key,
 * NULL if none found. The key is case-insensitive.
 * @param req The current apreq_request_t object.
 * @param key Nul-terminated search key.  Returns the first table value 
 * if NULL.
 * @remark Also parses the request as necessary.
 */

APREQ_DECLARE(apr_table_t *) apreq_params(apr_pool_t *p,
                                          const apreq_request_t *req);


/**
 * Returns a ", " -separated string containing all parameters 
 * for the requested key, NULL if none found.  The key is case-insensitive.
 * @param req The current apreq_request_t object.
 * @param key Nul-terminated search key.  Returns the first table value 
 * if NULL.
 * @remark Also parses the request as necessary.
 */
#define apreq_params_as_string(req,key,pool, mode) \
 apreq_join(pool, ", ", apreq_params(req,pool,key), mode)


/**
 * Url-decodes a name=value pair into a param.
 * @param pool  Pool from which the param is allocated.
 * @param word  Start of the name=value pair.
 * @param nlen  Length of urlencoded name.
 * @param vlen  Length of urlencoded value.
 * @remark      Unless vlen == 0, this function assumes there is
 *              exactly one character ('=') which separates the pair.
 *            
 */

APREQ_DECLARE(apreq_param_t *) apreq_decode_param(apr_pool_t *pool, 
                                                  const char *word,
                                                  const apr_size_t nlen, 
                                                  const apr_size_t vlen);
/**
 * Url-encodes the param into a name-value pair.
 */

APREQ_DECLARE(char *) apreq_encode_param(apr_pool_t *pool, 
                                         const apreq_param_t *param);

/**
 * Parse a url-encoded string into a param table.
 * @param pool    pool used to allocate the param data.
 * @param table   table to which the params are added.
 * @param qs      Query string to url-decode.
 * @remark        This function uses [&;] as the set of tokens
 *                to delineate words, and will treat a word w/o '='
 *                as a name-value pair with value-length = 0.
 *
 */

APREQ_DECLARE(apr_status_t) apreq_parse_query_string(apr_pool_t *pool,
                                                     apr_table_t *t, 
                                                     const char *qs);

/**
 * Parse a brigade as incoming POST data.
 * @param req Current request.
 * @param bb  Brigade to parse. See remarks below.
 * @return    APR_INCOMPLETE if the parse is incomplete,
 *            APR_SUCCESS if the parser is finished (saw eos),
 *            unrecoverable error value otherwise.
 *
 * @remark    Polymorphic buckets (file, pipe, socket, etc.)
 *            will generate new buckets during parsing, which
 *            may cause problems with the configuration checks.
 *            To be on the safe side, the caller should avoid
 *            placing such buckets in the passed brigade.
 */

APREQ_DECLARE(apr_status_t)apreq_parse_request(apreq_request_t *req, 
                                               apr_bucket_brigade *bb);
/**
 * Returns a table of all params in req->body with non-NULL bucket brigades.
 * @param pool Pool which allocates the table struct.
 * @param req  Current request.
 */

APREQ_DECLARE(apr_table_t *) apreq_uploads(apr_pool_t *pool,
                                           const apreq_request_t *req);

/**
 * Returns the first param in req->body which has both param->v.name 
 * matching key and param->bb != NULL.
 */

APREQ_DECLARE(apreq_param_t *) apreq_upload(const apreq_request_t *req,
                                            const char *key);

APREQ_DECLARE(apr_status_t) 
    apreq_request_config(apreq_request_t *req, 
                         const char *attr, apr_size_t alen,
                         const char *val, apr_size_t vlen);

/** @} */
#ifdef __cplusplus
}
#endif


#endif /* APREQ_PARAM_H */



