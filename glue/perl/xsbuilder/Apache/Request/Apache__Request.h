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
        req = (apreq_request_t *)SvIVX(obj);                            \
        s = req->args_status;                                           \
        if (s == APR_SUCCESS && req->parser)                            \
            s = apreq_parse_request(req, NULL);                         \
        switch (s) {                                                    \
        case APR_INCOMPLETE:                                            \
        case APR_SUCCESS:                                               \
            break;                                                      \
        default:                                                        \
            APREQ_XS_THROW_ERROR(request, s, "Apache::Request::param",  \
                                 "Apache::Request::Error");             \
       }                                                                \
    }                                                                   \
} while (0)

#define apreq_xs_request_args_error_check do {                          \
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
        req = (apreq_request_t *)SvIVX(obj);                            \
        s = req->args_status;                                           \
        if (s != APR_SUCCESS)                                           \
            APREQ_XS_THROW_ERROR(request, s, "Apache::Request::args",   \
                                 "Apache::Request::Error");             \
    }                                                                   \
} while (0)

#define apreq_xs_request_body_error_check          do {                 \
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
        req = (apreq_request_t *)SvIVX(obj);                            \
        if (req->parser == NULL)                                        \
           break;                                                       \
        switch (s = apreq_parse_request(req,NULL)) {                    \
        case APR_INCOMPLETE:                                            \
        case APR_SUCCESS:                                               \
            break;                                                      \
        default:                                                        \
            APREQ_XS_THROW_ERROR(request, s, "Apache::Request::body",   \
                                 "Apache::Request::Error");             \
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
    apreq_request_t *req = (apreq_request_t *)SvIVX(sv);                \
    apr_status_t s;                                                     \
    apr_table_do(apreq_xs_do(request), d, req->args, key, NULL);        \
    do s = apreq_env_read(req->env, APR_BLOCK_READ, READ_BLOCK_SIZE);   \
    while (s == APR_INCOMPLETE);                                        \
    if (req->body)                                                      \
        apr_table_do(apreq_xs_do(request), d, req->body, key, NULL);    \
} while (0)
#define apreq_xs_request_args_push(sv,d,k) apreq_xs_push(request_args,sv,d,k)
#define apreq_xs_request_body_push(sv,d,k) apreq_xs_push(request_body,sv,d,k)
#define apreq_xs_table_push(sv,d,k) apreq_xs_push(table,sv,d,k)

#define apreq_xs_request_sv2table(sv) apreq_params(apreq_env_pool(env), \
                                        (apreq_request_t *)SvIVX(sv))
#define apreq_xs_request_args_sv2table(sv)  ((apreq_request_t *)SvIVX(sv))->args
#define apreq_xs_request_body_sv2table(sv)  ((apreq_request_t *)SvIVX(sv))->body
#define apreq_xs_table_sv2table(sv) ((apr_table_t *)SvIVX(sv))
#define apreq_xs_request_sv2env(sv) ((apreq_request_t *)SvIVX(sv))->env
#define apreq_xs_request_args_sv2env(sv)    ((apreq_request_t *)SvIVX(sv))->env
#define apreq_xs_request_body_sv2env(sv)    ((apreq_request_t *)SvIVX(sv))->env
#define apreq_xs_table_sv2env(sv)   apreq_xs_sv2env(sv)

#define apreq_xs_request_param(sv,k) apreq_param((apreq_request_t *)SvIVX(sv),k)
#define apreq_xs_request_args_param(sv,k) \
                     S2P(apr_table_get(apreq_xs_request_args_sv2table(sv),k))
#define apreq_xs_request_body_param(sv,k) \
                     S2P(apr_table_get(apreq_xs_request_body_sv2table(sv),k))
#define apreq_xs_table_param(sv,k) \
                     S2P(apr_table_get(apreq_xs_table_sv2table(sv),k))

APREQ_XS_DEFINE_TABLE_GET(request,         PARAM_TABLE, param, NULL, 1);
APREQ_XS_DEFINE_TABLE_GET(request_args,    PARAM_TABLE, param, NULL, 1);
APREQ_XS_DEFINE_TABLE_GET(request_body,    PARAM_TABLE, param, NULL, 1);
APREQ_XS_DEFINE_TABLE_GET(table,           PARAM_TABLE, param, NULL, 1);

APREQ_XS_DEFINE_POOL(request);
APREQ_XS_DEFINE_POOL(table);

APREQ_XS_DEFINE_TABLE_MAKE(request);
APREQ_XS_DEFINE_TABLE_METHOD_N(param,set);
APREQ_XS_DEFINE_TABLE_METHOD_N(param,add);


struct hook_ctx {
    SV                  *hook_data;
    SV                  *hook;
    SV                  *bucket_data;
    SV                  *parent;
    PerlInterpreter     *perl;
};


#define DEREF(slot) if (ctx->slot) SvREFCNT_dec(ctx->slot)

static apr_status_t upload_hook_cleanup(void *ctx_)
{
    struct hook_ctx *ctx = ctx_;

#ifdef USE_ITHREADS
    dTHXa(ctx->perl);
#endif

    DEREF(hook_data);
    DEREF(hook);
    DEREF(bucket_data);
    DEREF(parent);
    return APR_SUCCESS;
}

APR_INLINE
static apr_status_t eval_upload_hook(pTHX_ apreq_param_t *upload, 
                                     void *env, struct hook_ctx *ctx)
{
    dSP;
    SV *sv = ctx->bucket_data;
    STRLEN len = SvPOK(sv) ? SvCUR(sv) : 0;

    PUSHMARK(SP);
    EXTEND(SP, 4);
    ENTER;
    SAVETMPS;

    PUSHs(sv_2mortal(apreq_xs_2sv(upload, "Apache::Upload", ctx->parent)));
    PUSHs(sv);
    PUSHs(sv_2mortal(newSViv(len)));
    if (ctx->hook_data)
        PUSHs(ctx->hook_data);

    PUTBACK;
    perl_call_sv(ctx->hook, G_EVAL|G_DISCARD);
    FREETMPS;
    LEAVE;

    if (SvTRUE(ERRSV)) {
        Perl_warn(aTHX_ "Upload hook failed: %s", SvPV_nolen(ERRSV));
        return APR_EGENERAL;
    }
    return APR_SUCCESS;
}


static apr_status_t apreq_xs_upload_hook(APREQ_HOOK_ARGS)
{
    struct hook_ctx *ctx = hook->ctx; /* ctx set during $req->config */
    apr_bucket *e;
    apr_status_t s = APR_SUCCESS;
#ifdef USE_ITHREADS
    dTHXa(ctx->perl);
#endif

    for (e = APR_BRIGADE_FIRST(bb); e!= APR_BRIGADE_SENTINEL(bb);
         e = APR_BUCKET_NEXT(e))
    {
        apr_off_t len;
        const char *data;
        apreq_log(APREQ_DEBUG 0, env, "looping through buckets");

        if (APR_BUCKET_IS_EOS(e)) {  /*last call on this upload */           
            SV *sv = ctx->bucket_data;
            ctx->bucket_data = &PL_sv_undef;
            s = eval_upload_hook(aTHX_ param, env, ctx);
            ctx->bucket_data = sv;
            if (s != APR_SUCCESS)
                return s;
            apreq_log(APREQ_DEBUG 0, env, "upload hook saw eos bucket");

            break;
        }

        s = apr_bucket_read(e, &data, &len, APR_BLOCK_READ);
        if (s != APR_SUCCESS) {
            apreq_log(APREQ_WARN s, env, "Can't read bucket, skipping it.");
            s = APR_SUCCESS;
            continue;
        }
        sv_setpvn(ctx->bucket_data, data, (STRLEN)len);
        s = eval_upload_hook(aTHX_ param, env, ctx);

        if (s != APR_SUCCESS)
            return s;

    }

    if (hook->next)
        s = APREQ_RUN_HOOK(hook->next, env, param, bb);

    return s;
}



static XS(apreq_xs_request_config)
{
    dXSARGS;
    apr_pool_t *pool;
    apreq_request_t *req;
    apreq_hook_t *upload_hook = NULL;
    SV *sv, *obj;
    SV *hook_data = NULL;
    int j;

    if (items % 2 != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "usage: $req->config(%settings)");

    sv = ST(0);
    obj = apreq_xs_find_obj(aTHX_ sv, "request");
    req = (apreq_request_t *)SvIVX(obj);
    pool = apreq_env_pool(req->env);

    for (j = 1; j + 1 < items; j += 2) {
        STRLEN alen;
        const char *attr = SvPVbyte(ST(j),alen);

        if (strcasecmp(attr,"POST_MAX")== 0
            || strcasecmp(attr, "MAX_BODY") == 0) 
        {
            const char *val = SvPV_nolen(ST(j+1));
            apreq_env_max_body(req->env,
                               (apr_off_t)apreq_atoi64f(val));
        }
        else if (strcasecmp(attr, "TEMP_DIR") == 0) {
            const char *val = SvPV_nolen(ST(j+1));
             apreq_env_temp_dir(req->env, val);
        }
        else if (strcasecmp(attr, "MAX_BRIGADE") == 0) {
            const char *val = SvPV_nolen(ST(j+1));
            apreq_env_max_brigade(req->env, (apr_ssize_t)apreq_atoi64f(val));
        }
        else if (strcasecmp(attr, "DISABLE_UPLOADS") == 0) {
            if (req->parser == NULL) {
                req->parser = apreq_parser(req->env, NULL);
                if (req->parser == NULL) {
                    Perl_warn(aTHX_ "Apache::Request::config: "
                              "cannot disable/enable uploads (parser not found)");
                    continue;
                }
            }
            if (SvTRUE(ST(j+1))) {
                /* add disable_uploads hook */
                if (req->parser == NULL)
                    req->parser = apreq_parser(req->env, NULL);
                if (req->parser != NULL)
                    apreq_add_hook(req->parser,
                                   apreq_make_hook(pool, 
                                                   apreq_hook_disable_uploads,
                                                   NULL, NULL));
            }
            else {
                /* remove all disable_uploads hooks */
                apreq_hook_t *first = req->parser->hook;

                while (first != NULL && first->hook == apreq_hook_disable_uploads)
                    first = first->next;

                req->parser->hook = first;

                if (first != NULL) {
                    apreq_hook_t *cur;

                    for (cur = first->next; cur != NULL; cur = cur->next) {
                        if (cur->hook == apreq_hook_disable_uploads)
                            first->next = cur->next;
                        else
                            first = cur;
                    }
                }
            }
        }
        else if (strcasecmp(attr, "UPLOAD_HOOK") == 0) {
            struct hook_ctx *ctx = apr_palloc(apreq_env_pool(req->env), sizeof *ctx);
            if (upload_hook)
                Perl_croak(aTHX_ "Apache::Request::config: "
                           "cannot set UPLOAD_HOOK more than once");

            ctx->hook_data = NULL;
            ctx->hook = newSVsv(ST(j+1));
            ctx->bucket_data = newSV(8000);
            ctx->parent = SvREFCNT_inc(obj);
#ifdef USE_ITHREADS
            ctx->perl = aTHX;
#endif
            upload_hook = apreq_make_hook(pool, apreq_xs_upload_hook, NULL, ctx);
            apreq_add_hook(req->parser, upload_hook);
            apr_pool_cleanup_register(pool, ctx, upload_hook_cleanup, NULL);
        }

        else if (strcasecmp(attr, "HOOK_DATA") == 0) {
            if (hook_data)
                Perl_croak(aTHX_ "Apache::Request::config: "
                           "cannot set HOOK_DATA more than once");
            hook_data = ST(j+1);
        }
        else {
            Perl_warn(aTHX_ "Apache::Request::config: "
                      "Unrecognized attribute %s, skipped", attr);
        }
    }

    if (upload_hook && hook_data)
        ((struct hook_ctx *)upload_hook->ctx)->hook_data = newSVsv(hook_data);

    XSRETURN(0);
}

static XS(apreq_xs_request_parse)
{
    dXSARGS;
    apreq_request_t *req;
    apr_status_t s;
    SV *sv, *obj;
    if (items != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "usage: $req->parse()");

    sv = ST(0);
    obj = apreq_xs_find_obj(aTHX_ sv, "request");
    req = (apreq_request_t *)SvIVX(obj);

    do s = apreq_env_read(req->env, APR_BLOCK_READ, READ_BLOCK_SIZE);
    while (s == APR_INCOMPLETE);

    if (GIMME_V != G_VOID)
        XSRETURN_IV(s);

    if (s != APR_SUCCESS)
        APREQ_XS_THROW_ERROR(request, s, "Apache::Request::parse", "Apache::Request::Error");
}
