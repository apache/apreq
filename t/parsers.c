/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2002-2003 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#include "apreq_env.h"
#include "test_apreq.h"
#include "apreq.h"
#include "apreq_params.h"
#include "apr_strings.h"
#include "apreq_parsers.h"

#define CRLF "\015\012"

static char url_data[] = "alpha=one&beta=two;omega=last";
static char form_data[] = 
"--AaB03x" CRLF
"content-disposition: form-data; name=\"field1\"" CRLF
"content-type: text/plain;charset=windows-1250" CRLF
"content-transfer-encoding: quoted-printable" CRLF CRLF
"Joe owes =80100." CRLF
"--AaB03x" CRLF
"content-disposition: form-data; name=\"pics\"; filename=\"file1.txt\"" CRLF
"Content-Type: text/plain" CRLF CRLF
"... contents of file1.txt ..." CRLF
"--AaB03x--" CRLF;

extern apr_bucket_brigade *bb;
extern apr_table_t *table;
extern apreq_cfg_t *config;

static void parse_urlencoded(CuTest *tc)
{
    const char *val;
    apreq_request_t *req = apreq_request(APREQ_URL_ENCTYPE,"");
    apr_status_t rv;
    CuAssertPtrNotNull(tc, req);

    bb = apr_brigade_create(p, apr_bucket_alloc_create(p));

    APR_BRIGADE_INSERT_HEAD(bb,
        apr_bucket_immortal_create(url_data,strlen(url_data), 
                                   bb->bucket_alloc));
    APR_BRIGADE_INSERT_TAIL(bb,
           apr_bucket_eos_create(bb->bucket_alloc));

    do rv = apreq_parse_request(req,bb);
    while (rv == APR_INCOMPLETE);

    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    val = apr_table_get(req->body,"alpha");

    CuAssertStrEquals(tc, "one", val);
    val = apr_table_get(req->body,"beta");
    CuAssertStrEquals(tc, "two", val);
    val = apr_table_get(req->body,"omega");
    CuAssertStrEquals(tc, "last", val);

}

static void parse_multipart(CuTest *tc)
{
    const char *val;
    apr_size_t len;
    apr_table_t *t;
    apr_status_t rv;
    apreq_request_t *req = apreq_request(APREQ_MFD_ENCTYPE
                         "; charset=\"iso-8859-1\"; boundary=\"AaB03x\"" ,"");
    apr_bucket_brigade *bb = apr_brigade_create(p, 
                                   apr_bucket_alloc_create(p));

    CuAssertPtrNotNull(tc, req);
    CuAssertStrEquals(tc, req->env, apreq_env_content_type(req->env));

    APR_BRIGADE_INSERT_HEAD(bb,
        apr_bucket_immortal_create(form_data,strlen(form_data), 
                                   bb->bucket_alloc));
    APR_BRIGADE_INSERT_TAIL(bb,
           apr_bucket_eos_create(bb->bucket_alloc));

    do rv = apreq_parse_request(req,bb);
    while (rv == APR_INCOMPLETE);

    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, req->body);
    CuAssertIntEquals(tc, 2, apr_table_nelts(req->body));

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
    CuAssertIntEquals(tc,strlen("... contents of file1.txt ..."), len);
    CuAssertStrNEquals(tc,"... contents of file1.txt ...", val, len);
    val = apr_table_get(t, "content-type");
    CuAssertStrEquals(tc, "text/plain", val);
}


CuSuite *testparser(void)
{
    CuSuite *suite = CuSuiteNew("Parsers");
    SUITE_ADD_TEST(suite, parse_urlencoded);
    SUITE_ADD_TEST(suite, parse_multipart);
    return suite;
}

