#ifndef APREQ_XS_POSTPERL_H
#define APREQ_XS_POSTPERL_H


APR_INLINE
static SV *apreq_xs_find_obj(SV *in, const char *key)
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

APR_INLINE
static void *apreq_xs_perl2c(SV* in, const char *name)
{
    SV *sv = apreq_xs_find_obj(in, name);
    if (sv == NULL)
        return NULL;
    else 
        return (void *)SvIVX(sv);
}

APR_INLINE
static void *apreq_xs_perl2env(SV *sv, const char *name)
{
    MAGIC *mg;
    sv = apreq_xs_find_obj(sv, name);
    if (sv != NULL && (mg = mg_find(sv, PERL_MAGIC_ext)))
        return mg->mg_ptr;
    return NULL;
}

APR_INLINE
static SV *apreq_xs_c2perl(pTHX_ void *obj, void *env, const char *class)
{
    SV *rv = sv_setref_pv(newSV(0), class, obj);
    if (env)
        sv_magic(SvRV(rv), Nullsv, PERL_MAGIC_ext, env, 0);
    return rv;
}

APR_INLINE
static SV *apreq_xs_table_c2perl(pTHX_ void *obj, void *env, 
                                 const char *class)
{
    SV *sv = (SV *)newHV();
    SV *rv = sv_setref_pv(newSV(0), class, obj);
    if (env)
        sv_magic(SvRV(rv), Nullsv, PERL_MAGIC_ext, env, 0);

    sv_magic(sv, rv, PERL_MAGIC_tied, Nullch, 0);
    SvREFCNT_dec(rv); /* corrects SvREFCNT_inc(rv) implicit in sv_magic */

    return sv_bless(newRV_noinc(sv), SvSTASH(SvRV(rv)));
}


#define apreq_xs_2sv(t,class) apreq_xs_c2perl(aTHX_ t, env, class)
#define apreq_xs_sv2(type,sv)((apreq_##type##_t *)apreq_xs_perl2c(sv, #type))
#define apreq_xs_sv2env(type,sv) apreq_xs_perl2env(sv,#type)

#define APREQ_XS_DEFINE_ENV(type)                       \
APR_INLINE                                              \
static XS(apreq_xs_##type##_env)                        \
{                                                       \
    char *class = NULL;                                 \
    dXSARGS;                                            \
    /* map environment to package */                    \
                                                        \
    if (strcmp(apreq_env, "APACHE2") == 0)              \
        class = "Apache::RequestRec";                   \
                                                        \
    /* else if ... add more conditionals here as        \
       additional environments become supported */      \
                                                        \
    if (class == NULL)                                  \
        XSRETURN(0);                                    \
                                                        \
    if (SvROK(ST(0))) {                                 \
        void *env = apreq_xs_sv2env(type, ST(0));       \
                                                        \
        if (env)                                        \
            ST(0) = sv_setref_pv(newSV(0), class, env); \
        else                                            \
            ST(0) = &PL_sv_undef;                       \
    }                                                   \
    else                                                \
        ST(0) = newSVpv(class, 0);                      \
                                                        \
    XSRETURN(1);                                        \
}


/* requires type##2sv macro */

#define APREQ_XS_DEFINE_OBJECT(type,class)                              \
static XS(apreq_xs_##type)                                              \
{                                                                       \
    dXSARGS;                                                            \
    void *env;                                                          \
    apr_pool_t *pool;                                                   \
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
    ST(0) = obj ? sv_2mortal(apreq_xs_2sv(obj,class)) :                 \
                  &PL_sv_undef;                                         \
    XSRETURN(1);                                                        \
}


/* requires definition of apreq_xs_##type##2sv(t,class) macro */

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
    key = SvPV(ST(2), klen);                                            \
    val = SvPV(ST(3), vlen);                                            \
    t = apreq_make_##type(pool, key, klen, val, vlen);                  \
                                                                        \
    ST(0) = sv_2mortal(apreq_xs_##type##2sv(t,class));                  \
    XSRETURN(1);                                                        \
}  

struct apreq_xs_do_arg {
    void            *env;
    PerlInterpreter *perl;
};

static int apreq_xs_table_keys(void *data, const char *key,
                               const char *val)
{
    struct apreq_xs_do_arg *d = (struct apreq_xs_do_arg *)data;
    void *env = d->env;
    dTHXa(d->perl);
    dSP;
    if (key)
        XPUSHs(sv_2mortal(newSVpv(key,0)));
    else
        XPUSHs(&PL_sv_undef);

    PUTBACK;
    return 1;
}

/* requires definition of type##2sv macro */
#define apreq_table_t apr_table_t
#define apreq_xs_table_sv2table(sv) apreq_xs_sv2(table,sv)

#define APREQ_XS_DEFINE_GET(type, subtype, subclass)                    \
static int apreq_xs_##type##_table_values(void *data, const char *key,  \
                                          const char *val)              \
{                                                                       \
    struct apreq_xs_do_arg *d = (struct apreq_xs_do_arg *)data;         \
    void *env = d->env;                                                 \
    dTHXa(d->perl);                                                     \
    dSP;                                                                \
    if (val)                                                            \
        XPUSHs(sv_2mortal(apreq_xs_##subtype##2sv(                      \
            apreq_value_to_##subtype(apreq_strtoval(val)), subclass))); \
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
        apr_table_t *t  = apreq_xs_##type##_sv2table(ST(0));            \
        void *env = apreq_xs_sv2env(type, ST(0));                       \
        struct apreq_xs_do_arg d = { env, aTHX };                       \
                                                                        \
        if (items == 2)                                                 \
            key = SvPV_nolen(ST(1));                                    \
                                                                        \
        if (t == NULL)                                                  \
            Perl_croak(aTHX_ "usage: $table->get($key)");               \
                                                                        \
        switch (GIMME_V) {                                              \
            const char *val;                                            \
                                                                        \
        case G_ARRAY:                                                   \
            XSprePUSH;                                                  \
            PUTBACK;                                                    \
            apr_table_do(items == 1 ? apreq_xs_table_keys :             \
                apreq_xs_##type##_table_values, &d, t, key, NULL);      \
            break;                                                      \
                                                                        \
        case G_SCALAR:                                                  \
            if (items == 1) {                                           \
               ST(0) = sv_2mortal(apreq_xs_table2sv(t));                \
               XSRETURN(1);                                             \
            }                                                           \
                                                                        \
            val = apr_table_get(t, key);                                \
            if (val == NULL)                                            \
                XSRETURN_UNDEF;                                         \
            ST(0) = sv_2mortal(apreq_xs_##subtype##2sv(                 \
              apreq_value_to_##subtype(apreq_strtoval(val)),subclass)); \
            XSRETURN(1);                                                \
                                                                        \
        default:                                                        \
            XSRETURN(0);                                                \
        }                                                               \
    }                                                                   \
    else                                                                \
	Perl_croak(aTHX_ "Usage: $table->get($key)");                   \
}

/* requires type##2env & type##2##subtype macros */

#define APREQ_XS_DEFINE_TABLE(type, subtype)                            \
static XS(apreq_xs_##type##_##subtype)                                  \
{                                                                       \
    dXSARGS;                                                            \
    apreq_##type##_t *obj;                                              \
    apr_table_t *t;                                                     \
    void *env;                                                          \
    SV *sv;                                                             \
                                                                        \
    if (items != 1)                                                     \
        Perl_croak(aTHX_ "Usage: " #type "->" #subtype "()");           \
                                                                        \
    sv = ST(0);                                                         \
    obj = apreq_xs_sv2(type, sv);                                       \
    env = apreq_xs_sv2env(type, sv);                                    \
    t   = apreq_xs_##type##2##subtype(obj);                             \
    ST(0) = sv_2mortal(apreq_xs_table2sv(t));                           \
    XSRETURN(1);                                                        \
}



#endif /* APREQ_XS_POSTPERL_H */
