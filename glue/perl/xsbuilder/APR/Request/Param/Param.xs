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

static int apreq_xs_table_keys(void *data, const char *key, const char *val)
{
    struct apreq_xs_do_arg *d = (struct apreq_xs_do_arg *)data;
    dTHXa(d->perl);
    dSP;
    apreq_param_t *p = apreq_value_to_param(val);
    SV *sv = newSVpv(key, 0);
    if (apreq_param_is_tainted(p))
        SvTAINTED_on(sv);

    XPUSHs(sv_2mortal(sv));
    PUTBACK;
    return 1;
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
    iv = SvIVX(SvRV(obj));
    req = INT2PTR(apreq_handle_t *, iv);


    if (items == 2 && GIMME_V == G_SCALAR) {
        apreq_param_t *p = apreq_args_get(req, SvPV_nolen(ST(1)));

        if (p != NULL) {
            ST(0) = apreq_xs_param2sv(aTHX_ p, elt_pkg, obj);
            sv_2mortal(ST(0));
            XSRETURN(1);
        }
        else {
            const apr_table_t *t;
            apr_status_t s;
            s = apreq_args(req, &t);

            if (apreq_module_status_is_error(s))
                APREQ_XS_THROW_ERROR(r, s, "APR::Request::args", error_pkg);

            XSRETURN_UNDEF;
        }
    }
    else {
        struct apreq_xs_do_arg d = {NULL, NULL, NULL, aTHX};
        const apr_table_t *t;
        apr_status_t s;

        s = apreq_args(req, &t);

        if (apreq_module_status_is_error(s))
            APREQ_XS_THROW_ERROR(r, s, "APR::Request::args", error_pkg);

        if (t == NULL)
            XSRETURN_EMPTY;

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
    iv = SvIVX(SvRV(obj));
    req = INT2PTR(apreq_handle_t *, iv);


    if (items == 2 && GIMME_V == G_SCALAR) {
        apreq_param_t *p = apreq_body_get(req, SvPV_nolen(ST(1)));

        if (p != NULL) {
            ST(0) = apreq_xs_param2sv(aTHX_ p, elt_pkg, obj);
            sv_2mortal(ST(0));
            XSRETURN(1);
        }
        else {
            const apr_table_t *t;
            apr_status_t s;
            s = apreq_body(req, &t);

            if (apreq_module_status_is_error(s))
                APREQ_XS_THROW_ERROR(r, s, "APR::Request::body", error_pkg);

            XSRETURN_UNDEF;
        }
    }
    else {
        struct apreq_xs_do_arg d = {NULL, NULL, NULL, aTHX};
        const apr_table_t *t;
        apr_status_t s;

        s = apreq_body(req, &t);

        if (apreq_module_status_is_error(s))
            APREQ_XS_THROW_ERROR(r, s, "APR::Request::body", error_pkg);

        if (t == NULL)
            XSRETURN_EMPTY;

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


static XS(apreq_xs_table_get)
{
    dXSARGS;
    const apr_table_t *t;
    apreq_handle_t *req;
    const char *elt_pkg = "APR::Request::Param";
    SV *sv, *t_obj, *r_obj;
    IV iv;

    if (items == 0 || items > 2 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: APR::Request::body($req [,$name])");

    sv = ST(0);

    t_obj = apreq_xs_find_obj(aTHX_ sv, "param");
    iv = SvIVX(SvRV(t_obj));
    t = INT2PTR(const apr_table_t *, iv);

    r_obj = apreq_xs_find_obj(aTHX_ t_obj, "request");
    iv = SvIVX(SvRV(r_obj));
    req = INT2PTR(apreq_handle_t *, iv);

    if (items == 2 && GIMME_V == G_SCALAR) {
        const char *v = apr_table_get(t, SvPV_nolen(ST(1)));

        if (v != NULL) {
            ST(0) = apreq_xs_param2sv(aTHX_ apreq_value_to_param(v), elt_pkg, r_obj);
            sv_2mortal(ST(0));
            XSRETURN(1);
        }
        else {
            XSRETURN_UNDEF;
        }
    }
    else if (GIMME_V == G_ARRAY) {
        struct apreq_xs_do_arg d = {NULL, NULL, NULL, aTHX};

        d.pkg = elt_pkg;
        d.parent = r_obj;
        XSprePUSH;
        PUTBACK;
        if (items == 1)
            apr_table_do(apreq_xs_table_keys, &d, t, NULL);
        else
            apr_table_do(apreq_xs_table_values, &d, t, 
                         SvPV_nolen(ST(1)), NULL);
    }
    else
        XSRETURN(0);
}

static XS(apreq_xs_table_FETCH)
{
    dXSARGS;
    SV *sv, *t_obj, *r_obj;
    IV iv, idx;
    const char *key, *pkg;
    const char *val;
    const apr_table_t *t;
    const apr_array_header_t *arr;
    apr_table_entry_t *te;
    apreq_handle_t *req;

    if (items != 2 || !SvROK(ST(0)) || !SvOK(ST(1)))
        Perl_croak(aTHX_ "Usage: $table->FETCH($key)");

    sv  = ST(0);
    t_obj = apreq_xs_find_obj(aTHX_ sv, "param");
    iv = SvIVX(SvRV(t_obj));
    t = INT2PTR(const apr_table_t *, iv);

    r_obj = apreq_xs_find_obj(aTHX_ t_obj, "request");
    iv = SvIVX(SvRV(r_obj));
    req = INT2PTR(apreq_handle_t *, iv);

    pkg = "APR::Request::Param";

    key = SvPV_nolen(ST(1));
    idx = SvCUR(SvRV(r_obj));
    arr = apr_table_elts(t);
    te  = (apr_table_entry_t *)arr->elts;

    if (idx > 0 && idx <= arr->nelts
        && !strcasecmp(key, te[idx-1].key))
        val = te[idx-1].val;
    else
        val = apr_table_get(t, key);

    if (val != NULL) {
        ST(0) = apreq_xs_param2sv(aTHX_ apreq_value_to_param(val), pkg, r_obj);
        sv_2mortal(ST(0));
        XSRETURN(1);
    }
    else
        XSRETURN_UNDEF;
}

static XS(apreq_xs_table_NEXTKEY)
{
    dXSARGS;
    SV *sv, *obj;
    IV iv, idx;
    const apr_table_t *t;
    const apr_array_header_t *arr;
    apr_table_entry_t *te;

    if (!SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $table->NEXTKEY($prev)");

    sv  = ST(0);
    obj = apreq_xs_find_obj(aTHX_ sv, "param");
    obj = SvRV(obj);

    iv = SvIVX(obj);
    t = INT2PTR(const apr_table_t *, iv);
    arr = apr_table_elts(t);
    te  = (apr_table_entry_t *)arr->elts;

    if (items == 1)
        SvCUR(obj) = 0;

    if (SvCUR(obj) >= arr->nelts) {
        SvCUR(obj) = 0;
        XSRETURN_UNDEF;
    }
    idx = SvCUR(obj)++;
    sv = newSVpv(te[idx].key, 0);
    ST(0) = sv_2mortal(sv);
    XSRETURN(1);
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
    if (apreq_param_is_tainted(obj))
        SvTAINTED_on(RETVAL);

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
        &PL_sv_yes
    );
    newXS("APR::Request::Param::()", XS_APR__Request__Param_nil, file);
    newXS("APR::Request::Param::(\"\"", XS_APR__Request__Param_value, file);


MODULE = APR::Request::Param   PACKAGE = APR::Request::Param

SV *
name(obj)
    APR::Request::Param obj

  CODE:
    RETVAL = newSVpv(obj->v.name, 0);
    if (apreq_param_is_tainted(obj))
        SvTAINTED_on(RETVAL);

  OUTPUT:
    RETVAL


IV
tainted(obj, val=NULL)
    APR::Request::Param obj
    SV *val
  PREINIT:
    /*nada*/

  CODE:
    RETVAL = apreq_param_is_tainted(obj);

    if (items == 2) {
        if (SvTRUE(val))
           apreq_param_taint_on(obj);
        else
           apreq_param_taint_off(obj);
    }

  OUTPUT:
    RETVAL
