#include "apreq_xs_tables.h"
#define TABLE_CLASS "APR::Request::Param::Table"

#ifdef AP_DEBUG
/* Undo httpd.h's strchr override. */
#undef strchr
#endif

static int apreq_xs_table_do_sub(void *data, const char *key,
                                 const char *val)
{
    struct apreq_xs_do_arg *d = data;
    apreq_param_t *p = apreq_value_to_param(val);
    dTHXa(d->perl);
    dSP;
    SV *sv = apreq_xs_param2sv(aTHX_ p, d->pkg, d->parent);
    int rv;

    ENTER;
    SAVETMPS;

    PUSHMARK(SP);
    EXTEND(SP,2);

    PUSHs(sv_2mortal(newSVpvn(p->v.name, p->v.nlen)));
    PUSHs(sv_2mortal(sv));

    PUTBACK;
    rv = call_sv(d->sub, G_SCALAR);
    SPAGAIN;
    rv = (1 == rv) ? POPi : 1;
    PUTBACK;
    FREETMPS;
    LEAVE;

    return rv;
}

static XS(apreq_xs_table_do)
{
    dXSARGS;
    struct apreq_xs_do_arg d = { NULL, NULL, NULL, aTHX };
    const apr_table_t *t;
    int i, rv = 1;
    SV *sv, *t_obj;
    IV iv;
    MAGIC *mg;

    if (items < 2 || !SvROK(ST(0)) || !SvROK(ST(1)))
        Perl_croak(aTHX_ "Usage: $object->do(\\&callback, @keys)");
    sv = ST(0);

    t_obj = apreq_xs_sv2object(aTHX_ sv, TABLE_CLASS, 't');
    iv = SvIVX(t_obj);
    t = INT2PTR(const apr_table_t *, iv);
    mg = mg_find(t_obj, PERL_MAGIC_ext);
    d.parent = mg->mg_obj;
    d.pkg = mg->mg_ptr;
    d.sub = ST(1);

    if (items == 2) {
        rv = apr_table_do(apreq_xs_table_do_sub, &d, t, NULL);
        XSRETURN_IV(rv);
    }

    for (i = 2; i < items; ++i) {
        const char *key = SvPV_nolen(ST(i));
        rv = apr_table_do(apreq_xs_table_do_sub, &d, t, key, NULL);
        if (rv == 0)
            break;
    }
    XSRETURN_IV(rv);
}

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

SV *
upload_filename(obj)
    APR::Request::Param obj
  PREINIT:

  CODE:
    if (obj->upload != NULL)
        RETVAL = apreq_xs_param2sv(aTHX_ obj, NULL, NULL);
    else
        RETVAL = &PL_sv_undef;

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
    newXS("APR::Request::Param::Table::do", apreq_xs_table_do, file);


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
           apreq_param_tainted_on(obj);
        else
           apreq_param_tainted_off(obj);
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
        apreq_param_tainted_on(RETVAL);

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
   


MODULE = APR::Request::Param PACKAGE = APR::Request::Param

SV *
upload_link(param, path)
    APR::Request::Param param
    const char *path
  PREINIT:
    apr_file_t *f;
    const char *fname;
    apr_status_t s;

  CODE:
    if (param->upload == NULL)
        Perl_croak(aTHX_ "$param->upload_link($file): param has no upload brigade");
    f = apreq_brigade_spoolfile(param->upload);
    if (f == NULL) {
        apr_off_t len;
        s = apr_file_open(&f, path, APR_CREATE | APR_EXCL | APR_WRITE |
                          APR_READ | APR_BINARY,
                          APR_OS_DEFAULT,
                          param->upload->p);
        if (s == APR_SUCCESS) {
            s = apreq_brigade_fwrite(f, &len, param->upload);
            if (s == APR_SUCCESS)
                XSRETURN_YES;
        }
    }
    else {
        s = apr_file_name_get(&fname, f);
        if (s != APR_SUCCESS)
            Perl_croak(aTHX_ "$param->upload_link($file): can't get spoolfile name");
        if (PerlLIO_link(fname, path) >= 0)
            XSRETURN_YES;
        else {
            s = apr_file_copy(fname, path, APR_OS_DEFAULT, param->upload->p);
            if (s == APR_SUCCESS)
                XSRETURN_YES;
        }
    }
    RETVAL = &PL_sv_undef;

  OUTPUT:
    RETVAL

apr_size_t
upload_slurp(param, buffer)
    APR::Request::Param param
    SV *buffer
  PREINIT:
    apr_off_t len;
    apr_status_t s;
    char *data;

  CODE:
    if (param->upload == NULL)
        Perl_croak(aTHX_ "$param->upload_slurp($data): param has no upload brigade");

    s = apr_brigade_length(param->upload, 0, &len);
    if (s != APR_SUCCESS)
        Perl_croak(aTHX_ "$param->upload_slurp($data): can't get upload length");

    RETVAL = len;
    SvUPGRADE(buffer, SVt_PV);
    data = SvGROW(buffer, RETVAL + 1);
    data[RETVAL] = 0;
    SvCUR_set(buffer, RETVAL);
    SvPOK_only(buffer);
    s = apr_brigade_flatten(param->upload, data, &RETVAL);
    if (s != APR_SUCCESS)
        Perl_croak(aTHX_ "$param->upload_slurp($data): can't flatten upload");

    if (apreq_param_is_tainted(param))
        SvTAINTED_on(buffer);

    SvSETMAGIC(buffer);

  OUTPUT:
    RETVAL

UV
upload_size(param)
    APR::Request::Param param
  PREINIT:
    apr_off_t len;
    apr_status_t s;

  CODE:
    if (param->upload == NULL)
        Perl_croak(aTHX_ "$param->upload_size(): param has no upload brigade");

    s = apr_brigade_length(param->upload, 0, &len);
    if (s != APR_SUCCESS)
        Perl_croak(aTHX_ "$param->upload_size(): can't get upload length");

    RETVAL = len;    

  OUTPUT:
    RETVAL

SV *
upload_type(param)
    APR::Request::Param param
  PREINIT:
    const char *ct, *sc;
    STRLEN len;
  CODE:
    if (param->info == NULL)
        Perl_croak(aTHX_ "$param->upload_type(): param has no info table");

    ct = apr_table_get(param->info, "Content-Type");
    if (ct == NULL)
        Perl_croak(aTHX_ "$param->upload_type: can't find Content-Type header");
    
    if ((sc = strchr(ct, ';')))
        len = sc - ct;
    else
        len = strlen(ct);

    RETVAL = newSVpvn(ct, len);    
    if (apreq_param_is_tainted(param))
        SvTAINTED_on(RETVAL);

  OUTPUT:
    RETVAL


const char *
upload_tempname(param, req=apreq_xs_sv2handle(aTHX_ ST(0)))
    APR::Request::Param param
    APR::Request req

  PREINIT:
    apr_file_t *f;
    apr_status_t s;

  CODE:
    if (param->upload == NULL)
        Perl_croak(aTHX_ "$param->upload_tempname($req): param has no upload brigade");
    f = apreq_brigade_spoolfile(param->upload);
    if (f == NULL) {
        const char *path;
        s = apreq_temp_dir_get(req, &path);
        if (s != APR_SUCCESS)
            Perl_croak(aTHX_ "$param->upload_tempname($req): can't get temp_dir");
        s = apreq_brigade_concat(param->upload->p, path, 0, 
                                 param->upload, param->upload);
        if (s != APR_SUCCESS)
            Perl_croak(aTHX_ "$param->upload_tempname($req): can't make spool bucket");
        f = apreq_brigade_spoolfile(param->upload);
    }
    s = apr_file_name_get(&RETVAL, f);
    if (s != APR_SUCCESS)
        Perl_croak(aTHX_ "$param->upload_link($file): can't get spool file name");

  OUTPUT:
    RETVAL


MODULE = APR::Request::Param    PACKAGE = APR::Request::Param::Table

SV *
uploads(t, pool)
    APR::Request::Param::Table t
    APR::Pool pool
  PREINIT:
    SV *obj = apreq_xs_sv2object(aTHX_ ST(0), TABLE_CLASS, 't');
    SV *parent = apreq_xs_sv2object(aTHX_ ST(0), HANDLE_CLASS, 'r');
    MAGIC *mg = mg_find(obj, PERL_MAGIC_ext);
  CODE:
    RETVAL = apreq_xs_table2sv(aTHX_ apreq_uploads(t, pool), HvNAME(SvSTASH(obj)), 
                               parent, mg->mg_ptr, mg->mg_len);
  OUTPUT:
    RETVAL


