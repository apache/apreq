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

#include "apr_time.h"
#include "apr_strings.h"
#include "apreq_env.h"
#include "apreq.h"
#include "apreq_tables.h"
#include "apr_hash.h"
#include "test_apreq.h"
#if APR_HAVE_STDIO_H
#include <stdio.h>
#endif
#if APR_HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if APR_HAVE_STRING_H
#include <string.h>
#endif

static apreq_table_t *t = NULL;
static apreq_table_t *s = NULL;
static apreq_table_t *h = NULL;

#define NELTS 16
#define LOOP 10000
#define FAIL(MSG) \
  CuFail(tc, apr_psprintf(p, MSG ": APR = %.3f vs APREQ = %.3f (microseconds)", \
                         (double)apr_delta / LOOP, (double)apreq_delta / LOOP))
#define TEST_DELTAS(winner,loser,MSG)     if (winner##_delta > loser##_delta) FAIL(MSG)

#define RUN_GET(k,v) do {apr_time_t apr_delta,apreq_delta;const char *val; \
   int i; apreq_delta = apr_time_now(); for(i=0;i<LOOP;++i) val = \
   apreq_table_get(t,k); apreq_delta = apr_time_now() - apreq_delta; \
   if (v) { CuAssertPtrNotNull(tc,val);CuAssertStrEquals(tc,v,val);} \
   else CuAssertPtrEquals(tc,NULL,val); \
   apr_delta = apr_time_now(); for(i=0;i<LOOP;++i) val = apr_table_get(s,k); \
   apr_delta = apr_time_now() - apr_delta; \
   if (v) {CuAssertPtrNotNull(tc,val); CuAssertStrEquals(tc,v,val);} \
   else CuAssertPtrEquals(tc,NULL,val);\
   TEST_DELTAS(apreq,apr,"apr_table_get(" k ") wins"); apr_delta = apr_time_now(); \
/*   for(i=0;i<LOOP;++i) val = apr_hash_get(h,k,APR_HASH_KEY_STRING);\
   apr_delta = apr_time_now() - apr_delta; \
   if (v) {CuAssertPtrNotNull(tc,val);CuAssertStrEquals(tc,v,val);} \
   else CuAssertPtrEquals(tc,NULL,val); \
   TEST_DELTAS(apreq,apr,"APR hash_get(" k ") wins"); \
*/} while (0)



#define V(k,v) apreq_make_value(p,k,strlen(k),v,strlen(v))
#define VV(k)  { #k, 0, 1, 0 }

static char *headers[12] = {"Host", "User-Agent", "Accept",
                            "Accept-Language", "Accept-Encoding",
                            "Accept-Charset", "Keep-Alive",
                            "Connection", "Referer", "Cookie",
                            "Content-Type", "Content-Length"};

static apreq_table_t *init_apreq(int N)
{
    apreq_table_t *t = apreq_table_make(p,N);
    int j;
    static apreq_value_t v[] = { VV(Host), VV(User-Agent), VV(Accept),
                                 VV(Accept-Language), VV(Accept-Encoding),
                                 VV(Accept-Charset), VV(Keep-Alive),
                                 VV(Connection), VV(Referer), VV(Cookie),
                                 VV(Content-Type), VV(Content-Length) };
    for (j = 0; j < 12; ++j)
        apreq_table_addv(t,v + j);

    return t;
}

static apr_table_t *init_apr(int N)
{
    apr_table_t *t = apr_table_make(p,N);
    int j;

    for (j=0; j < 12; ++j)
        apr_table_addn(t, headers[j], "");

    return t;
}


static apr_hash_t *init_hash(void)
{
    apr_hash_t *h = apr_hash_make(p);
    int j;

    for (j = 0; j < 12; ++j) {
        const char *k = headers[j];
        int klen = strlen(k);
        const char *v = apr_hash_get(h, k, klen);
        if (v == NULL)
            apr_hash_set(h, k, klen, "");
        else 
            apr_hash_set(h, k, klen, apr_pstrcat(p,v,", ", k, NULL));
    }
    return h;
}


/* measures httpd server's request header initialization time */
static void perf_init(CuTest *tc)
{
    apr_time_t apreq_delta,apr_delta;
    int i;

    apr_delta = apr_time_now();
    for (i=0; i<LOOP;++i) {
       apr_table_t *s = init_apr(NELTS);
/*       apr_table_t *t = apr_table_make(p,NELTS);
//       apr_table_overlap(t,s,APR_OVERLAP_TABLES_MERGE);
*/      apr_table_compress(s);
    }
    apr_delta = apr_time_now() - apr_delta;

    apreq_delta = apr_time_now();
    for (i=0; i<LOOP;++i) {
        apreq_table_t *t = init_apreq(NELTS);
        apreq_table_normalize(t);
    }
    apreq_delta = apr_time_now() - apreq_delta;


    TEST_DELTAS(apreq,apr,"apreq should win");

    apr_delta = apr_time_now();
    for (i=0; i<LOOP;++i) {
        apr_hash_t *h = init_hash();
    }
    apr_delta = apr_time_now() - apr_delta;

    TEST_DELTAS(apr,apreq,"apr_hash should win");
}


#define GET_AVG(N) \
    apr_delta = apr_time_now(); \
    for (j = 0; j < LOOP; ++j) { \
        const char *val = apr_table_get(s, headers[j % N]); \
        CuAssertPtrNotNull(tc, val); \
    } \
    apr_delta = apr_time_now() - apr_delta; \
    apreq_delta = apr_time_now(); \
    for (j = 0; j < LOOP; ++j) { \
        const char *val = apreq_table_get(t, headers[j % N]); \
        CuAssertPtrNotNull(tc, val); \
    } \
    apreq_delta = apr_time_now() - apreq_delta;

static void perf_get_avg(CuTest *tc)
{
    apr_table_t *s = init_apr(NELTS);
    apreq_table_t *t = init_apreq(NELTS);
    apr_time_t apr_delta, apreq_delta;
    int j;

    GET_AVG (2); TEST_DELTAS(apr,apreq,"apr_tables should win (2)");
    GET_AVG (4); TEST_DELTAS(apr,apreq,"apr_tables should win (4)");
    GET_AVG (6); TEST_DELTAS(apreq,apr,"apreq should win (6)");
    GET_AVG (8); TEST_DELTAS(apreq,apr,"apreq should win (8)");
    GET_AVG(10); TEST_DELTAS(apreq,apr,"apreq should win (10)");
    GET_AVG(12); TEST_DELTAS(apreq,apr,"apreq should win (12)");
}


/*
 *                           nontrivial trees
 *                          ------------------ 
 * 
 *     (4) Accept-Encoding                     (7) Connection
 *         /        \                              /        \
 * (2) Accept  Accept-Language (3)          (9) Cookie   Content-Type (10)
 *         \                                                /
 *    (5) Accept-Charset                             Content-Length (11)
 *
 *    (checksum doesn't help here)           (checksum is a big win except
 *                                            for the Content-Length path.)
 *
 *
 *        apr_hash:      (0.5 ~ 0.6 microseconds / lookup)
 *      apreq_table:     (0.3 ~ 0.4 microseconds / entry)
 *        apr_table:     (0.2 ~ 0.3 microseconds / entry)
 */


static void perf_get(CuTest *tc)
{
    apr_hash_t *h = init_hash();
    apr_table_t *s = init_apr(NELTS);
    apreq_table_t *t = init_apreq(NELTS);
    apr_time_t apr_delta, apreq_delta;
    int j;

    /* expected apr winners: "Accept", "Connection", "Cookie" */

    RUN_GET("Accept-Encoding","");
    RUN_GET("Accept-Language","");
    RUN_GET("Accept-Charset","");
    RUN_GET("Content-Length","");
    RUN_GET("Content-Type","");  /* apr typically wins by ~.05 microseconds,
                                    or ~ 25 clock cycles on my 500Mhz CPU */
}



#ifdef footoo

static void table_set(CuTest *tc)
{
    const char *val;
    apreq_table_t *t = init_apreq(APREQ_NELTS);
    apr_table_t *s = init_apr(APREQ_NELTS);
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

    apreq_table_add(t1, V("add", "bar"));
    apreq_table_add(t1, V("add", "foo"));
    val = apreq_table_get(t1, "add");
    CuAssertStrEquals(tc, "bar", val);


    apreq_table_add(t1, V("f", "top"));
    apreq_table_add(t1, V("fo", "child"));
    apreq_table_add(t1, V("fro", "child-right"));
    apreq_table_add(t1, V("foo", "bar"));

    /*
     *     f(5)  black                fo(6) black   
     *      fo(6)  red       =>  f(5)red   fro(7)red
     *       fro(7) red
     *
     */


    apreq_table_add(t1, V("flo", "child-left"));
    apreq_table_add(t1, V("flr", "child-left-right"));
    apreq_table_add(t1, V("frr", "child-right-right"));
    apreq_table_add(t1, V("frl", "child-right-left"));
    apreq_table_add(t1, V("flr", "child-left-right"));
    apreq_table_add(t1, V("fll", "child-left-left"));

    apreq_table_add(t1, V("foot", "bart"));
    apreq_table_add(t1, V("foon", "barn"));
    apreq_table_add(t1, V("foot", "bart"));
    apreq_table_add(t1, V("foop", "barp"));
    apreq_table_add(t1, V("food", "bard"));
    apreq_table_add(t1, V("foof", "barf"));
    apreq_table_add(t1, V("fooz", "barz"));
    apreq_table_add(t1, V("fooq", "barq"));
    apreq_table_add(t1, V("foot", "bart"));
    apreq_table_add(t1, V("fooj", "barj"));
    apreq_table_add(t1, V("fook", "bark"));
    apreq_table_add(t1, V("fool", "barl"));
    apreq_table_add(t1, V("foor", "barr"));
    apreq_table_add(t1, V("foos", "bars"));

    apreq_table_add(t1, V("foot", "bart"));
    apreq_table_add(t1, V("foon", "barn"));
    apreq_table_add(t1, V("foot", "bart"));
    apreq_table_add(t1, V("foop", "barp"));
    apreq_table_add(t1, V("food", "bard"));
    apreq_table_add(t1, V("foof", "barf"));
    apreq_table_add(t1, V("fooz", "barz"));
    apreq_table_add(t1, V("fooq", "barq"));
    apreq_table_add(t1, V("foot", "bart"));
    apreq_table_add(t1, V("fooj", "barj"));
    apreq_table_add(t1, V("fook", "bark"));
    apreq_table_add(t1, V("fool", "barl"));
    apreq_table_add(t1, V("foor", "barr"));
    apreq_table_add(t1, V("foos", "bars"));

    apreq_table_add(t1, V("quux", "quux"));
    apreq_table_add(t1, V("quuxa", "quuxa"));
    apreq_table_add(t1, V("quuxz", "quuxz"));
    apreq_table_add(t1, V("quuxb", "quuxb"));
    apreq_table_add(t1, V("quuxg", "quuxg"));
    apreq_table_add(t1, V("quuxw", "quuxw"));
    apreq_table_add(t1, V("quuxr", "quuxr"));
    apreq_table_add(t1, V("alpha", "omega"));

    val = apreq_table_get(t1, "foo");
    CuAssertStrEquals(tc, "bar", val);
    val = apreq_table_get(t1, "quuxb");
    CuAssertStrEquals(tc, "quuxb", val);
    val = apreq_table_get(t1,"flo");
    CuAssertStrEquals(tc, "child-left", val);
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


static void table_perf(CuTest *tc)
{
    CuAssertStrEquals(tc, "quuxb", apreq_table_get(t1,"quuxb"));

    RUN_GET("alpha", "omega", "APR wins worst-case scenario");
    RUN_GET("quuxb", "quuxb", "APR wins multi-key case 1");
    RUN_GET("quuxw", "quuxw", "APR wins multi-key case 2");
    RUN_GET("foonot", NULL,   "APR wins missing-key case");
}

static void table_perf2(CuTest *tc)
{
    RUN_GET("fool",  "barl",  "APR wins multi-key case 3");
    RUN_GET(APREQ_URL_ENCTYPE, "foo","APR wins triple-key case");
    RUN_GET(APREQ_MFD_ENCTYPE, "bar","APR wins single-key case");
}

static void table_cache(CuTest *tc)
{
    apr_table_t *t2 = apreq_table_export(p,t1);
    apr_time_t start, apr_delta, apreq_delta;
    const char *val;
    int i;

    start = apr_time_now();
    for (i = 0; i < LOOP; ++i)
        val = apr_table_get(t2, "foo");
    apr_delta = apr_time_now() - start;
    CuAssertStrEquals(tc, "bar", val);
    start = apr_time_now();
    for (i = 0; i < LOOP; ++i)
        val = apreq_table_get(t1, "foo");
    apreq_delta = apr_time_now() - start;
    CuAssertStrEquals(tc, "bar", val);

//    CuAssertTrue(tc, apreq_delta > apr_delta);

    /* apr_tables should win (above), since foo is the first "f" item;
     * but it should lose to apreq_tables when cacheing is enabled (below).
     */
    CuAssertStrEquals(tc, "bar", apreq_table_cache(t1,"foo"));
    start = apr_time_now();
    for (i = 0; i < LOOP; ++i)
        val = apreq_table_get(t1, "foo");
    apreq_delta = apr_time_now() - start;
    CuAssertStrEquals(tc, "bar", val);

    TEST_DELTAS("APR beats apreq's cache");
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
    apreq_table_t *t2 = init_apreq(APREQ_NELTS);
    apr_table_t *r1, *r2;
    apr_status_t s;
    apr_time_t start, apreq_delta, apr_delta;
    t1 = apreq_table_make(p, APREQ_NELTS);

    r1 = apreq_table_export(p,t1);
    r2 = apreq_table_export(p,t2);

   /* APR_OVERLAP_TABLES_SET had funky semantics, so we ignore it here */

    start = apr_time_now();
    s = apreq_table_overlap(t1, t2, APR_OVERLAP_TABLES_MERGE);
    apreq_delta = apr_time_now() - start;
    CuAssertIntEquals(tc, APR_SUCCESS, s);
    CuAssertIntEquals(tc, 12, apreq_table_nelts(t1));
    start = apr_time_now();
    apr_table_overlap(r1, r2, APR_OVERLAP_TABLES_MERGE);
    apr_delta = apr_time_now() - start;
    CuAssertIntEquals(tc, 12, apr_table_elts(r1)->nelts);

    if (apr_delta < apreq_delta)
        CuFail(tc, apr_psprintf(p, "APR wins overlap contest #1: %" APR_INT64_T_FMT
                   " < %" APR_INT64_T_FMT " (microseconds)", apr_delta,apreq_delta));

    t1 = apreq_table_make(p, 1);
    t2 = apreq_table_make(p,1);
    apreq_table_add(t1, V("a", "0"));
    apreq_table_add(t1, V("g", "7"));

    apreq_table_add(t2, V("a", "1"));
    apreq_table_add(t2, V("b", "2"));
    apreq_table_add(t2, V("c", "3"));
    apreq_table_add(t2, V("b", "2.0"));
    apreq_table_add(t2, V("d", "4"));
    apreq_table_add(t2, V("e", "5"));
    apreq_table_add(t2, V("b", "2."));
    apreq_table_add(t2, V("f", "6"));

    r1 = apreq_table_export(p,t1);
    r2 = apreq_table_export(p,t2);
 
   /* APR_OVERLAP_TABLES_SET had funky semantics, so we ignore it here */

    val = apreq_table_get(t1, "a");
    CuAssertStrEquals(tc, "0",val);

    start = apr_time_now();
    s = apreq_table_overlap(t1, t2, APR_OVERLAP_TABLES_MERGE);
    apreq_delta = apr_time_now() - start;
    CuAssertIntEquals(tc, APR_SUCCESS, s);
    CuAssertIntEquals(tc, 7, apreq_table_nelts(t1));
    start = apr_time_now();
    apr_table_overlap(r1, r2, APR_OVERLAP_TABLES_MERGE);
    apr_delta = apr_time_now() - start;
    CuAssertIntEquals(tc, 7, apr_table_elts(r1)->nelts);


    if (apr_delta < apreq_delta)
        CuFail(tc, apr_psprintf(p, "APR wins overlap contest #2: %" APR_INT64_T_FMT
                   " < %" APR_INT64_T_FMT " (microseconds)", apr_delta,apreq_delta));


    t2 = apreq_table_copy(p,t1);

    CuAssertIntEquals(tc, 3, apreq_table_ghosts(t2));
    CuAssertIntEquals(tc, 9, apreq_table_exorcise(t2));
    CuAssertIntEquals(tc, 0, apreq_table_ghosts(t2));
    CuAssertIntEquals(tc, 7, apreq_table_nelts(t2));

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

    apreq_table_add(t1, V("a", "0"));
    apreq_table_add(t1, V("g", "7"));

    apreq_table_add(t2, V("a", "1"));
    apreq_table_add(t2, V("b", "2"));
    apreq_table_add(t2, V("c", "3"));
    apreq_table_add(t2, V("b", "2.0"));
    apreq_table_add(t2, V("d", "4"));
    apreq_table_add(t2, V("e", "5"));
    apreq_table_add(t2, V("b", "2."));
    apreq_table_add(t2, V("f", "6"));
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

static void table_values(CuTest *tc)
{
    const apreq_value_t *v;
    apr_array_header_t *a = apr_array_make(p,1,sizeof v);
    apr_status_t s = apreq_table_values(t1,"a",a);
    CuAssertIntEquals(tc, APR_SUCCESS, s);
    CuAssertIntEquals(tc, 2, a->nelts);
    v = ((const apreq_value_t **)a->elts)[0];
    CuAssertStrEquals(tc, "a", v->name);
    CuAssertStrEquals(tc, "0", v->data);
    v = ((const apreq_value_t **)a->elts)[1];
    CuAssertStrEquals(tc, "a", v->name);
    CuAssertStrEquals(tc, "1", v->data);
    a->nelts = 0;
    s = apreq_table_values(t1,"b",a);
    CuAssertIntEquals(tc, APR_SUCCESS, s);
    CuAssertIntEquals(tc, 3, a->nelts);
    v = ((const apreq_value_t **)a->elts)[0];
    CuAssertStrEquals(tc, "b", v->name);
    CuAssertStrEquals(tc, "2", v->data);
    v = ((const apreq_value_t **)a->elts)[1];
    CuAssertStrEquals(tc, "b", v->name);
    CuAssertStrEquals(tc, "2.0", v->data);
    v = ((const apreq_value_t **)a->elts)[2];
    CuAssertStrEquals(tc, "b", v->name);
    CuAssertStrEquals(tc, "2.", v->data);

    a->nelts = 0;
    s = apreq_table_values(t1,NULL,a);
    CuAssertIntEquals(tc, APR_SUCCESS, s);
    CuAssertIntEquals(tc, 7, a->nelts);
    
}
#endif

CuSuite *testperformance(void)
{
    CuSuite *suite = CuSuiteNew("Performance");

    SUITE_ADD_TEST(suite, perf_init);
    SUITE_ADD_TEST(suite, perf_get_avg); 
    SUITE_ADD_TEST(suite, perf_get); 
/*
    SUITE_ADD_TEST(suite, table_set);
    SUITE_ADD_TEST(suite, table_getnotthere);
    SUITE_ADD_TEST(suite, table_add);
    SUITE_ADD_TEST(suite, table_unset);
    SUITE_ADD_TEST(suite, table_perf);
    SUITE_ADD_TEST(suite, table_perf2);
    SUITE_ADD_TEST(suite, table_cache);
    SUITE_ADD_TEST(suite, table_nelts);
    SUITE_ADD_TEST(suite, table_clear);
    SUITE_ADD_TEST(suite, table_overlap);
    SUITE_ADD_TEST(suite, table_elts);
    SUITE_ADD_TEST(suite, table_overlay);
    SUITE_ADD_TEST(suite, table_keys);
    SUITE_ADD_TEST(suite, table_values);
*/
    return suite;
}

