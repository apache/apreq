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
    r = apreq_request(NULL,"a=1;quux=foo+bar&a=2&plus=%2B;uplus=%U002b;okie=dokie;novalue1;novalue2=");
    CuAssertPtrNotNull(tc, r);
    CuAssertIntEquals(tc,8, apr_table_elts(r->args)->nelts);
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

static void params_as(CuTest *tc)
{
    const char *val;
    apr_array_header_t *arr;
    arr = apreq_params_as_array(p,r,"a");
    CuAssertIntEquals(tc,2,arr->nelts);
    val = apreq_params_as_string(p,r,"a",APREQ_JOIN_AS_IS);
    CuAssertStrEquals(tc,"1, 2", val);
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

static void header_attributes(CuTest *tc)
{
    const char hdr[] = "text/plain; boundary=\"-foo-\", charset=ISO-8859-1";
    const char *val;
    apr_size_t vlen;
    apr_status_t s;

    s = apreq_header_attribute(hdr, "none", 4, &val, &vlen);
    CuAssertIntEquals(tc, APR_NOTFOUND, s);

    s = apreq_header_attribute(hdr, "set", 3, &val, &vlen);
    CuAssertIntEquals(tc, APR_NOTFOUND, s);

    s = apreq_header_attribute(hdr, "boundary", 8, &val, &vlen);
    CuAssertIntEquals(tc, APR_SUCCESS, s);
    CuAssertIntEquals(tc, 5, vlen);
    CuAssertStrNEquals(tc, "-foo-", val, 5);

    s = apreq_header_attribute(hdr, "charset", 7, &val, &vlen);
    CuAssertIntEquals(tc, APR_SUCCESS, s);
    CuAssertIntEquals(tc, 10, vlen);
    CuAssertStrNEquals(tc, "ISO-8859-1", val, 10);

}

static void make_values(CuTest *tc)
{
    apreq_value_t *v1, *v2;
    apr_size_t len = 4;
    char *name = apr_palloc(p,len);
    char *val = apr_palloc(p,len);
    strcpy(name, "foo");
    strcpy(val, "bar");
 
    v1 = apreq_make_value(p, name, len, val, len);
    CuAssertStrEquals(tc, name, v1->name);
    CuAssertIntEquals(tc, len, v1->size);
    CuAssertStrEquals(tc, val, v1->data);

    v2 = apreq_copy_value(p, v1);
    CuAssertStrEquals(tc, name, v2->name);
    CuAssertIntEquals(tc, len, v2->size);
    CuAssertStrEquals(tc, val, v2->data);
}

static void quote_strings(CuTest *tc)
{
    apr_size_t exp_len, res_len, res_quote_len;
    char *res = apr_palloc(p,24);
    char *res_quote = apr_palloc(p,24);
    char *exp = apr_palloc(p,24);
    int i;
    char * arr[] = {"cest", "\"cest", "ce\"st", "\"cest\""};
    char * arr_quote[] = 
        {"\"cest\"", "\"\\\"cest\"", "\"ce\\\"st\"", "\"\\\"cest\\\"\""};
    apr_size_t arr_len[] = {4, 5, 5, 6};
    apr_size_t arr_quote_len[] = {6, 8, 8, 10};

    for (i=0; i<4; i++) {
        res_len = apreq_quote(res, arr[i], arr_len[i]);
        CuAssertIntEquals(tc, arr_quote_len[i], res_len);
        CuAssertStrNEquals(tc, arr_quote[i], res, res_len);
        res_quote_len = apreq_quote_once(res_quote, res, res_len);
        CuAssertIntEquals(tc, res_len, res_quote_len);
        CuAssertStrNEquals(tc, res, res_quote, res_len);
        res_len = apreq_quote_once(res, arr[i], arr_len[i]);
        exp_len = (i == 3) ? arr_len[i] : arr_quote_len[i];
        exp = (i == 3) ? arr[i] : arr_quote[i];
        CuAssertIntEquals(tc, exp_len, res_len);
        CuAssertStrNEquals(tc, exp, res, exp_len);
    }
}

CuSuite *testparam(void)
{
    CuSuite *suite = CuSuiteNew("Param");

    SUITE_ADD_TEST(suite, request_make);
    SUITE_ADD_TEST(suite, request_args_get);
    SUITE_ADD_TEST(suite, params_as);
    SUITE_ADD_TEST(suite, string_decoding_in_place);
    SUITE_ADD_TEST(suite, header_attributes);
    SUITE_ADD_TEST(suite, make_values);
    SUITE_ADD_TEST(suite, quote_strings);

    return suite;
}

