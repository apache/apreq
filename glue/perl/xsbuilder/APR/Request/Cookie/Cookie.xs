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
        &PL_sv_undef
    );
    newXS("APR::Request::Cookie::()", XS_APR__Request__Cookie_nil, file);
    newXS("APR::Request::Cookie::(\"\"", XS_APR__Request__Cookie_value, file);
