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

#include "apreq_env.h"
#include "test_apreq.h"
#include "apreq.h"
#include "apreq_tables.h"
#if APR_HAVE_STDIO_H
#include <stdio.h>
#endif
#if APR_HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if APR_HAVE_STRING_H
#include <string.h>
#endif

static apreq_table_t *t1 = NULL;


#define V(k,v) apreq_make_value(p,k,strlen(k),v,strlen(v))

static void table_make(CuTest *tc)
{
    t1 = apreq_make_table(p, 5);
    CuAssertPtrNotNull(tc, t1);
}

static void table_get(CuTest *tc)
{
    const char *val;
    apreq_table_set(t1, V(APREQ_URL_ENCTYPE,"foo"));
    apreq_table_set(t1, V(APREQ_MFD_ENCTYPE,"bar"));

    val = apreq_table_get(t1,APREQ_URL_ENCTYPE);
    CuAssertStrEquals(tc,"foo",val);
    val = apreq_table_get(t1,APREQ_MFD_ENCTYPE);
    CuAssertStrEquals(tc,"bar",val);

}

static void table_set(CuTest *tc)
{
    const char *val;

    apreq_table_set(t1, V("setkey","bar"));
    apreq_table_set(t1, V("setkey","2ndtry"));
    val = apreq_table_get(t1,"setkey");
    CuAssertStrEquals(tc,"2ndtry",val);

}

static void table_getnotthere(CuTest *tc)
{
    const char *val;

    val = apreq_table_get(t1, "keynotthere");
    CuAssertPtrEquals(tc, NULL,val);
}

static void table_add(CuTest *tc)
{
    const char *val;

    apreq_table_addv(t1, V("add", "bar"));
    apreq_table_addv(t1, V("add", "foo"));
    val = apreq_table_get(t1, "add");
    CuAssertStrEquals(tc, "bar", val);


    apreq_table_addv(t1, V("f", "top"));
    apreq_table_addv(t1, V("fo", "child"));
    apreq_table_addv(t1, V("fro", "child-right"));

    /*
     *     f(5)  black                fo(6) black   
     *      fo(6)  red       =>  f(5)red   fro(7)red
     *       fro(7) red
     *
     */


    apreq_table_addv(t1, V("flo", "child-left"));
    apreq_table_addv(t1, V("flr", "child-left-right"));
    apreq_table_addv(t1, V("frr", "child-right-right"));
    apreq_table_addv(t1, V("frl", "child-right-left"));
    apreq_table_addv(t1, V("flr", "child-left-right"));
    apreq_table_addv(t1, V("fll", "child-left-left"));
    apreq_table_addv(t1, V("foo", "bar"));
    apreq_table_addv(t1, V("b", "bah humbug"));
    val = apreq_table_get(t1, "foo");
    CuAssertStrEquals(tc, "bar", val);
    val = apreq_table_get(t1, "flr");
    CuAssertStrEquals(tc, "child-left-right", val);
}

static void table_unset(CuTest *tc) {
    const char *val;
    apreq_table_unset(t1, "flo");
    val = apreq_table_get(t1, "flo");
    CuAssertPtrEquals(tc, NULL, val);

    val = apreq_table_get(t1, "fro");
    CuAssertStrEquals(tc, "child-right", val);
    val = apreq_table_get(t1, "fll");
    CuAssertStrEquals(tc, "child-left-left", val);
    val = apreq_table_get(t1, "frl");
    CuAssertStrEquals(tc, "child-right-left", val);

}

static void table_nelts(CuTest *tc)
{
    const char *val;
    apreq_table_t *t = apreq_table_make(p, 1);


    apreq_table_set(t, V("abc", "def"));
    apreq_table_set(t, V("def", "abc"));
    apreq_table_set(t, V("foo", "zzz"));
    val = apreq_table_get(t, "foo");
    CuAssertStrEquals(tc, val, "zzz");

    val = apreq_table_get(t, "abc");
    CuAssertStrEquals(tc, val, "def");
    val = apreq_table_get(t, "def");
    CuAssertStrEquals(tc, val, "abc");
    CuAssertIntEquals(tc, 3, apreq_table_nelts(t));
}

static void table_clear(CuTest *tc)
{
    apreq_table_clear(t1);
    CuAssertIntEquals(tc, 0, apreq_table_nelts(t1));
}

static void table_overlap(CuTest *tc)
{
    const char *val;
    apreq_table_t *t2 = apreq_table_make(p, 1);
    apr_status_t s;

    t1 = apreq_table_make(p, 1);

    apreq_table_addv(t1, V("a", "0"));
    apreq_table_addv(t1, V("g", "7"));

    apreq_table_addv(t2, V("a", "1"));
    apreq_table_addv(t2, V("b", "2"));
    apreq_table_addv(t2, V("c", "3"));
    apreq_table_addv(t2, V("b", "2.0"));
    apreq_table_addv(t2, V("d", "4"));
    apreq_table_addv(t2, V("e", "5"));
    apreq_table_addv(t2, V("b", "2."));
    apreq_table_addv(t2, V("f", "6"));

    /* APR_OVERLAP_TABLES_SET had funky semantics, so we ignore it here */
    s = apreq_table_overlap(t1, t2, APR_OVERLAP_TABLES_MERGE);
    CuAssertIntEquals(tc, APR_SUCCESS, s);

    t2 = apreq_table_copy(p,t1);
    CuAssertIntEquals(tc, 7, apreq_table_nelts(t2));
    CuAssertIntEquals(tc, 3, apreq_table_ghosts(t2));
    CuAssertIntEquals(tc, 9, apreq_table_exorcise(t2));
    CuAssertIntEquals(tc, 0, apreq_table_ghosts(t2));

    val = apreq_table_get(t2, "a");
    CuAssertStrEquals(tc, "0, 1",val);
    val = apreq_table_get(t2, "b");
    CuAssertStrEquals(tc, "2, 2.0, 2.",val);
    val = apreq_table_get(t2, "c");
    CuAssertStrEquals(tc, "3",val);
    val = apreq_table_get(t2, "d");
    CuAssertStrEquals(tc, "4",val);
    val = apreq_table_get(t2, "e");
    CuAssertStrEquals(tc, "5",val);
    val = apreq_table_get(t2, "f");
    CuAssertStrEquals(tc, "6",val);
    val = apreq_table_get(t2, "g");
    CuAssertStrEquals(tc, "7",val);

}

#define APREQ_ELT(a,i) (*(apreq_value_t **)((a)->elts + (i) * (a)->elt_size))

static void table_elts(CuTest *tc)
{
    const char *val;
    const apreq_value_t *v;
    const apr_array_header_t *a;

    CuAssertIntEquals(tc, 7, apreq_table_nelts(t1));
    CuAssertIntEquals(tc, 3, apreq_table_ghosts(t1));

    a = apreq_table_elts(t1);

    CuAssertIntEquals(tc, 7, apreq_table_nelts(t1));
    CuAssertIntEquals(tc, 0, apreq_table_ghosts(t1));

    v = APREQ_ELT(a,0);
    CuAssertStrEquals(tc,"0, 1", v->data);
    v = APREQ_ELT(a,1);
    CuAssertStrEquals(tc,"7", v->data);
    v = APREQ_ELT(a,2);
    CuAssertStrEquals(tc,"2, 2.0, 2.", v->data);
    v = APREQ_ELT(a,3);
    CuAssertStrEquals(tc,"3", v->data);

    v = APREQ_ELT(a,a->nelts-1);
    CuAssertStrEquals(tc,"6", v->data);
    v = APREQ_ELT(a,a->nelts-2);
    CuAssertStrEquals(tc,"5", v->data);
    v = APREQ_ELT(a,a->nelts-3);
    CuAssertStrEquals(tc,"4", v->data);

}

static void table_overlay(CuTest *tc)
{
    const char *val;
    apreq_table_t *t2 = apreq_table_make(p, 1);
    apr_status_t s;

    t1 = apreq_table_make(p, 1);

    apreq_table_addv(t1, V("a", "0"));
    apreq_table_addv(t1, V("g", "7"));

    apreq_table_addv(t2, V("a", "1"));
    apreq_table_addv(t2, V("b", "2"));
    apreq_table_addv(t2, V("c", "3"));
    apreq_table_addv(t2, V("b", "2.0"));
    apreq_table_addv(t2, V("d", "4"));
    apreq_table_addv(t2, V("e", "5"));
    apreq_table_addv(t2, V("b", "2."));
    apreq_table_addv(t2, V("f", "6"));
    t1 = apreq_table_overlay(p, t1, t2);

    CuAssertIntEquals(tc, 10, apreq_table_nelts(t1));

    val = apreq_table_get(t1, "a");
    CuAssertStrEquals(tc, "0",val);
    val = apreq_table_get(t1, "b");
    CuAssertStrEquals(tc, "2",val);
    val = apreq_table_get(t1, "c");
    CuAssertStrEquals(tc, "3",val);
    val = apreq_table_get(t1, "d");
    CuAssertStrEquals(tc, "4",val);
    val = apreq_table_get(t1, "e");
    CuAssertStrEquals(tc, "5",val);
    val = apreq_table_get(t1, "f");
    CuAssertStrEquals(tc, "6",val);
    val = apreq_table_get(t1, "g");
    CuAssertStrEquals(tc, "7",val);
}

static void table_keys(CuTest *tc)
{
    const char *k;
    apr_array_header_t *a = apr_array_make(p,1, sizeof k);

    apr_status_t s = apreq_table_keys(t1,a);
    CuAssertIntEquals(tc, APR_SUCCESS, s);
    CuAssertIntEquals(tc, 7, a->nelts);
    k = ((const char **)a->elts)[0];
    CuAssertStrEquals(tc, "a", k);
    k = ((const char **)a->elts)[1];
    CuAssertStrEquals(tc, "g", k);
    k = ((const char **)a->elts)[2];
    CuAssertStrEquals(tc, "b", k);
    k = ((const char **)a->elts)[3];
    CuAssertStrEquals(tc, "c", k);
    k = ((const char **)a->elts)[4];
    CuAssertStrEquals(tc, "d", k);
    k = ((const char **)a->elts)[5];
    CuAssertStrEquals(tc, "e", k);
    k = ((const char **)a->elts)[6];
    CuAssertStrEquals(tc, "f", k);
}

static int push_value(void *d, const char *key, const char *val)
{
    apr_array_header_t *a = d;
    *(const char **)apr_array_push(a) = val;
    return 1;
}

static void table_values(CuTest *tc)
{
    const char *v;
    apr_array_header_t *a = apr_array_make(p,1,sizeof v);
    apreq_table_do(push_value, a, t1, "a", NULL);

    CuAssertIntEquals(tc, 2, a->nelts);
    v = ((const char **)a->elts)[0];
    CuAssertStrEquals(tc, "0", v);
    v = ((const char **)a->elts)[1];
    CuAssertStrEquals(tc, "1", v);

    a->nelts = 0;
    apreq_table_do(push_value, a, t1, "b", NULL);

    CuAssertIntEquals(tc, 3, a->nelts);
    v = ((const char **)a->elts)[0];
    CuAssertStrEquals(tc, "2", v);
    v = ((const char **)a->elts)[1];
    CuAssertStrEquals(tc, "2.0", v);
    v = ((const char **)a->elts)[2];
    CuAssertStrEquals(tc, "2.", v);
}


CuSuite *testtable(void)
{
    CuSuite *suite = CuSuiteNew("Table");

    SUITE_ADD_TEST(suite, table_make);
    SUITE_ADD_TEST(suite, table_get); 
    SUITE_ADD_TEST(suite, table_set);
    SUITE_ADD_TEST(suite, table_getnotthere);
    SUITE_ADD_TEST(suite, table_add);
    SUITE_ADD_TEST(suite, table_unset);
    SUITE_ADD_TEST(suite, table_nelts);
    SUITE_ADD_TEST(suite, table_clear);
    SUITE_ADD_TEST(suite, table_overlap);
    SUITE_ADD_TEST(suite, table_elts);
    SUITE_ADD_TEST(suite, table_overlay);
    SUITE_ADD_TEST(suite, table_keys);
    SUITE_ADD_TEST(suite, table_values);

    return suite;
}

