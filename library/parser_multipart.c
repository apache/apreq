#include "apreq_parser.h"
#include "apr_strings.h"
#include "apr_strmatch.h"

#ifndef CRLF
#define CRLF    "\015\012"
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define PARSER_STATUS_CHECK(PREFIX)   do {         \
    if (ctx->status == PREFIX##_ERROR)             \
        return APR_EGENERAL;                       \
    else if (ctx->status == PREFIX##_COMPLETE)     \
        return APR_SUCCESS;                        \
    else if (bb == NULL)                           \
        return APR_INCOMPLETE;                     \
} while (0);



struct mfd_ctx {
    apr_table_t                 *info;
    apr_bucket_brigade          *in;
    apr_bucket_brigade          *bb;
    apreq_parser_t              *hdr_parser;
    apreq_parser_t              *mix_parser;
    const apr_strmatch_pattern  *pattern;
    char                        *bdry;
    enum {
        MFD_INIT,  
        MFD_NEXTLINE, 
        MFD_HEADER,
        MFD_POST_HEADER,
        MFD_PARAM, 
        MFD_UPLOAD,
        MFD_MIXED,
        MFD_COMPLETE,
        MFD_ERROR 
    }                            status;
    apr_bucket                  *eos;
    char                        *param_name;
    apreq_param_t               *upload;
};


/********************* multipart/form-data *********************/

APR_INLINE
static apr_status_t brigade_start_string(apr_bucket_brigade *bb,
                                         const char *start_string)
{
    apr_bucket *e;
    apr_size_t slen = strlen(start_string);

    for (e = APR_BRIGADE_FIRST(bb); e != APR_BRIGADE_SENTINEL(bb);
         e = APR_BUCKET_NEXT(e))
    {
        const char *buf;
        apr_status_t s, bytes_to_check;
        apr_size_t blen;

        if (slen == 0)
            return APR_SUCCESS;

        if (APR_BUCKET_IS_EOS(e))
            return APR_EOF;

        s = apr_bucket_read(e, &buf, &blen, APR_BLOCK_READ);

        if (s != APR_SUCCESS)
            return s;

        if (blen == 0)
            continue;

        bytes_to_check = MIN(slen,blen);

        if (strncmp(buf,start_string,bytes_to_check) != 0)
            return APR_EGENERAL;

        slen -= bytes_to_check;
        start_string += bytes_to_check;
    }

    /* slen > 0, so brigade isn't large enough yet */
    return APR_INCOMPLETE;
}


static apr_status_t split_on_bdry(apr_bucket_brigade *out,
                                  apr_bucket_brigade *in,
                                  const apr_strmatch_pattern *pattern,
                                  const char *bdry)
{
    apr_bucket *e = APR_BRIGADE_FIRST(in);
    apr_size_t blen = strlen(bdry), off = 0;

    while ( e != APR_BRIGADE_SENTINEL(in) ) {
        apr_ssize_t idx;
        apr_size_t len;
        const char *buf;
        apr_status_t s;

        if (APR_BUCKET_IS_EOS(e))
            return APR_EOF;

        s = apr_bucket_read(e, &buf, &len, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            return s;

        if (len == 0) {
            apr_bucket *f = e;
            e = APR_BUCKET_NEXT(e);
            apr_bucket_delete(f);
            continue;
        }

    look_for_boundary_up_front:
        if (strncmp(bdry + off, buf, MIN(len, blen - off)) == 0) {
            if ( len >= blen - off ) {
                /* complete match */
                if (len > blen - off)
                    apr_bucket_split(e, blen - off);
                e = APR_BUCKET_NEXT(e);

                do {
                    apr_bucket *f = APR_BRIGADE_FIRST(in);
                    apr_bucket_delete(f);
                } while (APR_BRIGADE_FIRST(in) != e);

                return APR_SUCCESS;
            }
            /* partial match */
            off += len;
            e = APR_BUCKET_NEXT(e);
            continue;
        }
        else if (off > 0) {
            /* prior (partial) strncmp failed, 
             * so we can move previous buckets across
             * and retest buf against the full bdry.
             */
            do {
                apr_bucket *f = APR_BRIGADE_FIRST(in);
                APR_BUCKET_REMOVE(f);
                APR_BRIGADE_INSERT_TAIL(out, f);
            } while (e != APR_BRIGADE_FIRST(in));
            off = 0;
            goto look_for_boundary_up_front;
        }

        if (pattern != NULL && len >= blen) {
            const char *match = apr_strmatch(pattern, buf, len);
            if (match != NULL)
                idx = match - buf;
            else {
                idx = apreq_index(buf + len-blen, blen, bdry, blen, 
                                  APREQ_MATCH_PARTIAL);
                if (idx >= 0)
                    idx += len-blen;
            }
        }
        else
            idx = apreq_index(buf, len, bdry, blen, APREQ_MATCH_PARTIAL);

        /* Theoretically idx should never be 0 here, because we 
         * already tested the front of the brigade for a potential match.
         * However, it doesn't hurt to allow for the possibility,
         * since this will just start the whole loop over again.
         */
        if (idx >= 0)
            apr_bucket_split(e, idx);

        APR_BUCKET_REMOVE(e);
        APR_BRIGADE_INSERT_TAIL(out, e);
        e = APR_BRIGADE_FIRST(in);
    }

    return APR_INCOMPLETE;
}


static
struct mfd_ctx * create_multipart_context(const char *content_type,
                                          apr_pool_t *pool,
                                          apr_bucket_alloc_t *ba,
                                          apr_size_t brigade_limit,
                                          const char *temp_dir)

{
    apr_status_t s;
    apr_size_t blen;
    struct mfd_ctx *ctx = apr_palloc(pool, sizeof *ctx);
    char *ct = apr_pstrdup(pool, content_type);
 
    ct = strchr(ct, ';');
    if (ct == NULL)
        return NULL; /* missing semicolon */

    *ct++ = 0;
    s = apreq_header_attribute(ct, "boundary", 8,
                               (const char **)&ctx->bdry, &blen);

    if (s != APR_SUCCESS)
        return NULL; /* missing boundary */

    ctx->bdry[blen] = 0;

    *--ctx->bdry = '-';
    *--ctx->bdry = '-';
    *--ctx->bdry = '\n';
    *--ctx->bdry = '\r';

    ctx->status = MFD_INIT;
    ctx->pattern = apr_strmatch_precompile(pool, ctx->bdry, 1);
    ctx->hdr_parser = apreq_make_parser(pool, ba, "", apreq_parse_headers,
                                        brigade_limit, temp_dir, NULL, NULL);
    ctx->info = NULL;
    ctx->bb = apr_brigade_create(pool, ba);
    ctx->in = apr_brigade_create(pool, ba);
    ctx->eos = apr_bucket_eos_create(ba);
    ctx->mix_parser = NULL;
    ctx->param_name = NULL;
    ctx->upload = NULL;

    return ctx;
}

APREQ_DECLARE_PARSER(apreq_parse_multipart)
{
    apr_pool_t *pool = parser->pool;
    apr_bucket_alloc_t *ba = parser->bucket_alloc;
    struct mfd_ctx *ctx = parser->ctx;
    apr_status_t s;

    if (ctx == NULL) {
        ctx = create_multipart_context(parser->content_type,
                                       pool, ba, 
                                       parser->brigade_limit,
                                       parser->temp_dir);
        if (ctx == NULL)
            return APR_EGENERAL;


        ctx->mix_parser = apreq_make_parser(pool, ba, "", 
                                            apreq_parse_multipart,
                                            parser->brigade_limit,
                                            parser->temp_dir,
                                            parser->hook,
                                            NULL);
        parser->ctx = ctx;
    }

    PARSER_STATUS_CHECK(MFD);
    APR_BRIGADE_CONCAT(ctx->in, bb);

 mfd_parse_brigade:

    switch (ctx->status) {

    case MFD_INIT:
        {
            s = split_on_bdry(ctx->bb, ctx->in, NULL, ctx->bdry + 2);
            if (s != APR_SUCCESS) {
                apreq_brigade_setaside(ctx->in, pool);
                apreq_brigade_setaside(ctx->bb, pool);
                return s;
            }
            ctx->status = MFD_NEXTLINE;
            /* Be polite and return any preamble text to the caller. */
            APR_BRIGADE_CONCAT(bb, ctx->bb);
        }

        /* fall through */

    case MFD_NEXTLINE:
        {
            s = split_on_bdry(ctx->bb, ctx->in, NULL, CRLF);
            if (s != APR_SUCCESS) {
                apreq_brigade_setaside(ctx->in, pool);
                apreq_brigade_setaside(ctx->bb, pool);
                return s;
            }
            if (!APR_BRIGADE_EMPTY(ctx->bb)) {
                /* ctx->bb probably contains "--", but we'll stop here
                 *  without bothering to check, and just
                 * return any postamble text to caller.
                 */
                APR_BRIGADE_CONCAT(bb, ctx->in);
                ctx->status = MFD_COMPLETE;
                return APR_SUCCESS;
            }

            ctx->status = MFD_HEADER;
            ctx->info = NULL;
        }
        /* fall through */

    case MFD_HEADER:
        {
            if (ctx->info == NULL) {
                ctx->info = apr_table_make(pool, APREQ_DEFAULT_NELTS);
                /* flush out old header parser internal structs for reuse */
                ctx->hdr_parser->ctx = NULL;
            }
            s = apreq_run_parser(ctx->hdr_parser, ctx->info, ctx->in);
            switch (s) {
            case APR_SUCCESS:
                ctx->status = MFD_POST_HEADER;
                break;
            case APR_INCOMPLETE:
                apreq_brigade_setaside(ctx->in, pool);
                return APR_INCOMPLETE;
            default:
                ctx->status = MFD_ERROR;
                return s;
            }
        }
        /* fall through */

    case MFD_POST_HEADER:
        {
            /* Must handle special case of missing CRLF (mainly
               coming from empty file uploads). See RFC2065 S5.1.1:

                 body-part = MIME-part-header [CRLF *OCTET]

               So the CRLF we already matched in MFD_HEADER may have been 
               part of the boundary string! Both Konqueror (v??) and 
               Mozilla-0.97 are known to emit such blocks.

               Here we first check for this condition with 
               brigade_start_string, and prefix the brigade with
               an additional CRLF bucket if necessary.
            */

            const char *cd, *name, *filename;
            apr_size_t nlen, flen;
            apr_bucket *e;

            switch (brigade_start_string(ctx->in, ctx->bdry + 2)) {

            case APR_INCOMPLETE:
                apreq_brigade_setaside(ctx->in, pool);
                return APR_INCOMPLETE;

            case APR_SUCCESS:
                /* part has no body- return CRLF to front */
                e = apr_bucket_immortal_create(CRLF, 2,
                                                ctx->bb->bucket_alloc);
                APR_BRIGADE_INSERT_HEAD(ctx->in,e);
                break;

            default:
                ; /* has body, ok */
            }

            cd = apr_table_get(ctx->info, "Content-Disposition");

            if (cd != NULL) {

                if (ctx->mix_parser != NULL) {
                    /* multipart/form-data */

                    s = apreq_header_attribute(cd, "name", 4, &name, &nlen);
                    if (s != APR_SUCCESS) {
                        ctx->status = MFD_ERROR;
                        return APR_EGENERAL;
                    }

                    s = apreq_header_attribute(cd, "filename", 
                                               8, &filename, &flen);
                    if (s != APR_SUCCESS) {
                        const char *ct = apr_table_get(ctx->info, 
                                                       "Content-Type");
                        if (ct != NULL 
                            && strncmp(ct, "multipart/mixed", 15) == 0)
                        {
                            struct mfd_ctx *mix_ctx;
                            mix_ctx = create_multipart_context(ct, pool, ba,
                                                               parser->brigade_limit,
                                                               parser->temp_dir);
                            if (mix_ctx == NULL) {
                                ctx->status = MFD_ERROR;
                                return APR_EGENERAL;
                            }
                                
                            mix_ctx->param_name = apr_pstrmemdup(pool,
                                                                 name, nlen);
                            ctx->mix_parser->ctx = mix_ctx;
                            ctx->status = MFD_MIXED;
                            goto mfd_parse_brigade;
                        }
                        ctx->param_name = apr_pstrmemdup(pool, name, nlen);
                        ctx->status = MFD_PARAM;
                    }
                    else {
                        apreq_param_t *param;

                        param = apreq_make_param(pool, name, nlen, 
                                                 filename, flen);
                        param->info = ctx->info;
                        param->upload = apr_brigade_create(pool, 
                                                       ctx->bb->bucket_alloc);
                        ctx->upload = param;
                        ctx->status = MFD_UPLOAD;
                        goto mfd_parse_brigade;

                    }
                }
                else {
                    /* multipart/mixed */
                    s = apreq_header_attribute(cd, "filename", 
                                               8, &filename, &flen);
                    if (s != APR_SUCCESS) {
                        ctx->status = MFD_PARAM;
                    }
                    else {
                        apreq_param_t *param;
                        name = ctx->param_name;
                        nlen = strlen(name);
                        param = apreq_make_param(pool, name, nlen, 
                                                 filename, flen);
                        param->info = ctx->info;
                        param->upload = apr_brigade_create(pool, 
                                                       ctx->bb->bucket_alloc);
                        ctx->upload = param;
                        ctx->status = MFD_UPLOAD;
                        goto mfd_parse_brigade;
                    }
                }
            }
            else {
                /* multipart/related */
                apreq_param_t *param;
                cd = apr_table_get(ctx->info, "Content-ID");
                if (cd == NULL) {
                    ctx->status = MFD_ERROR;
                    return APR_EGENERAL;
                }
                name = cd;
                nlen = strlen(name);
                filename = "";
                flen = 0;
                param = apreq_make_param(pool, name, nlen, 
                                         filename, flen);
                param->info = ctx->info;
                param->upload = apr_brigade_create(pool, 
                                               ctx->bb->bucket_alloc);
                ctx->upload = param;
                ctx->status = MFD_UPLOAD;
                goto mfd_parse_brigade;
            }

        }
        /* fall through */

    case MFD_PARAM:
        {
            apreq_param_t *param;
            apreq_value_t *v;
            apr_size_t len;
            apr_off_t off;
            
            s = split_on_bdry(ctx->bb, ctx->in, ctx->pattern, ctx->bdry);

            switch (s) {

            case APR_INCOMPLETE:
                apreq_brigade_setaside(ctx->in, pool);
                apreq_brigade_setaside(ctx->bb, pool);
                return s;

            case APR_SUCCESS:
                s = apr_brigade_length(ctx->bb, 1, &off);
                if (s != APR_SUCCESS) {
                    ctx->status = MFD_ERROR;
                    return s;
                }
                len = off;
                param = apr_palloc(pool, len + sizeof *param);
                param->upload = NULL;
                param->info = ctx->info;

                *(const apreq_value_t **)&v = &param->v;
                v->name = ctx->param_name;
                apr_brigade_flatten(ctx->bb, v->data, &len);
                v->size = len;
                v->data[v->size] = 0;

                if (parser->hook != NULL) {
                    s = apreq_run_hook(parser->hook, param, NULL);
                    if (s != APR_SUCCESS) {
                        ctx->status = MFD_ERROR;
                        return s;
                    }
                }

                apr_table_addn(t, v->name, v->data);
                ctx->status = MFD_NEXTLINE;
                ctx->param_name = NULL;
                apr_brigade_cleanup(ctx->bb);
                goto mfd_parse_brigade;

            default:
                ctx->status = MFD_ERROR;
                return s;
            }


        }
        break;  /* not reached */

    case MFD_UPLOAD:
        {
            apreq_param_t *param = ctx->upload;

            s = split_on_bdry(ctx->bb, ctx->in, ctx->pattern, ctx->bdry);
            switch (s) {

            case APR_INCOMPLETE:
                if (parser->hook != NULL) {
                    s = apreq_run_hook(parser->hook, param, ctx->bb);
                    if (s != APR_SUCCESS) {
                        ctx->status = MFD_ERROR;
                        return s;
                    }
                }
                apreq_brigade_setaside(ctx->bb, pool);
                apreq_brigade_setaside(ctx->in, pool);
                s = apreq_brigade_concat(pool, parser->temp_dir, parser->brigade_limit, 
                                         param->upload, ctx->bb);
                return (s == APR_SUCCESS) ? APR_INCOMPLETE : s;

            case APR_SUCCESS:
                if (parser->hook != NULL) {
                    APR_BRIGADE_INSERT_TAIL(ctx->bb, ctx->eos);
                    s = apreq_run_hook(parser->hook, param, ctx->bb);
                    APR_BUCKET_REMOVE(ctx->eos);
                    if (s != APR_SUCCESS) {
                        ctx->status = MFD_ERROR;
                        return s;
                    }
                }
                apr_table_addn(t, param->v.name, param->v.data);
                apreq_brigade_setaside(ctx->bb, pool);
                s = apreq_brigade_concat(pool, parser->temp_dir, parser->brigade_limit,
                                         param->upload, ctx->bb);

                if (s != APR_SUCCESS)
                    return s;

                ctx->status = MFD_NEXTLINE;
                goto mfd_parse_brigade;

            default:
                ctx->status = MFD_ERROR;
                return s;
            }

        }
        break;  /* not reached */


    case MFD_MIXED:
        {
            s = apreq_run_parser(ctx->mix_parser, t, ctx->in);
            switch (s) {
            case APR_SUCCESS:
                ctx->status = MFD_INIT;
                goto mfd_parse_brigade;
            case APR_INCOMPLETE:
                apr_brigade_cleanup(ctx->in);
                return APR_INCOMPLETE;
            default:
                ctx->status = MFD_ERROR;
                return s;
            }

        }
        break; /* not reached */

    default:
        return APR_EGENERAL;
    }

    return APR_INCOMPLETE;
}