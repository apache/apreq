#ifndef APREQ_XS_POSTPERL_H
#define APREQ_XS_POSTPERL_H


/* conversion function template based on modperl-2's sv2request_rec */

#define APREQ_XS_DEFINE_SV_CONVERT(type, class)                         \
APR_INLINE static apreq_##type##_t *apreq_xs_sv2##type(pTHX_ SV* in)    \
{                                                                       \
    while (in && SvROK(in) & sv_derived_from(in, class)) {              \
        SV *sv = SvRV(sv);                                              \
        switch (SvTYPE(sv)) {                                           \
            SV **svp;                                                   \
        case SVt_PVHV:                                                  \
            if ((svp = hv_fetch((HV *)sv, #type, 1, FALSE)) ||          \
                (svp = hv_fetch((HV *)sv, "_" #type, 2, FALSE)))        \
            {                                                           \
                in = *svp;                                              \
                break;                                                  \
            }                                                           \
            Perl_croak(aTHX_  class "- derived `%s' object has no `"    \
                             #type "' key!", HvNAME(SvSTASH(sv)));      \
        case SVt_PVMG:                                                  \
            return (apreq_##type##_t *)SvIVX(sv);                       \
        default:                                                        \
            Perl_croak(aTHX_ "panic: unsupported apreq_" #type          \
                             "_t type \%d",                             \
                     SvTYPE(sv));                                       \
       }                                                                \
    }                                                                   \
    return NULL;                                                        \
}                                                                       \
APR_INLINE static SV *apreq_xs_##type##2sv(apreq_##type##_t *t)         \
{                                                                       \
    SV *sv = newSViv(0);                                                \
    SvUPGRADE(sv, SVt_PVIV);                                            \
    SvGROW(sv, sizeof *t);                                              \
    SvIVX(sv) = (IV)t;                                                  \
    SvIOK_on(sv);                                                       \
    SvPOK_on(sv);                                                       \
    sv_magic(sv, Nullsv, PERL_MAGIC_ext, (char *)t->env, 0);            \
                                                                        \
    /* initialize sv as an object */                                    \
    SvSTASH(sv) = gv_stashpv(class, TRUE);                              \
    SvOBJECT_on(sv);                                                    \
    return newRV_noinc(sv);                                             \
}


#define APREQ_XS_DEFINE_SV_TIE(type, class)                             \
static int apreq_xs_##type##_free(pTHX_ SV* sv, MAGIC *mg)              \
{                                                                       \
    /* need to prevent perl from freeing the apreq value */             \
    SvPVX(sv) = NULL;                                                   \
    SvCUR_set(sv,0);                                                    \
    SvPOK_off(sv);                                                      \
    return 0;                                                           \
}                                                                       \
const static MGVTBL apreq_xs_##type##_magic = {0, 0, 0, 0,              \
                                               apreq_xs_##type##_free };\
                                                                        \
APR_INLINE static SV *apreq_xs_##type##2sv(apreq_##type##_t *t)         \
{                                                                       \
    SV *sv = newSV(0);                                                  \
    SvUPGRADE(sv, SVt_PV);                                              \
    SvPVX(sv) = t->v.data;                                              \
    SvCUR_set(sv, t->v.size);                                           \
                                                                        \
    sv_magicext(sv, Nullsv, PERL_MAGIC_tiedscalar,                      \
                (MGVTBL *)&apreq_xs_##type##_magic, (char *)t, 0);      \
                                                                        \
    /* initialize sv as object, so "tied" will return object ref */     \
    SvSTASH(sv) = gv_stashpv(class, TRUE);                              \
    SvOBJECT_on(sv);                                                    \
                                                                        \
    SvREADONLY_on(sv);                                                  \
    SvTAINT(sv);                                                        \
    return sv;                                                          \
}                                                                       \
APR_INLINE                                                              \
static apreq_##type##_t *apreq_xs_sv2##type(pTHX_ SV *sv)               \
{                                                                       \
   return apreq_value_to_##type(apreq_strtoval(SvPVX(sv)));             \
}

#endif /* APREQ_XS_POSTPERL_H */
