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

#ifndef APREQ_PARAMS_H
#define APREQ_PARAMS_H

#include "apreq.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 * @file apreq_params.h
 * @brief Request parsing and parameter API
 * @ingroup libapreq2
 */

/** Common data structure for params and file uploads */
typedef struct apreq_param_t {
    apr_table_t         *info; /**< header table associated with the param */
    apr_bucket_brigade  *bb;   /**< brigade to spool upload files */
    apreq_value_t        v;    /**< underlying name/value/status info */
} apreq_param_t;

/* These structs are defined below */
typedef struct apreq_hook_t apreq_hook_t;
typedef struct apreq_parser_t apreq_parser_t;

/** accessor macros */
#define apreq_value_to_param(ptr) apreq_attr_to_type(apreq_param_t, v, ptr)
#define apreq_param_name(p)      ((p)->v.name)
#define apreq_param_value(p)     ((p)->v.data)
#define apreq_param_info(p)      ((p)->info)
#define apreq_param_brigade(p) ((p)->bb ? apreq_copy_brigade((p)->bb) : NULL)

/** creates a param from name/value information */
APREQ_DECLARE(apreq_param_t *) apreq_make_param(apr_pool_t *p, 
                                                const char *name, 
                                                const apr_size_t nlen, 
                                                const char *val, 
                                                const apr_size_t vlen);

/** Structure which manages the request data. */
typedef struct apreq_request_t {
    apr_table_t        *args;         /**< parsed query-string */
    apr_table_t        *body;         /**< parsed post data */
    apreq_parser_t     *parser;       /**< active parser for this request */
    apreq_env_handle_t *env;          /**< request environment */
    apr_status_t        args_status;  /**< status of query-string parse */
    apr_status_t        body_status;  /**< status of post data parse */
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

APREQ_DECLARE(apreq_request_t *)apreq_request(apreq_env_handle_t *env,
                                              const char *qs);


/**
 * Returns the first parameter value with the desired name,
 * NULL if none found. The name is case-insensitive.
 * @param req The current apreq_request_t object.
 * @param name Nul-terminated search key.  Returns the first table value 
 * if NULL.
 * @return First matching parameter.
 * @remark Also parses the request as necessary.
 */
APREQ_DECLARE(apreq_param_t *) apreq_param(const apreq_request_t *req, 
                                           const char *name); 


/**
 * Returns a table containing key-value pairs for the full request
 * (args + body).
 * @param p Allocates the returned table.
 * @param req The current apreq_request_t object.
 * @remark Also parses the request if necessary.
 */
APREQ_DECLARE(apr_table_t *) apreq_params(apr_pool_t *p,
                                          const apreq_request_t *req);



/**
 * Returns an array of parameters (apreq_param_t *) matching the given key.
 * The key is case-insensitive.
 * @param p Allocates the returned array.
 * @param req The current apreq_request_t object.
 * @param key Null-terminated search key.  key==NULL fetches all parameters.
 * @remark Also parses the request if necessary.
 */
APREQ_DECLARE(apr_array_header_t *) apreq_params_as_array(apr_pool_t *p,
                                                          apreq_request_t *req,
                                                          const char *key);

/**
 * Returns a ", " -separated string containing all parameters 
 * for the requested key, NULL if none are found.  The key is case-insensitive.
 * @param p Allocates the return string.
 * @param req The current apreq_request_t object.
 * @param key Null-terminated parameter name. key==NULL fetches all values. 
 * @param mode Join type- see apreq_join().
 * @return Returned string is the data attribute of an apreq_value_t,
 *         so it is safe to use in apreq_strlen() and apreq_strtoval().
 * @remark Also parses the request if necessary.
 */
APREQ_DECLARE(const char *) apreq_params_as_string(apr_pool_t *p,
                                                   apreq_request_t *req,
                                                   const char *key,
                                                   apreq_join_t mode);


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
 * @param pool Pool which allocates the returned string.
 * @param param Param to encode.
 * @return name-value pair representing the param.
 */

APREQ_DECLARE(char *) apreq_encode_param(apr_pool_t *pool, 
                                         const apreq_param_t *param);

/**
 * Parse a url-encoded string into a param table.
 * @param pool    pool used to allocate the param data.
 * @param t       table to which the params are added.
 * @param qs      Query string to url-decode.
 * @return        APR_SUCCESS if successful, error otherwise.
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
 *            APR_ENOTIMPL if no parser is available for this request
 *                         (i.e. unrecognized Content-Type header),
 *            unrecoverable error value otherwise.
 */

APREQ_DECLARE(apr_status_t)apreq_parse_request(apreq_request_t *req, 
                                               apr_bucket_brigade *bb);
/**
 * Returns a table of all params in req->body with non-NULL bucket brigades.
 * @param pool Pool which allocates the table struct.
 * @param req  Current request.
 * @return Upload table.
 * @remark Will parse the request if necessary.
 */

APREQ_DECLARE(apr_table_t *) apreq_uploads(apr_pool_t *pool,
                                           const apreq_request_t *req);

/**
 * Returns the first param in req->body which has both param->v.name 
 * matching key and param->bb != NULL.
 * @param req The current request.
 * @param key Parameter name. key == NULL returns first upload.
 * @return Corresponding upload, NULL if none found.
 * @remark Will parse the request as necessary.
 */

APREQ_DECLARE(apreq_param_t *) apreq_upload(const apreq_request_t *req,
                                            const char *key);
#include "apreq.h"

/** Parser arguments. */
#define APREQ_PARSER_ARGS  apreq_parser_t *parser,     \
                           apreq_env_handle_t *env,        \
                           apr_table_t *t,             \
                           apr_bucket_brigade *bb

/** Hook arguments */
#define APREQ_HOOK_ARGS    apreq_hook_t *hook,         \
                           apreq_env_handle_t *env,        \
                           apreq_param_t *param,       \
                           apr_bucket_brigade *bb

typedef apr_status_t (*apreq_parser_function_t)(APREQ_PARSER_ARGS);
typedef apr_status_t (*apreq_hook_function_t)(APREQ_HOOK_ARGS);

/**
 * Declares a API parser.
 */
#define APREQ_DECLARE_PARSER(f) APREQ_DECLARE_NONSTD(apr_status_t) \
                                (f) (APREQ_PARSER_ARGS)

/**
 * Declares an API hook.
 */
#define APREQ_DECLARE_HOOK(f)   APREQ_DECLARE_NONSTD(apr_status_t) \
                                (f) (APREQ_HOOK_ARGS)

/**
 * Singly linked list of hooks.
 *
 */
struct apreq_hook_t {
    apreq_hook_function_t hook;
    apreq_hook_t   *next;
    void           *ctx;
};

/**
 * Request parser with associated enctype and hooks. 
 *
 */
struct apreq_parser_t {
    apreq_parser_function_t parser;
    const char    *enctype;
    apreq_hook_t  *hook;
    void          *ctx;
};




/**
 * Parse the incoming brigade into a table.  Parsers normally
 * consume all the buckets of the brigade during parsing. However
 * parsers may leave "rejected" data in the brigade, even during a
 * successful parse, so callers may need to clean up the brigade
 * themselves (in particular, rejected buckets should not be 
 * passed back to the parser again).
 * @remark  bb == NULL is valid: the parser should return its 
 * public status: APR_INCOMPLETE, APR_SUCCESS, or an error code.
 */
#define APREQ_RUN_PARSER(psr,env,t,bb) (psr)->parser(psr,env,t,bb)

/**
 * Run the hook with the current parameter and the incoming 
 * bucket brigade.  The hook may modify the brigade if necessary.
 * Once all hooks have completed, the contents of the brigade will 
 * be added to the parameter's bb attribute.
 * @return APR_SUCCESS on success. All other values represent errors.
 */
#define APREQ_RUN_HOOK(h,env,param,bb) (h)->hook(h,env,param,bb)

/**
 * Concatenates the brigades, spooling large brigades into
 * a tempfile bucket according to the environment's max_brigade
 * setting- see apreq_env_max_brigade().
 * @param env Environment.
 * @param out Resulting brigade.
 * @param in Brigade to append.
 * @return APR_SUCCESS on success, error code otherwise.
 */
APREQ_DECLARE(apr_status_t) apreq_brigade_concat(apreq_env_handle_t *env,
                                                 apr_bucket_brigade *out, 
                                                 apr_bucket_brigade *in);


/**
 * Rfc822 Header parser. It will reject all data
 * after the first CRLF CRLF sequence (an empty line).
 * See #APREQ_RUN_PARSER for more info on rejected data.
 */
APREQ_DECLARE_PARSER(apreq_parse_headers);

/**
 * Rfc2396 application/x-www-form-urlencoded parser.
 */
APREQ_DECLARE_PARSER(apreq_parse_urlencoded);

/**
 * Rfc2388 multipart/form-data (and XForms 1.0 multipart/related)
 * parser. It will reject any buckets representing preamble and 
 * postamble text (this is normal behavior, not an error condition).
 * See #APREQ_RUN_PARSER for more info on rejected data.
 */
APREQ_DECLARE_PARSER(apreq_parse_multipart);

/**
 * Generic parser.  No table entries will be added to
 * the req->body table by this parser.  The parser creates
 * a dummy apreq_param_t to pass to any configured hooks.  If
 * no hooks are configured, the dummy param's bb slot will
 * contain a copy of the request body.  It can be retrieved
 * by casting the parser's ctx pointer to (apreq_param_t **).
 */
APREQ_DECLARE_PARSER(apreq_parse_generic);

/**
 * apr_xml_parser hook. It will parse until EOS appears.
 * The parsed document isn't available until parsing has
 * completed successfully.  The hook's ctx pointer may 
 * be cast as (apr_xml_doc **) to retrieve the 
 * parsed document.
 */
APREQ_DECLARE_HOOK(apreq_hook_apr_xml_parser);

/**
 * Construct a parser.
 *
 * @param pool Pool used to allocate the parser.
 * @param enctype Content-type that this parser can deal with.
 * @param parser The parser function.
 * @param hook Hooks to asssociate this parser with.
 * @param ctx Parser's internal scratch pad.
 * @return New parser.
 */
APREQ_DECLARE(apreq_parser_t *)
        apreq_make_parser(apr_pool_t *pool,
                          const char *enctype,
                          apreq_parser_function_t parser,
                          apreq_hook_t *hook,
                          void *ctx);

/**
 * Construct a hook.
 *
 * @param pool used to allocate the hook.
 * @param hook The hook function.
 * @param next List of other hooks for this hook to call on.
 * @param ctx Hook's internal scratch pad.
 * @return New hook.
 */
APREQ_DECLARE(apreq_hook_t *)
        apreq_make_hook(apr_pool_t *pool,
                        apreq_hook_function_t hook,
                        apreq_hook_t *next,
                        void *ctx);


/**
 * Add a new hook to the end of the parser's hook list.
 *
 * @param p Parser.
 * @param h Hook to append.
 */
APREQ_DECLARE(void) apreq_add_hook(apreq_parser_t *p, 
                                   apreq_hook_t *h);


/**
 * Create the default parser associated with the
 * current request's Content-Type (if possible).
 * @param env The current environment.
 * @param hook Hook(s) to add to the parser.
 * @return New parser, NULL if the Content-Type is
 * unrecognized.
 *
 * @param env The current environment.
 * @param hook Additional hooks to supply the parser with.
 * @return The parser; NULL if the environment's
 * Content-Type is unrecognized.
 */
APREQ_DECLARE(apreq_parser_t *)apreq_parser(apreq_env_handle_t *env,
                                            apreq_hook_t *hook);


/**
 * Register a new parsing function with a MIME enctype.
 * Registered parsers are added to apreq_parser()'s
 * internal lookup table.
 *
 * @param enctype The MIME type.
 * @param parser  The function to use during parsing. Setting
 *                parser == NULL will remove an existing parser.
 * @remark This is not a thread-safe operation, so applications 
 * should only call this during process startup,
 * or within a request-thread mutex.
 */

APREQ_DECLARE(void) apreq_register_parser(const char *enctype, 
                                          apreq_parser_function_t parser);


/**
 * Returns APR_EGENERAL.  Effectively disables mfd parser
 * if a file-upload field is present.
 *
 */
APREQ_DECLARE_HOOK(apreq_hook_disable_uploads);

/**
 * Calls apr_brigade_cleanup on the incoming brigade
 * after passing the brigade to any subsequent hooks.
 */
APREQ_DECLARE_HOOK(apreq_hook_discard_brigade);

#ifdef __cplusplus
}

#endif
#endif /* APREQ_PARAMS_H */



