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

#define apreq_xs_jar2cookie(j) j->cookies
#define apreq_xs_jar2env(j) j->env

#define apreq_xs_cookie2sv(c,class) apreq_xs_2sv(c,class)
#define apreq_xs_sv2cookie(sv) ((apreq_cookie_t *)SvIVX(SvRV(sv)))

/** 
 * env 
 * @param The CooKIE
 */
APREQ_XS_DEFINE_ENV(cookie);
APREQ_XS_DEFINE_ENV(jar);
APREQ_XS_DEFINE_MAKE(cookie);
APREQ_XS_DEFINE_OBJECT(jar);

#define apreq_xs_jar_error_check   do {                                 \
    int n = PL_stack_sp - (PL_stack_base + ax - 1);                     \
    apr_status_t s;                                                     \
    switch (GIMME_V) {                                                  \
    case G_VOID:                                                        \
        break;                                                          \
    case G_SCALAR:                                                      \
        if (n == 1 && items == 2)                                       \
            break;                                                      \
    default:                                                            \
        s = apreq_xs_sv2(jar, sv)->status;                              \
        if (s != APR_SUCCESS) {                                         \
            apreq_xs_croak(aTHX_ newHV(), s, "Apache::Cookie::Jar::get",\
                           "Apache::Cookie::Error");                    \
        }                                                               \
    }                                                                   \
} while (0)


#define apreq_xs_table_error_check 


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

/**
 *Returns serialized version of cookie.
 *@param $cookie cookie
 */
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

/**
 * Cookie exipiration time.  Depends on cookie version.
 * @param cookie
 * @return expiration date.
 */

static XS(apreq_xs_cookie_expires)
{
    dXSARGS;
    apreq_cookie_t *c;

    if (items == 0)
        XSRETURN_UNDEF;

    c = apreq_xs_sv2(cookie,ST(0));

    if (items > 1) {
        const char *s = SvPV_nolen(ST(1));
        apreq_cookie_expires(c, s);
    }

    if (c->max_age == -1)
        XSRETURN_UNDEF;

    if (c->version == APREQ_COOKIE_VERSION_NETSCAPE) {
        char expires[APR_RFC822_DATE_LEN] = {0};
        apr_rfc822_date(expires, c->max_age + apr_time_now());
        expires[7] = '-';
        expires[11] = '-';
        XSRETURN_PV(expires);
    }
    XSRETURN_IV(c->max_age);
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

    c = apreq_xs_sv2(cookie,ST(0));
    p = apreq_env_pool(apreq_xs_sv2env(SvRV(ST(0))));

    for (j = 1; j + 1 < items; j += 2) {
        STRLEN alen, vlen;
        const char *attr = SvPVbyte(ST(j),alen), 
                    *val = SvPVbyte(ST(j+1),vlen);
        status = apreq_cookie_attr(p, c, attr, alen, val, vlen); 
        if (status != APR_SUCCESS)
            break;
    }
    XSRETURN_IV(status);
}

static XS(apreq_xs_encode)
{
    dXSARGS;
    STRLEN slen;
    const char *src;

    if (items != 1)
        Perl_croak(aTHX_ "Usage: encode($string)");

    src = SvPVbyte(ST(0), slen);
    if (src == NULL)
        XSRETURN_UNDEF;

    ST(0) = sv_newmortal();
    SvUPGRADE(ST(0), SVt_PV);
    SvGROW(ST(0), 3 * slen + 1);
    SvCUR(ST(0)) = apreq_encode(SvPVX(ST(0)), src, slen);
    SvPOK_on(ST(0));
    XSRETURN(1);
}

static XS(apreq_xs_decode)
{
    dXSARGS;
    STRLEN slen;
    apr_ssize_t len;
    const char *src;

    if (items != 1)
        Perl_croak(aTHX_ "Usage: decode($string)");

    src = SvPVbyte(ST(0), slen);
    if (src == NULL)
        XSRETURN_UNDEF;

    ST(0) = sv_newmortal();
    SvUPGRADE(ST(0), SVt_PV);
    SvGROW(ST(0), slen + 1);
    len = apreq_decode(SvPVX(ST(0)), src, slen);
    if (len < 0)
        XSRETURN_UNDEF;
    SvCUR_set(ST(0),len);
    SvPOK_on(ST(0));
    XSRETURN(1);
}
