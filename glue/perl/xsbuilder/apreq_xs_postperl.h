#ifndef APREQ_XS_POSTPERL_H
#define APREQ_XS_POSTPERL_H


/* conversion function template based on modperl-2's sv2request_rec */

#define APREQ_XS_DEFINE_SV2(type)                                        \
static                                                                   \
SV *apreq_xs_hv_find_##type(pTHX_ SV *in)                                \
{                                                                        \
    static char *keys[] = { #type, "_" #type, NULL };                    \
    HV *hv = (HV *)SvRV(in);                                             \
    SV *sv = Nullsv;                                                     \
    int i;                                                               \
                                                                         \
    for (i=0; keys[i]; i++) {                                            \
        int klen = i + 1; /* assumes keys[] will never change */         \
        SV **svp;                                                        \
                                                                         \
        if ((svp = hv_fetch(hv, keys[i], klen, FALSE)) && (sv = *svp)) { \
            if (SvROK(sv) && (SvTYPE(SvRV(sv)) == SVt_PVHV)) {           \
                /* dig deeper */                                         \
                return apreq_xs_hv_find_##type(aTHX_ sv);                \
            }                                                            \
            break;                                                       \
        }                                                                \
    }                                                                    \
                                                                         \
    if (!sv) {                                                           \
        Perl_croak(aTHX_                                                 \
                   "`%s' object hash no `" #type "' key!",               \
                   HvNAME(SvSTASH(SvRV(in))));                           \
    }                                                                    \
                                                                         \
    return SvROK(sv) ? SvRV(sv) : sv;                                    \
}                                                                        \
APR_INLINE                                                               \
static                                                                   \
apreq_##type##_t *apreq_xs_sv2##type(pTHX_ SV* in)                       \
{                                                                        \
    SV *sv = Nullsv;                                                     \
    MAGIC *mg;                                                           \
                                                                         \
    if (SvROK(in)) {                                                     \
        SV *rv = (SV*)SvRV(in);                                          \
        switch (SvTYPE(rv)) {                                            \
        case SVt_PVHV:                                                   \
            rv = apreq_xs_hv_find_##type(aTHX_ in);                      \
            if (SvTYPE(rv) != SVt_PVIV)                                  \
                break;                                                   \
        case SVt_PVIV:                                                   \
            return (apreq_##type##_t *)SvIVX(rv);                        \
        default:                                                         \
            Perl_croak(aTHX_ "panic: unsupported apreq_" #type           \
                             "_t type \%d",                              \
                     SvTYPE(rv));                                        \
       }                                                                 \
    }                                                                    \
    return NULL;                                                         \
}                                                                        \
APR_INLINE                                                               \
static                                                                   \
SV *apreq_xs_##type##2sv(apreq_##type##_t *t)                            \
{                                                                        \
    SV *sv = newSViv(0);                                                 \
    SvUPGRADE(sv, SVt_PVIV);                                             \
    SvGROW(sv, sizeof *t);                                               \
    SvIVX(sv) = (IV)t;                                                   \
    SvIOK_on(sv);                                                        \
    SvPOK_on(sv);                                                        \
    sv_magic(sv, Nullsv, PERL_MAGIC_ext, (char *)t->env, 0);             \
    return newRV_noinc(sv);                                              \
}


#define APREQ_XS_DEFINE_TIEDSV(type, class)                              \
static int apreq_xs_##type##_free(SV* sv, MAGIC *mg);                    \
const static MGVTBL apreq_xs_##type##_magic = { 0, 0, 0, 0,              \
                                               apreq_xs_##type##_free }; \
                                                                         \
static int apreq_xs_##type##_free(SV* sv, MAGIC *mg)                     \
{                                                                        \
    /* need to prevent perl from freeing the apreq value */              \
    SvPVX(sv) = NULL;                                                    \
    SvCUR_set(sv,0);                                                     \
    SvPOK_off(sv);                                                       \
    return 0;                                                            \
}                                                                        \
                                                                         \
                                                                         \
APR_INLINE                                                               \
static SV *apreq_xs_##type##2sv(apreq_##type##_t *t)                     \
{                                                                        \
    SV *sv = newSV(0);                                                   \
    SvUPGRADE(sv, SVt_PV);                                               \
    SvPVX(sv) = t->v.data;                                               \
    SvCUR_set(sv, t->v.size);                                            \
                                                                         \
    sv_magicext(sv, Nullsv, PERL_MAGIC_tiedscalar,                       \
                (MGVTBL *)&apreq_xs_##type##_magic, (char *)t, 0);       \
                                                                         \
    /* initialize sv as an object, so "tied" will return object ref */   \
    SvSTASH(sv) = gv_stashpv(class, TRUE);                               \
    SvOBJECT_on(sv);                                                     \
                                                                         \
    SvREADONLY_on(sv);                                                   \
    SvTAINT(sv);                                                         \
    return sv;                                                           \
}                                                                        \
APR_INLINE                                                               \
static apreq_##type##_t *apreq_xs_sv2##type(pTHX_ SV *sv)                \
{                                                                        \
   return apreq_value_to_##type(apreq_strtoval(SvPVX(sv)));              \
}

#endif /* APREQ_XS_POSTPERL_H */
