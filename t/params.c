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
#include "apreq_params.h"
#include "apr_strings.h"
#include "at.h"

static apreq_request_t *r = NULL;
extern apr_pool_t *p;

static void request_make(dAT)
{
    r = apreq_request(NULL,"a=1;quux=foo+bar&a=2&plus=%2B;uplus=%U002b;okie=dokie;novalue1;novalue2=");
    AT_not_null(r);
    AT_int_eq(apr_table_elts(r->args)->nelts, 8);
}

static void request_args_get(dAT)
{
    const char *val;
    apreq_value_t *v;

    AT_str_eq(apr_table_get(r->args,"a"), "1");

    val = apr_table_get(r->args,"quux");
    AT_str_eq(val, "foo bar");
    v = apreq_strtoval(val);
    AT_int_eq(v->size, 7);

    AT_str_eq(apr_table_get(r->args,"plus"), "+");
    AT_str_eq(apr_table_get(r->args,"uplus"), "+");
    AT_str_eq(apr_table_get(r->args,"okie"), "dokie");
    AT_str_eq(apr_table_get(r->args,"novalue1"), "");
    AT_str_eq(apr_table_get(r->args,"novalue2"),"");
}

static void params_as(dAT)
{
    const char *val;
    apr_array_header_t *arr;
    arr = apreq_params_as_array(p,r,"a");
    AT_int_eq(arr->nelts, 2);
    val = apreq_params_as_string(p,r,"a",APREQ_JOIN_AS_IS);
    AT_str_eq(val, "1, 2");
}

static void string_decoding_in_place(dAT)
{
    char *s1 = apr_palloc(p,4096);
    char *s2 = apr_palloc(p,4096);
    char *s3;

    strcpy(s1, "bend it like beckham");
    strcpy(s2, "dandy %3Edons");

    AT_str_eq(s1,"bend it like beckham");
    apreq_unescape(s1);
    AT_str_eq(s1, "bend it like beckham");
    s3 = apreq_escape(p, s1, 20);
    AT_str_eq(s3, "bend+it+like+beckham");
    apreq_unescape(s3);
    AT_str_eq(s3,"bend it like beckham");

    AT_str_eq(s2,"dandy %3Edons");
    apreq_unescape(s2);
    AT_str_eq(s2,"dandy >dons");
    s3 = apreq_escape(p, s2, 11);
    AT_str_eq(s3,"dandy+%3edons");
    apreq_unescape(s3);
    AT_str_eq(s3,"dandy >dons"); 
}

static void header_attributes(dAT)
{
    const char *hdr = "text/plain; boundary=\"-foo-\", charset=ISO-8859-1";
    const char *val;
    apr_size_t vlen;
    apr_status_t s;

    s = apreq_header_attribute(hdr, "none", 4, &val, &vlen);
    AT_int_eq(s, APR_NOTFOUND);

    s = apreq_header_attribute(hdr, "set", 3, &val, &vlen);
    AT_int_eq(s, APR_NOTFOUND);

    s = apreq_header_attribute(hdr, "boundary", 8, &val, &vlen);
    AT_int_eq(s, APR_SUCCESS);
    AT_int_eq(vlen, 5);
    AT_mem_eq(val, "-foo-", 5);

    s = apreq_header_attribute(hdr, "charset", 7, &val, &vlen);
    AT_int_eq(s, APR_SUCCESS);
    AT_int_eq(vlen, 10);
    AT_mem_eq(val, "ISO-8859-1", 10);

    hdr = "max-age=20; no-quote=\"...";

    s = apreq_header_attribute(hdr, "max-age", 7, &val, &vlen);
    AT_int_eq(s, APR_SUCCESS);
    AT_int_eq(vlen, 2);
    AT_mem_eq(val, "20", 2);

    s = apreq_header_attribute(hdr, "age", 3, &val, &vlen);
    AT_int_eq(s, APR_EGENERAL);

    s = apreq_header_attribute(hdr, "no-quote", 8, &val, &vlen);
    AT_int_eq(s, APR_EGENERAL);

}

static void make_values(dAT)
{
    apreq_value_t *v1, *v2;
    apr_size_t len = 4;
    char *name = apr_palloc(p,len);
    char *val = apr_palloc(p,len);
    strcpy(name, "foo");
    strcpy(val, "bar");
 
    v1 = apreq_make_value(p, name, len, val, len);
    AT_str_eq(v1->name, name);
    AT_int_eq(v1->size, len);
    AT_str_eq(v1->data, val);

    v2 = apreq_copy_value(p, v1);
    AT_str_eq(v2->name, name);
    AT_int_eq(v2->size, len);
    AT_str_eq(v2->data, val);
}

static void make_param(dAT)
{
    apreq_param_t *param, *decode;
    apr_size_t nlen = 3, vlen = 11;
    char *name = apr_palloc(p,nlen+1);
    char *val = apr_palloc(p,vlen+1);
    char *encode;
    strcpy(name, "foo");
    strcpy(val, "bar > alpha");
 
    param = apreq_make_param(p, name, nlen, val, vlen);
    AT_str_eq(param->v.name, name);
    AT_int_eq(param->v.size, vlen);
    AT_str_eq(param->v.data, val);

    encode = apreq_encode_param(p, param);
    AT_str_eq(encode, "foo=bar+%3e+alpha");

    decode = apreq_decode_param(p, encode, nlen, vlen+2);
    AT_str_eq(decode->v.name, name);
    AT_int_eq(decode->v.size, vlen);
    AT_str_eq(decode->v.data, val);
}

static void quote_strings(dAT)
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
        AT_int_eq(res_len, arr_quote_len[i]);
        AT_mem_eq(res, arr_quote[i], res_len);
        res_quote_len = apreq_quote_once(res_quote, res, res_len);
        AT_int_eq(res_quote_len, res_len);
        AT_mem_eq(res_quote, res, res_len);
        res_len = apreq_quote_once(res, arr[i], arr_len[i]);
        exp_len = (i == 3) ? arr_len[i] : arr_quote_len[i];
        exp = (i == 3) ? arr[i] : arr_quote[i];
        AT_int_eq(res_len, exp_len);
        AT_mem_eq(res, exp, exp_len);
    }
}

static void test_memmem(dAT)
{
    char *hay = apr_palloc(p,29);
    char *partial, *full;
    apr_size_t hlen = 28;
    strcpy(hay, "The fear of fear is fearful.");

    partial = apreq_memmem(hay, hlen, "fear of fly", 11, APREQ_MATCH_PARTIAL);
    AT_is_null(partial);
    partial = apreq_memmem(hay, hlen, "fear ", 5, APREQ_MATCH_PARTIAL);
    AT_str_eq(partial, "fear of fear is fearful.");
    partial = apreq_memmem(hay, hlen, "fear is", 7, APREQ_MATCH_PARTIAL);
    AT_str_eq(partial, "fear is fearful.");
    partial = apreq_memmem(hay, hlen, hay, hlen, APREQ_MATCH_PARTIAL);
    AT_str_eq(partial, hay);

    full = apreq_memmem(hay, hlen, "fear is", 7, APREQ_MATCH_FULL);
    AT_not_null(full);
    partial = apreq_memmem(hay, hlen, "fear of fly", 11, APREQ_MATCH_FULL);
    AT_is_null(partial);
    full = apreq_memmem(hay, hlen, hay, hlen, APREQ_MATCH_FULL);
    AT_str_eq(full, hay);
}

#define dT(func, plan) {#func, func, plan}

int main(int argc, char *argv[])
{
    extern const apreq_env_t test_module;
    extern apr_pool_t *p;
    int i, plan = 0;
    dAT;
    at_test_t test_list [] = {
        dT(request_make, 2),
        dT(request_args_get, 8),
        dT(params_as, 2),
        dT(string_decoding_in_place, 8),
        dT(header_attributes, 13),
        dT(make_values, 6),
        dT(make_param, 7),
        dT(quote_strings, 24),
        dT(test_memmem, 7),
    };

    apr_initialize();
    atexit(apr_terminate);
    apreq_env_module(&test_module);

    apr_pool_create(&p, NULL);

    AT = at_create(p, 0, at_report_stdout_make(p)); 
    AT_trace_on();
    for (i = 0; i < sizeof(test_list) / sizeof(at_test_t);  ++i)
        plan += test_list[i].plan;

    AT_begin(plan);

    for (i = 0; i < sizeof(test_list) / sizeof(at_test_t);  ++i)
        AT_run(&test_list[i]);

    AT_end();

    return 0;
}

