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

#define READ_BLOCK_SIZE (1024 * 256)

#define apreq_xs_param2rv(ptr, class) apreq_xs_2sv(ptr,class)
#define apreq_xs_rv2param(sv) ((apreq_param_t *)SvIVX(SvRV(sv)))
#define apreq_xs_param2sv(ptr,class) newSVpvn((ptr)->v.data,(ptr)->v.size)

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

#define apreq_xs_request_sv2table(sv) apreq_params(apreq_env_pool(env), \
                                                   apreq_xs_sv2(request,sv))
#define apreq_xs_args_sv2table(sv) apreq_xs_sv2(request,sv)->args
#define apreq_xs_body_sv2table(sv) apreq_xs_sv2(request,sv)->body
#define apreq_xs_table_sv2table(sv) apreq_xs_sv2table(sv)
#define apreq_xs_upload_sv2table(sv) apreq_uploads(apreq_env_pool(env), \
                                                   apreq_xs_sv2(request,sv))
#define apreq_xs_upload_table_sv2table(sv) apreq_xs_sv2table(sv)

#define apreq_xs_request_sv2env(sv) apreq_xs_sv2(request,sv)->env
#define apreq_xs_args_sv2env(sv) apreq_xs_sv2(request,sv)->env
#define apreq_xs_body_sv2env(sv) apreq_xs_sv2(request,sv)->env
#define apreq_xs_upload_sv2env(sv) apreq_xs_sv2(request,sv)->env
#define apreq_xs_table_sv2env(sv) apreq_xs_sv2env(SvRV(sv))
#define apreq_xs_upload_table_sv2env(sv) apreq_xs_sv2env(SvRV(sv))

#define apreq_xs_request_param(sv,k) apreq_param(apreq_xs_sv2(request,sv),k)
#define apreq_xs_args_param(sv,k) \
                     S2P(apr_table_get(apreq_xs_args_sv2table(sv),k))
#define apreq_xs_body_param(sv,k) \
                     S2P(apr_table_get(apreq_xs_body_sv2table(sv),k))
#define apreq_xs_table_param(sv,k) \
                     S2P(apr_table_get(apreq_xs_sv2table(sv),k))
#define apreq_xs_upload_param(sv,k) apreq_upload(apreq_xs_sv2(request,sv),k)
#define apreq_xs_upload_table_param(sv,k) \
                     S2P(apr_table_get(apreq_xs_sv2table(sv),k))

#define PARAM_TABLE   "Apache::Request::Table"

APREQ_XS_DEFINE_GET(request, PARAM_TABLE, param, NULL, 1);
APREQ_XS_DEFINE_GET(args,    PARAM_TABLE, param, NULL, 1);
APREQ_XS_DEFINE_GET(body,    PARAM_TABLE, param, NULL, 1);
APREQ_XS_DEFINE_GET(table,   PARAM_TABLE, param, NULL, 1);

/* Upload API */
/* supercede earlier function definition */
#undef apreq_xs_param2sv
#define apreq_xs_param2sv(param,class) apreq_xs_param2rv(param,class)
#define apreq_xs_sv2param(sv) apreq_xs_rv2param(sv)

static int apreq_xs_upload_table_keys(void *data, const char *key,
                                      const char *val)
{
    struct apreq_xs_do_arg *d = (struct apreq_xs_do_arg *)data;
    dTHXa(d->perl);
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
    bb = apreq_xs_rv2param(ST(0))->bb;
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
    apr_off_t len;
    apr_bucket_brigade *bb;
    apr_status_t s;

    if (items != 2 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $upload->slurp($data)");

    if (!(mg = mg_find(SvRV(ST(0)), PERL_MAGIC_ext)))
        Perl_croak(aTHX_ "$upload->slurp($data): can't find env");

    env = mg->mg_ptr;
    bb = apreq_xs_rv2param(ST(0))->bb;

    s = apr_brigade_length(bb, 0, &len);
    if (s != APR_SUCCESS)
        XSRETURN_IV(s);

    SvUPGRADE(ST(1), SVt_PV);
    data = SvGROW(ST(1), len + 1);
    data[len] = 0;
    SvCUR_set(ST(1), len);
    SvPOK_only(ST(1));

    s = apr_brigade_flatten(bb, data, &len);
    XSRETURN_IV(s);
}

static XS(apreq_xs_request_config)
{
    dXSARGS;
    apreq_request_t *req;
    apr_status_t status = APR_SUCCESS;
    int j;
    if (items == 0)
        XSRETURN_UNDEF;
    if (!SvROK(ST(0)))
        Perl_croak(aTHX_ "usage: $req->config(@settings)");

    req = apreq_xs_sv2(request,ST(0));

    for (j = 1; j + 1 < items; j += 2) {
        STRLEN alen, vlen;
        const char *attr = SvPVbyte(ST(j),alen), 
                    *val = SvPVbyte(ST(j+1),vlen);

        if (strcasecmp(attr,"POST_MAX")== 0
            || strcasecmp(attr, "MAX_BODY") == 0) 
        {
            status = apreq_env_max_body(req->env,(apr_off_t)apreq_atoi64f(val));
        }
        else if (strcasecmp(attr, "TEMP_DIR") == 0) {
            apreq_env_temp_dir(req->env, val);
        }
        else if (strcasecmp(attr, "MAX_BRIGADE") == 0) {
            apreq_env_max_brigade(req->env, (apr_ssize_t)apreq_atoi64f(val));
        }
        else {
            Perl_warn(aTHX_ "Apache::Request::config: "
                      "Unrecognized attribute %s", attr);
        }

        if (status != APR_SUCCESS)
            break;
    }
    XSRETURN_IV(status);
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
    XSRETURN_IV(s);
}
