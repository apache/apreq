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

#include <stdio.h>
#include <stdlib.h>

#include "apreq_env.h"
#include "apr_strings.h"

apr_pool_t *p;

/* rigged environent for unit tests */

#define APREQ_MODULE_NAME "TEST"
#define APREQ_MODULE_MAGIC_NUMBER 20040324

#define CRLF "\015\012"

apr_bucket_brigade *bb;
apr_table_t *table;

static apr_pool_t *test_pool(void *env)
{
    return p;
}

static apr_status_t bucket_alloc_cleanup(void *data)
{
    apr_bucket_alloc_t *ba = data;
    apr_bucket_alloc_destroy(ba);
    return APR_SUCCESS;
}

static apr_bucket_alloc_t *test_bucket_alloc(void *env)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_pool_cleanup_register(p, ba, bucket_alloc_cleanup, NULL);
    return ba;
}

static const char *test_header_in(void *env, const char *name)
{
    return env;
}

static apr_status_t test_header_out(void *env, 
                                    const char *name, 
                                    char *value)
{    
    return printf("%s: %s" CRLF, name, value) > 0 ? APR_SUCCESS : APR_EGENERAL;
}

static const char *test_query_string(void *env)
{
    return env;
}

static apreq_jar_t *test_jar(void *env, apreq_jar_t *jar)
{
    return NULL;
}

static apreq_request_t *test_request(void *env, apreq_request_t *req)
{
    return NULL;
}

static void test_log(const char *file, int line, int level, 
                     apr_status_t status, void *env, const char *fmt,
                     va_list vp)
{
    char buf[256];
    if (level < APREQ_LOG_ERR)
        fprintf(stderr, "[%s(%d)]%s (%s)\n", file, line, apr_strerror(status,buf,255), apr_pvsprintf(p,fmt,vp));
}

static apr_status_t test_read(void *env, apr_read_type_e block, 
                              apr_off_t bytes)
{
    return APR_ENOTIMPL;
}

static const char *test_temp_dir(void *env, const char *path)
{
    static const char *temp_dir;
    if (path != NULL) {
        const char *rv = temp_dir;
        temp_dir = apr_pstrdup(p, path);
        return rv;
    }
    if (temp_dir == NULL) {
        if (apr_temp_dir_get(&temp_dir, p) != APR_SUCCESS)
            temp_dir = NULL;
    }

    return temp_dir;
}


static apr_off_t test_max_body(void *env, apr_off_t bytes)
{
    static apr_off_t max_body = -1;
    if (bytes >= 0) {
        apr_off_t rv = max_body;
        max_body = bytes;
        return rv;
    }
    return max_body;
}


static apr_ssize_t test_max_brigade(void *env, apr_ssize_t bytes)
{
    static apr_ssize_t max_brigade = -1;

    if (bytes >= 0) {
        apr_ssize_t rv = max_brigade;
        max_brigade = bytes;
        return rv;
    }
    return max_brigade;
}



APREQ_ENV_MODULE(test, APREQ_MODULE_NAME,
                 APREQ_MODULE_MAGIC_NUMBER);

