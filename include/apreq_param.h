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

#ifndef APREQ_PARAM_H
#define APREQ_PARAM_H

#include "apreq.h"
#include "apr_buckets.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @file apreq_param.h
 * @brief Request parsing and parameter API
 * @ingroup libapreq2
 */


/** Common data structure for params and file uploads */
typedef struct apreq_param_t {
    apr_table_t         *info;   /**< header table associated with the param */
    apr_bucket_brigade  *upload; /**< brigade used to spool upload files */
    unsigned             flags;  /**< charsets, taint marks, app-specific bits */
    const apreq_value_t  v;      /**< underlying name/value info */
} apreq_param_t;


/** @return 1 if the taint flag is set, 0 otherwise. */
static APR_INLINE
unsigned apreq_param_is_tainted(const apreq_param_t *p) {
    return APREQ_FLAGS_GET(p->flags, APREQ_TAINT);
}

/** Sets the tainted flag. */
static APR_INLINE
void apreq_param_taint_on(apreq_param_t *p) {
    APREQ_FLAGS_ON(p->flags, APREQ_TAINT);
}

/** Turns off the taint flag. */
static APR_INLINE
void apreq_param_taint_off(apreq_param_t *p) {
    APREQ_FLAGS_OFF(p->flags, APREQ_TAINT);
}

/** @return 1 if the UTF-8 flag is set, 0 otherwise. */
static APR_INLINE
unsigned apreq_param_is_utf8(const apreq_param_t *p) {
    return APREQ_CHARSET_UTF8
        & APREQ_FLAGS_GET(p->flags, APREQ_CHARSET);
}

/** Sets the charset of this parameter to UTf-8. */
static APR_INLINE
void apreq_param_utf8_on(apreq_param_t *p) {
    APREQ_FLAGS_SET(p->flags, APREQ_CHARSET, APREQ_CHARSET_UTF8);
}

/** Sets the charset of this parameter to 8 bit ASCII. */
static APR_INLINE
void apreq_param_utf8_off(apreq_param_t *p) {
    APREQ_FLAGS_SET(p->flags, APREQ_CHARSET, APREQ_CHARSET_ASCII);
}


/** Upgrades args and body table values to apreq_param_t structs. */
static APR_INLINE
apreq_param_t *apreq_value_to_param(const char *val)
{
    union { const char *in; char *out; } deconst;

    deconst.in = val;
    return apreq_attr_to_type(apreq_param_t, v,
           apreq_attr_to_type(apreq_value_t, data, deconst.out));
}



/** creates a param from name/value information */
APREQ_DECLARE(apreq_param_t *) apreq_param_make(apr_pool_t *p,
                                                const char *name,
                                                const apr_size_t nlen,
                                                const char *val,
                                                const apr_size_t vlen);

/**
 * Url-decodes a name=value pair into a param.
 *
 * @param param points to the decoded parameter on success
 * @param pool  Pool from which the param is allocated.
 * @param word  Start of the name=value pair.
 * @param nlen  Length of urlencoded name.
 * @param vlen  Length of urlencoded value.
 * @remark      Unless vlen == 0, this function assumes there is
 *              exactly one character ('=') which separates the pair.
 *            
 */
APREQ_DECLARE(apr_status_t) apreq_param_decode(apreq_param_t **param,
                                               apr_pool_t *pool,
                                               const char *word,
                                               apr_size_t nlen,
                                               apr_size_t vlen);

/**
 * Url-encodes the param into a name-value pair.
 * @param pool Pool which allocates the returned string.
 * @param param Param to encode.
 * @return name-value pair representing the param.
 */
APREQ_DECLARE(char *) apreq_param_encode(apr_pool_t *pool,
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
 * Returns an array of parameters (apreq_param_t *) matching the given key.
 * The key is case-insensitive.
 * @param p Allocates the returned array.
 * @param t the parameter table returned by apreq_args(), apreq_body()
 *    or apreq_params()
 * @param key Null-terminated search key, case insensitive.
 *    key==NULL fetches all parameters.
 * @return an array of apreq_param_t* (pointers)
 * @remark Also parses the request if necessary.
 */
APREQ_DECLARE(apr_array_header_t *) apreq_params_as_array(apr_pool_t *p,
                                                          const apr_table_t *t,
                                                          const char *key);

/**
 * Returns a ", " -joined string containing all parameters
 * for the requested key, NULL if none are found.  The key is case-insensitive.
 * @param p Allocates the return string.
 * @param t the parameter table returned by apreq_args(), apreq_body()
 *    or apreq_params()
 * @param key Null-terminated parameter name, case insensitive.
 *    key==NULL fetches all values.
 * @param mode Join type- see apreq_join().
 * @return the joined string
 * @remark Also parses the request if necessary.
 */
APREQ_DECLARE(const char *) apreq_params_as_string(apr_pool_t *p,
                                                   const apr_table_t *t,
                                                   const char *key,
                                                   apreq_join_t mode);

/**
 * Returns a table of all params in req->body with non-NULL upload brigades.
 * @param body parameter table returned by apreq_body() or apreq_params()
 * @param pool Pool which allocates the table struct.
 * @return Upload table.
 * @remark Will parse the request if necessary.
 */
APREQ_DECLARE(const apr_table_t *) apreq_uploads(const apr_table_t *body,
                                                 apr_pool_t *pool);

/**
 * Returns the first param in req->body which has both param->v.name 
 * matching key (case insensitive) and param->upload != NULL.
 * @param body parameter table returned by apreq_body() or apreq_params()
 * @param name Parameter name. key == NULL returns first upload.
 * @return Corresponding upload, NULL if none found.
 * @remark Will parse the request as necessary.
 */
APREQ_DECLARE(const apreq_param_t *) apreq_upload(const apr_table_t *body,
                                                  const char *name);


#ifdef __cplusplus
}
#endif

#endif /* APREQ_PARAM_H */



