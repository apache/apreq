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

#include "apreq_params.h"
#include "apreq_env.h"
#include "apr_strings.h"
#include "apr_lib.h"

#define MAX_LEN         (1024 * 1024)
#define MAX_BRIGADE_LEN (1024 * 256)
#define MAX_FIELDS      (200)
#define MAX_READ_AHEAD  (1024 * 64)
    
APREQ_DECLARE(apreq_param_t *) apreq_make_param(apr_pool_t *p, 
                                                const char *name, 
                                                const apr_size_t nlen, 
                                                const char *val, 
                                                const apr_size_t vlen)
{
    apreq_param_t *param = apr_palloc(p, nlen + vlen + 1 + sizeof *param);
    apreq_value_t *v = &param->v;
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


APREQ_DECLARE(apreq_request_t *) apreq_request(void *env, const char *qs)
{
    apreq_request_t *req;
    apr_pool_t *p;

    if (qs == NULL) {
        apreq_request_t *old_req = apreq_env_request(env,NULL);
        if (old_req != NULL)
            return old_req;

        p = apreq_env_pool(env);
        qs = apreq_env_query_string(env);

        req = apr_palloc(p, sizeof *req);
        req->env      = env;
        req->args     = apr_table_make(p, APREQ_NELTS);
        req->body     = NULL;
        req->parser   = apreq_parser(env, NULL);

        /* XXX get/set race condition here wrt apreq_env_request? */
        old_req = apreq_env_request(env, req);

        if (old_req != NULL) {
            apreq_log(APREQ_ERROR APR_EGENERAL, env, "race condition"
                      "between consecutive calls of apreq_env_request");
            apreq_env_request(env, old_req); /* reset old_req */
            return old_req;
        }

    }
    else {
        p = apreq_env_pool(env);

        req = apr_palloc(p, sizeof *req);
        req->env      = env;
        req->args     = apr_table_make(p, APREQ_NELTS);
        req->body     = NULL;
        req->parser   = apreq_parser(env, NULL);

    }

    if (qs != NULL) {
        apr_status_t s = apreq_parse_query_string(p, req->args, qs);
        if (s != APR_SUCCESS)
            apreq_log(APREQ_ERROR s, env, "invalid query string: %s", qs);
    }

    return req;
}


APREQ_DECLARE(apreq_param_t *)apreq_param(const apreq_request_t *req, 
                                          const char *name)
{
    const char *val = apr_table_get(req->args, name);

    if (val == NULL && req->body)
        val = apr_table_get(req->body, name);

    if (val == NULL) {
        apreq_env_read(req->env, APR_BLOCK_READ, APREQ_READ_AHEAD);
        if (req->body)
            val = apr_table_get(req->body, name);
    }

    return val ? apreq_value_to_param(apreq_strtoval(val)) : NULL;
}


APREQ_DECLARE(apr_table_t *) apreq_params(apr_pool_t *pool,
                                          const apreq_request_t *req)
{
    apr_status_t s;

    do s = apreq_env_read(req->env, APR_BLOCK_READ, APREQ_READ_AHEAD);
    while (s == APR_INCOMPLETE);

    return req->body ? apr_table_overlay(pool, req->args, req->body) :
        apr_table_copy(pool, req->args);
}


APREQ_DECLARE(apreq_param_t *) apreq_decode_param(apr_pool_t *pool, 
                                                  const char *word,
                                                  const apr_size_t nlen, 
                                                  const apr_size_t vlen)
{
    apreq_param_t *param;
    apr_ssize_t size;

    if (nlen == 0)
        return NULL;

    param = apr_palloc(pool, nlen + vlen + 1 + sizeof *param);
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

APREQ_DECLARE(apr_status_t) apreq_parse_query_string(apr_pool_t *pool,
                                                     apr_table_t *t,
                                                     const char *qs)
{
    const char *start = qs;
    apr_size_t nlen = 0;

    for (;;++qs) {
        switch (*qs) {

        case '=':
            if (nlen == 0) {
                nlen = qs - start;
            }
            break;

        case '&':
        case ';': 
        case 0:
            if (qs > start) {
                apr_size_t vlen = 0;
                apreq_param_t *param;
                if (nlen == 0)
                    nlen = qs - start;
                else
                    vlen = qs - start - nlen - 1;

                param = apreq_decode_param(pool, start, nlen, vlen);
                if (param->v.status != APR_SUCCESS)
                    return param->v.status;

                apr_table_addn(t, param->v.name, param->v.data);
            }

            if (*qs == 0)
                return APR_SUCCESS;

            nlen = 0;
            start = qs + 1;
        }
    }
    /* not reached */
    return APR_INCOMPLETE;
}

APREQ_DECLARE(apr_status_t) apreq_parse_request(apreq_request_t *req, 
                                                apr_bucket_brigade *bb)
{
    if (req->parser == NULL)
        req->parser = apreq_parser(req->env,NULL);
    if (req->parser == NULL)
        return APR_EINIT;

    if (req->body == NULL)
        req->body = apr_table_make(apreq_env_pool(req->env),APREQ_NELTS);

    return apreq_run_parser(req->parser, req->env, req->body, bb);
}


static int upload_push(void *data, const char *key, const char *val)
{
    apr_table_t *t = data;
    apreq_param_t *p = apreq_value_to_param(apreq_strtoval(val));
    if (p->bb)
        apr_table_addn(t, key, val);
    return 0;
}


APREQ_DECLARE(apr_table_t *) apreq_uploads(apr_pool_t *pool,
                                           const apreq_request_t *req)
{
    apr_table_t *t;
    apr_status_t s;
    do s = apreq_env_read(req->env, APR_BLOCK_READ, APREQ_READ_AHEAD);
    while (s == APR_INCOMPLETE);

    if (req->body == NULL)
        return NULL;

    t = apr_table_make(pool, APREQ_NELTS);
    /* XXX needs appropriate copy/merge callbacks */
    apr_table_do(upload_push, t, req->body, NULL);
    return t;
}

static int upload_get(void *data, const char *key, const char *val)
{
    apreq_param_t *p = apreq_value_to_param(apreq_strtoval(val));
    apreq_param_t **q = data;
    if (p->bb) {
        *q = p;
        return 1; /* upload found, stop */
    }
    else
        return 0; /* keep searching */
}

APREQ_DECLARE(apreq_param_t *) apreq_upload(const apreq_request_t *req,
                                            const char *key)
{
    apreq_param_t *param = NULL;
    apreq_env_read(req->env, APR_BLOCK_READ, APREQ_READ_AHEAD);
    if (req->body == NULL)
        return NULL;
    apr_table_do(upload_get, &param, req->body, key, NULL);
    return param;
}
