/* jar */

APREQ_XS_DEFINE_CONVERT(jar);

#define apreq_xs_sv2jar(sv) apreq_xs_jar_perl2c(aTHX_ sv)
#define apreq_xs_jar2sv(j,class) apreq_xs_jar_c2perl(aTHX_ j,j->env,class)
#define apreq_xs_jar_sv2table(sv) apreq_xs_sv2jar(sv)->cookies
#define apreq_xs_jar2env(j) j->env

APREQ_XS_DEFINE_OBJECT(jar);

/* cookie */

APREQ_XS_DEFINE_CONVERT(cookie);

#define apreq_xs_sv2cookie(sv) apreq_xs_cookie_perl2c(aTHX_ sv)
#define apreq_xs_cookie2sv(c,class) apreq_xs_cookie_c2perl(aTHX_ c,env,class)
#define apreq_xs_cookie_sv2env(sv) apreq_xs_cookie_perl2env(sv)

APREQ_XS_DEFINE_MAKE(cookie);

APREQ_XS_DEFINE_GET(jar,cookie);

/* table */
APREQ_XS_DEFINE_CONVERT(table);

#define apreq_xs_table2sv(t,class) apreq_xs_table_c2perl(aTHX_ t,env,class)
#define apreq_xs_sv2table(sv) apreq_xs_table_perl2c(aTHX_ sv)
#define apreq_xs_table_sv2table(sv) apreq_xs_sv2table(sv)
#define apreq_xs_table_sv2env(sv) apreq_xs_table_perl2env(sv)

#define apreq_xs_jar2cookies(j) j->cookies
APREQ_XS_DEFINE_TABLE(jar,cookies);

APREQ_XS_DEFINE_GET(table,cookie);

static XS(apreq_xs_cookie_as_string)
{
    dXSARGS;
    apreq_cookie_t *c;
    SV *sv;

    if (items != 1)
        Perl_croak(aTHX_ "Usage: $cookie->as_string()");

    sv = ST(0);
    c = apreq_xs_sv2cookie(sv);
    sv = NEWSV(0, apreq_serialize_cookie(NULL, 0, c));
    SvCUR(sv) = apreq_serialize_cookie(SvPVX(sv), SvLEN(sv), c);
    SvPOK_on(sv);
    ST(0) = sv_2mortal(sv);
    XSRETURN(1);
}

static XS(apreq_xs_cookie_expires)
{
    dXSARGS;
    apreq_cookie_t *c;

    if (items == 0)
        XSRETURN_UNDEF;

    c = apreq_xs_sv2cookie(ST(0));

    if (items > 1) {
        apr_pool_t *p = apreq_env_pool(apreq_xs_cookie_sv2env(ST(0)));
        const char *s = SvPV_nolen(ST(1));
        apreq_cookie_expires(p, c, s);
    }

    if (c->version == NETSCAPE)
        ST(0) = c->time.expires ? sv_2mortal(newSVpv(c->time.expires,0)) :
            &PL_sv_undef;
    else
        ST(0) = c->time.max_age >= 0 ? sv_2mortal(newSViv(c->time.max_age)) :
            &PL_sv_undef;

    XSRETURN(1);
}

static XS(apreq_xs_cookie_set_attr)
{
    dXSARGS;
    apreq_cookie_t *c;
    apr_pool_t *p;
    apr_status_t status = APR_SUCCESS;
    int j = 1;
    if (items == 0)
        XSRETURN_UNDEF;

    c = apreq_value_to_cookie(apreq_xs_sv2cookie(ST(0)));
    p = apreq_env_pool(apreq_xs_cookie_sv2env(ST(0)));

    for (j = 1; j + 1 < items; j += 2) {
        status = apreq_cookie_attr(p, c, SvPV_nolen(ST(j)), 
                                         SvPV_nolen(ST(j+1)));
        if (status != APR_SUCCESS)
            break;
    }
    ST(0) = sv_2mortal(newSViv(status));
    XSRETURN(1);
}

