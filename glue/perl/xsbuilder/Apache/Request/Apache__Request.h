#define apreq_xs_param2rv(ptr, class) apreq_xs_2sv(ptr,class)
#define apreq_xs_rv2param(sv) ((apreq_param_t *)SvIVX(SvRV(sv)))


APR_INLINE static SV *apreq_xs_param2sv(const apreq_param_t *param, 
                                        const char *class)
{
    SV *sv = newSVpvn(param->v.data, param->v.size);
    if (param->charset == UTF_8)
        SvUTF8_on(sv);

    return sv;
}

APREQ_XS_DEFINE_ENV(request);
APREQ_XS_DEFINE_OBJECT(request);
APREQ_XS_DEFINE_MAKE(param);

/* Too many GET macros :-( */

#define S2P(s) (s ? apreq_value_to_param(apreq_strtoval(s)) : NULL)
#define apreq_xs_request_push(sv,d,key) do {                            \
    apreq_request_t *req = apreq_xs_sv2(request,sv);                    \
    apr_table_do(apreq_xs_do(request), &d, req->args, key, NULL);       \
    if (req->body)                                                      \
        apr_table_do(apreq_xs_do(request), &d, req->body, key, NULL);   \
} while (0)
#define apreq_xs_args_push(sv,d,k) apreq_xs_push(args,sv,d,k)
#define apreq_xs_body_push(sv,d,k) apreq_xs_push(body,sv,d,k)
#define apreq_xs_table_push(sv,d,k) apreq_xs_push(table,sv,d,k)
#define apreq_xs_upload_push(sv,d,key) do {                             \
    apr_table_t *t = apreq_xs_body_sv2table(sv);                        \
    if (t)                                                              \
        apr_table_do(apreq_xs_do(upload), &d, t, key, NULL);            \
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
#define apreq_xs_param2sv(param,class) apreq_xs_param2rv(param,class)
#define apreq_xs_sv2param(sv) apreq_xs_rv2param(sv)


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
    apr_status_t s;
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
        s = apr_file_open(&f, name, APR_CREATE | APR_EXCL | APR_WRITE | APR_READ
                          | APR_BINARY, APR_OS_DEFAULT, apreq_env_pool(env));
        if (s != APR_SUCCESS)
            XSRETURN_IV(s);
        s = apreq_brigade_fwrite(f, &len, bb);
        XSRETURN_IV(s);
    }
    s = apr_file_name_get(&fname, f);
    if (s != APR_SUCCESS)
        XSRETURN_IV(s);

    s = PerlLIO_link(fname, name);
    XSRETURN_IV(s ? APR_EGENERAL : APR_SUCCESS);
}

