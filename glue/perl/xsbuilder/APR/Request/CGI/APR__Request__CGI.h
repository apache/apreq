#include "apreq_xs_tables.h"
#define TABLE_CLASS "APR::Request::Param::Table"

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

static XS(apreq_xs_cgi_param)
{
    dXSARGS;
    apreq_handle_t *req;
    SV *sv, *obj;
    IV iv;

    if (items == 0 || items > 2 || !SvROK(ST(0))
        || !sv_derived_from(ST(0), "APR::Request::CGI"))
        Perl_croak(aTHX_ "Usage: APR::Request::CGI::param($req [,$name])");

    sv = ST(0);
    obj = apreq_xs_sv2object(aTHX_ sv, HANDLE_CLASS, 'r');
    iv = SvIVX(obj);
    req = INT2PTR(apreq_handle_t *, iv);

    if (items == 2 && GIMME_V == G_SCALAR) {
        apreq_param_t *p = apreq_param(req, SvPV_nolen(ST(1)));

        if (p != NULL) {
            ST(0) = apreq_xs_param2sv(aTHX_ p, NULL, obj);
            sv_2mortal(ST(0));
            XSRETURN(1);
        }
        else {
            XSRETURN_UNDEF;
        }
    }
    else {
        struct apreq_xs_do_arg d = {NULL, NULL, NULL, aTHX};
        apr_pool_t *pool;
        const apr_table_t *t;
        SV *pool_obj;

        d.pkg = NULL;
        d.parent = obj;

        switch (GIMME_V) {

        case G_ARRAY:
            XSprePUSH;
            PUTBACK;
            if (items == 1) {
                apreq_args(req, &t);
                if (t != NULL)
                    apr_table_do(apreq_xs_table_keys, &d, t, NULL);
                apreq_body(req, &t);
                if (t != NULL)
                    apr_table_do(apreq_xs_table_keys, &d, t, NULL);

            }
            else {
                char *val = SvPV_nolen(ST(1));
                apreq_args(req, &t);
                if (t != NULL)
                    apr_table_do(apreq_xs_table_values, &d, t, val, NULL);
                apreq_body(req, &t);
                if (t != NULL)
                    apr_table_do(apreq_xs_table_values, &d, t, val, NULL);
            }
            return;

        case G_SCALAR:
            pool_obj = mg_find(obj, PERL_MAGIC_ext)->mg_obj;
            iv = SvIVX(obj);
            pool = INT2PTR(apr_pool_t *, iv);

            t = apreq_params(req, pool);
            if (t == NULL)
                XSRETURN_UNDEF;

            ST(0) = apreq_xs_table2sv(aTHX_ t, TABLE_CLASS, obj,
                                      NULL, 0);
            sv_2mortal(ST(0));
            XSRETURN(1);

        default:
            XSRETURN(0);
        }
    }
}
