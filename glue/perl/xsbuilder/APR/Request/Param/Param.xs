#include "apreq_xs_tables.h"

static APR_INLINE
SV *apreq_xs_param2sv(pTHX_ apreq_param_t *p, const char *class, SV *handle)
{
    SV *rv = sv_setref_pv(newSV(0), class, (void *)p);
    sv_magic(SvRV(rv), handle, PERL_MAGIC_ext, Nullch, 0);
    return rv;
}

static APR_INLINE
SV *apreq_xs_table2sv(pTHX_ const apr_table_t *t, const char *class, SV *handle)
{
    SV *sv = (SV *)newHV();
    SV *rv = sv_setref_pv(newSV(0), class, (void *)t);
    sv_magic(SvRV(rv), handle, PERL_MAGIC_ext, Nullch, 0);

#if (PERL_VERSION >= 8) /* MAGIC ITERATOR requires 5.8 */

    sv_magic(sv, NULL, PERL_MAGIC_ext, Nullch, -1);
    SvMAGIC(sv)->mg_virtual = (MGVTBL *)&apreq_xs_table_magic;
    SvMAGIC(sv)->mg_flags |= MGf_COPY;

#endif

    sv_magic(sv, rv, PERL_MAGIC_tied, Nullch, 0);
    SvREFCNT_dec(rv); /* corrects SvREFCNT_inc(rv) implicit in sv_magic */

    return sv_bless(newRV_noinc(sv), SvSTASH(SvRV(rv)));
}

static int apreq_xs_table_values(void *data, const char *key, const char *val)
{
    struct apreq_xs_do_arg *d = (struct apreq_xs_do_arg *)data;
    dTHXa(d->perl);
    dSP;
    apreq_param_t *p = apreq_value_to_param(val);
    SV *sv = apreq_xs_param2sv(aTHX_ p, d->pkg, d->parent);

    XPUSHs(sv_2mortal(sv));
    PUTBACK;
    return 1;
}

static XS(apreq_xs_args)
{
    dXSARGS;
    apreq_handle_t *req;
    const char *error_pkg  = "APR::Request::Error", 
               *table_pkg  = "APR::Request::Param::Table", 
               *elt_pkg    = "APR::Request::Param";
    SV *sv, *obj;
    IV iv;

    if (items == 0 || items > 2 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: APR::Request::args($req [,$name])");

    sv = ST(0);
    obj = apreq_xs_find_obj(aTHX_ sv, "r");
    iv = SvIVX(obj);
    req = INT2PTR(apreq_handle_t *, iv);


    if (items == 2 && GIMME_V == G_SCALAR) {
        apreq_param_t *c = apreq_args_get(req, SvPV_nolen(ST(1)));
        if (c != NULL) {
            ST(0) = apreq_xs_param2sv(aTHX_ c, elt_pkg, obj);
            sv_2mortal(ST(0));
            XSRETURN(1);
        }
        XSRETURN_UNDEF;
    }
    else {
        struct apreq_xs_do_arg d = {NULL, NULL, NULL, aTHX};
        const apr_table_t *t;
        apr_status_t s;

        s = apreq_args(req, &t);

        if (s != APR_SUCCESS && !sv_derived_from(sv, error_pkg))
            APREQ_XS_THROW_ERROR("r", s, "APR::Request::args", error_pkg);

        d.pkg = elt_pkg;
        d.parent = obj;

        switch (GIMME_V) {

        case G_ARRAY:
            XSprePUSH;
            PUTBACK;
            if (items == 1)
                apr_table_do(apreq_xs_table_keys, &d, t, NULL);
            else
                apr_table_do(apreq_xs_table_values, &d, t, 
                             SvPV_nolen(ST(1)), NULL);
            return;

        case G_SCALAR:
            ST(0) = apreq_xs_table2sv(aTHX_ t, table_pkg, obj);
            sv_2mortal(ST(0));
            XSRETURN(1);

        default:
           XSRETURN(0);
        }
    }
}

static XS(apreq_xs_body)
{
    dXSARGS;
    apreq_handle_t *req;
    const char *error_pkg  = "APR::Request::Error", 
               *table_pkg  = "APR::Request::Param::Table", 
               *elt_pkg    = "APR::Request::Param";
    SV *sv, *obj;
    IV iv;

    if (items == 0 || items > 2 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: APR::Request::body($req [,$name])");

    sv = ST(0);
    obj = apreq_xs_find_obj(aTHX_ sv, "r");
    iv = SvIVX(obj);
    req = INT2PTR(apreq_handle_t *, iv);


    if (items == 2 && GIMME_V == G_SCALAR) {
        apreq_param_t *c = apreq_body_get(req, SvPV_nolen(ST(1)));
        if (c != NULL) {
            ST(0) = apreq_xs_param2sv(aTHX_ c, elt_pkg, obj);
            sv_2mortal(ST(0));
            XSRETURN(1);
        }
        XSRETURN_UNDEF;
    }
    else {
        struct apreq_xs_do_arg d = {NULL, NULL, NULL, aTHX};
        const apr_table_t *t;
        apr_status_t s;

        s = apreq_body(req, &t);

        if (s != APR_SUCCESS && !sv_derived_from(sv, error_pkg))
            APREQ_XS_THROW_ERROR("r", s, "APR::Request::body", error_pkg);

        d.pkg = elt_pkg;
        d.parent = obj;

        switch (GIMME_V) {

        case G_ARRAY:
            XSprePUSH;
            PUTBACK;
            if (items == 1)
                apr_table_do(apreq_xs_table_keys, &d, t, NULL);
            else
                apr_table_do(apreq_xs_table_values, &d, t, 
                             SvPV_nolen(ST(1)), NULL);
            return;

        case G_SCALAR:
            ST(0) = apreq_xs_table2sv(aTHX_ t, table_pkg, obj);
            sv_2mortal(ST(0));
            XSRETURN(1);

        default:
           XSRETURN(0);
        }
    }
}
 
static XS(XS_APR__Request__Param_nil)
{
    dXSARGS;
    (void)items;
    XSRETURN_EMPTY;
}


MODULE = APR::Request::Param      PACKAGE = APR::Request::Param

SV *
value(obj, p1=NULL, p2=NULL)
    APR::Request::Param obj
    SV *p1
    SV *p2
  PREINIT:
    /*nada*/

  CODE:
    RETVAL = newSVpvn(obj->v.data, obj->v.size);

  OUTPUT:
    RETVAL



BOOT:
    /* register the overloading (type 'A') magic */
    PL_amagic_generation++;
    /* The magic for overload gets a GV* via gv_fetchmeth as */
    /* mentioned above, and looks in the SV* slot of it for */
    /* the "fallback" status. */
    sv_setsv(
        get_sv( "APR::Request::Param::()", TRUE ),
        &PL_sv_undef
    );
    newXS("APR::Request::Param::()", XS_APR__Request__Param_nil, file);
    newXS("APR::Request::Param::(\"\"", XS_APR__Request__Param_value, file);
