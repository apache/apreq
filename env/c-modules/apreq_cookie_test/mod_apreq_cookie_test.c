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

<Location /apreq_cookie_test>
   SetHandler apreq_cookie_test
</Location>

#endif

#define APACHE_HTTPD_TEST_HANDLER apreq_cookie_test_handler

#include "apache_httpd_test.h"
#include "apreq_params.h"
#include "apreq_env.h"
#include "httpd.h"

static int apreq_cookie_test_handler(request_rec *r)
{
    apreq_request_t *req;
    apr_status_t s;
    const apreq_jar_t *jar;
    const apreq_param_t *test, *key;
    apreq_cookie_t *cookie;
    apr_ssize_t ssize;
    apr_size_t size;
    char *dest;

    if (strcmp(r->handler, "apreq_cookie_test") != 0)
        return DECLINED;

    apreq_log(APREQ_DEBUG 0, r, "initializing request");
    req = apreq_request(r, NULL);
    test = apreq_param(req, "test");
    key = apreq_param(req, "key");

    apreq_log(APREQ_DEBUG 0, r, "initializing cookie");
    jar = apreq_jar(r, NULL);
    cookie = apreq_cookie(jar, key->v.data);    
    ap_set_content_type(r, "text/plain");

    if (strcmp(test->v.data, "bake") == 0) {
        s = apreq_cookie_bake(cookie, r);
    }
    else if (strcmp(test->v.data, "bake2") == 0) {
        s = apreq_cookie_bake2(cookie, r);
    }
    else {
        size = strlen(cookie->v.data);
        dest = apr_palloc(r->pool, size + 1);
        ssize = apreq_decode(dest, cookie->v.data, size);
        ap_rprintf(r, "%s", dest);
    }
    return OK;
}

APACHE_HTTPD_TEST_MODULE(apreq_cookie_test);
