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

#include "apreq_xs_tables.h"

#define READ_BLOCK_SIZE (1024 * 256)
#define PARAM_TABLE   "Apache::Request::Table"

#define apreq_xs_request_error_check do {                               \
    int n = PL_stack_sp - (PL_stack_base + ax - 1);                     \
    apreq_request_t *req;                                               \
    apr_status_t s;                                                     \
    switch (GIMME_V) {                                                  \
    case G_VOID:                                                        \
        break;                                                          \
    case G_SCALAR:                                                      \
        if (n == 1 && items == 2)                                       \
            break;                                                      \
    default:                                                            \
        req = apreq_xs_sv2(request, sv);                                \
        s = req->args_status;                                           \
        if (s == APR_SUCCESS && req->parser)                            \
            s = apreq_parse_request(req, NULL);                         \
        switch (s) {                                                    \
        case APR_INCOMPLETE:                                            \
        case APR_SUCCESS:                                               \
            break;                                                      \
        default:                                                        \
            apreq_xs_croak(aTHX_ newHV(), s, "Apache::Request::param",  \
                           "Apache::Request::Error");                   \
       }                                                                \
    }                                                                   \
} while (0)

#define apreq_xs_args_error_check do {                                  \
    int n = PL_stack_sp - (PL_stack_base + ax - 1);                     \
    apreq_request_t *req;                                               \
    apr_status_t s;                                                     \
    switch (GIMME_V) {                                                  \
    case G_VOID:                                                        \
        break;                                                          \
    case G_SCALAR:                                                      \
        if (n == 1 && items == 2)                                       \
            break;                                                      \
    default:                                                            \
        req = apreq_xs_sv2(request, sv);                                \
        s = req->args_status;                                           \
        if (s != APR_SUCCESS)                                           \
            apreq_xs_croak(aTHX_ newHV(), s, "Apache::Request::args",   \
                           "Apache::Request::Error");                   \
    }                                                                   \
} while (0)                                                             \

#define apreq_xs_body_error_check   do {                                \
    int n = PL_stack_sp - (PL_stack_base + ax - 1);                     \
    apreq_request_t *req;                                               \
    apr_status_t s;                                                     \
    switch (GIMME_V) {                                                  \
    case G_VOID:                                                        \
        break;                                                          \
    case G_SCALAR:                                                      \
        if (n == 1 && items == 2)                                       \
            break;                                                      \
    default:                                                            \
        req = apreq_xs_sv2(request, sv);                                \
        if (req->parser == NULL)                                        \
           break;                                                       \
        switch (s = apreq_parse_request(req,NULL)) {                    \
        case APR_INCOMPLETE:                                            \
        case APR_SUCCESS:                                               \
            break;                                                      \
        default:                                                        \
            apreq_xs_croak(aTHX_ newHV(), s, "Apache::Request::body",   \
                           "Apache::Request::Error");                   \
        }                                                               \
    }                                                                   \
} while (0)

#define apreq_xs_table_error_check

#define apreq_xs_param2sv(ptr, class, parent)                           \
                newSVpvn((ptr)->v.data,(ptr)->v.size)

APREQ_XS_DEFINE_ENV(request);
APREQ_XS_DEFINE_OBJECT(request);

/* Too many GET macros :-( */

#define S2P(s) (s ? apreq_value_to_param(apreq_strtoval(s)) : NULL)
#define apreq_xs_request_push(sv,d,key) do {                            \
    apreq_request_t *req = apreq_xs_sv2(request,sv);                    \
    apr_status_t s;                                                     \
    apr_table_do(apreq_xs_do(request), d, req->args, key, NULL);        \
    do s = apreq_env_read(req->env, APR_BLOCK_READ, READ_BLOCK_SIZE);   \
    while (s == APR_INCOMPLETE);                                        \
    if (req->body)                                                      \
        apr_table_do(apreq_xs_do(request), d, req->body, key, NULL);    \
} while (0)
#define apreq_xs_args_push(sv,d,k) apreq_xs_push(args,sv,d,k)
#define apreq_xs_body_push(sv,d,k) apreq_xs_push(body,sv,d,k)
#define apreq_xs_table_push(sv,d,k) apreq_xs_push(table,sv,d,k)

#define apreq_xs_request_sv2table(sv) apreq_params(apreq_env_pool(env), \
                                                   apreq_xs_sv2(request,sv))
#define apreq_xs_args_sv2table(sv) apreq_xs_sv2(request,sv)->args
#define apreq_xs_body_sv2table(sv) apreq_xs_sv2(request,sv)->body
#define apreq_xs_table_sv2table(sv) apreq_xs_sv2table(sv)
#define apreq_xs_request_sv2env(sv) apreq_xs_sv2(request,sv)->env
#define apreq_xs_args_sv2env(sv) apreq_xs_sv2(request,sv)->env
#define apreq_xs_body_sv2env(sv) apreq_xs_sv2(request,sv)->env
#define apreq_xs_table_sv2env(sv) apreq_xs_sv2env(SvRV(sv))

#define apreq_xs_request_param(sv,k) apreq_param(apreq_xs_sv2(request,sv),k)
#define apreq_xs_args_param(sv,k) \
                     S2P(apr_table_get(apreq_xs_args_sv2table(sv),k))
#define apreq_xs_body_param(sv,k) \
                     S2P(apr_table_get(apreq_xs_body_sv2table(sv),k))
#define apreq_xs_table_param(sv,k) \
                     S2P(apr_table_get(apreq_xs_sv2table(sv),k))

APREQ_XS_DEFINE_TABLE_GET(request, PARAM_TABLE, param, NULL, 1);
APREQ_XS_DEFINE_TABLE_GET(args,    PARAM_TABLE, param, NULL, 1);
APREQ_XS_DEFINE_TABLE_GET(body,    PARAM_TABLE, param, NULL, 1);
APREQ_XS_DEFINE_TABLE_GET(table,   PARAM_TABLE, param, NULL, 1);

APREQ_XS_DEFINE_POOL(request);
APREQ_XS_DEFINE_POOL(table);

APREQ_XS_DEFINE_TABLE_MAKE(request);
APREQ_XS_DEFINE_TABLE_METHOD_N(param,set);
APREQ_XS_DEFINE_TABLE_METHOD_N(param,add);

static XS(apreq_xs_request_config)
{
    dXSARGS;
    apreq_request_t *req;
    int j;
    if (items % 2 != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "usage: $req->config(%settings)");

    req = apreq_xs_sv2(request,ST(0));

    for (j = 1; j + 1 < items; j += 2) {
        STRLEN alen, vlen;
        const char *attr = SvPVbyte(ST(j),alen), 
                    *val = SvPVbyte(ST(j+1),vlen);

        if (strcasecmp(attr,"POST_MAX")== 0
            || strcasecmp(attr, "MAX_BODY") == 0) 
        {
            apreq_env_max_body(req->env,
                               (apr_off_t)apreq_atoi64f(val));
        }
        else if (strcasecmp(attr, "TEMP_DIR") == 0) {
            apreq_env_temp_dir(req->env, val);
        }
        else if (strcasecmp(attr, "MAX_BRIGADE") == 0) {
            apreq_env_max_brigade(req->env, (apr_ssize_t)apreq_atoi64f(val));
        }
        else if (strcasecmp(attr, "DISABLE_UPLOADS") == 0) {
            if (req->parser == NULL)
                req->parser = apreq_parser(req->env, NULL);
            if (req->parser != NULL)
                apreq_add_hook(req->parser,
                               apreq_make_hook(apreq_env_pool(req->env), 
                                               apreq_hook_disable_uploads,
                                               NULL, NULL));
        }
        else if (strcasecmp(attr, "UPLOAD_HOOK") == 0) {
            ;
        }
        else if (strcasecmp(attr, "HOOK_DATA") == 0) {
            ;
        }
        else {
            Perl_warn(aTHX_ "Apache::Request::config: "
                      "Unrecognized attribute %s, skipped", attr);
        }
    }
    XSRETURN(0);
}

static XS(apreq_xs_request_parse)
{
    dXSARGS;
    apreq_request_t *req;
    apr_status_t s;
    if (items != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "usage: $req->parse()");

    req = apreq_xs_sv2(request,ST(0));

    do s = apreq_env_read(req->env, APR_BLOCK_READ, READ_BLOCK_SIZE);
    while (s == APR_INCOMPLETE);

    if (GIMME_V != G_VOID)
        XSRETURN_IV(s);

    if (s != APR_SUCCESS)
        apreq_xs_croak(aTHX_ newHV(), s, "Apache::Request::parse", 
                       "Apache::Request::Error");
}
