static XS(XS_APR__Request__Error_nil)
{
    dXSARGS;
    (void)items;
    XSRETURN_EMPTY;
}

static XS(apreq_xs_parse)
{
    dXSARGS;
    apreq_handle_t *req;
    apr_status_t s;
    const apr_table_t *t;

    if (items != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: APR::Request::parse($req)");

    req = apreq_xs_sv2handle(aTHX_ ST(0));

    XSprePUSH;
    EXTEND(SP, 3);
    s = apreq_jar(req, &t);
    PUSHs(sv_2mortal(newSViv(s)));
    s = apreq_args(req, &t);
    PUSHs(sv_2mortal(newSViv(s)));
    s = apreq_body(req, &t);
    PUSHs(sv_2mortal(newSViv(s)));
    PUTBACK;
}

MODULE = APR::Request     PACKAGE = APR::Request

SV*
encode(in)
    SV *in
  PREINIT:
    STRLEN len;
    char *src;
  CODE:
    src = SvPV(in, len);
    RETVAL = newSV(3 * len);
    SvCUR_set(RETVAL, apreq_encode(SvPVX(RETVAL), src, len));
    SvPOK_on(RETVAL);

  OUTPUT:
    RETVAL

SV*
decode(in)
    SV *in
  PREINIT:
    STRLEN len;
    apr_size_t dlen;
    char *src;
  CODE:
    src = SvPV(in, len);
    RETVAL = newSV(len);
    apreq_decode(SvPVX(RETVAL), &dlen, src, len); /*XXX needs error-handling */
    SvCUR_set(RETVAL, dlen);
    SvPOK_on(RETVAL);
  OUTPUT:
    RETVAL

SV*
read_limit(req, val=NULL)
    APR::Request req
    SV *val
  PREINIT:
    /* nada */
  CODE:
    if (items == 1) {
        apr_status_t s;
        apr_uint64_t bytes;
        s = apreq_read_limit_get(req, &bytes);     
        if (s != APR_SUCCESS) {
            SV *sv = ST(0), *obj = ST(0);
            APREQ_XS_THROW_ERROR(r, s, 
                   "APR::Request::read_limit", "APR::Request::Error");
            RETVAL = &PL_sv_undef;
        }
        else {
            RETVAL = newSVuv(bytes);
        }
    }
    else {
        apr_status_t s = apreq_read_limit_set(req, SvUV(val));
        if (s != APR_SUCCESS) {
            if (GIMME_V == G_VOID) {
                SV *sv = ST(0), *obj = ST(0);
                APREQ_XS_THROW_ERROR(r, s, 
                    "APR::Request::read_limit", "APR::Request::Error");
            }
            RETVAL = &PL_sv_no;
        }
        else {
            RETVAL = &PL_sv_yes;
        }
    }

  OUTPUT:
    RETVAL

SV*
brigade_limit(req, val=NULL)
    APR::Request req
    SV *val
  PREINIT:
    /* nada */
  CODE:
    if (items == 1) {
        apr_status_t s;
        apr_size_t bytes;
        s = apreq_brigade_limit_get(req, &bytes);     
        if (s != APR_SUCCESS) {
            SV *sv = ST(0), *obj = ST(0);
            APREQ_XS_THROW_ERROR(r, s, 
                   "APR::Request::brigade_limit", "APR::Request::Error");
            RETVAL = &PL_sv_undef;
        }
        else {
            RETVAL = newSVuv(bytes);
        }
    }
    else {
        apr_status_t s = apreq_brigade_limit_set(req, SvUV(val));
        if (s != APR_SUCCESS) {
            if (GIMME_V == G_VOID) {
                SV *sv = ST(0), *obj = ST(0);
                APREQ_XS_THROW_ERROR(r, s, 
                    "APR::Request::brigade_limit", "APR::Request::Error");
            }
            RETVAL = &PL_sv_no;
        }
        else {
            RETVAL = &PL_sv_yes;
        }
    }

  OUTPUT:
    RETVAL


SV*
temp_dir(req, val=NULL)
    APR::Request req
    SV *val
  PREINIT:
    /* nada */
  CODE:
    if (items == 1) {
        apr_status_t s;
        const char *path;
        s = apreq_temp_dir_get(req, &path);     
        if (s != APR_SUCCESS) {
            SV *sv = ST(0), *obj = ST(0);
            APREQ_XS_THROW_ERROR(r, s, 
                   "APR::Request::temp_dir", "APR::Request::Error");
            RETVAL = &PL_sv_undef;
        }
        else {
            RETVAL = (path == NULL) ? &PL_sv_undef : newSVpv(path, 0);
        }
    }
    else {
        apr_status_t s = apreq_temp_dir_set(req, SvPV_nolen(val));
        if (s != APR_SUCCESS) {
            if (GIMME_V == G_VOID) {
                SV *sv = ST(0), *obj = ST(0);
                APREQ_XS_THROW_ERROR(r, s, 
                    "APR::Request::temp_dir", "APR::Request::Error");
            }
            RETVAL = &PL_sv_no;
        }
        else {
            RETVAL = &PL_sv_yes;
        }
    }

  OUTPUT:
    RETVAL


MODULE = APR::Request       PACKAGE = APR::Request::Error

SV *as_string(hv, p1=NULL, p2=NULL)
    APR::Request::Error hv
    SV *p1
    SV *p2
  PREINIT:
    SV **svp;

  CODE:
    svp = hv_fetch(hv, "rc", 2, FALSE);
    if (svp == NULL)
        RETVAL = &PL_sv_undef;
    else
        RETVAL = apreq_xs_strerror(aTHX_ SvIVX(*svp));

  OUTPUT:
    RETVAL

BOOT:
    /* register the overloading (type 'A') magic */
    PL_amagic_generation++;
    /* The magic for overload gets a GV* via gv_fetchmeth as */
    /* mentioned above, and looks in the SV* slot of it for */
    /* the "fallback" status. */
    sv_setsv(
        get_sv( "APR::Request::Error::()", TRUE ),
        &PL_sv_undef
    );
    newXS("APR::Request::Error::()", XS_APR__Request__Error_nil, file);
    newXS("APR::Request::Error::(\"\"", XS_APR__Request__Error_as_string, file);


