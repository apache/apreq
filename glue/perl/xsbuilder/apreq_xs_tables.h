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

#ifndef APREQ_XS_TABLES_H
#define APREQ_XS_TABLES_H

/* backward compatibility macros support */

#include "ppport.h"


#if (PERL_VERSION >= 8) /* MAGIC ITERATOR REQUIRES 5.8 */

/* Requires perl 5.8 or better. 
 * A custom MGVTBL with its "copy" slot filled allows
 * us to FETCH a table entry immediately during iteration.
 * For multivalued keys this is essential in order to get
 * the value corresponding to the current key, otherwise
 * values() will always report the first value repeatedly.
 * With this MGVTBL the keys() list always matches up with
 * the values() list, even in the multivalued case.
 * We only prefetch the value during iteration, because the
 * prefetch adds overhead to EXISTS and STORE operations.
 * They are only "penalized" when the perl program is iterating
 * via each(), which seems to be a reasonable tradeoff.
 */


static int apreq_xs_table_magic_copy(pTHX_ SV *sv, MAGIC *mg, SV *nsv, 
                                  const char *name, int namelen)
{
    /* Prefetch the value whenever the table iterator is > 0 */
    MAGIC *tie_magic = mg_find(nsv, PERL_MAGIC_tiedelem);
    SV *obj = SvRV(tie_magic->mg_obj);
    if (SvCUR(obj))
        SvGETMAGIC(nsv);
    return 0;
}

static const MGVTBL apreq_xs_table_magic = {0, 0, 0, 0, 0, 
                                            apreq_xs_table_magic_copy};

#endif

/**
 * Converts a C object, with environment, to a TIEHASH object.
 * @param obj C object.
 * @param env C environment.
 * @param class Class perl object will be blessed and tied to.
 * @return Reference to a new TIEHASH object in class.
 */
APR_INLINE
static SV *apreq_xs_table_c2perl(pTHX_ void *obj, const char *name, I32 nlen,
                                 const char *class, SV *parent, unsigned tainted)
{
    SV *sv = (SV *)newHV();
    /*upgrade ensures CUR and LEN are both 0 */
    SV *rv = sv_setref_pv(newSV(0), class, obj);

    sv_magic(SvRV(rv), parent, PERL_MAGIC_ext, name, nlen);
    if (tainted)
        SvTAINTED_on(SvRV(rv));

#if (PERL_VERSION >= 8) /* MAGIC ITERATOR requires 5.8 */

    sv_magic(sv, NULL, PERL_MAGIC_ext, Nullch, -1);
    SvMAGIC(sv)->mg_virtual = (MGVTBL *)&apreq_xs_table_magic;
    SvMAGIC(sv)->mg_flags |= MGf_COPY;

#endif

    sv_magic(sv, rv, PERL_MAGIC_tied, Nullch, 0);
    SvREFCNT_dec(rv); /* corrects SvREFCNT_inc(rv) implicit in sv_magic */

    return sv_bless(newRV_noinc(sv), SvSTASH(SvRV(rv)));
}


#define apreq_xs_sv2table(sv)      ((apr_table_t *) SvIVX(SvRV(sv)))
#define apreq_xs_table2sv(t,class,parent,name,tainted)          \
     apreq_xs_table_c2perl(aTHX_ t, name, name ? strlen(name) : 0, class, parent, tainted)


#define APREQ_XS_DEFINE_TABLE_MAKE(attr,pkg)                            \
static XS(apreq_xs_table_##attr##_make)                                 \
{                                                                       \
                                                                        \
    dXSARGS;                                                            \
    SV *sv, *parent;                                                    \
    void *env;                                                          \
    const char *class;                                                  \
    apr_table_t *t;                                                     \
                                                                        \
    if (items != 2 || !SvPOK(ST(0)) || !SvROK(ST(1)))                   \
        Perl_croak(aTHX_ "Usage: $class->make($env)");                  \
                                                                        \
                                                                        \
    class = SvPV_nolen(ST(0));                                          \
    parent = SvRV(ST(1));                                               \
    env = (void *)SvIVX(parent);                                        \
    t = apr_table_make(apreq_env_pool(env), APREQ_NELTS);               \
    sv = apreq_xs_table2sv(t, class, parent, pkg, SvTAINTED(parent));   \
    XSprePUSH;                                                          \
    PUSHs(sv);                                                          \
    XSRETURN(1);                                                        \
}

#define APREQ_XS_DEFINE_TABLE_METHOD_N(attr,method)                     \
static XS(apreq_xs_table_##attr##_##method)                             \
{                                                                       \
    dXSARGS;                                                            \
    void *env;                                                          \
    apr_table_t *t;                                                     \
    const char *key, *val;                                              \
    SV *sv, *obj;                                                       \
    STRLEN klen, vlen;                                                  \
    apreq_##attr##_t *RETVAL;                                           \
                                                                        \
    switch (items) {                                                    \
    case 2:                                                             \
    case 3:                                                             \
        if (!SvROK(ST(0)))                                              \
           break;                                                       \
        sv  = ST(0);                                                    \
        obj = apreq_xs_find_obj(aTHX_ sv, #attr);                       \
        env = apreq_xs_sv2env(obj);                                     \
        t   = (apr_table_t *)SvIVX(obj);                                \
                                                                        \
        if (SvROK(ST(items-1))) {                                       \
            RETVAL = (apreq_##attr##_t *)SvIVX(SvRV(ST(items-1)));      \
            if (SvTAINTED(SvRV(ST(items-1))))                           \
                SvTAINTED_on(obj);                                      \
        }                                                               \
        else if (items == 3) {                                          \
            key = SvPV(ST(1),klen);                                     \
            val = SvPV(ST(2),vlen);                                     \
            RETVAL = apreq_make_##attr(apreq_env_pool(env), key, klen,  \
                                       val, vlen);                      \
            if (SvTAINTED(ST(1)) || SvTAINTED(ST(2)))                   \
                SvTAINTED_on(obj);                                      \
        }                                                               \
        apr_table_##method##n(t, RETVAL->v.name, RETVAL->v.data);       \
        XSRETURN_EMPTY;                                                 \
    default:                                                            \
        ; /* usage */                                                   \
    }                                                                   \
                                                                        \
    Perl_croak(aTHX_ "Usage: $table->" #method                          \
           "([$key,] $val))");                                          \
}


/* TABLE_GET */
struct apreq_xs_table_key_magic {
    SV         *obj;
    const char *val;
};


struct apreq_xs_do_arg {
    void            *env;
    const char      *pkg;
    SV              *parent, *sub;
    PerlInterpreter *perl;
    unsigned         tainted;
};

static int apreq_xs_table_keys(void *data, const char *key,
                               const char *val)
{
#ifdef USE_ITHREADS
    struct apreq_xs_do_arg *d = (struct apreq_xs_do_arg *)data;
    dTHXa(d->perl);
#endif

    dSP;
    SV *sv = newSVpv(key,0);
    if (d->tainted)
        SvTAINTED_on(sv);
    XPUSHs(sv_2mortal(sv));
    PUTBACK;
    return 1;
}

#define apreq_xs_do(attr)          (items == 1 ? apreq_xs_table_keys    \
                                   : apreq_xs_##attr##_table_values)

#define apreq_xs_push(attr,sv,d,key) do {                       \
     apr_table_t *t = apreq_xs_##attr##_sv2table(sv);           \
     if (t != NULL) {                                           \
         if (items == 1) {                                      \
             t = apr_table_copy(apreq_env_pool(env), t);        \
             apr_table_compress(t, APR_OVERLAP_TABLES_SET);     \
             apr_table_do(apreq_xs_table_keys, d, t, NULL);     \
         }                                                      \
         else                                                   \
             apr_table_do(apreq_xs_##attr##_table_values, d,    \
                          t, key, NULL);                        \
     }                                                          \
} while (0)

/** 
 * @param attr       obj/attribute name.
 * @param class      perl class the attribute is in (usually a table class).
 * @param type       apreq data type: param or cookie.
 * @param subclass   perl class for returned "type2sv" scalars.
 * @param COND       expression that must be true for RETVAL to be added
 *                   to the return list.
 *
 * @remark
 * Requires macros for controlling behavior in context:
 *
 *             apreq_xs_##attr##_push           G_ARRAY
 *             apreq_xs_##attr##_sv2table       G_SCALAR (items==1)
 *             apreq_xs_##attr##_##type         G_SCALAR (items==2)
 *             apreq_xs_##type##2sv             G_ARRAY and G_SCALAR
 *
 */

#define APREQ_XS_DEFINE_TABLE_GET(attr, class, type, subclass, COND)    \
static int apreq_xs_##attr##_table_values(void *data, const char *key,  \
                                                      const char *val)  \
{                                                                       \
    struct apreq_xs_do_arg *d = (struct apreq_xs_do_arg *)data;         \
    void *env;                                                          \
    dTHXa(d->perl);                                                     \
    dSP;                                                                \
    env = d->env;                                                       \
    if (val) {                                                          \
        apreq_##type##_t *RETVAL =                                      \
                          apreq_value_to_##type(apreq_strtoval(val));   \
        if (COND) {                                                     \
            SV *sv = apreq_xs_##type##2sv(RETVAL,d->pkg,d->parent);     \
            if (d->tainted)                                             \
                SvTAINTED_on(SvROK(sv)? SvRV(sv) : sv);                 \
            XPUSHs(sv_2mortal(sv));                                     \
        }                                                               \
    } else                                                              \
        XPUSHs(&PL_sv_undef);                                           \
                                                                        \
    PUTBACK;                                                            \
    return 1;                                                           \
}                                                                       \
                                                                        \
static XS(apreq_xs_##attr##_get)                                        \
{                                                                       \
    dXSARGS;                                                            \
    const char *key = NULL;                                             \
    struct apreq_xs_do_arg d = { NULL, NULL, NULL, NULL, aTHX, 0 };     \
    void *env;                                                          \
    SV *sv, *obj;                                                       \
    MAGIC *mg;                                                          \
    if (items == 0 || items > 2 || !SvROK(ST(0)))                       \
        Perl_croak(aTHX_ "Usage: $object->get($key)");                  \
                                                                        \
    sv = ST(0);                                                         \
    obj = apreq_xs_find_obj(aTHX_ sv, #attr);                           \
    mg = mg_find(obj, PERL_MAGIC_ext);                                  \
    d.parent = mg->mg_obj;                                              \
    d.pkg = mg->mg_len > 0 ? mg->mg_ptr : subclass;                     \
    env = (void *)SvIVX(d.parent);                                      \
    d.env = env;                                                        \
                                                                        \
    d.tainted = SvTAINTED(obj);                                         \
    if (items == 2)                                                     \
        key = SvPV_nolen(ST(1));                                        \
                                                                        \
    XSprePUSH;                                                          \
    switch (GIMME_V) {                                                  \
        apreq_##type##_t *RETVAL;                                       \
                                                                        \
    case G_ARRAY:                                                       \
        PUTBACK;                                                        \
        apreq_xs_##attr##_push(obj, &d, key);                           \
        break;                                                          \
                                                                        \
    case G_SCALAR:                                                      \
        if (items == 1) {                                               \
            apr_table_t *t = apreq_xs_##attr##_sv2table(obj);           \
            if (t != NULL) {                                            \
                SV *tsv = apreq_xs_table2sv(t, class, d.parent,         \
                                            d.pkg, d.tainted);          \
                PUSHs(sv_2mortal(tsv));                                 \
            }                                                           \
            PUTBACK;                                                    \
            break;                                                      \
        }                                                               \
                                                                        \
        RETVAL = apreq_xs_##attr##_##type(obj, key);                    \
                                                                        \
        if (RETVAL && (COND)) {                                         \
            SV *ssv = apreq_xs_##type##2sv(RETVAL, d.pkg, d.parent);    \
            if (d.tainted)                                              \
                SvTAINTED_on(ssv);                                      \
            PUSHs(sv_2mortal(ssv));                                     \
        }                                                               \
                                                                        \
    default:                                                            \
        PUTBACK;                                                        \
    }                                                                   \
    apreq_xs_##attr##_error_check;                                      \
}

#define APREQ_XS_DEFINE_TABLE_FETCH(attr,type,subclass)         \
static XS(apreq_xs_##attr##_FETCH)                              \
{                                                               \
    dXSARGS;                                                    \
    SV *sv, *obj, *parent;                                      \
    IV idx;                                                     \
    MAGIC *mg;                                                  \
    const char *key, *pkg;                                      \
    const char *val;                                            \
    apr_table_t *t;                                             \
    const apr_array_header_t *arr;                              \
    apr_table_entry_t *te;                                      \
    void *env;                                                  \
                                                                \
    if (items != 2 || !SvROK(ST(0)) || !SvOK(ST(1)))            \
        Perl_croak(aTHX_ "Usage: $table->FETCH($key)");         \
                                                                \
    sv  = ST(0);                                                \
    obj = apreq_xs_find_obj(aTHX_ sv, #attr);                   \
    mg = mg_find(obj, PERL_MAGIC_ext);                          \
    parent = mg->mg_obj;                                        \
    pkg = mg->mg_len > 0 ? mg->mg_ptr : subclass;               \
    key = SvPV_nolen(ST(1));                                    \
    idx = SvCUR(obj);                                           \
    t   = apreq_xs_##attr##_sv2table(obj);                      \
    arr = apr_table_elts(t);                                    \
    te  = (apr_table_entry_t *)arr->elts;                       \
    env = apreq_xs_##attr##_sv2env(obj);                        \
                                                                \
    if (idx > 0 && idx <= arr->nelts                            \
        && !strcasecmp(key, te[idx-1].key))                     \
        val = te[idx-1].val;                                    \
    else                                                        \
        val = apr_table_get(t, key);                            \
                                                                \
    if (val != NULL) {                                          \
        apreq_##type##_t *RETVAL = apreq_value_to_##type(       \
                                          apreq_strtoval(val)); \
        sv = apreq_xs_##type##2sv(RETVAL, pkg, parent);         \
        if (SvTAINTED(obj))                                     \
            SvTAINTED_on(sv);                                   \
        ST(0) = sv_2mortal(sv);                                 \
        XSRETURN(1);                                            \
    }                                                           \
    else                                                        \
        XSRETURN_UNDEF;                                         \
}


#define APREQ_XS_DEFINE_TABLE_NEXTKEY(attr)                     \
static XS(apreq_xs_##attr##_NEXTKEY)                            \
{                                                               \
    dXSARGS;                                                    \
    SV *sv, *obj;                                               \
    IV idx;                                                     \
    apr_table_t *t;                                             \
    const apr_array_header_t *arr;                              \
    apr_table_entry_t *te;                                      \
                                                                \
    if (!SvROK(ST(0)))                                          \
        Perl_croak(aTHX_ "Usage: $table->NEXTKEY($prev)");      \
    obj = apreq_xs_find_obj(aTHX_ ST(0), #attr);                \
    t = apreq_xs_##attr##_sv2table(obj);                        \
    arr = apr_table_elts(t);                                    \
    te  = (apr_table_entry_t *)arr->elts;                       \
                                                                \
    if (items == 1)                                             \
        SvCUR(obj) = 0;                                         \
                                                                \
    if (SvCUR(obj) >= arr->nelts) {                             \
        SvCUR(obj) = 0;                                         \
        XSRETURN_UNDEF;                                         \
    }                                                           \
    idx = SvCUR(obj)++;                                         \
    sv = newSVpv(te[idx].key, 0);                               \
    if (SvTAINTED(obj))                                         \
        SvTAINTED_on(sv);                                       \
    ST(0) = sv_2mortal(sv);                                     \
    XSRETURN(1);                                                \
}


#define APREQ_XS_DEFINE_TABLE_DO(attr,type,subclass)                    \
static int apreq_xs_##attr##_do_sub(void *data, const char *key,        \
                                          const char *val)              \
{                                                                       \
    struct apreq_xs_do_arg *d = data;                                   \
    apreq_##type##_t *RETVAL = apreq_value_to_##type(                   \
                                    apreq_strtoval(val));               \
    dTHXa(d->perl);                                                     \
    dSP;                                                                \
    SV *sv;                                                             \
    void *env;                                                          \
    int rv;                                                             \
                                                                        \
    env = d->env;                                                       \
                                                                        \
    ENTER;                                                              \
    SAVETMPS;                                                           \
                                                                        \
    PUSHMARK(SP);                                                       \
    EXTEND(SP,2);                                                       \
                                                                        \
    sv = newSVpv(key, 0);                                               \
    if (d->tainted)                                                     \
        SvTAINTED_on(sv);                                               \
    PUSHs(sv_2mortal(sv));                                              \
                                                                        \
    sv = apreq_xs_##type##2sv(RETVAL, d->pkg, d->parent);               \
    if (d->tainted)                                                     \
        SvTAINTED_on(sv);                                               \
    PUSHs(sv_2mortal(sv));                                              \
                                                                        \
    PUTBACK;                                                            \
    rv = call_sv(d->sub, G_SCALAR);                                     \
    SPAGAIN;                                                            \
    rv = (1 == rv) ? POPi : 1;                                          \
    PUTBACK;                                                            \
    FREETMPS;                                                           \
    LEAVE;                                                              \
                                                                        \
    return rv;                                                          \
}                                                                       \
                                                                        \
static XS(apreq_xs_##attr##_do)                                         \
{                                                                       \
    dXSARGS;                                                            \
    struct apreq_xs_do_arg d = { NULL, NULL, NULL, NULL, aTHX, 0 };     \
    apr_table_t *t;                                                     \
    void *env;                                                          \
    int i, rv;                                                          \
    SV *sv, *obj;                                                       \
    MAGIC *mg;                                                          \
                                                                        \
    if (items < 2 || !SvROK(ST(0)) || !SvROK(ST(1)))                    \
        Perl_croak(aTHX_ "Usage: $object->do(\\&callback, @keys)");     \
    sv = ST(0);                                                         \
    obj = apreq_xs_find_obj(aTHX_ sv, #attr);                           \
    env = apreq_xs_##attr##_sv2env(obj);                                \
    t = apreq_xs_##attr##_sv2table(obj);                                \
    mg = mg_find(obj, PERL_MAGIC_ext);                                  \
    d.parent = mg->mg_obj;                                              \
    d.pkg = mg->mg_len > 0 ? mg->mg_ptr : subclass;                     \
    d.env = env;                                                        \
    d.sub = ST(1);                                                      \
    d.tainted = SvTAINTED(obj);                                         \
    if (items == 2) {                                                   \
        rv = apr_table_do(apreq_xs_##attr##_do_sub, &d, t, NULL);       \
        XSRETURN_IV(rv);                                                \
    }                                                                   \
                                                                        \
    for (i = 2; i < items; ++i) {                                       \
        const char *key = SvPV_nolen(ST(i));                            \
        rv = apr_table_do(apreq_xs_##attr##_do_sub, &d, t, key, NULL);  \
        if (rv == 0)                                                    \
            break;                                                      \
    }                                                                   \
    XSRETURN_IV(rv);                                                    \
}


#endif /* APREQ_XS_TABLES_H */
