/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2002-2003 The Apache Software Foundation.  All rights
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

#include "apreq_env.h"
#include "test_apreq.h"
#include "apreq.h"
#include "apreq_cookie.h"
#include "apr_strings.h"

static apreq_jar_t *j = NULL;

static void jar_make(CuTest *tc)
{
    j = apreq_jar(p,"a=1; foo=bar; fl=left; fr=right; frl=right-left; flr=left-right; fll=left-left; b=2");

    CuAssertPtrNotNull(tc, j);
}

static void jar_table_get(CuTest *tc)
{
    const char *val;

    val = apreq_table_get(j->cookies,"a");
    CuAssertStrEquals(tc,"1",val);
    val = apreq_table_get(j->cookies,"b");
    CuAssertStrEquals(tc,"2",val);

    val = apreq_table_get(j->cookies,"foo");
    CuAssertStrEquals(tc,"bar",val);
    val = apreq_table_get(j->cookies,"fl");
    CuAssertStrEquals(tc,"left",val);
    val = apreq_table_get(j->cookies,"fr");
    CuAssertStrEquals(tc,"right",val);
    val = apreq_table_get(j->cookies,"frl");
    CuAssertStrEquals(tc,"right-left",val);
    val = apreq_table_get(j->cookies,"flr");
    CuAssertStrEquals(tc,"left-right",val);
    val = apreq_table_get(j->cookies,"fll");
    CuAssertStrEquals(tc,"left-left",val);
}


static void netscape_cookie(CuTest *tc)
{
    apreq_cookie_t *c;
    apreq_cookie_version_t version = NETSCAPE;

    c = apreq_cookie(j,"foo");
    CuAssertStrEquals(tc,"bar",apreq_cookie_value(c));
    CuAssertIntEquals(tc, version,c->version);

    CuAssertStrEquals(tc,"foo=bar", apreq_cookie_as_string(p,c));
    c->domain = apr_pstrdup(p, "example.com");
    CuAssertStrEquals(tc,"foo=bar; domain=example.com", 
                      apreq_cookie_as_string(p,c));

    c->path = apr_pstrdup(p, "/quux");
    CuAssertStrEquals(tc, "foo=bar; path=/quux; domain=example.com",
                      apreq_cookie_as_string(p,c));
    apreq_cookie_expires(p, c, "+1y");
    CuAssertStrEquals(tc,apr_pstrcat(p,
                         "foo=bar; path=/quux; domain=example.com; expires=", 
                         apreq_expires(p,"+1y",NSCOOKIE), NULL), apreq_cookie_as_string(p,c));
}


static void rfc_cookie(CuTest *tc)
{
    apreq_cookie_t *c = apreq_make_cookie(p,RFC,"rfc",3,"out",3);
    apreq_cookie_version_t version = RFC;
    long expires = apreq_atol("+3m");

    CuAssertStrEquals(tc,"out",apreq_cookie_value(c));
    CuAssertIntEquals(tc, version,c->version);

    CuAssertStrEquals(tc,"rfc=out; Version=1", apreq_cookie_as_string(p,c));
    c->domain = apr_pstrdup(p, "example.com");
    CuAssertStrEquals(tc,"rfc=out; Version=1; domain=example.com", 
                      apreq_cookie_as_string(p,c));

    c->path = apr_pstrdup(p, "/quux");
    CuAssertStrEquals(tc, 
              "rfc=out; Version=1; path=/quux; domain=example.com",
                      apreq_cookie_as_string(p,c));


    apreq_cookie_expires(p, c, "+3m");
    CuAssertStrEquals(tc,apr_psprintf(p,
         "rfc=out; Version=1; path=/quux; domain=example.com; max-age=%ld", 
               expires), apreq_cookie_as_string(p,c));

}


CuSuite *testcookie(void)
{
    CuSuite *suite = CuSuiteNew("Cookie");

    SUITE_ADD_TEST(suite, jar_make);
    SUITE_ADD_TEST(suite, jar_table_get);
    SUITE_ADD_TEST(suite, netscape_cookie);
    SUITE_ADD_TEST(suite, rfc_cookie);

    return suite;
}

