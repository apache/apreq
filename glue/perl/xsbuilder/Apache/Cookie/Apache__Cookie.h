#define apreq_xs_jar_sv2table(sv) (apreq_xs_sv2(jar, sv)->cookies)
#define apreq_xs_jar2cookies(j) j->cookies
#define apreq_xs_jar2env(j) j->env

#define apreq_xs_table2sv(t) apreq_xs_table_c2perl(aTHX_ t, env, \
                                                   "Apache::Cookie::Table")
#define apreq_xs_cookie2sv(c,class) apreq_xs_2sv(c,class)

APREQ_XS_DEFINE_ENV(cookie);
APREQ_XS_DEFINE_ENV(jar);

/* jar */

APREQ_XS_DEFINE_OBJECT(jar, "Apache::Cookie::Jar");
APREQ_XS_DEFINE_TABLE(jar, cookies);
APREQ_XS_DEFINE_GET(jar, cookie, "Apache::Cookie");

/* cookie */

APREQ_XS_DEFINE_MAKE(cookie);
APREQ_XS_DEFINE_GET(table, cookie, "Apache::Cookie");


static XS(apreq_xs_cookie_as_string)
{
    dXSARGS;
    apreq_cookie_t *c;
    SV *sv;

    if (items != 1)
        Perl_croak(aTHX_ "Usage: $cookie->as_string()");

    sv = ST(0);
    c = apreq_xs_sv2(cookie,sv);
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

    c = apreq_xs_sv2(cookie,ST(0));

    if (items > 1) {
        apr_pool_t *p = apreq_env_pool(apreq_xs_sv2env(cookie,ST(0)));
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

    c = apreq_value_to_cookie(apreq_xs_sv2(cookie,ST(0)));
    p = apreq_env_pool(apreq_xs_sv2env(cookie,ST(0)));

    for (j = 1; j + 1 < items; j += 2) {
        status = apreq_cookie_attr(p, c, SvPV_nolen(ST(j)), 
                                         SvPV_nolen(ST(j+1)));
        if (status != APR_SUCCESS)
            break;
    }
    ST(0) = sv_2mortal(newSViv(status));
    XSRETURN(1);
}

