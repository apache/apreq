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
#include "apr_strings.h"
#include "test_apreq.h"
#include "apreq_cookie.h"

static const char nscookies[] = "a=1; foo=bar; fl=left; fr=right;bad; "
                                "ns=foo=1&bar=2,frl=right-left; "
                                "flr=left-right; fll=left-left; "
                                "good_one=1;bad";

static apr_table_t *jar;

static void jar_make(CuTest *tc)
{
    apr_status_t s;

    jar = apr_table_make(p, APREQ_DEFAULT_NELTS);
    CuAssertPtrNotNull(tc, jar);
    s = apreq_parse_cookie_header(p, jar, nscookies);
    CuAssertIntEquals(tc, APREQ_ERROR_NOTOKEN, s);
}


static void jar_get(CuTest *tc)
{
    const char *val;

    val = apr_table_get(jar,"a");
    CuAssertStrEquals(tc,"1",val);

    /* ignore wacky cookies that don't have an '=' sign */
    val = apr_table_get(jar,"bad");
    CuAssertPtrEquals(tc,NULL,val);
    /* accept wacky cookies that contain multiple '=' */
    val = apr_table_get(jar,"ns");
    CuAssertStrEquals(tc,"foo=1&bar=2",val);

    val = apr_table_get(jar,"foo");
    CuAssertStrEquals(tc,"bar",val);
    val = apr_table_get(jar,"fl");
    CuAssertStrEquals(tc,"left",val);
    val = apr_table_get(jar,"fr");
    CuAssertStrEquals(tc,"right",val);
    val = apr_table_get(jar,"frl");
    CuAssertStrEquals(tc,"right-left",val);
    val = apr_table_get(jar,"flr");
    CuAssertStrEquals(tc,"left-right",val);
    val = apr_table_get(jar,"fll");
    CuAssertStrEquals(tc,"left-left",val);
}


static void netscape_cookie(CuTest *tc)
{
    char *val;
    apreq_cookie_t *c;
    apreq_cookie_version_t version = APREQ_COOKIE_VERSION_NETSCAPE;

    *(const char **)&val = apr_table_get(jar, "foo");
    CuAssertPtrNotNull(tc, val);
    c = apreq_value_to_cookie(val);
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

    v = apreq_ua_cookie_version(apreq_handle_custom(p, NULL, NULL, NULL, NULL, 0, NULL));
    CuAssertIntEquals(tc, APREQ_COOKIE_VERSION_NETSCAPE, v);
    v = apreq_ua_cookie_version(apreq_handle_custom(p, NULL, NULL, version, NULL, 0, NULL));
    CuAssertIntEquals(tc, APREQ_COOKIE_VERSION_RFC, v);

}

CuSuite *testcookie(void)
{
    CuSuite *suite = CuSuiteNew("Cookie");

    SUITE_ADD_TEST(suite, jar_make);
    SUITE_ADD_TEST(suite, jar_get);
    SUITE_ADD_TEST(suite, netscape_cookie);
    SUITE_ADD_TEST(suite, rfc_cookie);
    SUITE_ADD_TEST(suite, ua_version);

    return suite;
}

