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

#include "apreq.h"
#include "apreq_env.h"
#include "apreq_params.h"
#include "apreq_parsers.h"
#include "apreq_cookie.h"
#include "apr_strings.h"
#include "apr_lib.h"
#include "apr_tables.h"

static int dump_table(void *count, const char *key, const char *value)
{
    int *c = (int *) count;
    int value_len = apreq_strlen(value);
    if (value_len) 
        *c += strlen(key) + value_len;
    return 1;
}

int main(int argc, char const * const * argv)
{
    apr_pool_t *pool;
    apreq_request_t *req;
    const apreq_param_t *foo, *bar, *test, *key;

    atexit(apr_terminate);
    if (apr_app_initialize(&argc, &argv, NULL) != APR_SUCCESS) {
        fprintf(stderr, "apr_app_initialize failed\n");
        exit(-1);
    }

    if (apr_pool_create(&pool, NULL) != APR_SUCCESS) {
        fprintf(stderr, "apr_pool_create failed\n");
        exit(-1);
    }

    apreq_log(APREQ_DEBUG 0, pool, "%s", "Creating apreq_request");
    req = apreq_request(pool, NULL);

    printf("%s", "Content-Type: text/plain\n\n");
    apreq_log(APREQ_DEBUG 0, pool, "%s", "Fetching the parameters");

    foo = apreq_param(req, "foo");
    bar = apreq_param(req, "bar");

    test = apreq_param(req, "test");
    key  = apreq_param(req, "key");

    if (foo || bar) {
        if (foo) {
            printf("\t%s => %s\n", "foo", foo->v.data);
            apreq_log(APREQ_DEBUG 0, pool, "%s => %s", "foo", foo->v.data);
        }
        if (bar) {
            printf("\t%s => %s\n", "bar", bar->v.data);
            apreq_log(APREQ_DEBUG 0, pool, "%s => %s", "bar", bar->v.data);
        }
    }
    
    else if (test && key) {
        const apreq_jar_t *jar = apreq_jar(pool, NULL);
        apreq_cookie_t *cookie;

        apreq_log(APREQ_DEBUG 0, pool, "Fetching Cookie %s", key->v.data);
        cookie = apreq_cookie(jar, key->v.data);

        if (cookie == NULL) {
            apreq_log(APREQ_DEBUG 0, pool, 
                      "No cookie for %s found!", key->v.data);
            exit(-1);
        }

        if (strcmp(test->v.data, "bake") == 0) {
            apreq_cookie_bake(cookie, pool);
        }
        else if (strcmp(test->v.data, "bake2") == 0) {
            apreq_cookie_bake2(cookie, pool);
        }
        else {
            char *dest = apr_pcalloc(pool, cookie->v.size + 1);
            if (apreq_decode(dest, cookie->v.data, cookie->v.size) >= 0)
                printf("%s", dest);
            else
                apreq_log(APREQ_DEBUG 0, pool, "Bad cookie encoding: %s", 
                          cookie->v.data);
        }
    }

    else { 
        apr_table_t *params = apreq_params(pool, req);
        int count = 0;

        apreq_log(APREQ_DEBUG 0, pool, "Fetching all parameters");

        if (params == NULL) {
            apreq_log(APREQ_DEBUG 0, pool, "No parameters found!");
            exit(-1);
        }
        apr_table_do(dump_table, &count, params, NULL);
        printf("%d", count);
    }
    
    return 0;
}
