#ifndef APREQ_XS_POSTPERL_H
#define APREQ_XS_POSTPERL_H

#define apreq_table_t apr_table_t
#define apreq_xs_class (SvROK(ST(0))  ? HvNAME(SvSTASH(ST(0))) : SvPV_nolen(ST(0)))
#define apreq_xs_value_class ( call_method("value_class", G_SCALAR), \
                                SvPV_nolen(ST(0)) )
#define apreq_xs_table_class ( call_method("table_class", G_SCALAR), \
                                SvPV_nolen(ST(0)) )

APR_INLINE
static XS(apreq_xs_env)
{
    dXSARGS;

    if (!SvROK(ST(0))) {
        ST(0) = newSVpv(apreq_env, 0);
    }
    else {
        MAGIC *mg;
        SV *sv = SvRV(ST(0));

        if (SvOBJECT(sv) && (mg = mg_find(sv, PERL_MAGIC_ext))) {
            ST(0) = sv_2mortal(sv_setref_pv(newSV(0), 
                                            apreq_env, mg->mg_ptr));
        }
        else
            ST(0) = &PL_sv_undef;
    }
    XSRETURN(1);
}

/* conversion function template based on modperl-2's sv2request_rec */

#define APREQ_XS_DEFINE_CONVERT(type)                                   \
APR_INLINE                                                              \
static apreq_##type##_t *apreq_xs_##type##_perl2c(pTHX_ SV* in)         \
{                                                                       \
    while (in && SvROK(in)) {                                           \
        SV *sv = SvRV(in);                                              \
        switch (SvTYPE(sv)) {                                           \
            SV **svp;                                                   \
        case SVt_PVHV:                                                  \
            if ((svp = hv_fetch((HV *)sv, #type, 1, FALSE)) ||          \
                (svp = hv_fetch((HV *)sv, "_" #type, 2, FALSE)))        \
            {                                                           \
                in = *svp;                                              \
                break;                                                  \
            }                                                           \
            Perl_croak(aTHX_  "%s object has no `"                      \
                             #type "' key!", HvNAME(SvSTASH(sv)));      \
        case SVt_PVMG:                                                  \
            if (SvOBJECT(sv) && SvIOK(sv))                              \
                return (apreq_##type##_t *)SvIVX(sv);                   \
        default:                                                        \
             Perl_croak(aTHX_ "panic: unsupported apreq_" #type         \
                             "_t type \%d",                             \
                     SvTYPE(sv));                                       \
       }                                                                \
    }                                                                   \
    return NULL;                                                        \
}                                                                       \
APR_INLINE                                                              \
static SV *apreq_xs_##type##_c2perl(pTHX_ apreq_##type##_t *t,          \
                                    void *env, const char *class)       \
{                                                                       \
    SV *rv = sv_setref_pv(newSV(0), class, t);                          \
    sv_magic(SvRV(rv), Nullsv, PERL_MAGIC_ext, env, 0);                 \
    return rv;                                                          \
}                                                                       \
                                                                        \
APR_INLINE                                                              \
static void *apreq_xs_##type##_perl2env(SV *sv)                         \
{                                                                       \
    MAGIC *mg = mg_find(SvROK(sv) ? SvRV(sv): sv, PERL_MAGIC_ext);      \
    return mg ? mg->mg_ptr : NULL;                                      \
}


/* requires type##2sv macro */

#define APREQ_XS_DEFINE_OBJECT(type)                                    \
static XS(apreq_xs_##type)                                              \
{                                                                       \
    dXSARGS;                                                            \
    void *env;                                                          \
    apr_pool_t *pool;                                                   \
    const char *class, *data;                                           \
    apreq_##type##_t *obj;                                              \
                                                                        \
    if (items < 2 || SvROK(ST(0)) || !SvROK(ST(1)))                     \
        Perl_croak(aTHX_ "Usage: $class->" #type "($env, $data)");      \
                                                                        \
    class = SvPV_nolen(ST(0));                                          \
    env = (void *)SvIVX(SvRV(ST(1)));                                   \
    data = (items == 3)  ?  SvPV_nolen(ST(2)) :  NULL;                  \
    obj = apreq_##type(env, data);                                      \
                                                                        \
    ST(0) = obj ? sv_2mortal(apreq_xs_##type##2sv(obj,class)) :         \
                  &PL_sv_undef;                                         \
    XSRETURN(1);                                                        \
}


/* requires definition of type##2sv macro */

#define APREQ_XS_DEFINE_MAKE(type)                                      \
static XS(apreq_xs_make_##type)                                         \
{                                                                       \
    dXSARGS;                                                            \
    void *env;                                                          \
    apr_pool_t *pool;                                                   \
    const char *class, *key, *val;                                      \
    STRLEN klen, vlen;                                                  \
    apreq_##type##_t *t;                                                \
                                                                        \
    if (items != 4 || SvROK(ST(0)) || !SvROK(ST(1)))                    \
        Perl_croak(aTHX_ "Usage: $class->make($env, $name, $val)");     \
                                                                        \
    class = SvPV_nolen(ST(0));                                          \
    env = (void *)SvIVX(SvRV(ST(1)));                                   \
    pool = apreq_env_pool(env);                                         \
    key = SvPV(ST(2), klen);                                            \
    val = SvPV(ST(3), vlen);                                            \
    t = apreq_make_##type(pool, key, klen, val, vlen);                  \
                                                                        \
    ST(0) = apreq_xs_##type##2sv(t, class);                             \
    XSRETURN(1);                                                        \
}  

struct do_arg {
    const char      *class;
    void            *env;
    PerlInterpreter *perl;
};

/* requires definition of type##2sv macro */

#define APREQ_XS_DEFINE_GET(type, subtype)                              \
static int apreq_xs_##type##_table_do(void *data, const char *key,      \
                               const char *val)                         \
{                                                                       \
    struct do_arg *d = (struct do_arg *)data;                           \
    void *env = d->env;                                                 \
    const char *class = d->class;                                       \
    dTHXa(d->perl);                                                     \
    dSP;                                                                \
    if (val)                                                            \
        XPUSHs(sv_2mortal(apreq_xs_##subtype##2sv(                      \
                   apreq_value_to_##subtype(apreq_strtoval(val)),       \
                   class)));                                            \
                                                                        \
    else                                                                \
        XPUSHs(&PL_sv_undef);                                           \
                                                                        \
    PUTBACK;                                                            \
    return 1;                                                           \
}                                                                       \
                                                                        \
static XS(apreq_xs_##type##_table_get)                                  \
{                                                                       \
    dXSARGS;                                                            \
    const char *key = NULL;                                             \
                                                                        \
    if (items == 1 || items == 2) {                                     \
        SV *sv = ST(0);                                                 \
        void *env = apreq_xs_##type##_perl2env(sv);                     \
        const char *class = apreq_xs_value_class;                       \
        struct do_arg d = { class, env, aTHX };                         \
        apr_table_t *t  = apreq_xs_##type##_sv2table(sv);               \
                                                                        \
        if (items == 2)                                                 \
            key = SvPV_nolen(ST(1));                                    \
                                                                        \
        if (t == NULL)                                                  \
            Perl_croak(aTHX_ "Usage: $table->get($key)");               \
                                                                        \
        switch (GIMME_V) {                                              \
            const char *val;                                            \
                                                                        \
        case G_ARRAY:                                                   \
            XSprePUSH;                                                  \
            PUTBACK;                                                    \
            apr_table_do(apreq_xs_##type##_table_do, &d, t, key, NULL); \
            break;                                                      \
                                                                        \
        case G_SCALAR:                                                  \
            val = apr_table_get(t, key);                                \
            if (val == NULL)                                            \
                XSRETURN_UNDEF;                                         \
            ST(0) = sv_2mortal(apreq_xs_##subtype##2sv(                 \
                        apreq_value_to_##subtype(                       \
                            apreq_strtoval(val)), class));              \
            XSRETURN(1);                                                \
                                                                        \
        default:                                                        \
            XSRETURN(0);                                                \
        }                                                               \
    }                                                                   \
    else                                                                \
	Perl_croak(aTHX_ "Usage: $table->get($key)");                   \
}

/* requires sv2##type, type##2env & type##2##subtype macros */

#define APREQ_XS_DEFINE_TABLE(type, subtype)                    \
static XS(apreq_xs_##type##_##subtype)                          \
{                                                               \
    dXSARGS;                                                    \
    apreq_##type##_t *obj;                                      \
    void *env;                                                  \
    SV *sv;                                                     \
                                                                \
    if (items != 1)                                             \
        Perl_croak(aTHX_ "Usage: " #type "->" #subtype "()");   \
                                                                \
    sv = ST(0);                                                 \
    obj = apreq_xs_sv2##type(sv);                               \
    env = apreq_xs_##type##2env(obj);                           \
    ST(0) = apreq_xs_table2sv(apreq_xs_##type##2##subtype(obj), \
                              apreq_xs_table_class);            \
    XSRETURN(1);                                                \
}



#endif /* APREQ_XS_POSTPERL_H */
