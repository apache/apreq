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
#include "test_apreq.h"

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
    {"version", testversion},
    {"cookies", testcookie},
    {"params", testparam},
    {"parsers", testparser},
    /*{"performance", testperformance},*/
    {"LastTest", NULL}
};


/* rigged environent for unit tests */

#define APREQ_MODULE_NAME "TEST"
#define APREQ_MODULE_MAGIC_NUMBER 20040324

#define CRLF "\015\012"

apr_table_t *table;

int main(int argc, char *argv[])
{
    CuSuiteList *alltests = NULL;
    CuString *output = CuStringNew();
    int i;
    int partial = 0;

    apr_initialize();
    atexit(apr_terminate);

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

