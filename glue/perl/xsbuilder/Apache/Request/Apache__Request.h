APREQ_XS_DEFINE_CONVERT(request);

#define apreq_xs_sv2request(sv) apreq_xs_request_perl2c(aTHX_ sv)
#define apreq_xs_request2sv(r,class) apreq_xs_request_c2perl(aTHX_ r, r->env, class)
#define apreq_xs_request_sv2env(sv) apreq_xs_sv2request(sv)->env
#define apreq_xs_request2env(r) r->env

APREQ_XS_DEFINE_OBJECT(request);


APR_INLINE
static SV *apreq_xs_param2sv(const apreq_param_t *param, 
                             const char *class)
{
    SV *sv = newSVpvn(param->v.data, param->v.size);
    if (param->charset == UTF_8)
        SvUTF8_on(sv);

    SvTAINT(sv);
    return sv;
}

/* there is no sv2param, since param isn't an object */

APREQ_XS_DEFINE_MAKE(param);

APREQ_XS_DEFINE_CONVERT(table);

#define apreq_xs_table2sv(t,class) apreq_xs_table_c2perl(aTHX_ t,env,class)
#define apreq_xs_sv2table(sv) apreq_xs_table_perl2c(aTHX_ sv)
#define apreq_xs_table_sv2table(sv) apreq_xs_sv2table(sv)

#define apreq_xs_request2args(r) r->args
#define apreq_xs_request2body(r) r->body
#define apreq_xs_request2params(r) apreq_params(apreq_env_pool(env),r)
APREQ_XS_DEFINE_TABLE(request,args);
APREQ_XS_DEFINE_TABLE(request,body);
APREQ_XS_DEFINE_TABLE(request,params);
APREQ_XS_DEFINE_GET(table,param);


/* might be used for making upload objects */

APR_INLINE
static SV *apreq_xs_param2rv(const apreq_param_t *param, 
                             const char *class)
{
    SV *rv = sv_setref_pv(newSV(0), class, (void *)param);
    SvTAINT(SvRV(rv));
    return rv;
}

#define apreq_xs_rv2param(sv) ((apreq_param_t *)SvIVX(SvRV(sv)))
#define apreq_xs_param_table_do (items==1 ? apreq_xs_param_table_keys : \
                                        apreq_xs_param_table_values)

static int apreq_xs_param_table_keys(void *data, const char *key,
                                     const char *val)
{
    struct do_arg *d = (struct do_arg *)data;
    void *env = d->env;
    const char *class = d->class;
    dTHXa(d->perl);
    dSP;
    if (val)
        XPUSHs(sv_2mortal(newSVpv(key, 0)));
    else
        XPUSHs(&PL_sv_undef);

    PUTBACK;
    return 1;
}
static int apreq_xs_param_table_values(void *data, const char *key,
                                       const char *val)
{
    struct do_arg *d = (struct do_arg *)data;
    void *env = d->env;
    const char *class = d->class;
    dTHXa(d->perl);
    dSP;
    if (val)
        XPUSHs(sv_2mortal(apreq_xs_param2sv(
                   apreq_value_to_param(apreq_strtoval(val)),class)));

    else
        XPUSHs(&PL_sv_undef);

    PUTBACK;
    return 1;
}

static XS(apreq_xs_param)
{
    dXSARGS;
    const char *key = NULL;

    if (items == 1 || items == 2) {
        SV *sv = ST(0);
        apreq_request_t *req = apreq_xs_sv2request(sv);
        void *env = req->env;
        const char *class = NULL; /* params aren't objects */
        struct do_arg d = { class, env, aTHX };

        if (items == 2)
            key = SvPV_nolen(ST(1));

        if (req == NULL)
            Perl_croak(aTHX_ "Usage: $req->param($key)");

        switch (GIMME_V) {
            const apreq_param_t *param;

        case G_ARRAY:
            XSprePUSH;
            PUTBACK;
            apr_table_do(apreq_xs_param_table_do, &d, req->args, key, NULL);
            if (req->body)
                apr_table_do(apreq_xs_param_table_do,&d,req->body,key,NULL);

            break;

        case G_SCALAR:
            param = apreq_param(req, key);
            if (param == NULL)
                XSRETURN_UNDEF;

            ST(0) = sv_2mortal(apreq_xs_param2sv(param, class));
            XSRETURN(1);

        default:
            XSRETURN(0);
        }
    }
    else
	Perl_croak(aTHX_ "Usage: $req->param($key)");
}
