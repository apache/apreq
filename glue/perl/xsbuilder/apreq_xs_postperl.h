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

#ifndef APREQ_XS_POSTPERL_H
#define APREQ_XS_POSTPERL_H

/* backward compatibility macros support */
#include "ppport.h"

/**
 * @file apreq_xs_postperl.h
 * @brief XS include file for making Cookie.so and Request.so
 *
 */
/**
 * @defgroup XS Perl
 * @ingroup GLUE
 * @{
 */

/** 
 * Trace through magic objects & hashrefs looking for original object.
 * @param in  The starting SV *.
 * @param key The first letter of key is used to search a hashref for 
 *            the desired object.
 * @return    The object, if found;  otherwise NULL.
 */
APR_INLINE
static SV *apreq_xs_find_obj(pTHX_ SV *in, const char *key)
{
    const char altkey[] = { '_', key[0] };

    while (in && SvROK(in)) {
        SV *sv = SvRV(in);
        switch (SvTYPE(sv)) {            
            MAGIC *mg;
            SV **svp;
        case SVt_PVHV:
            if (SvMAGICAL(sv) && (mg = mg_find(sv,PERL_MAGIC_tied))) {
               in = mg->mg_obj;
               break;
            }
            else if ((svp = hv_fetch((HV *)sv, key, 1, FALSE)) ||
                     (svp = hv_fetch((HV *)sv, altkey, 2, FALSE)))
            {
                in = *svp;
                break;
            }
            Perl_croak(aTHX_ "attribute hash has no '%s' key!", key);
        case SVt_PVMG:
            if (SvOBJECT(sv) && SvIOK(sv))
                return sv;
        default:
             Perl_croak(aTHX_ "panic: unsupported SV type: %d", SvTYPE(sv));
       }
    }
    return NULL;
}

/* conversion function templates based on modperl-2's sv2request_rec */

/**
 * Searches a perl object ref with apreq_xs_find_obj 
 * and produces a pointer to the object's C analog.
 */
APR_INLINE
static void *apreq_xs_perl2c(pTHX_ SV* in, const char *name)
{
    SV *sv = apreq_xs_find_obj(aTHX_ in, name);
    if (sv == NULL)
        return NULL;
    else 
        return (void *)SvIVX(sv);
}

/** 
 * Searches a perl object ref with apreq_xs_find_obj
 * and produces a pointer to the underlying C environment.
 */ 
APR_INLINE
static void *apreq_xs_perl2env(pTHX_ SV *sv)
{
    MAGIC *mg;
    if (sv != NULL && (mg = mg_find(sv, PERL_MAGIC_ext)))
        return mg->mg_ptr;
    return NULL;
}

/**
 * Converts a C object, with environment, to a Perl object.
 * @param obj C object.
 * @param env C environment.
 * @param class Class perl object will be blessed into.
 * @return Reference to the new Perl object in class.
 */
APR_INLINE
static SV *apreq_xs_c2perl(pTHX_ void *obj, void *env, const char *class, SV *parent)
{
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
    return rv;
}

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

    sv_magic(sv, rv, PERL_MAGIC_tied, Nullch, 0);
    SvREFCNT_dec(rv); /* corrects SvREFCNT_inc(rv) implicit in sv_magic */

    return sv_bless(newRV_noinc(sv), SvSTASH(SvRV(rv)));
}

#define apreq_xs_2sv(t,class,parent)                    \
             apreq_xs_c2perl(aTHX_ t, env, class, parent)

#define apreq_xs_sv2(type,sv)((apreq_##type##_t *)      \
                               apreq_xs_perl2c(aTHX_ sv, #type))

#define apreq_xs_sv2env(sv) apreq_xs_perl2env(aTHX_ sv)

/** Converts apreq_env to a Perl package, which forms the
 * base class for Apache::Request and Apache::Cookie::Jar objects.
 */
#define APREQ_XS_DEFINE_ENV(type)                       \
APR_INLINE                                              \
static XS(apreq_xs_##type##_env)                        \
{                                                       \
    char *class = NULL;                                 \
    dMARK; dAX;                                         \
                                                        \
    /* map environment to package */                    \
                                                        \
    if (strcmp(apreq_env_name, "APACHE2") == 0)         \
        class = "Apache::RequestRec";                   \
                                                        \
    /* else if ... add more conditionals here as        \
       additional environments become supported */      \
                                                        \
    if (class == NULL)                                  \
        XSRETURN(0);                                    \
                                                        \
    if (SvROK(ST(0))) {                                 \
        SV *sv = apreq_xs_find_obj(aTHX_ ST(0), #type); \
        void *env = apreq_xs_sv2env(sv);                \
                                                        \
        if (env)                                        \
            ST(0) = sv_2mortal(sv_setref_pv(newSV(0),   \
                                          class, env)); \
        else                                            \
            ST(0) = &PL_sv_undef;                       \
    }                                                   \
    else                                                \
        ST(0) = sv_2mortal(newSVpv(class, 0));          \
                                                        \
    XSRETURN(1);                                        \
}


/** requires apreq_##type (type is either request or jar) */

#define APREQ_XS_DEFINE_OBJECT(type)                                    \
static XS(apreq_xs_##type)                                              \
{                                                                       \
    dXSARGS;                                                            \
    void *env;                                                          \
    const char *data;                                                   \
    apreq_##type##_t *obj;                                              \
                                                                        \
    if (items < 2 || SvROK(ST(0)) || !SvROK(ST(1)))                     \
        Perl_croak(aTHX_ "Usage: $class->" #type "($env, $data)");      \
                                                                        \
    env = (void *)SvIVX(SvRV(ST(1)));                                   \
    data = (items == 3)  ?  SvPV_nolen(ST(2)) :  NULL;                  \
    obj = apreq_##type(env, data);                                      \
                                                                        \
    ST(0) = obj ? sv_2mortal(apreq_xs_2sv(obj, SvPV_nolen(ST(0)),ST(1)))\
                : &PL_sv_undef;                                         \
    XSRETURN(1);                                                        \
}


/** requires definition of apreq_xs_##type##2sv(t,class,parent) macro */

#define APREQ_XS_DEFINE_MAKE(type)                                      \
static XS(apreq_xs_make_##type)                                         \
{                                                                       \
    dXSARGS;                                                            \
    void *env;                                                          \
    apr_pool_t *pool;                                                   \
    const char *key, *val, *class;                                      \
    STRLEN klen, vlen;                                                  \
    apreq_##type##_t *t;                                                \
                                                                        \
    if (items != 4 || SvROK(ST(0)) || !SvROK(ST(1)))                    \
        Perl_croak(aTHX_ "Usage: $class->make($env, $name, $val)");     \
                                                                        \
    class = SvPV_nolen(ST(0));                                          \
    env = (void *)SvIVX(SvRV(ST(1)));                                   \
    pool = apreq_env_pool(env);                                         \
    key = SvPVbyte(ST(2), klen);                                        \
    val = SvPVbyte(ST(3), vlen);                                        \
    t = apreq_make_##type(pool, key, klen, val, vlen);                  \
                                                                        \
    ST(0) = sv_2mortal(apreq_xs_##type##2sv(t,class,ST(1)));            \
    XSRETURN(1);                                                        \
}  

static APR_INLINE
void apreq_xs_croak(pTHX_ HV *data, apr_status_t rc, const char *func, 
                   const char *class)
{
    HV *stash = gv_stashpvn(class, strlen(class), FALSE);

    sv_setsv(ERRSV, sv_2mortal(sv_bless(newRV_noinc((SV*)data), stash)));
    sv_setiv(*hv_fetch(data, "rc",   2, 1), rc);
    sv_setpv(*hv_fetch(data, "file", 4, 1), CopFILE(PL_curcop));
    sv_setiv(*hv_fetch(data, "line", 4, 1), CopLINE(PL_curcop));
    sv_setpv(*hv_fetch(data, "func", 4, 1), func);
    Perl_croak(aTHX_ Nullch);
}

#define APREQ_XS_DEFINE_POOL(attr)                              \
static XS(apreq_xs_##attr##_pool)                               \
{                                                               \
    dXSARGS;                                                    \
    void *env;                                                  \
                                                                \
    if (items != 1 || !SvROK(ST(0)))                            \
        Perl_croak(aTHX_ "Usage: $obj->pool()");                \
    env = apreq_xs_##attr##_sv2env(ST(0));                      \
    ST(0) = sv_2mortal(sv_setref_pv(newSV(0), "APR::Pool",      \
                                    apreq_env_pool(env)));      \
    XSRETURN(1);                                                \
}

/** @} */

#endif /* APREQ_XS_POSTPERL_H */
