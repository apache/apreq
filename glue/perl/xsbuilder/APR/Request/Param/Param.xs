#include "apreq_xs_tables.h"
#define TABLE_CLASS "APR::Request::Param::Table"

static int apreq_xs_table_keys(void *data, const char *key, const char *val)
{
    struct apreq_xs_do_arg *d = (struct apreq_xs_do_arg *)data;
    dTHXa(d->perl);
    dSP;
    apreq_param_t *p = apreq_value_to_param(val);
    SV *sv = newSVpvn(key, p->v.nlen);
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
    SV *sv, *obj;
    IV iv;

    if (items == 0 || items > 2 || !SvROK(ST(0))
        || !sv_derived_from(ST(0), HANDLE_CLASS))
        Perl_croak(aTHX_ "Usage: APR::Request::args($req [,$name])");

    sv = ST(0);
    obj = apreq_xs_sv2object(aTHX_ sv, HANDLE_CLASS, 'r');
    iv = SvIVX(obj);
    req = INT2PTR(apreq_handle_t *, iv);


    if (items == 2 && GIMME_V == G_SCALAR) {
        apreq_param_t *p = apreq_args_get(req, SvPV_nolen(ST(1)));

        if (p != NULL) {
            ST(0) = apreq_xs_param2sv(aTHX_ p, NULL, obj);
            sv_2mortal(ST(0));
            XSRETURN(1);
        }
        else {
            const apr_table_t *t;
            apr_status_t s;
            s = apreq_args(req, &t);

            if (apreq_module_status_is_error(s))
                APREQ_XS_THROW_ERROR(r, s, "APR::Request::args", ERROR_CLASS);

            XSRETURN_UNDEF;
        }
    }
    else {
        struct apreq_xs_do_arg d = {NULL, NULL, NULL, aTHX};
        const apr_table_t *t;
        apr_status_t s;

        s = apreq_args(req, &t);

        if (apreq_module_status_is_error(s))
            APREQ_XS_THROW_ERROR(r, s, "APR::Request::args", ERROR_CLASS);

        if (t == NULL)
            XSRETURN_EMPTY;

        d.pkg = NULL;
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
            ST(0) = apreq_xs_table2sv(aTHX_ t, TABLE_CLASS, obj, NULL, 0);
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
    SV *sv, *obj;
    IV iv;

    if (items == 0 || items > 2 || !SvROK(ST(0))
        || !sv_derived_from(ST(0),HANDLE_CLASS))
        Perl_croak(aTHX_ "Usage: APR::Request::body($req [,$name])");

    sv = ST(0);
    obj = apreq_xs_sv2object(aTHX_ sv, HANDLE_CLASS, 'r');
    iv = SvIVX(obj);
    req = INT2PTR(apreq_handle_t *, iv);


    if (items == 2 && GIMME_V == G_SCALAR) {
        apreq_param_t *p = apreq_body_get(req, SvPV_nolen(ST(1)));

        if (p != NULL) {
            ST(0) = apreq_xs_param2sv(aTHX_ p, NULL, obj);
            sv_2mortal(ST(0));
            XSRETURN(1);
        }
        else {
            const apr_table_t *t;
            apr_status_t s;
            s = apreq_body(req, &t);

            if (apreq_module_status_is_error(s))
                APREQ_XS_THROW_ERROR(r, s, "APR::Request::body", ERROR_CLASS);

            XSRETURN_UNDEF;
        }
    }
    else {
        struct apreq_xs_do_arg d = {NULL, NULL, NULL, aTHX};
        const apr_table_t *t;
        apr_status_t s;

        s = apreq_body(req, &t);

        if (apreq_module_status_is_error(s))
            APREQ_XS_THROW_ERROR(r, s, "APR::Request::body", ERROR_CLASS);

        if (t == NULL)
            XSRETURN_EMPTY;

        d.pkg = NULL;
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
            ST(0) = apreq_xs_table2sv(aTHX_ t, TABLE_CLASS, obj, NULL, 0);
            sv_2mortal(ST(0));
            XSRETURN(1);

        default:
           XSRETURN(0);
        }
    }
}


static XS(apreq_xs_table_FETCH)
{
    dXSARGS;
    const apr_table_t *t;
    const char *param_class;
    SV *sv, *t_obj, *parent;
    IV iv;
    MAGIC *mg;

    if (items != 2 || !SvROK(ST(0))
        || !sv_derived_from(ST(0), TABLE_CLASS))
        Perl_croak(aTHX_ "Usage: " TABLE_CLASS "::FETCH($table, $key)");

    sv = ST(0);

    t_obj = apreq_xs_sv2object(aTHX_ sv, TABLE_CLASS, 't');
    iv = SvIVX(t_obj);
    t = INT2PTR(const apr_table_t *, iv);

    mg = mg_find(t_obj, PERL_MAGIC_ext);
    param_class = mg->mg_ptr;
    parent = mg->mg_obj;


    if (GIMME_V == G_SCALAR) {
        IV idx;
        const char *key, *val;
        const apr_array_header_t *arr;
        apr_table_entry_t *te;
        key = SvPV_nolen(ST(1));

        idx = SvCUR(t_obj);
        arr = apr_table_elts(t);
        te  = (apr_table_entry_t *)arr->elts;

        if (idx > 0 && idx <= arr->nelts
            && !strcasecmp(key, te[idx-1].key))
            val = te[idx-1].val;
        else
            val = apr_table_get(t, key);

        if (val != NULL) {
            apreq_param_t *p = apreq_value_to_param(val);
            ST(0) = apreq_xs_param2sv(aTHX_ p, param_class, parent);
            sv_2mortal(ST(0));
            XSRETURN(1);
        }
        else {
            XSRETURN_UNDEF;
        }
    }
    else if (GIMME_V == G_ARRAY) {
        struct apreq_xs_do_arg d = {NULL, NULL, NULL, aTHX};
        d.pkg = param_class;
        d.parent = parent;
        XSprePUSH;
        PUTBACK;
        apr_table_do(apreq_xs_table_values, &d, t, SvPV_nolen(ST(1)), NULL);
    }
    else
        XSRETURN(0);
}

static XS(apreq_xs_table_NEXTKEY)
{
    dXSARGS;
    SV *sv, *obj;
    IV iv, idx;
    const apr_table_t *t;
    const apr_array_header_t *arr;
    apr_table_entry_t *te;

    if (!SvROK(ST(0)) || !sv_derived_from(ST(0), TABLE_CLASS))
        Perl_croak(aTHX_ "Usage: " TABLE_CLASS "::NEXTKEY($table, $key)");

    sv  = ST(0);
    obj = apreq_xs_sv2object(aTHX_ sv, TABLE_CLASS,'t');

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
    RETVAL = apreq_xs_param2sv(aTHX_ obj, NULL, NULL);

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
    RETVAL = newSVpvn(obj->v.name, obj->v.nlen);
    if (apreq_param_is_tainted(obj))
        SvTAINTED_on(RETVAL);

  OUTPUT:
    RETVAL


IV
is_tainted(obj, val=NULL)
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

IV
is_utf8(obj, val=NULL)
    APR::Request::Param obj
    SV *val
  PREINIT:
    /*nada*/

  CODE:
    RETVAL = apreq_param_is_utf8(obj);

    if (items == 2) {
        if (SvTRUE(val))
           apreq_param_utf8_on(obj);
        else
           apreq_param_utf8_off(obj);
    }

  OUTPUT:
    RETVAL

APR::Request::Param
make(class, pool, name, val)
    apreq_xs_subclass_t class
    APR::Pool pool
    SV *name
    SV *val
  PREINIT:
    STRLEN nlen, vlen;
    const char *n, *v;
    SV *parent = SvRV(ST(1));

  CODE:
    n = SvPV(name, nlen);
    v = SvPV(val, vlen);
    RETVAL = apreq_param_make(pool, n, nlen, v, vlen);
    if (SvTAINTED(name) || SvTAINTED(val))
        apreq_param_taint_on(RETVAL);

  OUTPUT:
    RETVAL


MODULE = APR::Request::Param PACKAGE = APR::Request::Param::Table

SV *
param_class(t, newclass=NULL)
    APR::Request::Param::Table t
    char *newclass

  PREINIT:
    SV *obj = apreq_xs_sv2object(aTHX_ ST(0), TABLE_CLASS, 't');
    MAGIC *mg = mg_find(obj, PERL_MAGIC_ext);
    char *curclass = mg->mg_ptr;

  CODE:
    RETVAL = (curclass == NULL) ? &PL_sv_undef : newSVpv(curclass, 0);

    if (newclass != NULL) {
        if (!sv_derived_from(ST(1), PARAM_CLASS))
            Perl_croak(aTHX_ "Usage: " TABLE_CLASS "::param_class($table, $class): "
                             "class %s is not derived from " PARAM_CLASS, newclass);
        mg->mg_ptr = savepv(newclass);
        mg->mg_len = strlen(newclass);

        if (curclass != NULL)
            Safefree(curclass);
    }

  OUTPUT:
    RETVAL

MODULE = APR::Request::Param  PACKAGE = APR::Request

SV*
param(handle, key)
    SV *handle
    char *key
  PREINIT:
    SV *obj;
    IV iv;
    apreq_handle_t *req;
    apreq_param_t *param;

  CODE:
    obj = apreq_xs_sv2object(aTHX_ handle, HANDLE_CLASS, 'r');
    iv = SvIVX(obj);
    req = INT2PTR(apreq_handle_t *, iv);
    param = apreq_param(req, key);

    if (param == NULL)
        RETVAL = &PL_sv_undef;
    else
        RETVAL = apreq_xs_param2sv(aTHX_ param, NULL, obj);


  OUTPUT:
    RETVAL

SV *
params(handle, pool)
    SV *handle
    APR::Pool pool
  PREINIT:
    SV *obj;
    IV iv;
    apreq_handle_t *req;
    apr_table_t *t;

  CODE:
    obj = apreq_xs_sv2object(aTHX_ handle, HANDLE_CLASS, 'r');
    iv = SvIVX(obj);
    req = INT2PTR(apreq_handle_t *, iv);
    t = apreq_params(req, pool);
    RETVAL = apreq_xs_table2sv(aTHX_ t, TABLE_CLASS, obj, NULL, 0);

  OUTPUT:
    RETVAL
   
