#define apreq_xs_jar2cookie(j) j->cookies
#define apreq_xs_jar2env(j) j->env

#define apreq_xs_cookie2sv(c,class) apreq_xs_2sv(c,class)
#define apreq_xs_sv2cookie(sv) ((apreq_cookie_t *)SvIVX(SvRV(sv)))

APREQ_XS_DEFINE_ENV(cookie);
APREQ_XS_DEFINE_ENV(jar);
APREQ_XS_DEFINE_MAKE(cookie);
APREQ_XS_DEFINE_OBJECT(jar);

/* GET macros */
#define S2C(s)  apreq_value_to_cookie(apreq_strtoval(s))
#define apreq_xs_jar_push(sv,d,key)   apreq_xs_push(jar,sv,d,key)
#define apreq_xs_table_push(sv,d,key) apreq_xs_push(table,sv,d,key)
#define apreq_xs_jar_sv2table(sv) (apreq_xs_sv2(jar, sv)->cookies)
#define apreq_xs_table_sv2table(sv) apreq_xs_sv2table(sv)
#define apreq_xs_jar_sv2env(sv) apreq_xs_sv2(jar,sv)->env
#define apreq_xs_table_sv2env(sv) apreq_xs_sv2env(SvRV(sv))

#define apreq_xs_jar_cookie(sv,k) \
                S2C(apr_table_get(apreq_xs_jar_sv2table(sv),k))
#define apreq_xs_table_cookie(sv,k) \
                S2C(apr_table_get(apreq_xs_table_sv2table(sv),k))

#define TABLE_PKG   "Apache::Cookie::Table"
#define COOKIE_PKG  "Apache::Cookie"

APREQ_XS_DEFINE_GET(jar,   TABLE_PKG, cookie, COOKIE_PKG, 1);
APREQ_XS_DEFINE_GET(table, TABLE_PKG, cookie, COOKIE_PKG, 1);


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
        apr_pool_t *p = apreq_env_pool(apreq_xs_sv2env(ST(0)));
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
    p = apreq_env_pool(apreq_xs_sv2env(ST(0)));

    for (j = 1; j + 1 < items; j += 2) {
        status = apreq_cookie_attr(p, c, SvPV_nolen(ST(j)), 
                                         SvPV_nolen(ST(j+1)));
        if (status != APR_SUCCESS)
            break;
    }
    ST(0) = sv_2mortal(newSViv(status));
    XSRETURN(1);
}

