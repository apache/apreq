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

#define CRLF "\015\012"

static char url_data[] = "alpha=one&beta=two;omega=last%2";
static char form_data[] = 
"--AaB03x" CRLF
"content-disposition: form-data; name=\"field1\"" CRLF
"content-type: text/plain;charset=windows-1250" CRLF
"content-transfer-encoding: quoted-printable" CRLF CRLF
"Joe owes =80100." CRLF
"--AaB03x" CRLF
"content-disposition: form-data; name=\"pics\"; filename=\"file1.txt\"" CRLF
"Content-Type: text/plain" CRLF CRLF
"... contents of file1.txt ..." CRLF CRLF
"--AaB03x--" CRLF;

extern apr_bucket_brigade *bb;
extern apr_table_t *table;

static void parse_urlencoded(CuTest *tc)
{
    const char *val;
    apreq_request_t *req = apreq_request(APREQ_URL_ENCTYPE,"");
    apr_status_t rv;
    const char *enctype;
    CuAssertPtrNotNull(tc, req);

    enctype = apreq_enctype(req->env);
    CuAssertStrEquals(tc, APREQ_URL_ENCTYPE, enctype);

    bb = apr_brigade_create(p, apr_bucket_alloc_create(p));

    APR_BRIGADE_INSERT_HEAD(bb,
        apr_bucket_immortal_create(url_data,strlen(url_data), 
                                   bb->bucket_alloc));

    rv = apreq_parse_request(req,bb);
    CuAssertIntEquals(tc, APR_INCOMPLETE, rv);

    APR_BRIGADE_INSERT_HEAD(bb,
        apr_bucket_immortal_create("blast",5, 
                                   bb->bucket_alloc));
    APR_BRIGADE_INSERT_TAIL(bb,
           apr_bucket_eos_create(bb->bucket_alloc));

    rv = apreq_parse_request(req,bb);

    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    val = apr_table_get(req->body,"alpha");

    CuAssertStrEquals(tc, "one", val);
    val = apr_table_get(req->body,"beta");
    CuAssertStrEquals(tc, "two", val);
    val = apr_table_get(req->body,"omega");
    CuAssertStrEquals(tc, "last+last", val);

}

static void parse_multipart(CuTest *tc)
{
    const char *val;
    apr_size_t len;
    apr_table_t *t;
    apr_status_t rv;
    const char *enctype;
    apreq_request_t *req = apreq_request(APREQ_MFD_ENCTYPE
                         "; charset=\"iso-8859-1\"; boundary=\"AaB03x\"" ,"");
    apr_bucket_brigade *bb = apr_brigade_create(p, 
                                   apr_bucket_alloc_create(p));
    apr_size_t j;

    CuAssertPtrNotNull(tc, req);
    CuAssertStrEquals(tc, req->env, apreq_env_content_type(req->env));

    enctype = apreq_enctype(req->env);
    CuAssertStrEquals(tc, APREQ_MFD_ENCTYPE, enctype);

    /* strlen(form_data) == 319 */
    for (j = 0; j <= strlen(form_data); ++j) {
        apr_bucket *e = apr_bucket_immortal_create(form_data,
                                                   strlen(form_data),
                                                   bb->bucket_alloc);
        apr_bucket_brigade *tail;
        APR_BRIGADE_INSERT_HEAD(bb, e);
        APR_BRIGADE_INSERT_TAIL(bb, apr_bucket_eos_create(bb->bucket_alloc));

        /* Split e into two buckets, leaving e with the first j bytes. 
         * Then put all the buckets after e into the tail brigade,
         * drop any existing parser data (from a previous pass through
         * the loop) and run the parser tests.
         */
        apr_bucket_split(e,j);
        tail = apr_brigade_split(bb, APR_BUCKET_NEXT(e));
        req->body = NULL;
        req->parser = NULL;

        rv = apreq_parse_request(req,bb);
        CuAssertIntEquals(tc, (j < strlen(form_data)) ? APR_INCOMPLETE : APR_SUCCESS, rv);
        rv = apreq_parse_request(req, tail);
        CuAssertIntEquals(tc, APR_SUCCESS, rv);
        CuAssertPtrNotNull(tc, req->body);
        CuAssertIntEquals(tc, 2, apr_table_elts(req->body)->nelts);

        val = apr_table_get(req->body,"field1");
        CuAssertStrEquals(tc, "Joe owes =80100.", val);
        t = apreq_value_to_param(apreq_strtoval(val))->info;
        val = apr_table_get(t, "content-transfer-encoding");
        CuAssertStrEquals(tc,"quoted-printable", val);

        val = apr_table_get(req->body, "pics");
        CuAssertStrEquals(tc, "file1.txt", val);
        t = apreq_value_to_param(apreq_strtoval(val))->info;
        bb = apreq_value_to_param(apreq_strtoval(val))->bb;
        apr_brigade_pflatten(bb, (char **)&val, &len, p);
        CuAssertIntEquals(tc,strlen("... contents of file1.txt ..." CRLF), len);
        CuAssertStrNEquals(tc,"... contents of file1.txt ..." CRLF, val, len);
        val = apr_table_get(t, "content-type");
        CuAssertStrEquals(tc, "text/plain", val);
    }
}
static void parse_disable_uploads(CuTest *tc)
{
    const char *val;
    apr_table_t *t;
    apr_status_t rv;
    apreq_request_t *req = apreq_request(APREQ_MFD_ENCTYPE
                         "; charset=\"iso-8859-1\"; boundary=\"AaB03x\"" ,"");
    apr_bucket_brigade *bb = apr_brigade_create(p, 
                                   apr_bucket_alloc_create(p));
    apr_bucket *e = apr_bucket_immortal_create(form_data,
                                                   strlen(form_data),
                                                   bb->bucket_alloc);

    CuAssertPtrNotNull(tc, req);
    CuAssertStrEquals(tc, req->env, apreq_env_content_type(req->env));

    APR_BRIGADE_INSERT_HEAD(bb, e);
    APR_BRIGADE_INSERT_TAIL(bb, apr_bucket_eos_create(bb->bucket_alloc));

    req->body = NULL;
    req->parser = apreq_parser(req->env, apreq_make_hook(p, 
                                 apreq_hook_disable_uploads, NULL, NULL));

    rv = apreq_parse_request(req,bb);
    CuAssertIntEquals(tc, APR_EGENERAL, rv);
    CuAssertPtrNotNull(tc, req->body);
    CuAssertIntEquals(tc, 1, apr_table_elts(req->body)->nelts);

    val = apr_table_get(req->body,"field1");
    CuAssertStrEquals(tc, "Joe owes =80100.", val);
    t = apreq_value_to_param(apreq_strtoval(val))->info;
    val = apr_table_get(t, "content-transfer-encoding");
    CuAssertStrEquals(tc,"quoted-printable", val);

    val = apr_table_get(req->body, "pics");
    CuAssertPtrEquals(tc, NULL, val);
}


CuSuite *testparser(void)
{
    CuSuite *suite = CuSuiteNew("Parsers");
    SUITE_ADD_TEST(suite, parse_urlencoded);
    SUITE_ADD_TEST(suite, parse_multipart);
    SUITE_ADD_TEST(suite, parse_disable_uploads);
    return suite;
}

