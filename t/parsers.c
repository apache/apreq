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
#include "apr_xml.h"

#define CRLF "\015\012"

static char url_data[] = "alpha=one&beta=two;omega=last%2";
static char form_data[] = 
"--AaB03x" CRLF                                           /* 10 chars
 012345678901234567890123456789012345678901234567890123456789 */
"content-disposition: form-data; name=\"field1\"" CRLF    /* 47 chars */
"content-type: text/plain;charset=windows-1250" CRLF
"content-transfer-encoding: quoted-printable" CRLF CRLF
"Joe owes =80100." CRLF
"--AaB03x" CRLF
"content-disposition: form-data; name=\"pics\"; filename=\"file1.txt\"" CRLF
"Content-Type: text/plain" CRLF CRLF
"... contents of file1.txt ..." CRLF CRLF
"--AaB03x--" CRLF;

static char xml_data[] =
"<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>" /* length == 42 */
"<methodCall>"
"  <methodName>foo.bar</methodName>"
"  <params>"
"    <param><value><int>1</int></value></param>"
"  </params>"
"</methodCall>";


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
    apr_status_t rv;
    apr_size_t i, j;

    for (j = 0; j <= strlen(form_data); ++j) {
        const char *enctype;
        apr_bucket_brigade *bb;
        apreq_request_t *req = apreq_request(APREQ_MFD_ENCTYPE
                         "; charset=\"iso-8859-1\"; boundary=\"AaB03x\"" ,"");

        CuAssertPtrNotNull(tc, req);
        CuAssertStrEquals(tc, req->env, apreq_env_content_type(req->env));

        enctype = apreq_enctype(req->env);
        CuAssertStrEquals(tc, APREQ_MFD_ENCTYPE, enctype);

        bb = apr_brigade_create(p, apr_bucket_alloc_create(p));

        for (i = 0; i <= strlen(form_data); ++i) {
            const char *val;
            apr_size_t len;
            apr_table_t *t;

            apr_bucket *e = apr_bucket_immortal_create(form_data,
                                                       strlen(form_data),
                                                       bb->bucket_alloc);
            apr_bucket *f;
            apr_bucket_brigade *tail;
            APR_BRIGADE_INSERT_HEAD(bb, e);
            APR_BRIGADE_INSERT_TAIL(bb, apr_bucket_eos_create(bb->bucket_alloc));

            /* Split e into three buckets */
            apr_bucket_split(e, j);
            f = APR_BUCKET_NEXT(e);
            if (i < j)
                apr_bucket_split(e, i);
            else
                apr_bucket_split(f, i - j);

            tail = apr_brigade_split(bb, f);
            req->body = NULL;
            req->parser = NULL;
            req->body_status = APR_EINIT;
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
            apr_brigade_cleanup(bb);
        }

        apr_pool_clear(p);
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

static void parse_xml(CuTest *tc)
{
    const char *val;
    apr_size_t vlen;
    apr_status_t rv;
    int ns_map = 0;
    apr_xml_doc *doc;
    apreq_request_t *req = apreq_request(APREQ_XML_ENCTYPE, "");
    apr_bucket_brigade *bb = apr_brigade_create(p, 
                                   apr_bucket_alloc_create(p));
    apr_bucket *e = apr_bucket_immortal_create(xml_data,
                                                   strlen(xml_data),
                                                   bb->bucket_alloc);

    CuAssertPtrNotNull(tc, req);
    APR_BRIGADE_INSERT_HEAD(bb, e);
    APR_BRIGADE_INSERT_TAIL(bb, apr_bucket_eos_create(bb->bucket_alloc));

    req->body = NULL;
    req->parser = apreq_make_parser(p, APREQ_XML_ENCTYPE, 
                                    apreq_parse_xml, NULL, NULL);
    rv = apreq_parse_request(req,bb);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    doc = *(apr_xml_doc **)req->parser->ctx;
    CuAssertPtrNotNull(tc, doc);
    apr_xml_to_text(p, doc->root, APR_XML_X2T_FULL, 
                    doc->namespaces, &ns_map, &val, &vlen);
    CuAssertIntEquals(tc, strlen(xml_data), vlen + 42);
    CuAssertStrEquals(tc, xml_data + 43, val);

}

CuSuite *testparser(void)
{
    CuSuite *suite = CuSuiteNew("Parsers");
    SUITE_ADD_TEST(suite, parse_urlencoded);
    SUITE_ADD_TEST(suite, parse_multipart);
    SUITE_ADD_TEST(suite, parse_disable_uploads);
    SUITE_ADD_TEST(suite, parse_xml);
    return suite;
}

