static XS(XS_APR__Request__Error_nil)
{
    dXSARGS;
    (void)items;
    XSRETURN_EMPTY;
}


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

