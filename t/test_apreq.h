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

#ifndef APREQ_TEST_INCLUDES
#define APREQ_TEST_INCLUDES

#include "CuTest.h"
#include "apr_pools.h"
#ifndef apr_table_nelts
#define apr_table_nelts(t) apr_table_elts(t)->nelts
#endif

/* Some simple functions to make the test apps easier to write and
 * a bit more consistent...
 */

extern apr_pool_t *p;

CuSuite *getsuite(void);
CuSuite *testversion(void);
CuSuite *testtable(void);
CuSuite *testcookie(void);
CuSuite *testenv(void);
CuSuite *testparam(void);
CuSuite *testparser(void);
CuSuite *testperformance(void);
/* Assert that RV is an APR_SUCCESS value; else fail giving strerror
 * for RV and CONTEXT message. */
void apr_assert_success(CuTest* tc, const char *context, apr_status_t rv);


#endif /* APR_TEST_INCLUDES */
