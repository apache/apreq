/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2003 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */


#define apreq_xs_param2rv(ptr, class) apreq_xs_2sv(ptr,class)
#define apreq_xs_rv2param(sv) ((apreq_param_t *)SvIVX(SvRV(sv)))


APR_INLINE static SV *apreq_xs_param2sv(const apreq_param_t *param, 
                                        const char *class)
{
    SV *sv = newSVpvn(param->v.data, param->v.size);
    if (param->charset == UTF_8)
        SvUTF8_on(sv);

    return sv;
}

APREQ_XS_DEFINE_ENV(request);
APREQ_XS_DEFINE_OBJECT(request);
APREQ_XS_DEFINE_MAKE(param);

/* Too many GET macros :-( */

#define S2P(s) (s ? apreq_value_to_param(apreq_strtoval(s)) : NULL)
#define apreq_xs_request_push(sv,d,key) do {                            \
    apreq_request_t *req = apreq_xs_sv2(request,sv);                    \
    apr_table_do(apreq_xs_do(request), &d, req->args, key, NULL);       \
    if (req->body)                                                      \
        apr_table_do(apreq_xs_do(request), &d, req->body, key, NULL);   \
} while (0)
#define apreq_xs_args_push(sv,d,k) apreq_xs_push(args,sv,d,k)
#define apreq_xs_body_push(sv,d,k) apreq_xs_push(body,sv,d,k)
#define apreq_xs_table_push(sv,d,k) apreq_xs_push(table,sv,d,k)
#define apreq_xs_upload_push(sv,d,key) do {                             \
    apr_table_t *t = apreq_xs_body_sv2table(sv);                        \
    if (t)                                                              \
        apr_table_do(apreq_xs_do(upload), &d, t, key, NULL);            \
} while (0)

#define apreq_xs_upload_table_push(sv,d,k) apreq_xs_push(upload_table,sv,d,k)

#define apreq_xs_request_sv2table(sv) apreq_params(apreq_env_pool(env), \
                                                   apreq_xs_sv2(request,sv))
#define apreq_xs_args_sv2table(sv) apreq_xs_sv2(request,sv)->args
#define apreq_xs_body_sv2table(sv) apreq_xs_sv2(request,sv)->body
#define apreq_xs_table_sv2table(sv) apreq_xs_sv2table(sv)
#define apreq_xs_upload_sv2table(sv) apreq_uploads(apreq_env_pool(env), \
                                                   apreq_xs_sv2(request,sv))
#define apreq_xs_upload_table_sv2table(sv) apreq_xs_sv2table(sv)

#define apreq_xs_request_sv2env(sv) apreq_xs_sv2(request,sv)->env
#define apreq_xs_args_sv2env(sv) apreq_xs_sv2(request,sv)->env
#define apreq_xs_body_sv2env(sv) apreq_xs_sv2(request,sv)->env
#define apreq_xs_upload_sv2env(sv) apreq_xs_sv2(request,sv)->env
#define apreq_xs_table_sv2env(sv) apreq_xs_sv2env(SvRV(sv))
#define apreq_xs_upload_table_sv2env(sv) apreq_xs_sv2env(SvRV(sv))

#define apreq_xs_request_param(sv,k) apreq_param(apreq_xs_sv2(request,sv),k)
#define apreq_xs_args_param(sv,k) \
                     S2P(apr_table_get(apreq_xs_args_sv2table(sv),k))
#define apreq_xs_body_param(sv,k) \
                     S2P(apr_table_get(apreq_xs_body_sv2table(sv),k))
#define apreq_xs_table_param(sv,k) \
                     S2P(apr_table_get(apreq_xs_sv2table(sv),k))
#define apreq_xs_upload_param(sv,k) apreq_upload(apreq_xs_sv2(request,sv),k)
#define apreq_xs_upload_table_param(sv,k) \
                     S2P(apr_table_get(apreq_xs_sv2table(sv),k))

#define PARAM_TABLE   "Apache::Request::Table"

APREQ_XS_DEFINE_GET(request, PARAM_TABLE, param, NULL, 1);
APREQ_XS_DEFINE_GET(args,    PARAM_TABLE, param, NULL, 1);
APREQ_XS_DEFINE_GET(body,    PARAM_TABLE, param, NULL, 1);
APREQ_XS_DEFINE_GET(table,   PARAM_TABLE, param, NULL, 1);

/* Upload API */
/* supercede earlier function definition */
#define apreq_xs_param2sv(param,class) apreq_xs_param2rv(param,class)
#define apreq_xs_sv2param(sv) apreq_xs_rv2param(sv)


#define UPLOAD_TABLE  "Apache::Upload::Table"
#define UPLOAD_PKG    "Apache::Upload"
APREQ_XS_DEFINE_GET(upload, UPLOAD_TABLE, param, UPLOAD_PKG, RETVAL->bb);
APREQ_XS_DEFINE_GET(upload_table, UPLOAD_TABLE, param, UPLOAD_PKG, 1);


APR_INLINE
static XS(apreq_xs_upload_link)
{
    dXSARGS;
    MAGIC *mg;
    void *env;
    const char *name, *fname;
    apr_bucket_brigade *bb;
    apr_file_t *f;

    if (items != 2 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $upload->link($name)");

    if (!(mg = mg_find(SvRV(ST(0)), PERL_MAGIC_ext)))
        Perl_croak(aTHX_ "$upload->link($name): can't find env");

    env = mg->mg_ptr;
    bb = apreq_xs_rv2param(ST(0))->bb;
    name = SvPV_nolen(ST(1));

    f = apreq_brigade_spoolfile(bb);
    if (f == NULL) {
        apr_off_t len;
        apr_status_t s;
        s = apr_file_open(&f, name, APR_CREATE | APR_EXCL | APR_WRITE |
                          APR_READ | APR_BINARY | APR_BUFFERED, 
                          APR_OS_DEFAULT,
                          apreq_env_pool(env));
        if (s != APR_SUCCESS || 
            apreq_brigade_fwrite(f, &len, bb) != APR_SUCCESS)
            XSRETURN_UNDEF;
    
        XSRETURN_YES;
    }
    if (apr_file_name_get(&fname, f) != APR_SUCCESS)
        XSRETURN_UNDEF;

    if (PerlLIO_link(fname, name) >= 0)
        XSRETURN_YES;

    XSRETURN_UNDEF;
}

