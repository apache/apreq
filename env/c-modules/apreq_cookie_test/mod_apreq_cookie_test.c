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

#ifdef CONFIG_FOR_HTTPD_TEST
#if CONFIG_FOR_HTTPD_TEST

<Location /apreq_cookie_test>
   SetHandler apreq_cookie_test
</Location>

#endif
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
