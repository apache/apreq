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

#include "test_apreq.h"
#include "apreq_env.h"
#include "apr_strings.h"

/* Top-level pool which can be used by tests. */
apr_pool_t *p;

void apr_assert_success(CuTest* tc, const char* context, apr_status_t rv)
{
    if (rv == APR_ENOTIMPL) {
        CuNotImpl(tc, context);
    }

    if (rv != APR_SUCCESS) {
        char buf[STRING_MAX], ebuf[128];
        sprintf(buf, "%s (%d): %s\n", context, rv,
                apr_strerror(rv, ebuf, sizeof ebuf));
        CuFail(tc, buf);
    }
}

static const struct testlist {
    const char *testname;
    CuSuite *(*func)(void);
} tests[] = {
//    {"tables", testtable},
    {"cookies", testcookie},
    {"params", testparam},
    {"parsers", testparser},
//    {"performance", testperformance},
    {"LastTest", NULL}
};


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
    if (level < APREQ_LOG_ERR)
        fprintf(stderr, "[%s(%d)] %s\n", file, line, apr_pvsprintf(p,fmt,vp));
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



static APREQ_ENV_MODULE(test, APREQ_MODULE_NAME,
                       APREQ_MODULE_MAGIC_NUMBER);


int main(int argc, char *argv[])
{
    CuSuiteList *alltests = NULL;
    CuString *output = CuStringNew();
    int i;
    int partial = 0;

    apr_initialize();
    atexit(apr_terminate);
    apreq_env_module(&test_module);

    CuInit(argc, argv);

    apr_pool_create(&p, NULL);

    /* build the list of tests to run */
    for (i = 1; i < argc; i++) {
        int j;
        if (!strcmp(argv[i], "-v")) {
            continue;
        }
        for (j = 0; tests[j].func != NULL; j++) {
            if (!strcmp(argv[i], tests[j].testname)) {
                if (!partial) {
                    alltests = CuSuiteListNew("Partial APREQ Tests");
                    partial = 1;
                }

                CuSuiteListAdd(alltests, tests[j].func());
                break;
            }
        }
    }

    if (!partial) {
        alltests = CuSuiteListNew("All APREQ Tests");
        for (i = 0; tests[i].func != NULL; i++) {
            CuSuiteListAdd(alltests, tests[i].func());
        }
    }

    CuSuiteListRunWithSummary(alltests);
    i = CuSuiteListDetails(alltests, output);
    printf("%s\n", output->buffer);

    return i > 0 ? 1 : 0;
}

