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

#include "apreq_env.h"
#include "apreq.h"
#include "apreq_cookie.h"
#include "apr_strings.h"
#include "test_apreq.h"

static apreq_jar_t *j = NULL;

static void jar_make(CuTest *tc)
{
    j = apreq_jar(p,"a=1; foo=bar; fl=left; fr=right;bad; ns=foo=1&bar=2,"
                  "frl=right-left; flr=left-right; fll=left-left; good_one=1;bad");
    CuAssertPtrNotNull(tc, j);
}

static void jar_table_get(CuTest *tc)
{
    const char *val;

    val = apr_table_get(j->cookies,"a");
    CuAssertStrEquals(tc,"1",val);

    /* ignore wacky cookies that don't have an '=' sign */
    val = apr_table_get(j->cookies,"bad");
    CuAssertPtrEquals(tc,NULL,val);
    /* accept wacky cookies that contain multiple '=' */
    val = apr_table_get(j->cookies,"ns");
    CuAssertStrEquals(tc,"foo=1&bar=2",val);

    val = apr_table_get(j->cookies,"foo");
    CuAssertStrEquals(tc,"bar",val);
    val = apr_table_get(j->cookies,"fl");
    CuAssertStrEquals(tc,"left",val);
    val = apr_table_get(j->cookies,"fr");
    CuAssertStrEquals(tc,"right",val);
    val = apr_table_get(j->cookies,"frl");
    CuAssertStrEquals(tc,"right-left",val);
    val = apr_table_get(j->cookies,"flr");
    CuAssertStrEquals(tc,"left-right",val);
    val = apr_table_get(j->cookies,"fll");
    CuAssertStrEquals(tc,"left-left",val);
}


static void netscape_cookie(CuTest *tc)
{
    apreq_cookie_t *c;
    apreq_cookie_version_t version = APREQ_COOKIE_VERSION_NETSCAPE;

    c = apreq_cookie(j,"foo");
    CuAssertStrEquals(tc,"bar",apreq_cookie_value(c));
    CuAssertIntEquals(tc, version,c->version);

    CuAssertStrEquals(tc,"foo=bar", apreq_cookie_as_string(c,p));
    c->domain = apr_pstrdup(p, "example.com");
    CuAssertStrEquals(tc,"foo=bar; domain=example.com", 
                      apreq_cookie_as_string(c,p));

    c->path = apr_pstrdup(p, "/quux");
    CuAssertStrEquals(tc, "foo=bar; path=/quux; domain=example.com",
                      apreq_cookie_as_string(c,p));
    apreq_cookie_expires(c, "+1y");
    CuAssertStrEquals(tc,apr_pstrcat(p,
                         "foo=bar; path=/quux; domain=example.com; expires=", 
                         apreq_expires(p,"+1y",APREQ_EXPIRES_NSCOOKIE), NULL), 
                      apreq_cookie_as_string(c,p));
}


static void rfc_cookie(CuTest *tc)
{
    apreq_cookie_t *c = apreq_make_cookie(p,"rfc",3,"out",3);
    long expires; 

    CuAssertStrEquals(tc,"out",apreq_cookie_value(c));
    c->version = APREQ_COOKIE_VERSION_RFC;

    CuAssertStrEquals(tc,"rfc=out; Version=1", apreq_cookie_as_string(c,p));
    c->domain = apr_pstrdup(p, "example.com");
    CuAssertStrEquals(tc,"rfc=out; Version=1; domain=\"example.com\"", 
                      apreq_cookie_as_string(c,p));

    c->path = apr_pstrdup(p, "/quux");
    CuAssertStrEquals(tc, 
              "rfc=out; Version=1; path=\"/quux\"; domain=\"example.com\"",
                      apreq_cookie_as_string(c,p));

    apreq_cookie_expires(c, "+3m");
    expires = apreq_atoi64t("+3m");
    CuAssertStrEquals(tc,apr_psprintf(p,
         "rfc=out; Version=1; path=\"/quux\"; domain=\"example.com\"; max-age=%ld",
               expires), apreq_cookie_as_string(c,p));

}

static void ua_version(CuTest *tc)
{
    apreq_cookie_version_t v;
    char version[] = "$Version=\"1\"";

    v = apreq_ua_cookie_version(NULL);
    CuAssertIntEquals(tc, APREQ_COOKIE_VERSION_NETSCAPE, v);
    v = apreq_ua_cookie_version(version);
    CuAssertIntEquals(tc, APREQ_COOKIE_VERSION_RFC, v);

}

CuSuite *testcookie(void)
{
    CuSuite *suite = CuSuiteNew("Cookie");

    SUITE_ADD_TEST(suite, jar_make);
    SUITE_ADD_TEST(suite, jar_table_get);
    SUITE_ADD_TEST(suite, netscape_cookie);
    SUITE_ADD_TEST(suite, rfc_cookie);
    SUITE_ADD_TEST(suite, ua_version);

    return suite;
}

