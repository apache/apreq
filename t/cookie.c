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
#include "at.h"

extern apr_pool_t *p;
static apreq_jar_t *j = NULL;

#define dTEST(func, plan) static const at_test_t test_##func = \
      {#func, func, plan, NULL, NULL, NULL}

static void jar_make(dAT)
{
    j = apreq_jar(p,"a=1; foo=bar; fl=left; fr=right;bad; ns=foo=1&bar=2,"
                  "frl=right-left; flr=left-right; fll=left-left; good_one=1;bad");
    AT_not_null(j);
}
dTEST(jar_make, 1);


static void jar_get(dAT)
{

    AT_str_eq(apr_table_get(j->cookies,"a"), "1");

    /* ignore wacky cookies that don't have an '=' sign */
    AT_is_null(apr_table_get(j->cookies,"bad"));
    /* accept wacky cookies that contain multiple '=' */
    AT_str_eq(apr_table_get(j->cookies,"ns"), "foo=1&bar=2");

    AT_str_eq(apr_table_get(j->cookies,"foo"), "bar");
    AT_str_eq(apr_table_get(j->cookies,"fl"), "left");
    AT_str_eq(apr_table_get(j->cookies,"fr"), "right");
    AT_str_eq(apr_table_get(j->cookies,"frl"),"right-left");
    AT_str_eq(apr_table_get(j->cookies,"flr"),"left-right");
    AT_str_eq(apr_table_get(j->cookies,"fll"),"left-left");
}
dTEST(jar_get, 9);


static void netscape_cookie(dAT)
{
    apreq_cookie_t *c;
    char *one_year_expiration;
    c = apreq_cookie(j,"foo");
    AT_str_eq(apreq_cookie_value(c), "bar");
    AT_int_eq(c->version, APREQ_COOKIE_VERSION_NETSCAPE);
    AT_str_eq(apreq_cookie_as_string(c,p), "foo=bar");

    c->domain = apr_pstrdup(p, "example.com");
    AT_str_eq(apreq_cookie_as_string(c,p),
              "foo=bar; domain=example.com");


    c->path = apr_pstrdup(p, "/quux");
    AT_str_eq(apreq_cookie_as_string(c,p),
              "foo=bar; path=/quux; domain=example.com");
    one_year_expiration = apr_pstrcat(p,
                      "foo=bar; path=/quux; domain=example.com; expires=", 
                      apreq_expires(p,"+1y",APREQ_EXPIRES_NSCOOKIE), NULL);
                     
    apreq_cookie_expires(c, "+1y");
    AT_str_eq(apreq_cookie_as_string(c,p), one_year_expiration);
}
dTEST(netscape_cookie, 6);


static void rfc_cookie(dAT)
{
    apreq_cookie_t *c = apreq_make_cookie(p,"rfc",3,"out",3);
    char *three_month_expiration;

    AT_str_eq(apreq_cookie_value(c), "out");
    c->version = APREQ_COOKIE_VERSION_RFC;

    AT_str_eq(apreq_cookie_as_string(c,p), "rfc=out; Version=1");

    c->domain = apr_pstrdup(p, "example.com");
    AT_str_eq(apreq_cookie_as_string(c,p),
              "rfc=out; Version=1; domain=\"example.com\"");

    c->path = apr_pstrdup(p, "/quux");
    AT_str_eq(apreq_cookie_as_string(c,p),
              "rfc=out; Version=1; path=\"/quux\"; domain=\"example.com\"");

    apreq_cookie_expires(c, "+3m");
    three_month_expiration = apr_psprintf(p,
        "rfc=out; Version=1; path=\"/quux\"; domain=\"example.com\"; max-age=%ld",
         apreq_atoi64t("+3m"));
    AT_str_eq(apreq_cookie_as_string(c,p), three_month_expiration);

}
dTEST(rfc_cookie, 5);


static void ua_version(dAT)
{
    AT_int_eq(apreq_ua_cookie_version(NULL), APREQ_COOKIE_VERSION_NETSCAPE);
    AT_int_eq(apreq_ua_cookie_version("$Version=\"1\""), APREQ_COOKIE_VERSION_RFC);
}
dTEST(ua_version, 2);


int main(int argc, char *argv[])
{
    int i, plan = 0;
    extern const apreq_env_t test_module;
    dAT;
    at_test_t cookie_list [] = {
        test_jar_make,
        test_jar_get,
        test_netscape_cookie,
        test_rfc_cookie,
        test_ua_version,
    };

    apr_initialize();
    atexit(apr_terminate);
    apreq_env_module(&test_module);

    apr_pool_create(&p, NULL);

    AT = at_create(p, 0, at_report_stdout_make(p)); 

    for (i = 0; i < sizeof(cookie_list) / sizeof(at_test_t);  ++i)
        plan += cookie_list[i].plan;

    AT_begin(plan);

    for (i = 0; i < sizeof(cookie_list) / sizeof(at_test_t);  ++i)
        AT_run(&cookie_list[i]);

    AT_end();

    return 0;
}
