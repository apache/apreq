/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 *
 * Portions of this software are based upon public domain software
 * originally written at the National Center for Supercomputing Applications,
 * University of Illinois, Urbana-Champaign.
 */

#ifndef APREQ_PARAM_H
#define APREQ_PARAM_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "apreq_tables.h"
#include "apr_buckets.h"

/***************************************************
 * API: apreq_params.c
 *
 */

typedef struct apreq_param_t {
    enum { ASCII, UTF_8, UTF_16, IS0_LATIN_1 } charset;
    char                *language;
    apreq_table_t       *info;

    apr_bucket_brigade  *bb;

    apreq_value_t        v;
} apreq_param_t;

#define apreq_value_to_param(ptr) apreq_attr_to_type(apreq_param_t, \
                                                      v, ptr)
#define apreq_param_name(p)  ((p)->v.name)
#define apreq_param_value(p) ((p)->v.data)



/* XXX this might be better as an ADT */
typedef struct apreq_request_t {
    apreq_table_t      *args;         /* query_string params */
    apreq_table_t      *body;

    void               *ctx;
    apr_status_t        status;
} apreq_request_t;

/**
 * Creates an apreq_request_t object.
 * @param ctx The current request context.
 */

APREQ_DECLARE(apreq_request_t *)apreq_request(void *ctx);

APREQ_DECLARE(apr_status_t)apreq_parse(apreq_request_t *req);

/**
 * Returns the first parameter value for the requested key,
 * NULL if none found. The key is case-insensitive.
 * @param req The current apreq_request_t object.
 * @param key Nul-terminated search key.  Returns the first table value 
 * if NULL.
 * @remark Also parses the request as necessary.
 */

APREQ_DECLARE(const apreq_param_t *) apreq_param(const apreq_request_t *req, 
                                        const char *name); 

APREQ_DECLARE(const char *) apreq_arg(const apreq_request_t *req, 
                                      const char *name);
#define apreq_arg(r,k) apreq_table_get((req)->arg, k)


APREQ_DECLARE(const apreq_table_t *) apreq_args(const apreq_request_t *req);
#define apreq_args(req) (const apreq_table_t *)((req)->args)

APREQ_DECLARE(const apreq_table_t *) apreq_body(const apreq_request_t *req);
/* Danger Will Robinson!  */
#define apreq_body(req) (const apreq_table_t *)((req)->body)


/**
 * Returns all parameters for the requested key,
 * NULL if none found. The key is case-insensitive.
 * @param req The current apreq_request_t object.
 * @param key Nul-terminated search key.  Returns the first table value 
 * if NULL.
 * @remark Also parses the request as necessary.
 */

APREQ_DECLARE(apr_array_header_t *) apreq_params(
                                            apr_pool_t *p,
                                            const apreq_request_t *req,
                                            const char *name);

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
 * Returns an array of param keys; the array elements are unique.
 * @param req The current apreq_request_t object.
 * @remark Also parses the request as necessary.
 */
APREQ_DECLARE(apr_array_header_t *) apreq_keys(apreq_request_t *req);

APREQ_DECLARE(apr_status_t)  apreq_param_split(apr_pool_t *pool,
                                               apreq_table_t *t, 
                                               const char *data, 
                                               const apr_size_t dlen);

APREQ_DECLARE(apreq_param_t *) apreq_param_decode(apr_pool_t *pool, 
                                                  const char *word,
                                                  const apr_size_t nlen, 
                                                  const apr_size_t vlen);
APREQ_DECLARE(char *) apreq_param_encode(apr_pool_t *pool, 
                                         const apreq_param_t *param);


#ifdef __cplusplus
}
#endif


#endif /* APREQ_PARAM_H */



