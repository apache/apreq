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
#include "apr_optional.h"

/* avoid namespace collisions from perl's XSUB.h */
#include "modperl_perl_unembed.h"

/* Temporary work-around for missing apr_perlio.h file.
 * #include "apr_perlio.h" 
 */
#ifndef APR_PERLIO_H

#ifdef PERLIO_LAYERS
#include "perliol.h"
#else 
#include "iperlsys.h"
#endif

#include "apr_portable.h"
#include "apr_file_io.h"
#include "apr_errno.h"

#ifndef MP_SOURCE_SCAN
#include "apr_optional.h"
#endif

/* 5.6.0 */
#ifndef IoTYPE_RDONLY
#define IoTYPE_RDONLY '<'
#endif
#ifndef IoTYPE_WRONLY
#define IoTYPE_WRONLY '>'
#endif

typedef enum {
    APR_PERLIO_HOOK_READ,
    APR_PERLIO_HOOK_WRITE
} apr_perlio_hook_e;

void apr_perlio_init(pTHX);

/* The following functions can be used from other .so libs, they just
 * need to load APR::PerlIO perl module first
 */
#ifndef MP_SOURCE_SCAN

#ifdef PERLIO_LAYERS
PerlIO *apr_perlio_apr_file_to_PerlIO(pTHX_ apr_file_t *file, apr_pool_t *pool,
                                      apr_perlio_hook_e type);
APR_DECLARE_OPTIONAL_FN(PerlIO *,
                        apr_perlio_apr_file_to_PerlIO,
                        (pTHX_ apr_file_t *file, apr_pool_t *pool,
                         apr_perlio_hook_e type));
#endif /* PERLIO_LAYERS */


SV *apr_perlio_apr_file_to_glob(pTHX_ apr_file_t *file, apr_pool_t *pool,
                                apr_perlio_hook_e type);
APR_DECLARE_OPTIONAL_FN(SV *,
                        apr_perlio_apr_file_to_glob,
                        (pTHX_ apr_file_t *file, apr_pool_t *pool,
                         apr_perlio_hook_e type));
#endif /* MP_SOURCE_SCAN */

#endif /*APR_PERLIO_H*/





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
#define apreq_xs_param2sv(ptr,class)  apreq_xs_2sv(ptr,class)
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

static APR_OPTIONAL_FN_TYPE(apr_perlio_apr_file_to_glob) *f2g;

static XS(apreq_xs_upload_fh)
{
    dXSARGS;
    MAGIC *mg;
    void *env;
    apr_bucket_brigade *bb;
    apr_status_t s;
    apr_off_t len;
    apr_file_t *file;

    if (items != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $upload->fh()");

    if (!(mg = mg_find(SvRV(ST(0)), PERL_MAGIC_ext)))
        Perl_croak(aTHX_ "$upload->fh(): can't find env");

    env = mg->mg_ptr;
    bb = apreq_xs_sv2param(ST(0))->bb;
    file = apreq_brigade_spoolfile(bb);

    if (file == NULL) {
        apr_bucket *last;
        const char *tmpdir = apreq_env_temp_dir(env, NULL);

        s = apreq_file_mktemp(&file, apreq_env_pool(env), tmpdir);

        if (s != APR_SUCCESS) {
            apreq_log(APREQ_ERROR s, env, "apreq_xs_upload_fh:"
                      "apreq_file_mktemp failed");
            Perl_croak(aTHX_ "$upload->fh: can't make tempfile");
        }

        s = apreq_brigade_fwrite(file, &len, bb);

        if (s != APR_SUCCESS) {
            apreq_log(APREQ_ERROR s, env, "apreq_xs_upload_fh:"
                      "apreq_brigade_fwrite failed");
            Perl_croak(aTHX_ "$upload->fh: can't write brigade to tempfile");
        }

        last = apr_bucket_file_create(file, len, 0, bb->p, bb->bucket_alloc);
        APR_BRIGADE_INSERT_TAIL(bb, last);
    }

    /* Reset seek pointer before passing apr_file_t to perl. */
    len = 0;
    apr_file_seek(file, 0, &len);

    /* Should we pass a dup(2) of the file instead? */
    ST(0) = f2g(aTHX_ file, bb->p, APR_PERLIO_HOOK_READ);
    XSRETURN(1);
}

struct hook_ctx {
    SV                  *hook_data;
    SV                  *hook;
    SV                  *upload;
    SV                  *bucket_data;
    PerlInterpreter     *perl;
};

#ifdef IMPLEMENT_UPLOAD_HOOKS

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
