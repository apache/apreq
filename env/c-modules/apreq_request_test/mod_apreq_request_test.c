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

    if (strcmp(r->handler, "apreq_request_test") != 0)
        return DECLINED;

    apreq_log(APREQ_DEBUG 0, r, "initializing request");
    req = apreq_request(r, NULL);
    bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);

    apreq_log(APREQ_DEBUG 0, r, "start parsing body");    
    while ((s =ap_get_brigade(r->input_filters, bb, AP_MODE_READBYTES,
                              APR_BLOCK_READ, HUGE_STRING_LEN)) == APR_SUCCESS)
    {
        if (APR_BUCKET_IS_EOS(APR_BRIGADE_LAST(bb)))
            break;

        apr_brigade_cleanup(bb);

    }
    apreq_log(APREQ_DEBUG s, r, "finished parsing body");

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
