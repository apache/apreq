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


/**
 * Converts a C object, with environment, to a TIEHASH object.
 * @param obj C object.
 * @param env C environment.
 * @param class Class perl object will be blessed and tied to.
 * @return Reference to a new TIEHASH object in class.
 */
APR_INLINE
static SV *apreq_xs_table_c2perl(pTHX_ void *obj, void *env, 
                                 const char *class, SV *parent)
{
    SV *sv = (SV *)newHV();
    /*upgrade ensures CUR and LEN are both 0 */
    SV *rv = sv_setref_pv(newSV(0), class, obj);
    if (env) {
        /* We use the old idiom for sv_magic() below,
         * because perl 5.6 mangles the env pointer on
         * the recommended 5.8.x invocation
         *
         *   sv_magic(SvRV(rv), Nullsv, PERL_MAGIC_ext, env, 0);
         *
         * 5.8.x is OK with the old way as well, but in the future
         * we may have to use "#if PERL_VERSION < 8" ...
         */
        sv_magic(SvRV(rv), parent, PERL_MAGIC_ext, Nullch, -1);
        SvMAGIC(SvRV(rv))->mg_ptr = env;
    }
    sv_magic(sv, NULL, PERL_MAGIC_ext, Nullch, -1);
    SvMAGIC(sv)->mg_virtual = (MGVTBL *)&apreq_xs_table_magic;
    SvMAGIC(sv)->mg_flags |= MGf_COPY;
    sv_magic(sv, rv, PERL_MAGIC_tied, Nullch, 0);
    SvREFCNT_dec(rv); /* corrects SvREFCNT_inc(rv) implicit in sv_magic */

    return sv_bless(newRV_noinc(sv), SvSTASH(SvRV(rv)));
}


#define apreq_xs_sv2table(sv)      ((apr_table_t *) SvIVX(SvRV(sv)))
#define apreq_xs_table2sv(t,class,parent)                               \
                  apreq_xs_table_c2perl(aTHX_ t, env, class, parent)


#define APREQ_XS_DEFINE_TABLE_MAKE(attr)                        \
static XS(apreq_xs_table_##attr##_make)                         \
{                                                               \
                                                                \
    dXSARGS;                                                    \
    apreq_##attr##_t *obj;                                      \
    void *env;                                                  \
    const char *class;                                          \
    apr_table_t *t;                                             \
                                                                \
    if (items != 2 || !SvPOK(ST(0)) || !SvROK(ST(1)))           \
        Perl_croak(aTHX_ "Usage: $class->make($object)");       \
                                                                \
                                                                \
    class = SvPV_nolen(ST(0));                                  \
    obj = apreq_xs_sv2(attr,ST(1));                             \
    env = obj->env;                                             \
    t = apr_table_make(apreq_env_pool(env), APREQ_NELTS);       \
                                                                \
    ST(0) = apreq_xs_table_c2perl(aTHX_ t, env, class, ST(1));  \
    XSRETURN(1);                                                \
}

#define APREQ_XS_DEFINE_TABLE_METHOD_N(attr,method)                     \
static XS(apreq_xs_table_##attr##_##method)                             \
{                                                                       \
    dXSARGS;                                                            \
    void *env;                                                          \
    apr_table_t *t;                                                     \
    const char *key, *val;                                              \
    SV *sv;                                                             \
    STRLEN klen, vlen;                                                  \
    apreq_##attr##_t *obj;                                              \
                                                                        \
    if (items != 3 || !SvROK(ST(0)) || !SvPOK(ST(1)))                   \
        Perl_croak(aTHX_ "Usage: $table->" #method "($key, $val)");     \
                                                                        \
    sv  = apreq_xs_find_obj(aTHX_ ST(0), #attr);                        \
    if (sv == NULL)                                                     \
         Perl_croak(aTHX_ "Cannot find object");                        \
    env = apreq_xs_sv2env(sv);                                          \
    t   = (apr_table_t *) SvIVX(sv);                                    \
    key = SvPV(ST(1), klen);                                            \
                                                                        \
    if (SvROK(ST(2))) {                                                 \
        obj = (apreq_##attr##_t *) SvIVX(SvRV(ST(2)));                  \
    }                                                                   \
    else if (SvPOK(ST(2))) {                                            \
        val = SvPV(ST(2), vlen);                                        \
        obj = apreq_make_##attr(apreq_env_pool(env), key, klen,         \
                                                     val, vlen);        \
    }                                                                   \
    else                                                                \
        Perl_croak(aTHX_ "Usage: $table->" #method "($key, $val): "     \
                   "unrecognized SV type for $val");                    \
    val = obj->v.data;                                                  \
                                                                        \
    apr_table_##method##n(t, key, val);                                 \
    XSRETURN_EMPTY;                                                     \
}



/* TABLE_GET */
struct apreq_xs_table_key_magic {
    SV         *obj;
    const char *val;
};

/* Ignore KEY_MAGIC for now - testing safer MGVTBL approach.
** Comment the define of APREQ_XS_TABLE_USE_KEY_MAGIC out
** if perl still chokes on key magic
** Need 5.8.1 or higher for PERL_MAGIC_vstring
*/
#if 0 && PERL_REVISION == 5 && PERL_VERSION == 8 && PERL_SUBVERSION >= 1
#define APREQ_XS_TABLE_USE_KEY_MAGIC
#endif

#ifdef APREQ_XS_TABLE_USE_KEY_MAGIC

#define APREQ_XS_TABLE_ADD_KEY_MAGIC(p, sv, o, v) do {                  \
    struct apreq_xs_table_key_magic *info = apr_palloc(p,sizeof *info); \
    info->obj = o;                                                      \
    info->val = v;                                                      \
    sv_magic(sv, Nullsv, PERL_MAGIC_vstring, Nullch, -1);               \
    SvMAGIC(sv)->mg_ptr = (char *)info;                                 \
    SvRMAGICAL_on(sv);                                                  \
} while (0)

#else

#define APREQ_XS_TABLE_ADD_KEY_MAGIC(p,sv,o,v) /* noop */

#endif

struct apreq_xs_do_arg {
    void            *env;
    SV              *parent;
    PerlInterpreter *perl;
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
    APREQ_XS_TABLE_ADD_KEY_MAGIC(apreq_env_pool(d->env),sv,d->parent,val);
    XPUSHs(sv_2mortal(sv));
    PUTBACK;
    return 1;
}

#define apreq_xs_do(attr)          (items == 1 ? apreq_xs_table_keys    \
                                   : apreq_xs_##attr##_table_values)

#define apreq_xs_push(attr,sv,d,key) do {                               \
     apr_table_t *t = apreq_xs_##attr##_sv2table(sv);                   \
     if (t)                                                             \
         apr_table_do(apreq_xs_do(attr), d, t, key, NULL);              \
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
        if (COND)                                                       \
            XPUSHs(sv_2mortal(                                          \
                   apreq_xs_##type##2sv(RETVAL,subclass,d->parent)));   \
    } else                                                              \
        XPUSHs(&PL_sv_undef);                                           \
                                                                        \
    PUTBACK;                                                            \
    return 1;                                                           \
}                                                                       \
static XS(apreq_xs_##attr##_get)                                        \
{                                                                       \
    dXSARGS;                                                            \
    const char *key = NULL;                                             \
    struct apreq_xs_do_arg d = { NULL, NULL, aTHX };                    \
    void *env;                                                          \
    SV *sv, *obj;                                                       \
                                                                        \
    if (items == 0 || items > 2 || !SvROK(ST(0)))                       \
        Perl_croak(aTHX_ "Usage: $object->get($key)");                  \
                                                                        \
    sv = ST(0);                                                         \
    obj = apreq_xs_find_obj(aTHX_ sv, #attr);                           \
    env = apreq_xs_##attr##_sv2env(obj);                                \
    d.env = env;                                                        \
    d.parent = obj;                                                     \
                                                                        \
    if (items == 2)                                                     \
        key = SvPV_nolen(ST(1));                                        \
                                                                        \
    XSprePUSH;                                                          \
    switch (GIMME_V) {                                                  \
        apreq_##type##_t *RETVAL;                                       \
        IV idx;                                                         \
        MAGIC *mg;                                                      \
    case G_ARRAY:                                                       \
        PUTBACK;                                                        \
        apreq_xs_##attr##_push(obj, &d, key);                           \
        break;                                                          \
                                                                        \
    case G_SCALAR:                                                      \
        if (items == 1) {                                               \
            apr_table_t *t = apreq_xs_##attr##_sv2table(obj);           \
            if (t != NULL)                                              \
                XPUSHs(sv_2mortal(apreq_xs_table2sv(t,class,obj)));     \
            PUTBACK;                                                    \
            break;                                                      \
        }                                                               \
        else if ((idx = SvCUR(obj)) > 0) {                              \
            const apr_array_header_t *arr = apr_table_elts(             \
                                   apreq_xs_##attr##_sv2table(obj));    \
            apr_table_entry_t *te = (apr_table_entry_t *)arr->elts;     \
                                                                        \
            if (idx <= arr->nelts && !strcasecmp(key, te[idx-1].key))   \
            {                                                           \
                RETVAL = apreq_value_to_##type(                         \
                               apreq_strtoval(te[idx-1].val));          \
                if (COND) {                                             \
                    XPUSHs(sv_2mortal(apreq_xs_##type##2sv(             \
                                          RETVAL,subclass,obj)));       \
                    PUTBACK;                                            \
                    break;                                              \
               }                                                        \
            }                                                           \
        }                                                               \
        if (SvMAGICAL(ST(1))                                            \
            && (mg = mg_find(ST(1),PERL_MAGIC_vstring))                 \
            && mg->mg_len == -1 /*&& mg->mg_obj == obj*/)               \
        {                                                               \
            struct apreq_xs_table_key_magic *info = (void*)mg->mg_ptr;  \
            if (info->obj == obj) {                                     \
                RETVAL = apreq_value_to_##type(                         \
                                       apreq_strtoval(info->val));      \
                if (!strcasecmp(key,RETVAL->v.name) && (COND)) {        \
                        XPUSHs(sv_2mortal(apreq_xs_##type##2sv(         \
                                              RETVAL,subclass,obj)));   \
                        PUTBACK;                                        \
                        break;                                          \
                }                                                       \
            }                                                           \
        }                                                               \
                                                                        \
                                                                        \
        RETVAL = apreq_xs_##attr##_##type(obj, key);                    \
                                                                        \
        if (RETVAL && (COND))                                           \
            XPUSHs(sv_2mortal(                                          \
                   apreq_xs_##type##2sv(RETVAL,subclass,obj)));         \
                                                                        \
    default:                                                            \
        PUTBACK;                                                        \
    }                                                                   \
    apreq_xs_##attr##_error_check;                                      \
}

#define APREQ_XS_DEFINE_TABLE_NEXTKEY(attr)                     \
static XS(apreq_xs_##attr##_NEXTKEY)                            \
{                                                               \
    dXSARGS;                                                    \
    SV *sv, *obj;                                               \
    IV idx;                                                     \
    const apr_array_header_t *arr;                              \
    apr_table_entry_t *te;                                      \
                                                                \
    if (!SvROK(ST(0)))                                          \
        Perl_croak(aTHX_ "Usage: $table->NEXTKEY($prev)");      \
    obj = apreq_xs_find_obj(aTHX_ ST(0), #attr);                \
    arr = apr_table_elts(apreq_xs_##attr##_sv2table(obj));      \
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
    APREQ_XS_TABLE_ADD_KEY_MAGIC(arr->pool,sv,obj,te[idx].val); \
    ST(0) = sv_2mortal(sv);                                     \
    XSRETURN(1);                                                \
}


#endif /* APREQ_XS_TABLES_H */
