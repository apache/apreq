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


static apreq_request_t *r = NULL;

static void request_make(CuTest *tc)
{
    r = apreq_request(NULL,"a=1;quux=foo+bar&plus=%2B;uplus=%U002b;okie=dokie;novalue1;novalue2=");
    CuAssertPtrNotNull(tc, r);
    CuAssertIntEquals(tc,7, apr_table_nelts(r->args));
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
    apreq_value_t *v;
    char *s1 = malloc(4096);
    char *s2 = malloc(4096);

    strcpy(s1, "bend it like beckham");
    strcpy(s2, "dandy %3Edons");

    CuAssertStrEquals(tc,"bend it like beckham",s1);
    apreq_unescape(s1);
    CuAssertStrEquals(tc,"bend it like beckham",s1);
    CuAssertStrEquals(tc,"dandy %3Edons",s2);
    apreq_unescape(s2);
    CuAssertStrEquals(tc,"dandy >dons",s2);
    
    free(s2);
    free(s1);
}


CuSuite *testparam(void)
{
    CuSuite *suite = CuSuiteNew("Param");

    SUITE_ADD_TEST(suite, request_make);
    SUITE_ADD_TEST(suite, request_args_get);
    SUITE_ADD_TEST(suite, string_decoding_in_place);
    return suite;
}

