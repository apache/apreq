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

#include "apreq.h"
#include "apreq_env.h"
#include "apr_strings.h"
#include "apr_lib.h"
#include "apr_tables.h"
#include <stdlib.h>

static int dump_table(void *count, const char *key, const char *value)
{
    int *c = (int *) count;
    int value_len = apreq_value_to_param(value)->v.size;
    if (value_len) 
        *c += strlen(key) + value_len;
    return 1;
}

int main(int argc, char const * const * argv)
{
    apr_pool_t *pool;
    apreq_env_handle_t *req;
    const apreq_param_t *foo, *bar, *test, *key;
    apr_file_t *out;

    atexit(apr_terminate);
    if (apr_app_initialize(&argc, &argv, NULL) != APR_SUCCESS) {
        fprintf(stderr, "apr_app_initialize failed\n");
        exit(-1);
    }

    if (apr_pool_create(&pool, NULL) != APR_SUCCESS) {
        fprintf(stderr, "apr_pool_create failed\n");
        exit(-1);
    }

    if (apreq_initialize(pool) != APR_SUCCESS) {
        fprintf(stderr, "apreq_initialize failed\n");
        exit(-1);
    }

    req = apreq_handle_cgi(pool);

    apr_file_open_stdout(&out, pool);

//    apreq_log(APREQ_DEBUG 0, env, "%s", "Creating apreq_request");

//    apreq_log(APREQ_DEBUG 0, env, "%s", "Fetching the parameters");

    foo = apreq_param(req, "foo");
    bar = apreq_param(req, "bar");

    test = apreq_param(req, "test");
    key  = apreq_param(req, "key");

    if (foo || bar) {
        apr_file_printf(out, "%s", "Content-Type: text/plain\n\n");

        if (foo) {
            apr_file_printf(out, "\t%s => %s\n", "foo", foo->v.data);
//            apreq_log(APREQ_DEBUG 0, env, "%s => %s", "foo", foo->v.data);
        }
        if (bar) {
            apr_file_printf(out, "\t%s => %s\n", "bar", bar->v.data);
//            apreq_log(APREQ_DEBUG 0, env, "%s => %s", "bar", bar->v.data);
        }
    }
    
    else if (test && key) {
        apreq_cookie_t *cookie;
        char *dest;
        apr_size_t dlen;

//        apreq_log(APREQ_DEBUG 0, env, "Fetching Cookie %s", key->v.data);
        cookie = apreq_cookie(req, key->v.data);

        if (cookie == NULL) {
//            apreq_log(APREQ_DEBUG APR_EGENERAL, env,
//                      "No cookie for %s found!", key->v.data);
            exit(-1);
        }

        if (strcmp(test->v.data, "bake") == 0) {
            apreq_cookie_bake(cookie, req);
        }
        else if (strcmp(test->v.data, "bake2") == 0) {
            apreq_cookie_bake2(cookie, req);
        }

        apr_file_printf(out, "%s", "Content-Type: text/plain\n\n");
        dest = apr_pcalloc(pool, cookie->v.size + 1);
        if (apreq_decode(dest, &dlen, cookie->v.data, cookie->v.size) == APR_SUCCESS)
            apr_file_printf(out, "%s", dest);
        else {
//            apreq_log(APREQ_ERROR APR_EGENERAL, env,
//                      "Bad cookie encoding: %s", 
//                      cookie->v.data);
            exit(-1);
        }
    }

    else { 
        const apr_table_t *params = apreq_params(pool, req);
        int count = 0;
        apr_file_printf(out, "%s", "Content-Type: text/plain\n\n");

//        apreq_log(APREQ_DEBUG 0, env, "Fetching all parameters");

        if (params == NULL) {
//            apreq_log(APREQ_ERROR APR_EGENERAL, env, "No parameters found!");
            exit(-1);
        }
        apr_table_do(dump_table, &count, params, NULL);
        apr_file_printf(out, "%d", count);
    }

    return 0;
}
