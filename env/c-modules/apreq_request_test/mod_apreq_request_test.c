/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2003 The Apache Software Foundation.  All rights
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

#if CONFIG_FOR_HTTPD_TEST

<Location /apreq_request_test>
   SetHandler apreq_request_test
</Location>

#endif

#define APACHE_HTTPD_TEST_HANDLER apreq_request_test_handler

#include "apache_httpd_test.h"
#include "apreq_params.h"
#include "apreq_env.h"
#include "httpd.h"

static int dump_table(void *ctx, const char *key, const char *value)
{
    request_rec *r = ctx;
    apreq_log(APREQ_DEBUG 0, r, "%s => %s", key, value);
    ap_rprintf(r, "\t%s => %s\n", key, value);
    return 1;
}

static int apreq_request_test_handler(request_rec *r)
{
    apr_bucket_brigade *bb;
    apreq_request_t *req;
    apr_status_t s;
    int saw_eos = 0;

    if (strcmp(r->handler, "apreq_request_test") != 0)
        return DECLINED;

    apreq_log(APREQ_DEBUG 0, r, "initializing request");
    req = apreq_request(r, NULL);


    bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);
    
    do {
        apr_bucket *e;
        apreq_log(APREQ_DEBUG 0, r, "pulling content thru input filters");
        s = ap_get_brigade(r->input_filters, bb, AP_MODE_READBYTES,
                           APR_BLOCK_READ, HUGE_STRING_LEN);

        APR_BRIGADE_FOREACH(e,bb) {
            if (APR_BUCKET_IS_EOS(e)) {
                saw_eos = 1;
                break;
            }
        }

        apr_brigade_cleanup(bb);

    } while (!saw_eos);
    apreq_log(APREQ_DEBUG 0, r, "no more content");

    ap_set_content_type(r, "text/plain");
    ap_rputs("ARGS:\n",r);
    apr_table_do(dump_table, r, req->args, NULL);
    if (req->body) {
        ap_rputs("BODY:\n",r);
        apr_table_do(dump_table, r, req->body, NULL);
    }
    return OK;
}

APACHE_HTTPD_TEST_MODULE(apreq_request_test);
