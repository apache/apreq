/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2003 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

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

