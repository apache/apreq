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

#define APREQ_XS_DEFINE_TABLE_METHOD_N(attr,method)                             \
static XS(apreq_xs_table_##attr##_##method)                                     \
{                                                                               \
    dXSARGS;                                                                    \
    void *env;                                                                  \
    apr_table_t *t;                                                             \
    const char *key, *val;                                                      \
    SV *sv;                                                                     \
    STRLEN klen, vlen;                                                          \
    apreq_##attr##_t *obj;                                                      \
                                                                                \
    if (items != 3 || !SvROK(ST(0)) || !SvPOK(ST(1)))                           \
        Perl_croak(aTHX_ "Usage: $table->" #method "($key, $val)");             \
                                                                                \
    sv  = apreq_xs_find_obj(aTHX_ ST(0), #attr);                                \
    if (sv == NULL)                                                             \
         Perl_croak(aTHX_ "Cannot find object");                                \
    env = apreq_xs_sv2env(sv);                                                  \
    t   = (apr_table_t *) SvIVX(sv);                                            \
    key = SvPV(ST(1), klen);                                                    \
                                                                                \
    if (SvROK(ST(2))) {                                                         \
        obj = (apreq_##attr##_t *) SvIVX(SvRV(ST(2)));                          \
    }                                                                           \
    else if (SvPOK(ST(2))) {                                                    \
        val = SvPV(ST(2), vlen);                                                \
        obj = apreq_make_##attr(apreq_env_pool(env), key, klen, val, vlen);     \
    }                                                                           \
    else                                                                        \
        Perl_croak(aTHX_ "Usage: $table->" #method "($key, $val): "             \
                   "unrecognized SV type for $val");                            \
    val = obj->v.data;                                                          \
                                                                                \
    apr_table_##method##n(t, key, val);                                         \
    XSRETURN_EMPTY;                                                             \
}



/* TABLE_GET */


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

    if (key)
        XPUSHs(sv_2mortal(newSVpv(key,0)));
    else
        XPUSHs(&PL_sv_undef);

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
    SV *sv;                                                             \
                                                                        \
    if (items == 0 || items > 2 || !SvROK(ST(0)))                       \
        Perl_croak(aTHX_ "Usage: $object->get($key)");                  \
                                                                        \
    sv = apreq_xs_find_obj(aTHX_ ST(0), #attr);                         \
    env = apreq_xs_##attr##_sv2env(sv);                                 \
    d.env = env;                                                        \
    d.parent = sv;                                                      \
    if (items == 2)                                                     \
        key = SvPV_nolen(ST(1));                                        \
                                                                        \
    XSprePUSH;                                                          \
    switch (GIMME_V) {                                                  \
        apreq_##type##_t *RETVAL;                                       \
                                                                        \
    case G_ARRAY:                                                       \
        PUTBACK;                                                        \
        apreq_xs_##attr##_push(sv, &d, key);                            \
        break;                                                          \
                                                                        \
    case G_SCALAR:                                                      \
        if (items == 1) {                                               \
            apr_table_t *t = apreq_xs_##attr##_sv2table(sv);            \
            if (t != NULL)                                              \
                XPUSHs(sv_2mortal(apreq_xs_table2sv(t,class,sv)));      \
            PUTBACK;                                                    \
            break;                                                      \
        }                                                               \
                                                                        \
        RETVAL = apreq_xs_##attr##_##type(sv, key);                     \
        if (RETVAL && (COND))                                           \
            XPUSHs(sv_2mortal(                                          \
                   apreq_xs_##type##2sv(RETVAL,subclass,sv)));          \
                                                                        \
    default:                                                            \
        PUTBACK;                                                        \
    }                                                                   \
    apreq_xs_##attr##_error_check;                                      \
}



#endif /* APREQ_XS_TABLES_H */
