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
#include "test_apreq.h"
#include "apreq.h"
#include "apreq_params.h"
#include "apr_strings.h"

static apreq_request_t *r = NULL;

static void request_make(CuTest *tc)
{
    r = apreq_request(NULL,"a=1;quux=foo+bar&plus=%2B;uplus=%U002b;okie=dokie;novalue1;novalue2=");
    CuAssertPtrNotNull(tc, r);
    CuAssertIntEquals(tc,7, apr_table_elts(r->args)->nelts);
}

static void request_args_get(CuTest *tc)
{
    const char *val;
    apreq_value_t *v;

    val = apr_table_get(r->args,"a");
    CuAssertStrEquals(tc,"1",val);
    val = apr_table_get(r->args,"quux");
    CuAssertStrEquals(tc,"foo bar",val);
    v = apreq_strtoval(val);
    CuAssertIntEquals(tc, 7, v->size);
    val = apr_table_get(r->args,"plus");
    CuAssertStrEquals(tc,"+",val);
    val = apr_table_get(r->args,"uplus");
    CuAssertStrEquals(tc,"+",val);
    val = apr_table_get(r->args,"okie");
    CuAssertStrEquals(tc,"dokie",val);
    val = apr_table_get(r->args,"novalue1");
    CuAssertStrEquals(tc,"",val);
    val = apr_table_get(r->args,"novalue2");
    CuAssertStrEquals(tc,"",val);

}

static void string_decoding_in_place(CuTest *tc)
{
    char *s1 = apr_palloc(p,4096);
    char *s2 = apr_palloc(p,4096);

    strcpy(s1, "bend it like beckham");
    strcpy(s2, "dandy %3Edons");

    CuAssertStrEquals(tc,"bend it like beckham",s1);
    apreq_unescape(s1);
    CuAssertStrEquals(tc,"bend it like beckham",s1);
    CuAssertStrEquals(tc,"dandy %3Edons",s2);
    apreq_unescape(s2);
    CuAssertStrEquals(tc,"dandy >dons",s2);
    
}


CuSuite *testparam(void)
{
    CuSuite *suite = CuSuiteNew("Param");

    SUITE_ADD_TEST(suite, request_make);
    SUITE_ADD_TEST(suite, request_args_get);
    SUITE_ADD_TEST(suite, string_decoding_in_place);
    return suite;
}

