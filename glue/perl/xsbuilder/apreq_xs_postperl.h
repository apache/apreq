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

/* avoid namespace collisions from perl's XSUB.h */
#include "modperl_perl_unembed.h"

/* required for modperl's T_HASHOBJ (typemap) */
#include "modperl_common_util.h"

/* backward compatibility macros support */
#include "ppport.h"

/* ExtUtils::XSBuilder::ParseSoure trickery... */
typedef apreq_handle_t apreq_xs_handle_cgi_t;
typedef apreq_handle_t apreq_xs_handle_apache2_t;
typedef apr_table_t    apreq_xs_param_table_t;
typedef apr_table_t    apreq_xs_cookie_table_t;
typedef HV             apreq_xs_error_t;
typedef char*          apreq_xs_subclass_t;

#define HANDLE_CLASS "APR::Request"
#define COOKIE_CLASS "APR::Request::Cookie"
#define PARAM_CLASS  "APR::Request::Param"
#define ERROR_CLASS  "APR::Request::Error"

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
 * @return    Reference to the object.
 */
APR_INLINE
static SV *apreq_xs_find_obj(pTHX_ SV *in, const char key)
{
    const char altkey[] = { '_', key };

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
            else if ((svp = hv_fetch((HV *)sv, altkey+1, 1, FALSE)) ||
                     (svp = hv_fetch((HV *)sv, altkey, 2, FALSE)))
            {
                in = *svp;
                break;
            }
            Perl_croak(aTHX_ "attribute hash has no '%s' key!", key);
        case SVt_PVMG:
            if (SvOBJECT(sv) && SvIOKp(sv))
                return in;
        default:
             Perl_croak(aTHX_ "panic: unsupported SV type: %d", SvTYPE(sv));
       }
    }

    Perl_croak(aTHX_ "apreq_xs_find_obj: object attr `%c' not found", key);
    return NULL;
}

/* conversion function templates based on modperl-2's sv2request_rec */

/**
 * Searches a perl object ref with apreq_xs_find_obj 
 * and produces a pointer to the object's C analog.
 */
APR_INLINE
static void *apreq_xs_perl2c(pTHX_ SV* in, const char name)
{
    SV *sv = apreq_xs_find_obj(aTHX_ in, name);
    IV iv = SvIVX(SvRV(sv));
    return INT2PTR(void *, iv);
}

APR_INLINE
static SV *apreq_xs_perl_sv2env(pTHX_ SV *sv)
{
    MAGIC *mg;
    if ((mg = mg_find(sv, PERL_MAGIC_ext)))
        return mg->mg_obj;

    Perl_croak(aTHX_ "Can't find magic environment");
    return NULL; /* not reached */
}



static APR_INLINE
SV *apreq_xs_object2sv(pTHX_ void *ptr, const char *class, SV *parent, const char *base)
{
    SV *rv = sv_setref_pv(newSV(0), class, (void *)ptr);
    sv_magic(SvRV(rv), parent, PERL_MAGIC_ext, Nullch, 0);
    if (!sv_derived_from(rv, base))
        croak("apreq_xs_object2sv failed: target class %s isn't derived from %s",
              class, base);
    return rv;
}


APR_INLINE
static SV *apreq_xs_handle2sv(pTHX_ apreq_handle_t *req, 
                              const char *class, SV *parent)
{
    return apreq_xs_object2sv(aTHX_ req, class, parent, HANDLE_CLASS);
}

APR_INLINE
static SV *apreq_xs_param2sv(pTHX_ apreq_param_t *p, 
                              const char *class, SV *parent)
{
    if (class == NULL) {
        SV *rv = newSVpvn(p->v.data, apreq_param_vlen(p));
        if (apreq_param_is_tainted(p))
            SvTAINTED_on(rv);
        /*XXX add charset fixups */
        return rv;
    }

    return apreq_xs_object2sv(aTHX_ p, class, parent, PARAM_CLASS);
}

APR_INLINE
static SV *apreq_xs_cookie2sv(pTHX_ apreq_cookie_t *c, 
                              const char *class, SV *parent)
{
    if (class == NULL) {
        SV *rv = newSVpvn(c->v.data, apreq_cookie_vlen(c));
        if (apreq_cookie_is_tainted(c))
            SvTAINTED_on(rv);
        /*XXX add charset fixups? */
        return rv;
    }

    return apreq_xs_object2sv(aTHX_ c, class, parent, COOKIE_CLASS);
}


APR_INLINE
static SV* apreq_xs_error2sv(pTHX_ apr_status_t s)
{
    char buf[256];
    SV *sv = newSV(0);

    sv_upgrade(sv, SVt_PVIV);

    apreq_strerror(s, buf, sizeof buf);
    sv_setpvn(sv, buf, strlen(buf));
    SvPOK_on(sv);

    SvIVX(sv) = s;
    SvIOK_on(sv);

    SvREADONLY_on(sv);

    return sv;
}

APR_INLINE
static SV *apreq_xs_sv2object(pTHX_ SV *sv, const char *class, const char attr)
{
    SV *obj;
    MAGIC *mg;
    sv = apreq_xs_find_obj(aTHX_ sv, attr);

    /* XXX sv_derived_from is expensive; how to optimize it? */
    if (sv_derived_from(sv, class)) {
        return SvRV(sv);
    }

    /* else check if parent (mg->mg_obj) is the right object type */
    if ((mg = mg_find(SvRV(sv), PERL_MAGIC_ext)) != NULL
        && (obj = mg->mg_obj) != NULL
        && SvOBJECT(obj))
    {
        sv = sv_2mortal(newRV_inc(obj));
        if (sv_derived_from(sv, class))
            return obj;
    }

    Perl_croak(aTHX_ "apreq_xs_sv2object: %s object not found", class);
    return NULL;
}

APR_INLINE
static apreq_handle_t *apreq_xs_sv2handle(pTHX_ SV *sv)
{
    SV *obj = apreq_xs_sv2object(aTHX_ sv, HANDLE_CLASS, 'r');
    IV iv = SvIVX(obj);
    return INT2PTR(apreq_handle_t *, iv);
}


static APR_INLINE
apreq_param_t *apreq_xs_sv2param(pTHX_ SV *sv)
{
    SV *obj = apreq_xs_sv2object(aTHX_ sv, PARAM_CLASS, 'p');
    IV iv = SvIVX(obj);
    return INT2PTR(apreq_param_t *, iv);
}

static APR_INLINE
apreq_cookie_t *apreq_xs_sv2cookie(pTHX_ SV *sv)
{
    SV *obj = apreq_xs_sv2object(aTHX_ sv, COOKIE_CLASS, 'c');
    IV iv = SvIVX(obj);
    return INT2PTR(apreq_cookie_t *, iv);
}



/** 
 * Searches a perl object ref with apreq_xs_find_obj
 * and produces a pointer to the underlying C environment.
 */ 

/**
 * Converts a C object, with environment, to a Perl object.
 * @param obj C object.
 * @param env C environment.
 * @param class Class perl object will be blessed into.
 * @param parent XXX
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

#define apreq_xs_2sv(t,class,parent)                    \
             apreq_xs_c2perl(aTHX_ t, env, class, parent)

#define apreq_xs_sv2(type,sv)((apreq_##type##_t *)      \
                               apreq_xs_perl2c(aTHX_ sv, #type))

#define apreq_xs_sv2env(sv) ((void *)SvIVX((apreq_xs_perl_sv2env(aTHX_ sv))))

/** Converts apreq_env to a Perl package, which forms the
 * base class for Apache::Request and Apache::Cookie::Jar objects.
 */
#define APREQ_XS_DEFINE_ENV(type)                       \
static XS(apreq_xs_##type##_env)                        \
{                                                       \
    char *class = NULL;                                 \
    dXSARGS;                                            \
    SV *sv, *obj;                                       \
    /* map environment to package */                    \
    if (items != 1)                                     \
        Perl_croak(aTHX_ "Usage: $obj->env");           \
                                                        \
    if (strcmp(apreq_env_name, "APACHE2") == 0)         \
        class = "Apache::RequestRec";                   \
    else if (strcmp(apreq_env_name, "CGI") == 0)        \
        class = "APR::Pool";                            \
                                                        \
    /* else if ... add more conditionals here as        \
       additional environments become supported */      \
                                                        \
    if (class == NULL)                                  \
        XSRETURN(0);                                    \
                                                        \
    XSprePUSH;                                          \
    if (SvROK(ST(0))) {                                 \
        obj = apreq_xs_find_obj(aTHX_ ST(0), #type);    \
        sv = apreq_xs_perl_sv2env(aTHX_ obj);           \
        XPUSHs(sv_2mortal(newRV_inc(sv)));              \
    }                                                   \
    else                                                \
        XPUSHs(sv_2mortal(newSVpv(class, 0)));          \
                                                        \
    XSRETURN(1);                                        \
}



#define APREQ_XS_DEFINE_CONFIG(attr)                                    \
static XS(apreq_xs_##attr##_config)                                     \
{                                                                       \
    dXSARGS;                                                            \
    SV *sv, *obj;                                                       \
    int j;                                                              \
                                                                        \
    if (items % 2 != 1 || !SvROK(ST(0)))                                \
        Perl_croak(aTHX_ "usage: $obj->config(%settings)");             \
                                                                        \
    sv = ST(0);                                                         \
    obj = apreq_xs_find_obj(aTHX_ sv, #attr);                           \
                                                                        \
    for (j = 1; j + 1 < items; j += 2) {                                \
        STRLEN alen;                                                    \
        const char *attr = SvPVbyte(ST(j),alen);                        \
                                                                        \
        if (strcasecmp(attr,"VALUE_CLASS") == 0)                        \
        {                                                               \
            STRLEN vlen;                                                \
            const char *val = SvPV(ST(j+1), vlen);                      \
            MAGIC *mg = mg_find(obj, PERL_MAGIC_ext);                   \
                                                                        \
            if (mg->mg_len > 0) {                                       \
                Safefree(mg->mg_ptr);                                   \
            }                                                           \
            mg->mg_ptr = savepvn(val, vlen);                            \
            mg->mg_len = vlen;                                          \
                                                                        \
        }                                                               \
        else {                                                          \
            Perl_warn(aTHX_ "$obj->config(%settings): "                 \
                      "Unrecognized attribute %s, skipped", attr);      \
        }                                                               \
    }                                                                   \
                                                                        \
    XSRETURN(0);                                                        \
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
    XSprePUSH;                                                          \
    XPUSHs(sv_2mortal(apreq_xs_##type##2sv(t,class,SvRV(ST(1)))));      \
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

#define APREQ_XS_THROW_ERROR(attr, status, func, errpkg)  do {          \
    if (!sv_derived_from(sv, errpkg)) {                                 \
        HV *hv = newHV();                                               \
        SV *rv = newRV_inc(obj);                                        \
        sv_setsv(*hv_fetch(hv, "_" #attr, 2, 1), sv_2mortal(rv));       \
        apreq_xs_croak(aTHX_ hv, status, func, errpkg);                 \
    }                                                                   \
} while (0)


static APR_INLINE
const char *apreq_xs_helper_class(pTHX_ SV **SP, SV *sv, const char *method)
{
        PUSHMARK(SP);
        XPUSHs(sv);
        PUTBACK;
        call_method(method, G_SCALAR);
        SPAGAIN;
        sv = POPs;
        PUTBACK;
        return SvPV_nolen(sv);
}



/** @} */

#endif /* APREQ_XS_POSTPERL_H */
