
#if (PERL_VERSION >= 8) /* MAGIC ITERATOR REQUIRES 5.8 */

/* Requires perl 5.8 or better. 
 * A custom MGVTBL with its "copy" slot filled allows
 * us to FETCH a table entry immediately during iteration.
 * For multivalued keys this is essential in order to get
 * the value corresponding to the current key, otherwise
 * values() will always report the first value repeatedly.
 * With this MGVTBL the keys() list always matches up with
 * the values() list, even in the multivalued case.
 * We only prefetch the value during iteration, because the
 * prefetch adds overhead to EXISTS and STORE operations.
 * They are only "penalized" when the perl program is iterating
 * via each(), which seems to be a reasonable tradeoff.
 */

static int apreq_xs_table_magic_copy(pTHX_ SV *sv, MAGIC *mg, SV *nsv, 
                                  const char *name, int namelen)
{
    /* Prefetch the value whenever the table iterator is > 0 */
    MAGIC *tie_magic = mg_find(nsv, PERL_MAGIC_tiedelem);
    SV *obj = SvRV(tie_magic->mg_obj);
    IV idx = SvIVX(obj);
    const apr_table_t *t = INT2PTR(apr_table_t *, idx);
    const apr_array_header_t *arr = apr_table_elts(t);

    idx = SvCUR(obj);

    if (idx > 0 && idx <= arr->nelts) {
        const apr_table_entry_t *te = (const apr_table_entry_t *)arr->elts;
        const char *param_class = mg_find(obj, PERL_MAGIC_ext)->mg_ptr;
        apreq_param_t *p = apreq_value_to_param(te[idx-1].val);

        SvMAGICAL_off(nsv);
        sv_setsv(nsv, sv_2mortal(apreq_xs_param2sv(aTHX_ p, param_class, obj)));
    }

    return 0;
}

static const MGVTBL apreq_xs_table_magic = {0, 0, 0, 0, 0, 
                                            apreq_xs_table_magic_copy};

#endif

static APR_INLINE
SV *apreq_xs_table2sv(pTHX_ const apr_table_t *t, const char *class, SV *parent,
                      const char *value_class, I32 vclen)
{
    SV *sv = (SV *)newHV();
    SV *rv = sv_setref_pv(newSV(0), class, (void *)t);
    sv_magic(SvRV(rv), parent, PERL_MAGIC_ext, value_class, vclen);

#if (PERL_VERSION >= 8) /* MAGIC ITERATOR requires 5.8 */

    sv_magic(sv, NULL, PERL_MAGIC_ext, Nullch, -1);
    SvMAGIC(sv)->mg_virtual = (MGVTBL *)&apreq_xs_table_magic;
    SvMAGIC(sv)->mg_flags |= MGf_COPY;

#endif

    sv_magic(sv, rv, PERL_MAGIC_tied, Nullch, 0);
    SvREFCNT_dec(rv); /* corrects SvREFCNT_inc(rv) implicit in sv_magic */

    return sv_bless(newRV_noinc(sv), SvSTASH(SvRV(rv)));
}


/* Upload-related stuff */
#if 0

static XS(apreq_xs_upload_link)
{
    dXSARGS;
    MAGIC *mg;
    void *env;
    const char *name, *fname;
    apr_bucket_brigade *bb;
    apr_file_t *f;
    apr_status_t s = APR_SUCCESS;
    SV *sv, *obj;

    if (items != 2 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $upload->link($filename)");

    if (SvTAINTED(ST(1)))
        Perl_croak(aTHX_ "$upload->link($filename): Cannot link to tainted $filename: %s", 
                   SvPV_nolen(ST(1)));

    sv = ST(0);
    obj = apreq_xs_find_obj(aTHX_ sv, "upload");

    if (!(mg = mg_find(obj, PERL_MAGIC_ext)))
        Perl_croak(aTHX_ "$upload->link($name): panic: can't find env.");

    env = mg->mg_ptr;
    bb = ((apreq_param_t *)SvIVX(obj))->bb;
    name = SvPV_nolen(ST(1));

    f = apreq_brigade_spoolfile(bb);
    if (f == NULL) {
        apr_off_t len;

        s = apr_file_open(&f, name, APR_CREATE | APR_EXCL | APR_WRITE |
                          APR_READ | APR_BINARY,
                          APR_OS_DEFAULT,
                          apreq_env_pool(env));
        if (s == APR_SUCCESS) {
            s = apreq_brigade_fwrite(f, &len, bb);
            if (s == APR_SUCCESS)
                XSRETURN_YES;
        }
        goto link_error;
    }
    s = apr_file_name_get(&fname, f);
    if (s != APR_SUCCESS)
        goto link_error;

    if (PerlLIO_link(fname, name) >= 0)
        XSRETURN_YES;
    else {
        s = apr_file_copy(fname, name,
                          APR_OS_DEFAULT, 
                          apreq_env_pool(env));
        if (s == APR_SUCCESS)
            XSRETURN_YES;
    }

 link_error:
    APREQ_XS_THROW_ERROR(upload, s, "Apache::Upload::link", 
                         "Apache::Upload::Error");
    XSRETURN_UNDEF;
}


static XS(apreq_xs_upload_slurp)
{
    dXSARGS;
    MAGIC *mg;
    void *env;
    char *data;
    apr_off_t len_off;
    apr_size_t len_size;
    apr_bucket_brigade *bb;
    SV *sv, *obj;
    apr_status_t s;

    if (items != 2 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $upload->slurp($data)");

    sv = ST(0);
    obj = apreq_xs_find_obj(aTHX_ sv, "upload");

    if (!(mg = mg_find(obj, PERL_MAGIC_ext)))
        Perl_croak(aTHX_ "$upload->slurp($data): can't find env");

    env = mg->mg_ptr;
    bb = ((apreq_param_t *)SvIVX(obj))->bb;

    s = apr_brigade_length(bb, 0, &len_off);
    if (s != APR_SUCCESS) {
        APREQ_XS_THROW_ERROR(upload, s, "Apache::Upload::slurp", 
                             "Apache::Upload::Error");
        XSRETURN_UNDEF;
    }

    len_size = len_off; /* max_body setting will be low enough to prevent
                         * overflow, but even if it wasn't the code below will
                         * at worst truncate the slurp data (not segfault).
                         */
                         
    SvUPGRADE(ST(1), SVt_PV);
    data = SvGROW(ST(1), len_size + 1);
    data[len_size] = 0;
    SvCUR_set(ST(1), len_size);
    SvPOK_only(ST(1));
    s = apr_brigade_flatten(bb, data, &len_size);
    if (s != APR_SUCCESS) {
        APREQ_XS_THROW_ERROR(upload, s, "Apache::Upload::slurp", 
                             "Apache::Upload::Error");
        XSRETURN_UNDEF;
    }
    if (SvTAINTED(obj))
        SvTAINTED_on(ST(1));
    SvSETMAGIC(ST(1));
    XSRETURN_IV(len_size);
}

static XS(apreq_xs_upload_size)
{
    dXSARGS;
    MAGIC *mg;
    void *env;
    apr_bucket_brigade *bb;
    apr_status_t s;
    apr_off_t len;
    SV *sv, *obj;

    if (items != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $upload->size()");

    sv = ST(0);
    obj = apreq_xs_find_obj(aTHX_ sv, "upload");

    if (!(mg = mg_find(obj, PERL_MAGIC_ext)))
        Perl_croak(aTHX_ "$upload->size(): can't find env");

    env = mg->mg_ptr;
    bb = ((apreq_param_t *)SvIVX(obj))->bb;

    s = apr_brigade_length(bb, 1, &len);

    if (s != APR_SUCCESS) {
        APREQ_XS_THROW_ERROR(upload, s, "Apache::Upload::size", 
                             "Apache::Upload::Error");
        XSRETURN_UNDEF;
    }

    XSRETURN_IV((IV)len);
}

static XS(apreq_xs_upload_type)
{
    dXSARGS;
    apreq_param_t *upload;
    const char *ct, *sc;
    STRLEN len;
    SV *sv, *obj;

    if (items != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $upload->type()");

    sv = ST(0);
    obj = apreq_xs_find_obj(aTHX_ sv, "upload");

    upload = (apreq_param_t *)SvIVX(obj);
    ct = apr_table_get(upload->info, "Content-Type");
    if (ct == NULL)
        Perl_croak(aTHX_ "$upload->type: can't find Content-Type header");
    
    if ((sc = strchr(ct, ';')))
        len = sc - ct;
    else
        len = strlen(ct);

    sv = newSVpvn(ct, len);
    if (SvTAINTED(obj))
        SvTAINTED_on(sv);
    ST(0) = sv_2mortal(sv);
    XSRETURN(1);
}

APR_INLINE
static SV *apreq_xs_find_bb_obj(pTHX_ SV *in)
{
    while (in && SvROK(in)) {
        SV *sv = SvRV(in);
        switch (SvTYPE(sv)) {            
            MAGIC *mg;
        case SVt_PVIO:
            if (SvMAGICAL(sv) && (mg = mg_find(sv,PERL_MAGIC_tiedscalar))) {
               in = mg->mg_obj;
               break;
            }
            Perl_croak(aTHX_ "panic: cannot find tied scalar in pvio magic");
        case SVt_PVMG:
            if (SvOBJECT(sv) && SvIOKp(sv))
                return sv;
        default:
             Perl_croak(aTHX_ "panic: unsupported SV type: %d", SvTYPE(sv));
       }
    }
    return in;
}


static XS(apreq_xs_upload_brigade_copy)
{
    dXSARGS;
    apr_bucket_brigade *bb, *bb_copy;
    char *class;
    SV *sv, *obj;

    if (items != 2 || !SvPOK(ST(0)) || !SvROK(ST(1)))
        Perl_croak(aTHX_ "Usage: Apache::Upload::Brigade->new($bb)");

    class = SvPV_nolen(ST(0));
    obj = apreq_xs_find_bb_obj(aTHX_ ST(1));
    bb = (apr_bucket_brigade *)SvIVX(obj);
    bb_copy = apr_brigade_create(bb->p,bb->bucket_alloc);
    apreq_brigade_copy(bb_copy, bb);

    sv = sv_setref_pv(newSV(0), class, bb_copy);
    if (SvTAINTED(obj))
        SvTAINTED_on(SvRV(sv));
    ST(0) = sv_2mortal(sv);
    XSRETURN(1);
}

static XS(apreq_xs_upload_brigade_read)
{
    dXSARGS;
    apr_bucket_brigade *bb;
    apr_bucket *e, *end;
    IV want = -1, offset = 0;
    SV *sv, *obj;
    apr_status_t s;
    char *buf;

    switch (items) {
    case 4:
        offset = SvIV(ST(3));
    case 3:
        want = SvIV(ST(2));
    case 2:
        sv = ST(1);
        SvUPGRADE(sv, SVt_PV);
        if (SvROK(ST(0))) {
            obj = apreq_xs_find_bb_obj(aTHX_ ST(0));
            bb = (apr_bucket_brigade *)SvIVX(obj);
            break;
        }
    default:
        Perl_croak(aTHX_ "Usage: $bb->READ($buf,$len,$off)");
    }

    if (want == 0) {
        SvCUR_set(sv, offset);
        XSRETURN_IV(0);
    }

    if (APR_BRIGADE_EMPTY(bb)) {
        SvCUR_set(sv, offset);
        XSRETURN_UNDEF;
    }

    if (want == -1) {
        const char *data;
        apr_size_t dlen;
        e = APR_BRIGADE_FIRST(bb);
        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            apreq_xs_croak(aTHX_ newHV(), s, 
                           "Apache::Upload::Brigade::READ", 
                           "APR::Error");
        want = dlen;
        end = APR_BUCKET_NEXT(e);
    }
    else {
        switch (s = apr_brigade_partition(bb, (apr_off_t)want, &end)) {
            apr_off_t len;

        case APR_INCOMPLETE:
            s = apr_brigade_length(bb, 1, &len);
            if (s != APR_SUCCESS)
                apreq_xs_croak(aTHX_ newHV(), s, 
                               "Apache::Upload::Brigade::READ", 
                               "APR::Error");
            want = len;

        case APR_SUCCESS:
            break;

        default:
            apreq_xs_croak(aTHX_ newHV(), s, 
                           "Apache::Upload::Brigade::READ",
                           "APR::Error");
        }
    }

    SvGROW(sv, want + offset + 1);
    buf = SvPVX(sv) + offset;
    SvCUR_set(sv, want + offset);
    if (SvTAINTED(obj))
        SvTAINTED_on(sv);

    while ((e = APR_BRIGADE_FIRST(bb)) != end) {
        const char *data;
        apr_size_t dlen;
        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            apreq_xs_croak(aTHX_ newHV(), s, 
                           "Apache::Upload::Brigade::READ", "APR::Error");
        memcpy(buf, data, dlen);
        buf += dlen;
        apr_bucket_delete(e);
    }

    *buf = 0;
    SvPOK_only(sv);
    SvSETMAGIC(sv);
    XSRETURN_IV(want);
}

static XS(apreq_xs_upload_brigade_readline)
{
    dXSARGS;
    apr_bucket_brigade *bb;
    apr_bucket *e;
    SV *sv, *obj;
    apr_status_t s;
    unsigned tainted;

    if (items != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $bb->READLINE");

    obj = apreq_xs_find_bb_obj(aTHX_ ST(0));
    bb = (apr_bucket_brigade *)SvIVX(obj);

    if (APR_BRIGADE_EMPTY(bb))
        XSRETURN(0);

    tainted = SvTAINTED(obj);

    XSprePUSH;

    sv = sv_2mortal(newSVpvn("",0));
    if (tainted)
        SvTAINTED_on(sv);
        
    XPUSHs(sv);

    while (!APR_BRIGADE_EMPTY(bb)) {
        const char *data;
        apr_size_t dlen;
        const char *eol;

        e = APR_BRIGADE_FIRST(bb);
        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            apreq_xs_croak(aTHX_ newHV(), s, 
                           "Apache::Upload::Brigade::READLINE",
                           "APR::Error");

        eol = memchr(data, '\012', dlen); /* look for LF (linefeed) */

        if (eol != NULL) {
            if (eol < data + dlen - 1) {
                dlen = eol - data + 1;
                apr_bucket_split(e, dlen);
            }

            sv_catpvn(sv, data, dlen);
            apr_bucket_delete(e);

            if (GIMME_V != G_ARRAY || APR_BRIGADE_EMPTY(bb))
                break;

            sv = sv_2mortal(newSVpvn("",0));
            if (tainted)
                SvTAINTED_on(sv);
            XPUSHs(sv);
        }
        else {
            sv_catpvn(sv, data, dlen);
            apr_bucket_delete(e);
        }
    }

    PUTBACK;
}


static XS(apreq_xs_upload_tempname)
{
    dXSARGS;
    MAGIC *mg;
    void *env;
    apr_bucket_brigade *bb;
    apr_status_t s;
    apr_file_t *file;
    const char *path;
    SV *sv, *obj;

    if (items != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $upload->tempname()");

    sv = ST(0);
    obj = apreq_xs_find_obj(aTHX_ sv, "upload");

    if (!(mg = mg_find(obj, PERL_MAGIC_ext)))
        Perl_croak(aTHX_ "$upload->tempname(): can't find env");

    env = mg->mg_ptr;
    bb = ((apreq_param_t *)SvIVX(obj))->bb;
    file = apreq_brigade_spoolfile(bb);

    if (file == NULL) {
        apr_bucket *last;
        apr_off_t len;
        const char *tmpdir = apreq_env_temp_dir(env, NULL);

        s = apreq_file_mktemp(&file, apreq_env_pool(env), tmpdir);

        if (s != APR_SUCCESS)
            goto tempname_error;

        s = apreq_brigade_fwrite(file, &len, bb);

        if (s != APR_SUCCESS)
            goto tempname_error;

        last = apr_bucket_file_create(file, len, 0, bb->p, bb->bucket_alloc);
        APR_BRIGADE_INSERT_TAIL(bb, last);
    }

    s = apr_file_name_get(&path, file);
    if (s != APR_SUCCESS)
        goto tempname_error;

    sv = newSVpvn(path, strlen(path));
    ST(0) = sv_2mortal(sv);
    XSRETURN(1);

 tempname_error:
    APREQ_XS_THROW_ERROR(upload, s, "Apache::Upload::tempname", 
                         "Apache::Upload::Error");
    XSRETURN_UNDEF;

}
#endif
