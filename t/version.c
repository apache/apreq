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

#include "apreq_version.h"
#include "test_apreq.h"
#include "apreq.h"

apr_version_t v;
const char *vstring;

static void version_string(CuTest *tc)
{
    vstring = apreq_version_string();
    CuAssertPtrNotNull(tc, vstring);
    CuAssertStrEquals(tc, APREQ_VERSION_STRING, vstring);
}
static void version_number(CuTest *tc)
{
    apreq_version(&v);
    CuAssertIntEquals(tc, APREQ_MAJOR_VERSION, v.major);
    CuAssertIntEquals(tc, APREQ_MINOR_VERSION, v.minor);
    CuAssertIntEquals(tc, APREQ_PATCH_VERSION, v.patch);
#ifdef APREQ_IS_DEV_VERSION
    CuAssertIntEquals(tc, 1, v.is_dev);
#endif

}

CuSuite *testversion(void)
{
    CuSuite *suite = CuSuiteNew("Version");
    SUITE_ADD_TEST(suite, version_string);
    SUITE_ADD_TEST(suite, version_number);
    return suite;
}

