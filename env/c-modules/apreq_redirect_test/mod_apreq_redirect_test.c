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

<Location /apreq_redirect_test>
   TestAccess test
   SetHandler apreq_redirect_test
</Location>

#endif
#endif

#define APACHE_HTTPD_TEST_HANDLER apreq_redirect_test_handler

#include "apache_httpd_test.h"
#include "apreq_params.h"
#include "apreq_env.h"
#include "apreq_env_apache2.h"
#include "httpd.h"

static int apreq_redirect_test_handler(request_rec *r)
{
    apreq_env_handle_t *env;
    apreq_request_t *req;
    const apreq_param_t *loc;

    if (strcmp(r->handler, "apreq_redirect_test") != 0)
        return DECLINED;

    env = apreq_env_make_apache2(r);

    req = apreq_request(env, NULL);
    apreq_log(APREQ_DEBUG 0, env, "looking for new location");
    loc = apreq_param(req, "location");
    if (!loc)
        return DECLINED;
    apreq_log(APREQ_DEBUG 0, env, "redirecting to %s", loc->v.data);
    ap_internal_redirect(loc->v.data, r);
    return OK;
}

APACHE_HTTPD_TEST_MODULE(apreq_redirect_test);