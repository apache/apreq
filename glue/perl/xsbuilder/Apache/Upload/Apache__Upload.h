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

/* avoid namespace collisions from perl's XSUB.h */
#include "modperl_perl_unembed.h"

/* XXX modperl_* dependency for T_HASHOBJ support */
#include "modperl_common_util.h"

#define apreq_xs_upload_error_check   do {                              \
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
            apreq_xs_croak(aTHX_ newHV(), s, "Apache::Request::upload", \
                           "Apache::Request::Error");                   \
        }                                                               \
    }                                                                   \
} while (0)


#define apreq_xs_upload_table_error_check 


#define READ_BLOCK_SIZE (1024 * 256)
#define S2P(s) (s ? apreq_value_to_param(apreq_strtoval(s)) : NULL)
#define apreq_xs_upload_do      (items==1 ? apreq_xs_upload_table_keys  \
                                : apreq_xs_upload_table_values)

#define apreq_xs_upload_push(sv,d,key) do {                             \
    apreq_request_t *req = apreq_xs_sv2(request,sv);                    \
    apr_status_t s;                                                     \
    do s = apreq_env_read(req->env, APR_BLOCK_READ, READ_BLOCK_SIZE);   \
    while (s == APR_INCOMPLETE);                                        \
    if (req->body)                                                      \
        apr_table_do(apreq_xs_upload_do, d, req->body, key, NULL);      \
} while (0)

#define apreq_xs_upload_table_push(sv,d,k) apreq_xs_push(upload_table,sv,d,k)
#define apreq_xs_upload_sv2table(sv) apreq_uploads(apreq_env_pool(env), \
                                                   apreq_xs_sv2(request,sv))
#define apreq_xs_upload_table_sv2table(sv) apreq_xs_sv2table(sv)
#define apreq_xs_upload_sv2env(sv) apreq_xs_sv2(request,sv)->env
#define apreq_xs_upload_table_sv2env(sv) apreq_xs_sv2env(SvRV(sv))
#define apreq_xs_upload_param(sv,k) apreq_upload(apreq_xs_sv2(request,sv),k)
#define apreq_xs_upload_table_param(sv,k) \
                     S2P(apr_table_get(apreq_xs_sv2table(sv),k))


/* uploads are represented by the full apreq_param_t in C */
#define apreq_upload_t apreq_param_t
#define apreq_xs_param2sv(ptr,class,parent)  apreq_xs_2sv(ptr,class,parent)
#define apreq_xs_sv2param(sv) ((apreq_upload_t *)SvIVX(SvRV(sv)))

static int apreq_xs_upload_table_keys(void *data, const char *key,
                                      const char *val)
{
#ifdef USE_ITHREADS
    struct apreq_xs_do_arg *d = (struct apreq_xs_do_arg *)data;
    dTHXa(d->perl);
#endif

    dSP;

    if (key) {
        if (val && apreq_value_to_param(apreq_strtoval(val))->bb)
            XPUSHs(sv_2mortal(newSVpv(key,0)));
        else    /* not an upload, so skip it */
            return 1;
    }
    else
        XPUSHs(&PL_sv_undef);

    PUTBACK;
    return 1;

}

#define UPLOAD_TABLE  "Apache::Upload::Table"
#define UPLOAD_PKG    "Apache::Upload"
APREQ_XS_DEFINE_GET(upload, UPLOAD_TABLE, param, UPLOAD_PKG, RETVAL->bb);
APREQ_XS_DEFINE_GET(upload_table, UPLOAD_TABLE, param, UPLOAD_PKG, 1);
APREQ_XS_DEFINE_ENV(upload);

APREQ_XS_DEFINE_POOL(upload_table);

APR_INLINE
static XS(apreq_xs_upload_link)
{
    dXSARGS;
    MAGIC *mg;
    void *env;
    const char *name, *fname;
    apr_bucket_brigade *bb;
    apr_file_t *f;

    if (items != 2 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $upload->link($name)");

    if (!(mg = mg_find(SvRV(ST(0)), PERL_MAGIC_ext)))
        Perl_croak(aTHX_ "$upload->link($name): can't find env");

    env = mg->mg_ptr;
    bb = apreq_xs_sv2param(ST(0))->bb;
    name = SvPV_nolen(ST(1));

    f = apreq_brigade_spoolfile(bb);
    if (f == NULL) {
        apr_off_t len;
        apr_status_t s;

        s = apr_file_open(&f, name, APR_CREATE | APR_EXCL | APR_WRITE |
                          APR_READ | APR_BINARY | APR_BUFFERED,
                          APR_OS_DEFAULT,
                          apreq_env_pool(env));
        if (s != APR_SUCCESS || 
            apreq_brigade_fwrite(f, &len, bb) != APR_SUCCESS)
            XSRETURN_UNDEF;
    
        XSRETURN_YES;
    }
    if (apr_file_name_get(&fname, f) != APR_SUCCESS)
        XSRETURN_UNDEF;

    if (PerlLIO_link(fname, name) >= 0)
        XSRETURN_YES;
    else {
        apr_status_t s = apr_file_copy(fname, name,
                                       APR_OS_DEFAULT, 
                                       apreq_env_pool(env));
        if (s == APR_SUCCESS)
            XSRETURN_YES;
    }

    XSRETURN_UNDEF;
}


static XS(apreq_xs_upload_slurp)
{
    dXSARGS;
    MAGIC *mg;
    void *env;
    char *data;
    apr_off_t len_off;
    apr_size_t len_size;
    apr_bucket_brigade *bb;
    apr_status_t s;

    if (items != 2 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $upload->slurp($data)");

    if (!(mg = mg_find(SvRV(ST(0)), PERL_MAGIC_ext)))
        Perl_croak(aTHX_ "$upload->slurp($data): can't find env");

    env = mg->mg_ptr;
    bb = apreq_xs_sv2param(ST(0))->bb;

    s = apr_brigade_length(bb, 0, &len_off);
    if (s != APR_SUCCESS)
        XSRETURN_IV(s);

    len_size = len_off; /* max_body setting will be low enough to prevent
                         * overflow, but even if it wasn't the code below will
                         * at worst truncate the slurp data (not segfault).
                         */
                         
    SvUPGRADE(ST(1), SVt_PV);
    data = SvGROW(ST(1), len_size + 1);
    data[len_size] = 0;
    SvCUR_set(ST(1), len_size);
    SvPOK_only(ST(1));
    s = apr_brigade_flatten(bb, data, &len_size);
    XSRETURN_IV(s);
}

static XS(apreq_xs_upload_size)
{
    dXSARGS;
    MAGIC *mg;
    void *env;
    apr_bucket_brigade *bb;
    apr_status_t s;
    apr_off_t len;

    if (items != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $upload->size()");

    if (!(mg = mg_find(SvRV(ST(0)), PERL_MAGIC_ext)))
        Perl_croak(aTHX_ "$upload->size(): can't find env");

    env = mg->mg_ptr;
    bb = apreq_xs_sv2param(ST(0))->bb;

    s = apr_brigade_length(bb, 1, &len);

    if (s != APR_SUCCESS) {
        apreq_log(APREQ_ERROR s, env, "apreq_xs_upload_size:"
                  "apr_brigade_length failed");
        Perl_croak(aTHX_ "$upload->size: can't get brigade length");
    }
    XSRETURN_IV((IV)len);
}

static XS(apreq_xs_upload_type)
{
    dXSARGS;
    apreq_param_t *upload;
    const char *ct, *sc;
    STRLEN len;

    if (items != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $upload->type()");

    upload = apreq_xs_sv2param(ST(0));
    ct = apr_table_get(upload->info, "Content-Type");
    if (ct == NULL)
        Perl_croak(aTHX_ "$upload->type: can't find Content-Type header");
    
    if ((sc = strchr(ct, ';')))
        len = sc - ct;
    else
        len = strlen(ct);

    ST(0) = sv_2mortal(newSVpvn(ct,len));
    XSRETURN(1);
}

static XS(apreq_xs_upload_brigade_copy)
{
    dXSARGS;
    apr_bucket_brigade *bb, *bb_copy;
    char *class;

    if (items != 2 || !SvPOK(ST(0)) || !SvROK(ST(1)))
        Perl_croak(aTHX_ "Usage: Apache::Upload::Brigade->new($bb)");

    class = SvPV_nolen(ST(0));
    bb = (apr_bucket_brigade *)SvIVX(SvRV(ST(1)));
    bb_copy = apr_brigade_create(bb->p,bb->bucket_alloc);
    APREQ_BRIGADE_COPY(bb_copy, bb);

    ST(0) = sv_2mortal(sv_setref_pv(newSV(0), class, bb_copy));
    XSRETURN(1);
}

static XS(apreq_xs_upload_brigade_read)
{
    dXSARGS;
    apr_bucket_brigade *bb;
    apr_bucket *e, *end;
    IV want = -1, offset = 0;
    SV *sv;
    apr_status_t s;
    char *buf;

    switch (items) {
    case 4:
        offset = SvIV(ST(3));
    case 3:
        want = SvIV(ST(2));
    case 2:
        sv = ST(1);
        bb = (apr_bucket_brigade *)SvIVX(SvRV(ST(0)));
        break;
    default:
        Perl_croak(aTHX_ "Usage: $bb->READ($buf,$len,$off)");
    }

    if (want == 0)
        XSRETURN_IV(0);

    if (APR_BRIGADE_EMPTY(bb))
        XSRETURN_UNDEF;


    if (want == -1) {
        const char *data;
        apr_size_t dlen;
        e = APR_BRIGADE_FIRST(bb);
        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            apreq_xs_croak(aTHX_ newHV(), s, 
                           "Apache::Request::Upload::Brigade::READ "
                           "apr_bucket_read failed", "APR::Error");
        want = dlen;
        end = APR_BUCKET_NEXT(e);
    }
    else {
        switch (s = apr_brigade_partition(bb, (apr_off_t)want, &end)) {
            apr_off_t len;

        case APR_INCOMPLETE:
            s = apr_brigade_length(bb, 1, &len);
            if (s != APR_SUCCESS)
                apreq_xs_croak(aTHX_ newHV(), s, 
                               "Apache::Request::Upload::Brigade::READ "
                               "apr_brigade_length failed", "APR::Error");
            want = len;

        case APR_SUCCESS:
            break;

        default:
            apreq_xs_croak(aTHX_ newHV(), s, 
                           "Apache::Request::Upload::Brigade::READ "
                           "apr_brigade_partition failed", "APR::Error");
        }
    }

    SvUPGRADE(sv, SVt_PV);
    SvGROW(sv, want + offset + 1);
    buf = SvPVX(sv) + offset;
    SvCUR_set(sv, want + offset);

    while ((e = APR_BRIGADE_FIRST(bb)) != end) {
        const char *data;
        apr_size_t dlen;
        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            apreq_xs_croak(aTHX_ newHV(), s, 
                           "Apache::Request::Upload::Brigade::READ "
                           "apr_bucket_read failed", "APR::Error");
        memcpy(buf, data, dlen);
        buf += dlen;
        apr_bucket_delete(e);
    }

    *buf = 0;
    SvPOK_only(sv);
    XSRETURN_IV(want);
}

static XS(apreq_xs_upload_brigade_readline)
{
    dXSARGS;
    apr_bucket_brigade *bb;
    apr_bucket *e;
    SV *sv;
    apr_status_t s;

    if (items != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $bb->READLINE");

    bb = (apr_bucket_brigade *)SvIVX(SvRV(ST(0)));

    if (APR_BRIGADE_EMPTY(bb))
        XSRETURN(0);

    XSprePUSH;

    sv = sv_2mortal(newSVpvn("",0));
    XPUSHs(sv);

    while (!APR_BRIGADE_EMPTY(bb)) {
        const char *data;
        apr_size_t dlen;
        const char *eol;

        e = APR_BRIGADE_FIRST(bb);
        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            apreq_xs_croak(aTHX_ newHV(), s, 
                           "Apache::Request::Upload::Brigade::READLINE "
                           "apr_bucket_read failed", "APR::Error");

        eol = memchr(data, '\012', dlen); /* look for LF (linefeed) */

        if (eol != NULL) {
            if (eol < data + dlen - 1) {
                dlen = eol - data + 1;
                apr_bucket_split(e, dlen);
            }

            sv_catpvn(sv, data, dlen);
            apr_bucket_delete(e);

            if (GIMME_V != G_ARRAY || APR_BRIGADE_EMPTY(bb))
                break;

            sv = sv_2mortal(newSVpvn("",0));
            XPUSHs(sv);
        }
        else {
            sv_catpvn(sv, data, dlen);
            apr_bucket_delete(e);
        }
    }

    PUTBACK;
}


static XS(apreq_xs_upload_tempname)
{
    dXSARGS;
    MAGIC *mg;
    void *env;
    apr_bucket_brigade *bb;
    apr_status_t s;
    apr_file_t *file;
    const char *path;

    if (items != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $upload->tempname()");

    if (!(mg = mg_find(SvRV(ST(0)), PERL_MAGIC_ext)))
        Perl_croak(aTHX_ "$upload->tempname(): can't find env");

    env = mg->mg_ptr;
    bb = apreq_xs_sv2param(ST(0))->bb;
    file = apreq_brigade_spoolfile(bb);

    if (file == NULL) {
        apr_bucket *last;
        apr_off_t len;
        const char *tmpdir = apreq_env_temp_dir(env, NULL);

        s = apreq_file_mktemp(&file, apreq_env_pool(env), tmpdir);

        if (s != APR_SUCCESS) {
            apreq_log(APREQ_ERROR s, env, "apreq_xs_upload_tempname: "
                      "apreq_file_mktemp failed");
            Perl_croak(aTHX_ "$upload->tempname: can't make tempfile");
        }

        s = apreq_brigade_fwrite(file, &len, bb);

        if (s != APR_SUCCESS) {
            apreq_log(APREQ_ERROR s, env, "apreq_xs_upload_tempname: "
                      "apreq_brigade_fwrite failed");
            Perl_croak(aTHX_ "$upload->tempname: "
                       "can't write brigade to tempfile");
        }

        last = apr_bucket_file_create(file, len, 0, bb->p, bb->bucket_alloc);
        APR_BRIGADE_INSERT_TAIL(bb, last);
    }

    s = apr_file_name_get(&path, file);
    if (s != APR_SUCCESS)
        XSRETURN_UNDEF;

    ST(0) = sv_2mortal(newSVpvn(path, strlen(path)));
    XSRETURN(1);
}


#ifdef IMPLEMENT_UPLOAD_HOOKS

struct hook_ctx {
    SV                  *hook_data;
    SV                  *hook;
    SV                  *upload;
    SV                  *bucket_data;
    PerlInterpreter     *perl;
};


#define DEREF(slot) if (ctx->slot) SvREFCNT_dec(ctx->slot)

static void upload_hook_cleanup(void *ctx_)
{
    struct hook_ctx *ctx = ctx_;

#ifdef USE_ITHREADS
    dTHXa(ctx->perl);
#endif

    DEREF(hook_data);
    DEREF(hook);
    DEREF(upload);
    DEREF(bucket_data);
}


APR_INLINE
static void eval_upload_hook(pTHX_ SV *hook, SV* upload, 
                             SV *bucket_data, SV* hook_data)
{
    dSP;

    PUSHMARK(SP);
    EXTEND(SP, 3);
    ENTER;
    SAVETMPS;

    PUSHs(upload);
    PUSHs(bucket_data);
    PUSHs(hook_data);

    PUTBACK;
    perl_call_sv(hook, G_EVAL|G_DISCARD);
    FREETMPS;
    LEAVE;
}


static
APREQ_DECLARE_HOOK(apreq_xs_hook_wrapper)
{
    struct hook_ctx *ctx = hook->ctx; /* set ctx during config */
    apr_bucket *e;
    apr_status_t s = APR_SUCCESS;

#ifdef USE_ITHREADS
    dTHXa(ctx->perl);
#endif

    if (hook->next) {
        s = APREQ_RUN_HOOK(hook->next, env, param, bb);
        if (s != APR_SUCCESS)
            return s;
    }

    for (e = APR_BRIGADE_FIRST(bb); e!= APR_BRIGADE_SENTINEL(bb);
         e = APR_BUCKET_NEXT(e))
    {
        apr_off_t len;
        const char *data;

        if (APR_BUCKET_IS_EOS(e)) {
            /*last call */
            eval_upload_hook(aTHX_ ctx->hook, ctx->upload, 
                             &PL_sv_undef, ctx->hook_data);

            if (SvTRUE(ERRSV)) {
                /*XXX: handle error */
                s = APR_EGENERAL;
            }

            break;
        }

        s = apr_bucket_read(e, &data, &len, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            break;

        SvPVX(ctx->bucket_data) = (char *)data;
        SvCUR(ctx->bucket_data) = (STRLEN)len;

        eval_upload_hook(aTHX_ ctx->hook, ctx->upload, ctx->bucket_data,
                         ctx->hook_data);

        if (SvTRUE(ERRSV)) {
            /*XXX: handle error */
            s = APR_EGENERAL;
            break;
        }

    }

    return s;
}

static XS(apreq_xs_upload_hook)
{
    /*this needs to initialize hook_ctx (bucket_data must be CONSTANT with LEN=-1)*/

}

#endif
