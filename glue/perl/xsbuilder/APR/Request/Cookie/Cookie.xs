#include "apreq_xs_tables.h"
#define TABLE_CLASS "APR::Request::Cookie::Table"

static int apreq_xs_table_keys(void *data, const char *key, const char *val)
{
    struct apreq_xs_do_arg *d = (struct apreq_xs_do_arg *)data;
    dTHXa(d->perl);
    dSP;
    apreq_cookie_t *c = apreq_value_to_cookie(val);
    SV *sv = newSVpv(key, 0);
    if (apreq_cookie_is_tainted(c))
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
    apreq_cookie_t *c = apreq_value_to_cookie(val);
    SV *sv = apreq_xs_cookie2sv(aTHX_ c, d->pkg, d->parent);

    XPUSHs(sv_2mortal(sv));
    PUTBACK;
    return 1;
}

static XS(apreq_xs_jar)
{
    dXSARGS;
    apreq_handle_t *req;
    SV *sv, *obj;
    IV iv;

    if (items == 0 || items > 2 || !SvROK(ST(0))
        || !sv_derived_from(ST(0), "APR::Request"))
        Perl_croak(aTHX_ "Usage: APR::Request::jar($req [,$name])");

    sv = ST(0);
    obj = apreq_xs_sv2object(aTHX_ sv, HANDLE_CLASS, 'r');
    iv = SvIVX(obj);
    req = INT2PTR(apreq_handle_t *, iv);

    if (items == 2 && GIMME_V == G_SCALAR) {
        apreq_cookie_t *c = apreq_jar_get(req, SvPV_nolen(ST(1)));
        if (c != NULL) {
            ST(0) = apreq_xs_cookie2sv(aTHX_ c, COOKIE_CLASS, obj);
            sv_2mortal(ST(0));
            XSRETURN(1);
        }
        else {
            const apr_table_t *t;
            apr_status_t s;

            s = apreq_jar(req, &t);
            if (apreq_module_status_is_error(s))
                APREQ_XS_THROW_ERROR(r, s, "APR::Request::jar", ERROR_CLASS);

            XSRETURN_UNDEF;
        }
    }
    else {
        struct apreq_xs_do_arg d = {NULL, NULL, NULL, aTHX};
        const apr_table_t *t;
        apr_status_t s;

        s = apreq_jar(req, &t);

        if (apreq_module_status_is_error(s))
            APREQ_XS_THROW_ERROR(r, s, "APR::Request::jar", ERROR_CLASS);

        if (t == NULL)
            XSRETURN_EMPTY;

        d.pkg = COOKIE_CLASS;
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
            ST(0) = apreq_xs_table2sv(aTHX_ t, TABLE_CLASS, obj,
                                      COOKIE_CLASS, sizeof(COOKIE_CLASS)-1);
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
    const char *cookie_class;
    SV *sv, *obj, *parent;
    IV iv;
    MAGIC *mg;

    if (items != 2 || !SvROK(ST(0))
        || !sv_derived_from(ST(0), TABLE_CLASS))
        Perl_croak(aTHX_ "Usage: " TABLE_CLASS "::FETCH($table, $key)");

    sv = ST(0);

    obj = apreq_xs_sv2object(aTHX_ sv, TABLE_CLASS, 't');
    iv = SvIVX(obj);
    t = INT2PTR(const apr_table_t *, iv);

    mg = mg_find(obj, PERL_MAGIC_ext);
    cookie_class = mg->mg_ptr;
    parent = mg->mg_obj;

    if (GIMME_V == G_SCALAR) {
        IV idx;
        const char *key, *val;
        const apr_array_header_t *arr;
        apr_table_entry_t *te;
        key = SvPV_nolen(ST(1));

        idx = SvCUR(obj);
        arr = apr_table_elts(t);
        te  = (apr_table_entry_t *)arr->elts;

        if (idx > 0 && idx <= arr->nelts
            && !strcasecmp(key, te[idx-1].key))
            val = te[idx-1].val;
        else
            val = apr_table_get(t, key);

        if (val != NULL) {
            apreq_cookie_t *c = apreq_value_to_cookie(val);
            ST(0) = apreq_xs_cookie2sv(aTHX_ c, cookie_class, parent);
            sv_2mortal(ST(0));
            XSRETURN(1);
        }
        else {
            XSRETURN_UNDEF;
        }
    }
    else if (GIMME_V == G_ARRAY) {
        struct apreq_xs_do_arg d = {NULL, NULL, NULL, aTHX};
        d.pkg = cookie_class;
        d.parent = parent;
        XSprePUSH;
        PUTBACK;
        apr_table_do(apreq_xs_table_values, &d, t, 
                     SvPV_nolen(ST(1)), NULL);
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

    if (!SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $table->NEXTKEY($prev)");

    sv  = ST(0);
    obj = apreq_xs_sv2object(aTHX_ sv, TABLE_CLASS, 't');

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


static XS(XS_APR__Request__Cookie_nil)
{
    dXSARGS;
    (void)items;
    XSRETURN_EMPTY;
}


MODULE = APR::Request::Cookie      PACKAGE = APR::Request::Cookie

SV *
value(obj, p1=NULL, p2=NULL)
    APR::Request::Cookie obj
    SV *p1
    SV *p2
  PREINIT:
    /*nada*/

  CODE:
    RETVAL = newSVpvn(obj->v.data, obj->v.size);
    if (apreq_cookie_is_tainted(obj))
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
        get_sv( "APR::Request::Cookie::()", TRUE ),
        &PL_sv_yes
    );
    newXS("APR::Request::Cookie::()", XS_APR__Request__Cookie_nil, file);
    newXS("APR::Request::Cookie::(\"\"", XS_APR__Request__Cookie_value, file);


MODULE = APR::Request::Cookie   PACKAGE = APR::Request::Cookie

SV *
name(obj)
    APR::Request::Cookie obj

  CODE:
    RETVAL = newSVpv(obj->v.name, 0);
    if (apreq_cookie_is_tainted(obj))
        SvTAINTED_on(RETVAL);

  OUTPUT:
    RETVAL

UV
secure(obj, val=NULL)
    APR::Request::Cookie obj
    SV *val

  CODE:
    RETVAL = apreq_cookie_is_secure(obj);
    if (items == 2) {
        if (SvTRUE(val))
            apreq_cookie_secure_on(obj);
        else
            apreq_cookie_secure_off(obj);
    }

  OUTPUT:
    RETVAL

UV
version(obj, val=0)
    APR::Request::Cookie obj
    UV val

  CODE:
    RETVAL = apreq_cookie_version(obj);
    if (items == 2)
        apreq_cookie_version_set(obj, val);
 
  OUTPUT:
    RETVAL

IV
tainted(obj, val=NULL)
    APR::Request::Cookie obj
    SV *val
  PREINIT:
    /*nada*/

  CODE:
    RETVAL = apreq_cookie_is_tainted(obj);

    if (items == 2) {
        if (SvTRUE(val))
           apreq_cookie_taint_on(obj);
        else
           apreq_cookie_taint_off(obj);
    }

  OUTPUT:
    RETVAL

SV*
bind_handle(cookie, req)
    SV *cookie
    SV *req
  PREINIT:
    MAGIC *mg;
    SV *obj;
  CODE:
    obj = apreq_xs_sv2object(aTHX_ cookie, COOKIE_CLASS, 'c');
    mg = mg_find(obj, PERL_MAGIC_ext);
    req = apreq_xs_sv2object(aTHX_ req, HANDLE_CLASS, 'r');
    RETVAL = newRV_noinc(mg->mg_obj);
    SvREFCNT_inc(req);
    mg->mg_obj = req;

  OUTPUT:
    RETVAL

APR::Request::Cookie
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
    RETVAL = apreq_cookie_make(pool, n, nlen, v, vlen);
    if (SvTAINTED(name) || SvTAINTED(val))
        apreq_cookie_taint_on(RETVAL);

  OUTPUT:
    RETVAL

SV *
as_string(c)
    APR::Request::Cookie c
  PREINIT:
    char rv[APREQ_COOKIE_MAX_LENGTH];
    STRLEN len;

  CODE:
    len = apreq_cookie_serialize(c, rv, sizeof rv);
    RETVAL = newSVpvn(rv, len);
    if (apreq_cookie_is_tainted(c))
        SvTAINTED_on(RETVAL);

  OUTPUT:
    RETVAL

MODULE = APR::Request::Cookie PACKAGE = APR::Request::Cookie::Table

SV *
cookie_class(t, newclass=NULL)
    APR::Request::Cookie::Table t
    char *newclass
  PREINIT:
    SV *obj = apreq_xs_sv2object(aTHX_ ST(0), TABLE_CLASS, 't');
    MAGIC *mg = mg_find(obj, PERL_MAGIC_ext);
    char *curclass = mg->mg_ptr;

  CODE:
    RETVAL = newSVpv(curclass, 0);
    if (items == 2) {
        if (!sv_derived_from(ST(1), curclass))
            Perl_croak(aTHX_ "Usage: " TABLE_CLASS "::cookie_class($table, $class): "
                             "class %s is not derived from %s", newclass, curclass);
        Safefree(curclass);
        mg->mg_ptr = savepv(newclass);
    }

  OUTPUT:
    RETVAL
