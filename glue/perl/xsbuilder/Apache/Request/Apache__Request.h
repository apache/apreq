#define apreq_xs_request2env(r) r->env
#define apreq_xs_request2args(r) r->args
#define apreq_xs_request2body(r) r->body
#define apreq_xs_request2params(r) apreq_params(apreq_env_pool(env),r)
#define apreq_xs_param2rv(ptr) sv_setref_pv(newSV(0), "Apache::Upload", ptr)
#define apreq_xs_rv2param(sv) ((apreq_param_t *)SvIVX(SvRV(sv)))
#define apreq_xs_table2sv(t) apreq_xs_2sv(t,"Apache::Request::Table")

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

APREQ_XS_DEFINE_TABLE(request, args);
APREQ_XS_DEFINE_TABLE(request, body);
APREQ_XS_DEFINE_TABLE(request, params);

APREQ_XS_DEFINE_GET(table, param, NULL);

static int apreq_xs_param_table_values(void *data, const char *key,
                                       const char *val)
{
    struct apreq_xs_do_arg *d = (struct apreq_xs_do_arg *)data;
    void *env = d->env;
    dTHXa(d->perl);
    dSP;
    if (val)
        XPUSHs(sv_2mortal(apreq_xs_param2sv(apreq_value_to_param(
                                       apreq_strtoval(val)),NULL)));

    else
        XPUSHs(&PL_sv_undef);

    PUTBACK;
    return 1;
}


#ifdef apreq_xs_table_do
#undef apreq_xs_table_do
#endif

#define apreq_xs_table_do (items == 1 ? apreq_xs_table_keys \
                                      : apreq_xs_param_table_values)

static XS(apreq_xs_param)
{
    dXSARGS;
    const char *key = NULL;

    if (items == 1 || items == 2) {
        SV *sv = ST(0);
        apreq_request_t *req = apreq_xs_sv2(request,sv);
        void *env = req->env;
        struct apreq_xs_do_arg d = { env, aTHX };

        if (items == 2)
            key = SvPV_nolen(ST(1));

        if (req == NULL)
            Perl_croak(aTHX_ "Usage: $req->param($key)");

        switch (GIMME_V) {
            const apreq_param_t *param;

        case G_ARRAY:
            XSprePUSH;
            PUTBACK;
            apr_table_do(apreq_xs_table_do, &d, req->args, key, NULL);
            if (req->body)
                apr_table_do(apreq_xs_table_do,&d,req->body,key,NULL);

            break;

        case G_SCALAR:
            if (items == 1) {
                ST(0) = sv_2mortal(apreq_xs_table2sv(
                                   apreq_params(apreq_env_pool(env), req)));
                XSRETURN(1);
            }
            param = apreq_param(req, key);
            if (param == NULL)
                XSRETURN_UNDEF;
            ST(0) = sv_2mortal(apreq_xs_param2sv(param,NULL));
            XSRETURN(1);

        default:
            XSRETURN(0);
        }
    }
    else
	Perl_croak(aTHX_ "Usage: $req->param($key)");
}
