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
#include "apr_xml.h"
#include "test_apreq.h"

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
"<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>"
"<methodCall>"
"  <methodName>foo.bar</methodName>"
"  <params>"
"    <param><value><int>1</int></value></param>"
"  </params>"
"</methodCall>";

static char rel_data[] = /*offsets: 122, 522, */
"--f93dcbA3" CRLF
"Content-Type: application/xml; charset=UTF-8" CRLF
"Content-Length: 400" CRLF
"Content-ID: <980119.X53GGT@example.com>" CRLF CRLF /*122*/
"<?xml version=\"1.0\"?>" CRLF
"<uploadDocument>"
"  <title>My Proposal</title>"
"  <author>E. X. Ample</author>"
"  <summary>A proposal for a new project.</summary>"
"  <notes image=\"cid:980119.X17AXM@example.com\">(see handwritten region)</notes>"
"  <keywords>project proposal funding</keywords>"
"  <readonly>false</readonly>"
"  <filename>image.png</filename>"
"  <content>cid:980119.X25MNC@example.com</content>"
"</uploadDocument>" /*400*/ CRLF
"--f93dcbA3" CRLF /*14*/
"Content-Type: image/png" CRLF
"Content-Transfer-Encoding: binary" CRLF
"Content-ID: <980119.X25MNC@example.com>" CRLF CRLF /*103*/
"...Binary data here..."  /*22*/ CRLF
"--f93dcbA3" CRLF /*14*/
"Content-Type: image/png" CRLF
"Content-Transfer-Encoding: binary" CRLF
"Content-ID: <980119.X17AXM@example.com>" CRLF CRLF
"...Binary data here..." CRLF
"--f93dcbA3--" CRLF;

static char mix_data[] =
"--AaB03x" CRLF
"Content-Disposition: form-data; name=\"submit-name\"" CRLF CRLF
"Larry" CRLF
"--AaB03x" CRLF
"Content-Disposition: form-data; name=\"files\"" CRLF
"Content-Type: multipart/mixed; boundary=BbC04y" CRLF CRLF
"--BbC04y" CRLF
"Content-Disposition: file; filename=\"file1.txt\"" CRLF
"Content-Type: text/plain" CRLF CRLF
"... contents of file1.txt ..." CRLF
"--BbC04y" CRLF
"Content-Disposition: file; filename=\"file2.gif\"" CRLF
"Content-Type: image/gif" CRLF
"Content-Transfer-Encoding: binary" CRLF CRLF
"...contents of file2.gif..." CRLF
"--BbC04y--" CRLF
"--AaB03x" CRLF
"content-disposition: form-data; name=\"field1\"" CRLF
"content-type: text/plain;charset=windows-1250" CRLF
"content-transfer-encoding: quoted-printable" CRLF CRLF
"Joe owes =80100." CRLF
"--AaB03x--" CRLF;


#define URL_ENCTYPE "application/x-www-form-urlencoded"
#define MFD_ENCTYPE "multipart/form-data"
#define MR_ENCTYPE "multipart/related"
#define XML_ENCTYPE "application/xml"

static void locate_default_parsers(CuTest *tc)
{
    apreq_parser_function_t f;

    /* initialize default-parser hash */
    apreq_register_parser(NULL, NULL);
    
    f = apreq_parser(URL_ENCTYPE);
    CuAssertPtrNotNull(tc, f);
    CuAssertPtrEquals(tc, apreq_parse_urlencoded, f);

    f = apreq_parser(MFD_ENCTYPE);
    CuAssertPtrNotNull(tc, f);
    CuAssertPtrEquals(tc, apreq_parse_multipart, f);

    f = apreq_parser(MR_ENCTYPE);
    CuAssertPtrNotNull(tc, f);
    CuAssertPtrEquals(tc, apreq_parse_multipart, f);
}

static void parse_urlencoded(CuTest *tc)
{
    const char *val;
    apr_status_t rv;
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    apreq_parser_t *parser;
    apr_table_t *body;

    body = apr_table_make(p, APREQ_DEFAULT_NELTS);
    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);
    parser = apreq_make_parser(p, ba, URL_ENCTYPE, apreq_parse_urlencoded,
                               100, NULL, NULL, NULL);

    APR_BRIGADE_INSERT_HEAD(bb,
        apr_bucket_immortal_create(url_data,strlen(url_data), 
                                   bb->bucket_alloc));

    rv = apreq_run_parser(parser, body, bb);
    CuAssertIntEquals(tc, APR_INCOMPLETE, rv);

    APR_BRIGADE_INSERT_HEAD(bb,
        apr_bucket_immortal_create("blast",5, 
                                   bb->bucket_alloc));
    APR_BRIGADE_INSERT_TAIL(bb,
           apr_bucket_eos_create(bb->bucket_alloc));

    rv = apreq_run_parser(parser, body, bb);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    val = apr_table_get(body,"alpha");
    CuAssertStrEquals(tc, "one", val);
    val = apr_table_get(body,"beta");
    CuAssertStrEquals(tc, "two", val);
    val = apr_table_get(body,"omega");
    CuAssertStrEquals(tc, "last+last", val);

}

static void parse_multipart(CuTest *tc)
{
    apr_size_t i, j;
    apr_bucket_alloc_t *ba;


    for (j = 0; j <= strlen(form_data); ++j) {

        ba = apr_bucket_alloc_create(p);

        for (i = 0; i <= strlen(form_data); ++i) {
            const char *val;
            char *val2;
            apr_size_t len;
            apr_table_t *t, *body;
            apreq_parser_t *parser;
            apr_bucket_brigade *bb, *vb;
            apr_status_t rv;

            bb = apr_brigade_create(p, ba);
            body = apr_table_make(p, APREQ_DEFAULT_NELTS);
            parser = apreq_make_parser(p, ba, MFD_ENCTYPE
                                       "; charset=\"iso-8859-1\""
                                       "; boundary=\"AaB03x\"",
                                       apreq_parse_multipart,
                                       1000, NULL, NULL, NULL);

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
            rv = apreq_run_parser(parser, body, bb);
            CuAssertIntEquals(tc, (j < strlen(form_data)) ? APR_INCOMPLETE : APR_SUCCESS, rv);
            rv = apreq_run_parser(parser, body, tail);
            CuAssertIntEquals(tc, APR_SUCCESS, rv);
            CuAssertIntEquals(tc, 2, apr_table_elts(body)->nelts);

            val = apr_table_get(body,"field1");

            CuAssertStrEquals(tc, "Joe owes =80100.", val);
            t = apreq_value_to_param(val)->info;
            val = apr_table_get(t, "content-transfer-encoding");
            CuAssertStrEquals(tc,"quoted-printable", val);

            val = apr_table_get(body, "pics");
            CuAssertStrEquals(tc, "file1.txt", val);
            t = apreq_value_to_param(val)->info;
            vb = apreq_value_to_param(val)->upload;
            apr_brigade_pflatten(vb, &val2, &len, p);
            CuAssertIntEquals(tc,strlen("... contents of file1.txt ..." CRLF), len);
            CuAssertStrNEquals(tc,"... contents of file1.txt ..." CRLF, val2, len);
            val = apr_table_get(t, "content-type");
            CuAssertStrEquals(tc, "text/plain", val);
            apr_brigade_cleanup(vb);
            apr_brigade_cleanup(bb);
        }

        apr_bucket_alloc_destroy(ba);
        apr_pool_clear(p);
    }
}

static void parse_disable_uploads(CuTest *tc)
{
    const char *val;
    apr_table_t *t, *body;
    apr_status_t rv;
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    apr_bucket *e;
    apreq_parser_t *parser;
    apreq_hook_t *hook;

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    e = apr_bucket_immortal_create(form_data, strlen(form_data), ba);
    APR_BRIGADE_INSERT_HEAD(bb, e);
    APR_BRIGADE_INSERT_TAIL(bb, apr_bucket_eos_create(bb->bucket_alloc));

    body = apr_table_make(p, APREQ_DEFAULT_NELTS);
    hook = apreq_make_hook(p, apreq_hook_disable_uploads, NULL, NULL);

    parser = apreq_make_parser(p, ba, MFD_ENCTYPE
                               "; charset=\"iso-8859-1\""
                               "; boundary=\"AaB03x\"",
                               apreq_parse_multipart,
                               1000, NULL, hook, NULL);


    rv = apreq_run_parser(parser, body, bb);
    CuAssertIntEquals(tc, APR_EGENERAL, rv);
    CuAssertIntEquals(tc, 1, apr_table_elts(body)->nelts);

    val = apr_table_get(body,"field1");
    CuAssertStrEquals(tc, "Joe owes =80100.", val);
    t = apreq_value_to_param(val)->info;
    val = apr_table_get(t, "content-transfer-encoding");
    CuAssertStrEquals(tc,"quoted-printable", val);

    val = apr_table_get(body, "pics");
    CuAssertPtrEquals(tc, NULL, val);
}


static void parse_generic(CuTest *tc)
{
    char *val;
    apr_size_t vlen;
    apr_status_t rv;
    apreq_param_t *dummy;
    apreq_parser_t *parser;
    apr_table_t *body;
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb = apr_brigade_create(p, ba);
    apr_bucket *e = apr_bucket_immortal_create(xml_data,
                                               strlen(xml_data),
                                               ba);

    APR_BRIGADE_INSERT_HEAD(bb, e);
    APR_BRIGADE_INSERT_TAIL(bb, apr_bucket_eos_create(ba));

    body = apr_table_make(p, APREQ_DEFAULT_NELTS);

    parser = apreq_make_parser(p, ba, "application/xml", 
                               apreq_parse_generic, 1000, NULL, NULL, NULL);

    rv = apreq_run_parser(parser, body, bb);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    dummy = *(apreq_param_t **)parser->ctx;
    CuAssertPtrNotNull(tc, dummy);
    apr_brigade_pflatten(dummy->upload, &val, &vlen, p);

    CuAssertIntEquals(tc, strlen(xml_data), vlen);
    CuAssertStrNEquals(tc, xml_data, val, vlen);
}

static void hook_discard(CuTest *tc)
{
    apr_status_t rv;
    apreq_param_t *dummy;
    apreq_parser_t *parser;
    apreq_hook_t *hook;
    apr_table_t *body;
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb = apr_brigade_create(p, ba);
    apr_bucket *e = apr_bucket_immortal_create(xml_data,
                                               strlen(xml_data),
                                               ba);

    APR_BRIGADE_INSERT_HEAD(bb, e);
    APR_BRIGADE_INSERT_TAIL(bb, apr_bucket_eos_create(ba));

    body = apr_table_make(p, APREQ_DEFAULT_NELTS);

    hook = apreq_make_hook(p, apreq_hook_discard_brigade, NULL, NULL);
    parser = apreq_make_parser(p, ba, "application/xml", 
                               apreq_parse_generic, 1000, NULL, hook, NULL);


    rv = apreq_run_parser(parser, body, bb);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    dummy = *(apreq_param_t **)parser->ctx;
    CuAssertPtrNotNull(tc, dummy);
    CuAssertPtrNotNull(tc, dummy->upload);
    CuAssertTrue(tc, APR_BRIGADE_EMPTY(dummy->upload));
}


static void parse_related(CuTest *tc)
{
    char ct[] = "multipart/related; boundary=f93dcbA3; "
        "type=application/xml; start=\"<980119.X53GGT@example.com>\"";
    char data[] = "...Binary data here...";
    int dlen = strlen(data);
    const char *val;
    char *val2;
    apr_size_t vlen;
    apr_status_t rv;
    int ns_map = 0;
    apr_xml_doc *doc;
    apr_table_t *body;
    apreq_parser_t *parser;
    apreq_hook_t *xml_hook;
    apreq_param_t *param;
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb = apr_brigade_create(p, ba);
    apr_bucket *e = apr_bucket_immortal_create(rel_data,
                                                   strlen(rel_data),
                                                   bb->bucket_alloc);

    APR_BRIGADE_INSERT_HEAD(bb, e);
    APR_BRIGADE_INSERT_TAIL(bb, apr_bucket_eos_create(bb->bucket_alloc));
    xml_hook = apreq_make_hook(p, apreq_hook_apr_xml_parser, NULL, NULL);

    body =   apr_table_make(p, APREQ_DEFAULT_NELTS);
    parser = apreq_make_parser(p, ba, ct, apreq_parse_multipart, 
                               1000, NULL, xml_hook, NULL);

    rv = apreq_run_parser(parser, body, bb);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    val = apr_table_get(body, "<980119.X53GGT@example.com>");
    CuAssertPtrNotNull(tc, val);
    param = apreq_value_to_param(val);

    CuAssertPtrNotNull(tc, param);
    CuAssertPtrNotNull(tc, param->info);
    val = apr_table_get(param->info, "Content-Length");
    CuAssertStrEquals(tc, "400", val);
    CuAssertPtrNotNull(tc, param->upload);
    apr_brigade_pflatten(param->upload, &val2, &vlen, p);
    CuAssertIntEquals(tc, 400, vlen);
    CuAssertStrNEquals(tc,rel_data + 122, val2, 400);

    doc = *(apr_xml_doc **)xml_hook->ctx;
    apr_xml_to_text(p, doc->root, APR_XML_X2T_FULL,
                    doc->namespaces, &ns_map, &val, &vlen);
    CuAssertIntEquals(tc, 400 - 22, vlen);
    CuAssertStrNEquals(tc, rel_data + 122 + 23, val, 400 - 23);


    val = apr_table_get(body, "<980119.X25MNC@example.com>");
    CuAssertPtrNotNull(tc, val);
    param = apreq_value_to_param(val);
    CuAssertPtrNotNull(tc, param);
    CuAssertPtrNotNull(tc, param->upload);
    apr_brigade_pflatten(param->upload, &val2, &vlen, p);
    CuAssertIntEquals(tc, dlen, vlen);
    CuAssertStrNEquals(tc, data, val2, vlen);

    val = apr_table_get(body, "<980119.X17AXM@example.com>");
    CuAssertPtrNotNull(tc, val);
    param = apreq_value_to_param(val);
    CuAssertPtrNotNull(tc, param);
    CuAssertPtrNotNull(tc, param->upload);
    apr_brigade_pflatten(param->upload, &val2, &vlen, p);
    CuAssertIntEquals(tc, dlen, vlen);
    CuAssertStrNEquals(tc, data, val2, vlen);
}

typedef struct {
    const char *key;
    const char *val;
} array_elt;


static void parse_mixed(CuTest *tc)
{
    const char *val;
    char *val2;
    apr_size_t vlen;
    apr_status_t rv;
    apreq_param_t *param;
    const apr_array_header_t *arr;
    array_elt *elt;
    char ct[] = MFD_ENCTYPE "; charset=\"iso-8859-1\"; boundary=\"AaB03x\"";
    apreq_parser_t *parser;
    apr_table_t *body = apr_table_make(p, APREQ_DEFAULT_NELTS);
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb = apr_brigade_create(p, ba);
    apr_bucket *e = apr_bucket_immortal_create(mix_data,
                                                   strlen(mix_data),
                                                   bb->bucket_alloc);

    APR_BRIGADE_INSERT_HEAD(bb, e);
    APR_BRIGADE_INSERT_TAIL(bb, apr_bucket_eos_create(bb->bucket_alloc));

    parser = apreq_make_parser(p, ba, ct, apreq_parse_multipart, 
                               1000, NULL, NULL, NULL);

    rv = apreq_run_parser(parser, body, bb);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    val = apr_table_get(body, "submit-name");
    CuAssertPtrNotNull(tc, val);
    CuAssertStrEquals(tc, "Larry", val);

    val = apr_table_get(body,"field1");
    CuAssertStrEquals(tc, "Joe owes =80100.", val);

    val = apr_table_get(body, "files");
    CuAssertPtrNotNull(tc, val);
    CuAssertStrEquals(tc, "file1.txt", val);
    param = apreq_value_to_param(val);

    CuAssertPtrNotNull(tc, param->upload);
    apr_brigade_pflatten(param->upload, &val2, &vlen, p);
    CuAssertIntEquals(tc, strlen("... contents of file1.txt ..."), vlen);
    CuAssertStrNEquals(tc, "... contents of file1.txt ...", val2, vlen);

    arr = apr_table_elts(body);
    CuAssertIntEquals(tc, 4, arr->nelts);

    elt = (array_elt *)&arr->elts[2 * arr->elt_size];
    CuAssertStrEquals(tc, "files", elt->key);
    CuAssertStrEquals(tc, "file2.gif", elt->val);

    param = apreq_value_to_param(elt->val);
    CuAssertPtrNotNull(tc, param->upload);
    apr_brigade_pflatten(param->upload, &val2, &vlen, p);
    CuAssertIntEquals(tc, strlen("...contents of file2.gif..."), vlen);
    CuAssertStrNEquals(tc, "...contents of file2.gif...", val2, vlen);

}

CuSuite *testparser(void)
{
    CuSuite *suite = CuSuiteNew("Parsers");
    SUITE_ADD_TEST(suite, locate_default_parsers);
    SUITE_ADD_TEST(suite, parse_urlencoded);
    SUITE_ADD_TEST(suite, parse_multipart);
    SUITE_ADD_TEST(suite, parse_disable_uploads);
    SUITE_ADD_TEST(suite, parse_generic);
    SUITE_ADD_TEST(suite, hook_discard);
    SUITE_ADD_TEST(suite, parse_related);
    SUITE_ADD_TEST(suite, parse_mixed);
    return suite;
}

