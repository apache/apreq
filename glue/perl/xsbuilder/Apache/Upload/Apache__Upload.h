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

/* XXX modperl_* dependency for T_HASHOBJ support */
#include "modperl_common_util.h"

#define apreq_xs_request_upload_error_check   do {                      \
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
        switch(s = req->body_status) {                                  \
        case APR_EINIT:                                                 \
        case APR_SUCCESS:                                               \
            break;                                                      \
        default:                                                        \
            APREQ_XS_THROW_ERROR(request, s, "Apache::Request::upload", \
                                 "Apache::Request::Error");             \
        }                                                               \
    }                                                                   \
} while (0)


#define apreq_xs_upload_table_error_check


#define READ_BLOCK_SIZE (1024 * 256)
#define S2P(s) (s ? apreq_value_to_param(apreq_strtoval(s)) : NULL)

#define apreq_xs_request_upload_push(sv,d,key) do {                     \
    apreq_request_t *req = (apreq_request_t *)SvIVX(sv);                \
    if (items == 1) {                                                   \
        apr_table_t *t = apreq_uploads(apreq_env_pool(req->env), req);  \
        if (t != NULL) {                                                \
            apr_table_compress(t, APR_OVERLAP_TABLES_MERGE);            \
            apr_table_do(apreq_xs_table_keys, d, t, key, NULL);         \
        }                                                               \
    }                                                                   \
    else {                                                              \
        apr_status_t s;                                                 \
        s = req->body_status;                                           \
        switch (s) {                                                    \
        case APR_EINIT:                                                 \
        case APR_INCOMPLETE:                                            \
            do s = apreq_env_read(req->env, APR_BLOCK_READ,             \
                                  READ_BLOCK_SIZE);                     \
            while (s == APR_INCOMPLETE);                                \
        }                                                               \
        if (req->body)                                                  \
            apr_table_do(apreq_xs_request_upload_table_values, d,       \
                         req->body, key, NULL);                         \
    }                                                                   \
} while (0)

#define apreq_xs_upload_table_push(sv,d,k) apreq_xs_push(upload_table,sv,d,k)
#define apreq_xs_request_upload_sv2table(sv) apreq_uploads(apreq_env_pool(env), \
                                                (apreq_request_t *)SvIVX(sv))
#define apreq_xs_upload_table_sv2table(sv) ((apr_table_t *)SvIVX(sv))
#define apreq_xs_request_upload_sv2env(sv)         ((apreq_request_t *)SvIVX(sv))->env
#define apreq_xs_upload_table_sv2env(sv)   apreq_xs_sv2env(sv)
#define apreq_xs_request_upload_param(sv,k) apreq_upload((apreq_request_t *)SvIVX(sv),k)
#define apreq_xs_upload_table_param(sv,k) \
                        S2P(apr_table_get(apreq_xs_upload_table_sv2table(sv),k))


/* uploads are represented by the full apreq_param_t in C */
#define apreq_upload_t apreq_param_t
#define apreq_xs_param2sv(ptr,class,parent)  apreq_xs_2sv(ptr,class,parent)
#define apreq_xs_sv2param(sv) ((apreq_param_t *)SvIVX(SvRV(sv)))


#define UPLOAD_TABLE  "Apache::Upload::Table"
#define UPLOAD_PKG    "Apache::Upload"

APREQ_XS_DEFINE_TABLE_GET(request_upload, UPLOAD_TABLE, param, UPLOAD_PKG, RETVAL->bb);
APREQ_XS_DEFINE_TABLE_GET(upload_table, UPLOAD_TABLE, param, UPLOAD_PKG, 1);
APREQ_XS_DEFINE_TABLE_FETCH(upload_table, param, UPLOAD_PKG);
APREQ_XS_DEFINE_TABLE_DO(upload_table, param, UPLOAD_PKG);
APREQ_XS_DEFINE_ENV(upload);

APREQ_XS_DEFINE_TABLE_MAKE(request, UPLOAD_PKG, strlen(UPLOAD_PKG));
APREQ_XS_DEFINE_TABLE_METHOD_N(param,add);
APREQ_XS_DEFINE_TABLE_METHOD_N(param,set);
APREQ_XS_DEFINE_TABLE_NEXTKEY(upload_table);



static XS(apreq_xs_upload_make)
{
    dXSARGS;
    void *env;
    const char *class, *name, *value, *filename;
    STRLEN nlen, vlen;
    apreq_param_t *upload;
    apr_bucket *b;
    apr_status_t s;
    apr_pool_t *p;
    apr_file_t *f;
    apr_finfo_t finfo;
    SV *sv, *obj;


    if (items != 5 || SvROK(ST(0)) || !SvROK(ST(1)))
        Perl_croak(aTHX_ "Usage: Apache::Upload->"
                   "make($env, $name, $value, $filename)");

    class = SvPV_nolen(ST(0));
    obj = SvRV(ST(1));
    env = (void *)SvIVX(obj);
    name = SvPV(ST(2), nlen);
    value = SvPV(ST(3), vlen);
    filename = SvPV_nolen(ST(4));

    p = apreq_env_pool(env);
    if (SvTAINTED(ST(4)))
        Perl_croak(aTHX_ "Apache::Upload::make: "
                   "cannot open tainted filename: %s", filename);

    s = apr_file_open(&f, filename, APR_READ | APR_BINARY, APR_OS_DEFAULT, p);

    if (s != APR_SUCCESS)
        goto make_error;

    s = apr_file_info_get(&finfo, APR_FINFO_SIZE, f);

    if (s != APR_SUCCESS)
        goto make_error;

    upload = apreq_make_param(p, name, nlen, value, vlen);

    upload->bb = apr_brigade_create(p, apreq_env_bucket_alloc(env));
    b = apr_bucket_file_create(f, 0, finfo.size, p, upload->bb->bucket_alloc);
    APR_BRIGADE_INSERT_TAIL(upload->bb, b);
    b = apr_bucket_immortal_create("", 0, upload->bb->bucket_alloc);
    APR_BRIGADE_INSERT_TAIL(upload->bb, b);

    upload->info = apr_table_make(p, APREQ_NELTS);
    apr_table_addn(upload->info, "Content-Disposition", 
                  apr_psprintf(p, "form-data; name=\"%s\"; filename=\"%s\"",
                               name, value));
    apr_table_addn(upload->info, "Content-Type", "application/octet-stream");

    sv = apreq_xs_2sv(upload, class, obj);
    if (SvTAINTED(ST(2)) || SvTAINTED(ST(3)))
        SvTAINTED_on(SvRV(sv));
    ST(0) = sv_2mortal(sv);
    XSRETURN(1);

 make_error:
    apreq_xs_croak(aTHX_ newHV(), s, 
                   "Apache::Upload::make", 
                   "APR::Error");

    XSRETURN_UNDEF;
}



static XS(apreq_xs_upload_link)
{
    dXSARGS;
    MAGIC *mg;
    void *env;
    const char *name, *fname;
    apr_bucket_brigade *bb;
    apr_file_t *f;
    apr_status_t s = APR_SUCCESS;
    SV *sv, *obj;

    if (items != 2 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $upload->link($filename)");

    if (SvTAINTED(ST(1)))
        Perl_croak(aTHX_ "$upload->link($filename): Cannot link to tainted $filename: %s", 
                   SvPV_nolen(ST(1)));

    sv = ST(0);
    obj = apreq_xs_find_obj(aTHX_ sv, "upload");

    if (!(mg = mg_find(obj, PERL_MAGIC_ext)))
        Perl_croak(aTHX_ "$upload->link($name): panic: can't find env.");

    env = mg->mg_ptr;
    bb = ((apreq_param_t *)SvIVX(obj))->bb;
    name = SvPV_nolen(ST(1));

    f = apreq_brigade_spoolfile(bb);
    if (f == NULL) {
        apr_off_t len;

        s = apr_file_open(&f, name, APR_CREATE | APR_EXCL | APR_WRITE |
                          APR_READ | APR_BINARY,
                          APR_OS_DEFAULT,
                          apreq_env_pool(env));
        if (s == APR_SUCCESS) {
            s = apreq_brigade_fwrite(f, &len, bb);
            if (s == APR_SUCCESS)
                XSRETURN_YES;
        }
        goto link_error;
    }
    s = apr_file_name_get(&fname, f);
    if (s != APR_SUCCESS)
        goto link_error;

    if (PerlLIO_link(fname, name) >= 0)
        XSRETURN_YES;
    else {
        s = apr_file_copy(fname, name,
                          APR_OS_DEFAULT, 
                          apreq_env_pool(env));
        if (s == APR_SUCCESS)
            XSRETURN_YES;
    }

 link_error:
    APREQ_XS_THROW_ERROR(upload, s, "Apache::Upload::link", 
                         "Apache::Upload::Error");
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
    SV *sv, *obj;
    apr_status_t s;

    if (items != 2 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $upload->slurp($data)");

    sv = ST(0);
    obj = apreq_xs_find_obj(aTHX_ sv, "upload");

    if (!(mg = mg_find(obj, PERL_MAGIC_ext)))
        Perl_croak(aTHX_ "$upload->slurp($data): can't find env");

    env = mg->mg_ptr;
    bb = ((apreq_param_t *)SvIVX(obj))->bb;

    s = apr_brigade_length(bb, 0, &len_off);
    if (s != APR_SUCCESS) {
        APREQ_XS_THROW_ERROR(upload, s, "Apache::Upload::slurp", 
                             "Apache::Upload::Error");
        XSRETURN_UNDEF;
    }

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
    if (s != APR_SUCCESS) {
        APREQ_XS_THROW_ERROR(upload, s, "Apache::Upload::slurp", 
                             "Apache::Upload::Error");
        XSRETURN_UNDEF;
    }
    if (SvTAINTED(obj))
        SvTAINTED_on(ST(1));
    XSRETURN_IV(len_size);
}

static XS(apreq_xs_upload_size)
{
    dXSARGS;
    MAGIC *mg;
    void *env;
    apr_bucket_brigade *bb;
    apr_status_t s;
    apr_off_t len;
    SV *sv, *obj;

    if (items != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $upload->size()");

    sv = ST(0);
    obj = apreq_xs_find_obj(aTHX_ sv, "upload");

    if (!(mg = mg_find(obj, PERL_MAGIC_ext)))
        Perl_croak(aTHX_ "$upload->size(): can't find env");

    env = mg->mg_ptr;
    bb = ((apreq_param_t *)SvIVX(obj))->bb;

    s = apr_brigade_length(bb, 1, &len);

    if (s != APR_SUCCESS) {
        APREQ_XS_THROW_ERROR(upload, s, "Apache::Upload::size", 
                             "Apache::Upload::Error");
        XSRETURN_UNDEF;
    }

    XSRETURN_IV((IV)len);
}

static XS(apreq_xs_upload_type)
{
    dXSARGS;
    apreq_param_t *upload;
    const char *ct, *sc;
    STRLEN len;
    SV *sv, *obj;

    if (items != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $upload->type()");

    sv = ST(0);
    obj = apreq_xs_find_obj(aTHX_ sv, "upload");

    upload = (apreq_param_t *)SvIVX(obj);
    ct = apr_table_get(upload->info, "Content-Type");
    if (ct == NULL)
        Perl_croak(aTHX_ "$upload->type: can't find Content-Type header");
    
    if ((sc = strchr(ct, ';')))
        len = sc - ct;
    else
        len = strlen(ct);

    sv = newSVpvn(ct, len);
    if (SvTAINTED(obj))
        SvTAINTED_on(sv);
    ST(0) = sv_2mortal(sv);
    XSRETURN(1);
}

APR_INLINE
static SV *apreq_xs_find_bb_obj(pTHX_ SV *in)
{
    while (in && SvROK(in)) {
        SV *sv = SvRV(in);
        switch (SvTYPE(sv)) {            
            MAGIC *mg;
        case SVt_PVIO:
            if (SvMAGICAL(sv) && (mg = mg_find(sv,PERL_MAGIC_tiedscalar))) {
               in = mg->mg_obj;
               break;
            }
            Perl_croak(aTHX_ "panic: cannot find tied scalar in pvio magic");
        case SVt_PVMG:
            if (SvOBJECT(sv) && SvIOKp(sv))
                return sv;
        default:
             Perl_croak(aTHX_ "panic: unsupported SV type: %d", SvTYPE(sv));
       }
    }
    return in;
}


static XS(apreq_xs_upload_brigade_copy)
{
    dXSARGS;
    apr_bucket_brigade *bb, *bb_copy;
    char *class;
    SV *sv, *obj;

    if (items != 2 || !SvPOK(ST(0)) || !SvROK(ST(1)))
        Perl_croak(aTHX_ "Usage: Apache::Upload::Brigade->new($bb)");

    class = SvPV_nolen(ST(0));
    obj = apreq_xs_find_bb_obj(aTHX_ ST(1));
    bb = (apr_bucket_brigade *)SvIVX(obj);
    bb_copy = apr_brigade_create(bb->p,bb->bucket_alloc);
    APREQ_BRIGADE_COPY(bb_copy, bb);

    sv = sv_setref_pv(newSV(0), class, bb_copy);
    if (SvTAINTED(obj))
        SvTAINTED_on(SvRV(sv));
    ST(0) = sv_2mortal(sv);
    XSRETURN(1);
}

static XS(apreq_xs_upload_brigade_read)
{
    dXSARGS;
    apr_bucket_brigade *bb;
    apr_bucket *e, *end;
    IV want = -1, offset = 0;
    SV *sv, *obj;
    apr_status_t s;
    char *buf;

    switch (items) {
    case 4:
        offset = SvIV(ST(3));
    case 3:
        want = SvIV(ST(2));
    case 2:
        sv = ST(1);
        SvUPGRADE(sv, SVt_PV);
        if (SvROK(ST(0))) {
            obj = apreq_xs_find_bb_obj(aTHX_ ST(0));
            bb = (apr_bucket_brigade *)SvIVX(obj);
            break;
        }
    default:
        Perl_croak(aTHX_ "Usage: $bb->READ($buf,$len,$off)");
    }

    if (want == 0) {
        SvCUR_set(sv, offset);
        XSRETURN_IV(0);
    }

    if (APR_BRIGADE_EMPTY(bb)) {
        SvCUR_set(sv, offset);
        XSRETURN_UNDEF;
    }

    if (want == -1) {
        const char *data;
        apr_size_t dlen;
        e = APR_BRIGADE_FIRST(bb);
        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            apreq_xs_croak(aTHX_ newHV(), s, 
                           "Apache::Upload::Brigade::READ", 
                           "APR::Error");
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
                               "Apache::Upload::Brigade::READ", 
                               "APR::Error");
            want = len;

        case APR_SUCCESS:
            break;

        default:
            apreq_xs_croak(aTHX_ newHV(), s, 
                           "Apache::Upload::Brigade::READ",
                           "APR::Error");
        }
    }

    SvGROW(sv, want + offset + 1);
    buf = SvPVX(sv) + offset;
    SvCUR_set(sv, want + offset);
    if (SvTAINTED(obj))
        SvTAINTED_on(sv);

    while ((e = APR_BRIGADE_FIRST(bb)) != end) {
        const char *data;
        apr_size_t dlen;
        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            apreq_xs_croak(aTHX_ newHV(), s, 
                           "Apache::Upload::Brigade::READ", "APR::Error");
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
    SV *sv, *obj;
    apr_status_t s;
    unsigned tainted;

    if (items != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $bb->READLINE");

    obj = apreq_xs_find_bb_obj(aTHX_ ST(0));
    bb = (apr_bucket_brigade *)SvIVX(obj);

    if (APR_BRIGADE_EMPTY(bb))
        XSRETURN(0);

    tainted = SvTAINTED(obj);

    XSprePUSH;

    sv = sv_2mortal(newSVpvn("",0));
    if (tainted)
        SvTAINTED_on(sv);
        
    XPUSHs(sv);

    while (!APR_BRIGADE_EMPTY(bb)) {
        const char *data;
        apr_size_t dlen;
        const char *eol;

        e = APR_BRIGADE_FIRST(bb);
        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            apreq_xs_croak(aTHX_ newHV(), s, 
                           "Apache::Upload::Brigade::READLINE",
                           "APR::Error");

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
            if (tainted)
                SvTAINTED_on(sv);
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
    SV *sv, *obj;

    if (items != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $upload->tempname()");

    sv = ST(0);
    obj = apreq_xs_find_obj(aTHX_ sv, "upload");

    if (!(mg = mg_find(obj, PERL_MAGIC_ext)))
        Perl_croak(aTHX_ "$upload->tempname(): can't find env");

    env = mg->mg_ptr;
    bb = ((apreq_param_t *)SvIVX(obj))->bb;
    file = apreq_brigade_spoolfile(bb);

    if (file == NULL) {
        apr_bucket *last;
        apr_off_t len;
        const char *tmpdir = apreq_env_temp_dir(env, NULL);

        s = apreq_file_mktemp(&file, apreq_env_pool(env), tmpdir);

        if (s != APR_SUCCESS)
            goto tempname_error;

        s = apreq_brigade_fwrite(file, &len, bb);

        if (s != APR_SUCCESS)
            goto tempname_error;

        last = apr_bucket_file_create(file, len, 0, bb->p, bb->bucket_alloc);
        APR_BRIGADE_INSERT_TAIL(bb, last);
    }

    s = apr_file_name_get(&path, file);
    if (s != APR_SUCCESS)
        goto tempname_error;

    sv = newSVpvn(path, strlen(path));
    ST(0) = sv_2mortal(sv);
    XSRETURN(1);

 tempname_error:
    APREQ_XS_THROW_ERROR(upload, s, "Apache::Upload::tempname", 
                         "Apache::Upload::Error");
    XSRETURN_UNDEF;

}
