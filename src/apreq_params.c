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
#include "apreq_parsers.h"
#include "apreq_env.h"
#include "apr_strings.h"

#define p2v(param) ( (param) ? &(param)->v : NULL )
#define UPGRADE(s) apreq_value_to_param(apreq_char_to_value(s))


APREQ_DECLARE(apreq_param_t *) apreq_make_param(apr_pool_t *p, 
                                                const char *name, 
                                                const apr_ssize_t nlen, 
                                                const char *val, 
                                                const apr_ssize_t vlen)
{
    apreq_param_t *param = apr_palloc(p, nlen + vlen + 1 + sizeof *param);
    apreq_value_t *v = &param->v;
    param->charset = APREQ_CHARSET;
    param->language = NULL;
    param->info = NULL;
    param->bb = NULL;

    v->size = vlen;
    memcpy(v->data, val, vlen);
    v->data[vlen] = 0;
    v->name = v->data + vlen + 1;
    memcpy((char *)v->name, name, nlen);
    ((char *)v->name)[nlen] = 0;
    v->status = APR_SUCCESS;

    return param;
}

APR_INLINE
static const char * enctype(apr_pool_t *p, const char *ct)
{
    char *enctype, *semicolon;
    if (ct == NULL)
        enctype = NULL;
    else {
        enctype = apr_pstrdup(p, ct);
        semicolon = strchr(enctype, ';');
        if (semicolon)
            *semicolon = 0;
    }
    return enctype;
}

APREQ_DECLARE(apreq_request_t *) apreq_request(void *ctx, const char *args)
{
    apreq_request_t *req;
    apreq_parser_t *parser;
    apr_status_t s;

    if (args == NULL) {
        apreq_request_t *old_req = apreq_env_request(ctx,NULL);
        apr_pool_t *p;

        if (old_req != NULL)
            return old_req;

        p = apreq_env_pool(ctx);
        args = apreq_env_args(ctx);

        req = apr_palloc(p, sizeof(apreq_table_t *) + sizeof *req);
        req->pool = apreq_env_pool(ctx);
        *(apreq_table_t **)req->v.data = apreq_make_table(p, APREQ_NELTS);
        req->v.size   = sizeof(apreq_table_t *);
        req->v.status = APR_EINIT;
        req->env      = ctx;
        req->args     = apreq_make_table(p, APREQ_NELTS);
        req->body     = NULL;
        req->v.name = enctype(p, apreq_env_content_type(ctx));
        /* XXX need to install copy/merge callbacks for apreq_param_t */
        parser = apreq_make_parser(p, APREQ_URL_ENCTYPE,
                                   apreq_parse_urlencoded, NULL, req);
        apreq_register_parser(req, parser);
        parser = apreq_make_parser(p, APREQ_MFD_ENCTYPE,
                                   apreq_parse_multipart, NULL, req);
        apreq_register_parser(req, parser);

        /* XXX get/set race condition here wrt apreq_env_request? */
        old_req = apreq_env_request(ctx, req);
        if (old_req != NULL) {
            apreq_env_request(ctx, old_req); /* reset old_req */
            return old_req;
        }

    }
    else {
        apr_pool_t *p = apreq_env_pool(ctx);

        req = apr_palloc(p, sizeof(apreq_table_t *) + sizeof *req);
        req->pool = apreq_env_pool(ctx);
        *(apreq_table_t **)req->v.data = apreq_make_table(p, APREQ_NELTS);
        req->v.size   = sizeof(apreq_table_t *);
        req->v.status = APR_EINIT;
        req->env      = ctx;
        req->args     = apreq_make_table(p, APREQ_NELTS);
        req->body     = NULL;
        req->v.name = enctype(p, apreq_env_content_type(ctx));
        /* XXX need to install copy/merge callbacks for apreq_param_t */ 
        parser = apreq_make_parser(p, APREQ_URL_ENCTYPE,
                                   apreq_parse_urlencoded, NULL, req);
        apreq_register_parser(req, parser);
        parser = apreq_make_parser(p, APREQ_MFD_ENCTYPE,
                                   apreq_parse_multipart, NULL, req);
        apreq_register_parser(req, parser);
    }

    s = (args == NULL) ? APR_SUCCESS : 
        apreq_split_params(req->pool, req->args, args);

    if (s == APR_SUCCESS)
        req->v.status = (req->v.name ? APR_INCOMPLETE : APR_SUCCESS);

    return req;
}

APR_INLINE
APREQ_DECLARE(apreq_param_t *)apreq_param(const apreq_request_t *req, 
                                          const char *name)
{
    const char *arg = apreq_table_get(req->args, name);

    if (arg)
        return UPGRADE(arg);
    else
        return req->body ? UPGRADE(apreq_table_get(req->body, name)) : NULL;
}


APR_INLINE
APREQ_DECLARE(apreq_table_t *) apreq_params(apr_pool_t *pool,
                                            const apreq_request_t *req)
{
    return req->body ? apreq_table_overlay(pool, req->args, req->body) :
        apreq_table_copy(pool, req->args);
}


APREQ_DECLARE(apr_status_t) apreq_split_params(apr_pool_t *pool,
                                               apreq_table_t *t,
                                               const char *data)
{
    const char *start = data;
    apr_size_t nlen = 0;
    apr_status_t status = APR_SUCCESS;

    for (;;++data) {
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
            if (nlen > 0) {
                const apr_size_t vlen = data - start - nlen - 1;
                status = apreq_table_add(t, p2v(
                                apreq_decode_param( pool, start,
                                                    nlen, vlen )));

                if (*data == 0 || status != APR_SUCCESS)
                    return status;
            }

            nlen = 0;
            start = data + 1;
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

    if (nlen == 0 || word[nlen] != '=')
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
