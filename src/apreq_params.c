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

#include "apreq_params.h"
#include "apreq_env.h"

#define p2v(param) ( (param) ? &(param)->v : NULL )
#define UPGRADE(s) apreq_value_to_param(apreq_char_to_value(s))


APREQ_DECLARE(apreq_request_t *) apreq_request(void *ctx)
{

    apreq_request_t *req, *old_req = apreq_env_request(ctx, NULL);
    char *query_string;
    apr_pool_t      *p;

    if (old_req != NULL)
        return old_req;

    p = apreq_env_pool(ctx);
    req = apr_palloc(p, sizeof *req);

    req->status = APR_EINIT;
    req->ctx    = ctx;
    req->args   = apreq_table_make(p, APREQ_DEFAULT_NELTS);
    req->body   = NULL;

    /* XXX get/set race condition here wrt apreq_env_request.
     * apreq_env_request probably needs a write lock ???
     */

    old_req = apreq_env_request(ctx, req);

    if (old_req != NULL)
        return old_req;

#ifdef DEBUG
    apreq.debug(ctx, 0, "making new request");
#endif

    /* XXX need to install copy/merge callbacks for apreq_param_t */

    query_string = apreq_env_args(ctx);
    req->status = (query_string == NULL) ? APR_SUCCESS :
        apreq_split_params(p, req->args, query_string, strlen(query_string));
 
    return req;
}


APREQ_DECLARE(apr_status_t) apreq_parse(apreq_request_t *req)
{
    if (req->body == NULL)
        if (req->status == APR_SUCCESS)
            return apreq_env_parse(req);
        else
            return req->status;

    else if (req->status == APR_EAGAIN)
        return apreq_env_parse(req);

    else
        return req->status;
}


APREQ_DECLARE(const apreq_param_t *)apreq_param(const apreq_request_t *req, 
                                                const char *name)
{
    const apreq_param_t *param = UPGRADE(apreq_table_get(req->args, name));
    return param ? param : UPGRADE(apreq_table_get(req->body, name));
}


APREQ_DECLARE(apr_array_header_t *) apreq_params(apr_pool_t *pool,
                                                 const apreq_request_t *req, 
                                                 const char *name)
{
    apr_array_header_t *arr = apreq_table_values(pool, req->args, name);
    apr_array_cat(arr, apreq_table_values(pool, req->body, name));
    return arr;
}


APREQ_DECLARE(apr_status_t) apreq_split_params(apr_pool_t *pool,
                                               apreq_table_t *t,
                                               const char *data, 
                                               apr_size_t dlen)
{
    const char *start = data, *end = data + dlen;
    apr_size_t nlen = 0;
    apr_status_t status = APR_SUCCESS;

    for (; data < end; ++data) {
        switch (*data) {

        case '=':
            if (nlen == 0) {
                nlen = data - start;
                break;
            }
            else
                return APR_BADARG;

        case '&':
        case ';': 
        case 0:
            if (nlen == 0) {
                return APR_BADARG;
            }
            else {
                const apr_size_t vlen = data - start - nlen - 1;
                status = apreq_table_add(t, p2v(
                                apreq_decode_param( pool, start,
                                                    nlen, vlen )));

                if (status != APR_SUCCESS)
                    return status;
            }

            nlen = 0;
            start = data + 1;
            if (start < end)
                status = APR_INCOMPLETE;
        }
    }
    return status;
}


APREQ_DECLARE(apreq_param_t *) apreq_decode_param(apr_pool_t *pool, 
                                                  const char *word,
                                                  const apr_size_t nlen, 
                                                  const apr_size_t vlen)
{
    apreq_param_t *param;
    apr_ssize_t size;

    if (nlen == 0 || vlen == 0 || word[nlen] != '=')
        return NULL;

    param = apr_palloc(pool, nlen + vlen + 1 + sizeof *param);
    param->charset = ASCII;     /* XXX: UTF_8 */
    param->language = NULL;
    param->info = NULL;
    param->bb = NULL;

    param->v.status = APR_SUCCESS;
    param->v.name = NULL;

    size = apreq_decode(param->v.data, word + nlen + 1, vlen);

    if (size < 0) {
        param->v.size = 0;
        param->v.status = APR_BADARG;
        return param;
    }

    param->v.size = size;
    param->v.name = param->v.data + size + 1;

    if (apreq_decode(param->v.data + size + 1, word, nlen) < 0)
        param->v.status = APR_BADCH;

    return param;
}


APREQ_DECLARE(char *) apreq_encode_param(apr_pool_t *pool, 
                                         const apreq_param_t *param)
{
    apreq_value_t *v;
    apr_size_t nlen;

    if (param->v.name == NULL || param->v.status != APR_SUCCESS)
        return NULL;

    nlen = strlen(param->v.name);

    v = apr_palloc(pool, 3 * (nlen + param->v.size) + 2 + sizeof *v);
    v->name = param->v.name;
    v->status = APR_SUCCESS;
    v->size = apreq_encode(v->data, param->v.name, nlen);
    v->data[v->size++] = '=';
    v->size += apreq_encode(v->data + v->size, param->v.data, param->v.size);

    return v->data;
}

