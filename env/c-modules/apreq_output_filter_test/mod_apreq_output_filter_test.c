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

<Location />
   AddOutputFilter APREQ_OUTPUT_FILTER html
</Location>

#endif

#include "apache_httpd_test.h"
#include "apreq_params.h"
#include "apreq_env.h"
#include "httpd.h"
#include "util_filter.h"

static const char filter_name[] =  "APREQ_OUTPUT_FILTER";
module AP_MODULE_DECLARE_DATA apreq_output_filter_test_module;

static apr_status_t apreq_output_filter_test_init(ap_filter_t *f)
{
    apreq_request_t *req;
    req = apreq_request(f->r, NULL);
    return APR_SUCCESS;
}

struct ctx_t {
    request_rec *r;
    apr_bucket_brigade *bb;
};

static int dump_table(void *data, const char *key, const char *value)
{
    struct ctx_t *ctx = (struct ctx_t *)data;
    apreq_log(APREQ_DEBUG 0, ctx->r, "%s => %s", key, value);
    apr_brigade_printf(ctx->bb,NULL,NULL,"\t%s => %s\n", key, value);
    return 1;
}

static apr_status_t apreq_output_filter_test(ap_filter_t *f, apr_bucket_brigade *bb)
{
    request_rec *r = f->r;
    apreq_request_t *req;
    apr_bucket_brigade *eos;
    struct ctx_t ctx = {r, bb};

    if (!APR_BUCKET_IS_EOS(APR_BRIGADE_LAST(bb)))
        return ap_pass_brigade(f->next,bb);

    eos = apr_brigade_split(bb, APR_BRIGADE_LAST(bb));

    req = apreq_request(r, NULL);
    apreq_log(APREQ_DEBUG 0, r, "appending parsed data");
    apr_brigade_puts(bb, NULL, NULL, "\n--APREQ OUTPUT FILTER--\nARGS:\n");
    apr_table_do(dump_table, &ctx, req->args, NULL);

    if (req->body) {
        apr_brigade_puts(bb,NULL,NULL,"BODY:\n");
        apr_table_do(dump_table,&ctx,req->body,NULL);
    }
    APR_BRIGADE_CONCAT(bb,eos);
    return ap_pass_brigade(f->next,bb);
}

static void register_hooks (apr_pool_t *p)
{
    ap_register_output_filter(filter_name, apreq_output_filter_test, 
                              apreq_output_filter_test_init,
                              AP_FTYPE_CONTENT_SET);
}

module AP_MODULE_DECLARE_DATA apreq_output_filter_test_module =
{
	STANDARD20_MODULE_STUFF,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	register_hooks,			/* callback for registering hooks */
};
