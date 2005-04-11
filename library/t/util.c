/*
**  Copyright 2004-2005  The Apache Software Foundation
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

#include "apr_strings.h"
#include "apreq_error.h"
#include "apreq_util.h"
#include "at.h"

static void test_atoi64f(dAT)
{
    AT_int_eq(apreq_atoi64f("0"), 0);
    AT_int_eq(apreq_atoi64f("-1"), -1);
    AT_int_eq(apreq_atoi64f("-"), 0);
    AT_int_eq(apreq_atoi64f("5"), 5);
    AT_int_eq(apreq_atoi64f("3.333"), 3);
    AT_int_eq(apreq_atoi64f("33k"), 33 * 1024);
    AT_int_eq(apreq_atoi64f(" +8M "), 8 * 1024 * 1024);
    AT_ok(apreq_atoi64f("44GB") == (apr_int64_t)44 * 1024 * 1024 * 1024, 
          "44GB test");
    AT_ok(apreq_atoi64f("0xaBcDefg") == (apr_int64_t)11259375 * 1024 * 1024 * 1024,
          "hex test");
}

static void test_atoi64t(dAT)
{
    AT_int_eq(apreq_atoi64t("0"), 0);
    AT_int_eq(apreq_atoi64t("-1"), -1);
    AT_int_eq(apreq_atoi64t("-g088l3dyg00k"), 0);
    AT_int_eq(apreq_atoi64t("5s"), 5);
    AT_int_eq(apreq_atoi64t("3.333"), 3);
    AT_int_eq(apreq_atoi64t("33d"), 33 * 60 * 60 * 24);
    AT_int_eq(apreq_atoi64t(" +8M "), 8 * 60 * 60 * 24 * 30);
    AT_int_eq(apreq_atoi64t("+9m"), 9 * 60);
    AT_int_eq(apreq_atoi64t("6h"), 6 * 60 * 60);

}

static void test_index(dAT)
{
    const char haystack[] = "Four score and seven years ago";
    apr_size_t hlen = sizeof haystack - 1;
    AT_int_eq(apreq_index(haystack, hlen, "Four", 4, APREQ_MATCH_FULL),
              0);
    AT_int_eq(apreq_index(haystack, hlen, "Four", 4, APREQ_MATCH_PARTIAL),
              0);
    AT_int_eq(apreq_index(haystack, hlen, "Fourteen", 8, APREQ_MATCH_FULL),
              -1);
    AT_int_eq(apreq_index(haystack, hlen, "Fourteen", 8, APREQ_MATCH_PARTIAL),
              -1);
    AT_int_eq(apreq_index(haystack, hlen, "agoraphobia", 11, APREQ_MATCH_FULL),
              -1);
    AT_int_eq(apreq_index(haystack, hlen, "agoraphobia", 11, APREQ_MATCH_PARTIAL),
              hlen - 3);
}

static void test_decode(dAT)
{





}

static void test_decodev(dAT)
{
    char src1[] = "%2540%2";
    char src2[] = "0%u0";
    char src3[] = "041";
    struct iovec iovec1[] = {
        { src1, sizeof(src1) - 1 },
        { src2, sizeof(src2) - 1 },
        { src3, sizeof(src3) - 1 },
    };
    struct iovec iovec2[] = {
        { src1, sizeof(src1) - 1 },
        { src2, sizeof(src2) - 1 },
    };
    const char expect1[] = "%40 A";
    const char expect2[] = "%40 ";
    char dest[sizeof(src1) + sizeof(src2) + sizeof(src3)];
    apr_size_t dest_len;
    apr_status_t status;

    status = apreq_decodev(dest, &dest_len, iovec1, 3);
    AT_int_eq(status, APR_SUCCESS + APREQ_CHARSET_UTF8);
    AT_int_eq(dest_len, sizeof(expect1) - 1);
    AT_mem_eq(dest, expect1, sizeof(expect1) - 1);

    status = apreq_decodev(dest, &dest_len, iovec2, 2);
    AT_int_eq(status, APR_INCOMPLETE);
    AT_int_eq(dest_len, sizeof(expect2) - 1);
    AT_mem_eq(dest, expect2, sizeof(expect2) - 1);
}


static void test_encode(dAT)
{

}

static void test_cp1252_to_utf8(dAT)
{

}

static void test_quote(dAT)
{

}

static void test_quote_once(dAT)
{

}

static void test_join(dAT)
{

}

static void test_brigade_fwrite(dAT)
{

}

static void test_file_mktemp(dAT)
{


}

static void test_header_attribute(dAT)
{


}

static void test_brigade_concat(dAT)
{

}



#define dT(func, plan) #func, func, plan


int main(int argc, char *argv[])
{
    unsigned i, plan = 0;
    apr_pool_t *p;
    dAT;
    at_test_t test_list [] = {
        { dT(test_atoi64f, 9) },
        { dT(test_atoi64t, 9) },
        { dT(test_index, 6) },
        { dT(test_decode, 0) },
        { dT(test_decodev, 6) },
        { dT(test_encode, 0) },
        { dT(test_cp1252_to_utf8, 0) },
        { dT(test_quote, 0) },
        { dT(test_quote_once, 0), },
        { dT(test_join, 0) },
        { dT(test_brigade_fwrite, 0) },
        { dT(test_file_mktemp, 0) },
        { dT(test_header_attribute, 0) },
        { dT(test_brigade_concat, 0) },
    };

    apr_initialize();
    atexit(apr_terminate);

    apr_pool_create(&p, NULL);

    AT = at_create(p, 0, at_report_stdout_make(p)); 

    for (i = 0; i < sizeof(test_list) / sizeof(at_test_t);  ++i)
        plan += test_list[i].plan;

    AT_begin(plan);

    for (i = 0; i < sizeof(test_list) / sizeof(at_test_t);  ++i)
        AT_run(&test_list[i]);

    AT_end();

    return 0;
}
