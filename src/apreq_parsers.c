/*
**  Copyright 2003-2004  The Apache Software Foundation
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**      http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

#include "apreq_params.h"
#include "apreq_parsers.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "apr_strmatch.h"
#include "apr_xml.h"
#include "apr_hash.h"

#ifndef MAX
#define MAX(A,B)  ( (A) > (B) ? (A) : (B) )
#endif
#ifndef MIN
#define MIN(A,B)  ( (A) < (B) ? (A) : (B) )
#endif

#ifndef CRLF
#define CRLF    "\015\012"
#endif

#define PARSER_STATUS_CHECK(PREFIX)   do {         \
    if (ctx->status == PREFIX##_ERROR)             \
        return APR_EGENERAL;                       \
    else if (ctx->status == PREFIX##_COMPLETE)     \
        return APR_SUCCESS;                        \
    else if (bb == NULL)                           \
        return APR_INCOMPLETE;                     \
} while (0);


APREQ_DECLARE(apreq_parser_t *)
    apreq_make_parser(apr_pool_t *pool,
                      apr_bucket_alloc_t *ba,
                      const char *content_type,
                      apreq_parser_function_t parser,
                      apr_size_t brigade_limit,
                      const char *temp_dir,
                      apreq_hook_t *hook,
                      void *ctx)
{
    apreq_parser_t *p = apr_palloc(pool, sizeof *p);
    p->content_type = content_type;
    p->parser = parser;
    p->hook = hook;
    p->pool = pool;
    p->bucket_alloc = ba;
    p->brigade_limit = brigade_limit;
    p->temp_dir = temp_dir;
    p->ctx = ctx;
    return p;
}

APREQ_DECLARE(apreq_hook_t *)
    apreq_make_hook(apr_pool_t *pool,
                    apreq_hook_function_t hook,
                    apreq_hook_t *next,
                    void *ctx)
{
    apreq_hook_t *h = apr_palloc(pool, sizeof *h);
    h->hook = hook;
    h->next = next;
    h->pool = pool;
    h->ctx = ctx;
    return h;
}


/*XXX this may need to check the parser's state before modifying the hook list */
APREQ_DECLARE(apr_status_t) apreq_parser_add_hook(apreq_parser_t *p, 
                                                  apreq_hook_t *h)
{
    apreq_hook_t *last = h;

    while (last->next)
        last = last->next;

    last->next = p->hook;
    p->hook = h;

    return APR_SUCCESS;
}

static apr_hash_t *default_parsers;
static apr_pool_t *default_parser_pool;

static void apreq_parser_initialize(void)
{
    if (default_parsers != NULL)
        return;
    apr_pool_create(&default_parser_pool, NULL);
    default_parsers = apr_hash_make(default_parser_pool);

    apreq_register_parser("application/x-www-form-urlencoded",
                          apreq_parse_urlencoded);
    apreq_register_parser("multipart/form-data", apreq_parse_multipart);
    apreq_register_parser("multipart/related", apreq_parse_multipart);
}

APREQ_DECLARE(void) apreq_register_parser(const char *enctype,
                                          apreq_parser_function_t parser)
{
    apreq_parser_function_t *f = NULL;

    if (enctype == NULL) {
        apreq_parser_initialize();
        return;
    }
    if (parser != NULL) {
        f = apr_palloc(default_parser_pool, sizeof *f);
        *f = parser;
    }
    apr_hash_set(default_parsers, apr_pstrdup(default_parser_pool, enctype),
                 APR_HASH_KEY_STRING, f);

}
APREQ_DECLARE(apreq_parser_function_t)apreq_parser(const char *enctype)
{
    apreq_parser_function_t *f;
    apr_size_t tlen = 0;

    if (enctype == NULL || default_parsers == NULL)
        return NULL;

    while(enctype[tlen] && enctype[tlen] != ';')
        ++tlen;

    f = apr_hash_get(default_parsers, enctype, tlen);

    if (f != NULL)
        return *f;
    else
        return NULL;
}


/******************** application/x-www-form-urlencoded ********************/

static apr_status_t split_urlword(apreq_param_t **p, apr_pool_t *pool,
                                  apr_bucket_brigade *bb,
                                  const apr_size_t nlen,
                                  const apr_size_t vlen)
{
    apreq_param_t *param;
    apreq_value_t *v;
    apr_bucket *e, *end;
    apr_status_t s;
    struct iovec vec[APREQ_DEFAULT_NELTS];
    apr_array_header_t arr;

    if (nlen == 0)
        return APR_EBADARG;

    param = apr_palloc(pool, nlen + vlen + 1 + sizeof *param);
    *(const apreq_value_t **)&v = &param->v;

    arr.pool     = pool;
    arr.elt_size = sizeof(struct iovec);
    arr.nelts    = 0;
    arr.nalloc   = APREQ_DEFAULT_NELTS;
    arr.elts     = (char *)vec;

    param->upload = NULL;
    param->info = NULL;

    v->name = v->data + vlen + 1;

    apr_brigade_partition(bb, nlen+1, &end);

    for (e = APR_BRIGADE_FIRST(bb); e != end; e = APR_BUCKET_NEXT(e)) {
        struct iovec *iov = apr_array_push(&arr);
        apr_size_t len;
        s = apr_bucket_read(e, (const char **)&iov->iov_base,
                            &len, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            return s;
        iov->iov_len = len;
    }

    ((struct iovec *)arr.elts)[arr.nelts - 1].iov_len--; /* drop '=' sign */

    s = apreq_decodev(v->name, &v->size,
                      (struct iovec *)arr.elts, arr.nelts);
    if (s != APR_SUCCESS)
        return s;

    while ((e = APR_BRIGADE_FIRST(bb)) != end)
        apr_bucket_delete(e);

    arr.nelts = 0;
    apr_brigade_partition(bb, vlen + 1, &end);

    for (e = APR_BRIGADE_FIRST(bb); e != end; e = APR_BUCKET_NEXT(e)) {
        struct iovec *iov = apr_array_push(&arr);
        apr_size_t len;
        s = apr_bucket_read(e, (const char **)&iov->iov_base, 
                            &len, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            return s;
        iov->iov_len = len;
    }

    if (end != APR_BRIGADE_SENTINEL(bb))
        ((struct iovec *)arr.elts)[arr.nelts - 1].iov_len--; /* drop '=' sign */

    s = apreq_decodev(v->data, &v->size,
                      (struct iovec *)arr.elts, arr.nelts);

    if (s != APR_SUCCESS)
        return s;

    while ((e = APR_BRIGADE_FIRST(bb)) != end)
        apr_bucket_delete(e);

    *p = param;
    return APR_SUCCESS;
}

struct url_ctx {
    apr_bucket_brigade *bb;
    enum {
        URL_NAME, 
        URL_VALUE,
        URL_COMPLETE,
        URL_ERROR
    }                   status;
};

APREQ_DECLARE_PARSER(apreq_parse_urlencoded)
{
    apr_pool_t *pool = parser->pool;
    apr_ssize_t nlen, vlen;
    apr_bucket *e;
    struct url_ctx *ctx;

    if (parser->ctx == NULL) {
        ctx = apr_palloc(pool, sizeof *ctx);
        ctx->bb = apr_brigade_create(pool, parser->bucket_alloc);
        parser->ctx = ctx;
        ctx->status = URL_NAME;
    }
    else
        ctx = parser->ctx;

    PARSER_STATUS_CHECK(URL);
    APR_BRIGADE_CONCAT(ctx->bb, bb);

 parse_url_brigade:

    ctx->status = URL_NAME;

    for (e  =  APR_BRIGADE_FIRST(ctx->bb), nlen = vlen = 0;
         e !=  APR_BRIGADE_SENTINEL(ctx->bb);
         e  =  APR_BUCKET_NEXT(e))
    {
        apreq_param_t *param;
        apr_size_t off = 0, dlen;
        const char *data;
        apr_status_t s;

        if (APR_BUCKET_IS_EOS(e)) {
            if (ctx->status == URL_NAME) {
                s = APR_SUCCESS;
            }
            else {
                s = split_urlword(&param, pool, ctx->bb, nlen, vlen);
                if (parser->hook != NULL && s == APR_SUCCESS)
                    s = APREQ_RUN_HOOK(parser->hook, param, NULL);

                if (s == APR_SUCCESS) {
                    apr_table_addn(t, param->v.name,  param->v.data);
                    ctx->status = URL_COMPLETE;
                }
                else {
                    ctx->status = URL_ERROR;
                }
            }

            APR_BRIGADE_CONCAT(bb, ctx->bb);
            return s;
        }

        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);
        if ( s != APR_SUCCESS ) {
            ctx->status = URL_ERROR;
            return s;
        }

    parse_url_bucket:

        switch (ctx->status) {

        case URL_NAME:
            while (off < dlen) {
                switch (data[off++]) {
                case '=':
                    ctx->status = URL_VALUE;
                    goto parse_url_bucket;
                default:
                    ++nlen;
                }
            }
            break;

        case URL_VALUE:
            while (off < dlen) {

                switch (data[off++]) {
                case '&':
                case ';':
                    s = split_urlword(&param, pool, ctx->bb, nlen, vlen);
                    if (parser->hook != NULL && s == APR_SUCCESS)
                        s = APREQ_RUN_HOOK(parser->hook, param, NULL);

                    if (s != APR_SUCCESS) {
                        ctx->status = URL_ERROR;
                        return s;
                    }

                    apr_table_addn(t, param->v.name, param->v.data);
                    goto parse_url_brigade;

                default:
                    ++vlen;
                }
            }
            break;
        default:
            ; /* not reached */
        }
    }
    APREQ_BRIGADE_SETASIDE(ctx->bb, pool);
    return APR_INCOMPLETE;
}


/********************* header parsing utils ********************/


static apr_status_t split_header_line(apreq_param_t **p,
                                      apr_pool_t *pool, 
                                      apr_bucket_brigade *bb,
                                      const apr_size_t nlen, 
                                      const apr_size_t glen,
                                      const apr_size_t vlen)
{
    apreq_param_t *param;
    apreq_value_t *v;
    apr_size_t off;

    if (nlen == 0)
        return APR_EBADARG;

    param = apr_palloc(pool, nlen + vlen + sizeof *param);
    param->upload = NULL;
    param->info = NULL;

    *(const apreq_value_t **)&v = &param->v;
    v->name = v->data + vlen; /* no +1, since vlen includes extra (CR)LF */

    /* copy name */

    off = 0;
    while (off < nlen) {
        apr_size_t dlen;
        const char *data;
        apr_bucket *f = APR_BRIGADE_FIRST(bb);
        apr_status_t s = apr_bucket_read(f, &data, &dlen, APR_BLOCK_READ);

        if ( s != APR_SUCCESS )
            return s;

        if (dlen > nlen - off) {
            apr_bucket_split(f, nlen - off);
            dlen = nlen - off;
        }

        memcpy(v->name + off, data, dlen);
        off += dlen;
        apr_bucket_delete(f);
    }

    /* skip gap */

    off = 0;
    while (off < glen) {
        apr_size_t dlen;
        const char *data;
        apr_bucket *f = APR_BRIGADE_FIRST(bb);
        apr_status_t s = apr_bucket_read(f, &data, &dlen, APR_BLOCK_READ);

        if ( s != APR_SUCCESS )
            return s;

        if (dlen > glen - off) {
            apr_bucket_split(f, glen - off);
            dlen = glen - off;
        }

        off += dlen;
        apr_bucket_delete(f);
    }

    /* copy value */

    off = 0;
    while (off < vlen) {
        apr_size_t dlen;
        const char *data;
        apr_bucket *f = APR_BRIGADE_FIRST(bb);
        apr_status_t s = apr_bucket_read(f, &data, &dlen, APR_BLOCK_READ);

        if ( s != APR_SUCCESS )
            return s;

        if (dlen > vlen - off) {
            apr_bucket_split(f, vlen - off);
            dlen = vlen - off;
        }

        memcpy(v->data + off, data, dlen);
        off += dlen;
        apr_bucket_delete(f);
    }

    v->name[nlen] = 0;

    /* remove trailing (CR)LF from value */
    v->size = vlen - 1;
    if ( v->size > 0 && v->data[v->size-1] == '\r')
        v->size--;

    v->data[v->size] = 0;

    *p = param;
    return APR_SUCCESS;

}

struct hdr_ctx {
    apr_bucket_brigade *bb;
   enum {
       HDR_NAME, 
       HDR_GAP, 
       HDR_VALUE, 
       HDR_NEWLINE, 
       HDR_CONTINUE,
       HDR_COMPLETE,
       HDR_ERROR
   }                    status;
};

APREQ_DECLARE_PARSER(apreq_parse_headers)
{
    apr_pool_t *pool = parser->pool;
    apr_ssize_t nlen, glen, vlen;
    apr_bucket *e;
    struct hdr_ctx *ctx;

    if (parser->ctx == NULL) {
        ctx = apr_pcalloc(pool, sizeof *ctx);
        ctx->bb = apr_brigade_create(pool, parser->bucket_alloc);
        parser->ctx = ctx;
    }
    else
        ctx = parser->ctx;

    PARSER_STATUS_CHECK(HDR);
    APR_BRIGADE_CONCAT(ctx->bb, bb);

 parse_hdr_brigade:

    ctx->status = HDR_NAME;

    /* parse the brigade for CRLF_CRLF-terminated header block, 
     * each time starting from the front of the brigade.
     */

    for (e = APR_BRIGADE_FIRST(ctx->bb), nlen = glen = vlen = 0;
         e != APR_BRIGADE_SENTINEL(ctx->bb); e = APR_BUCKET_NEXT(e))
    {
        apr_size_t off = 0, dlen;
        const char *data;
        apr_status_t s;
        apreq_param_t *param;
        if (APR_BUCKET_IS_EOS(e)) {
            ctx->status = HDR_COMPLETE;
            APR_BRIGADE_CONCAT(bb, ctx->bb);
            return APR_SUCCESS;
        }
        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);

        if ( s != APR_SUCCESS ) {
            ctx->status = HDR_ERROR;
            return s;
        }
        if (dlen == 0)
            continue;

    parse_hdr_bucket:

        /*              gap           nlen = 13
         *              vvv           glen =  3
         * Sample-Header:  grape      vlen =  5
         * ^^^^^^^^^^^^^   ^^^^^
         *     name        value
         */

        switch (ctx->status) {

        case HDR_NAME:

            while (off < dlen) {
                switch (data[off++]) {

                case '\n':
                    if (off < dlen)
                        apr_bucket_split(e, off);
                    e = APR_BUCKET_NEXT(e);

                    do {
                        apr_bucket *f = APR_BRIGADE_FIRST(ctx->bb);
                        apr_bucket_delete(f);
                    } while (e != APR_BRIGADE_FIRST(ctx->bb));
                    APR_BRIGADE_CONCAT(bb, ctx->bb);
                    ctx->status = HDR_COMPLETE;
                    return APR_SUCCESS;

                case ':':
                    ++glen; 
                    ctx->status = HDR_GAP;
                    goto parse_hdr_bucket;

                default:
                    ++nlen;
                }

            }

            break;


        case HDR_GAP:

            while (off < dlen) {
                switch (data[off++]) {
                case ' ':
                case '\t':
                    ++glen;
                    break;

                case '\n':
                    ctx->status = HDR_NEWLINE;
                    goto parse_hdr_bucket;

                default:
                    ctx->status = HDR_VALUE;
                    ++vlen;
                    goto parse_hdr_bucket;
                }
            }
            break;


        case HDR_VALUE:

            while (off < dlen) {
                ++vlen;
                if (data[off++] == '\n') {
                    ctx->status = HDR_NEWLINE;
                    goto parse_hdr_bucket;
                }
            }
            break;


        case HDR_NEWLINE:

            if (off == dlen) 
                break;
            else {
                switch (data[off]) {

                case ' ':
                case '\t':
                    ctx->status = HDR_CONTINUE;
                    ++off;
                    vlen += 2;
                    break;

                default:
                    /* can parse brigade now */
                    if (off > 0)
                        apr_bucket_split(e, off);
                    s = split_header_line(&param, pool, ctx->bb, nlen, glen, vlen);
                    if (parser->hook != NULL && s == APR_SUCCESS)
                        s = APREQ_RUN_HOOK(parser->hook, param, NULL);

                    if (s != APR_SUCCESS) {
                        ctx->status = HDR_ERROR;
                        return s;
                    }

                    apr_table_addn(t, param->v.name, param->v.data);
                    goto parse_hdr_brigade;
                }

                /* cases ' ', '\t' fall through to HDR_CONTINUE */
            }


        case HDR_CONTINUE:

            while (off < dlen) {
                switch (data[off++]) {
                case ' ':
                case '\t':
                    ++vlen;
                    break;

                case '\n':
                    ctx->status = HDR_NEWLINE;
                    goto parse_hdr_bucket;

                default:
                    ctx->status = HDR_VALUE;
                    ++vlen;
                    goto parse_hdr_bucket;
                }
            }
            break;

        default:
            ; /* not reached */
        }
    }
    APREQ_BRIGADE_SETASIDE(ctx->bb,pool);
    return APR_INCOMPLETE;
}


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
                APREQ_BRIGADE_SETASIDE(ctx->in, pool);
                APREQ_BRIGADE_SETASIDE(ctx->bb, pool);
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
                APREQ_BRIGADE_SETASIDE(ctx->in, pool);
                APREQ_BRIGADE_SETASIDE(ctx->bb, pool);
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
            s = APREQ_RUN_PARSER(ctx->hdr_parser, ctx->info, ctx->in);
            switch (s) {
            case APR_SUCCESS:
                ctx->status = MFD_POST_HEADER;
                break;
            case APR_INCOMPLETE:
                APREQ_BRIGADE_SETASIDE(ctx->in, pool);
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
                APREQ_BRIGADE_SETASIDE(ctx->in, pool);
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
                APREQ_BRIGADE_SETASIDE(ctx->in, pool);
                APREQ_BRIGADE_SETASIDE(ctx->bb, pool);
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
                    s = APREQ_RUN_HOOK(parser->hook, param, NULL);
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
                    s = APREQ_RUN_HOOK(parser->hook, param, ctx->bb);
                    if (s != APR_SUCCESS) {
                        ctx->status = MFD_ERROR;
                        return s;
                    }
                }
                APREQ_BRIGADE_SETASIDE(ctx->bb, pool);
                APREQ_BRIGADE_SETASIDE(ctx->in, pool);
                s = apreq_brigade_concat(pool, parser->temp_dir, parser->brigade_limit, 
                                         param->upload, ctx->bb);
                return (s == APR_SUCCESS) ? APR_INCOMPLETE : s;

            case APR_SUCCESS:
                if (parser->hook != NULL) {
                    APR_BRIGADE_INSERT_TAIL(ctx->bb, ctx->eos);
                    s = APREQ_RUN_HOOK(parser->hook, param, ctx->bb);
                    APR_BUCKET_REMOVE(ctx->eos);
                    if (s != APR_SUCCESS) {
                        ctx->status = MFD_ERROR;
                        return s;
                    }
                }
                apr_table_addn(t, param->v.name, param->v.data);
                APREQ_BRIGADE_SETASIDE(ctx->bb, pool);
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
            s = APREQ_RUN_PARSER(ctx->mix_parser, t, ctx->in);
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

APREQ_DECLARE_HOOK(apreq_hook_disable_uploads)
{
    return (bb == NULL) ? APR_SUCCESS : APR_EGENERAL;
}

APREQ_DECLARE_HOOK(apreq_hook_discard_brigade)
{
    apr_status_t s = APR_SUCCESS;
    if (hook->next)
        s = APREQ_RUN_HOOK(hook->next, param, bb);
    if (bb != NULL)
        apr_brigade_cleanup(bb);
    return s;
}


/* generic parser */

struct gen_ctx {
    apreq_param_t               *param;
    enum {
        GEN_INCOMPLETE,
        GEN_COMPLETE,
        GEN_ERROR
    }                            status;
};

APREQ_DECLARE_PARSER(apreq_parse_generic)
{
    struct gen_ctx *ctx = parser->ctx;
    apr_pool_t *pool = parser->pool;
    apr_status_t s = APR_SUCCESS;
    apr_bucket *e = APR_BRIGADE_LAST(bb);
    unsigned saw_eos = 0;

    if (ctx == NULL) {
        parser->ctx = ctx = apr_palloc(pool, sizeof *ctx);
        ctx->status = GEN_INCOMPLETE;
        ctx->param = apreq_make_param(pool, 
                                      "_dummy_", strlen("_dummy_"), "", 0);
        ctx->param->upload = apr_brigade_create(pool, parser->bucket_alloc);
        ctx->param->info = apr_table_make(pool, APREQ_DEFAULT_NELTS);
    }


    PARSER_STATUS_CHECK(GEN);

    while (e != APR_BRIGADE_SENTINEL(bb)) {
        if (APR_BUCKET_IS_EOS(e)) {
            saw_eos = 1;
            break;
        }
        e = APR_BUCKET_PREV(e);
    }

    if (parser->hook != NULL) {
        s = APREQ_RUN_HOOK(parser->hook, ctx->param, bb);
        if (s != APR_SUCCESS) {
            ctx->status = GEN_ERROR;
            return s;
        }
    }

    APREQ_BRIGADE_SETASIDE(bb, pool);
    s = apreq_brigade_concat(pool, parser->temp_dir, parser->brigade_limit,
                             ctx->param->upload, bb);

    if (s != APR_SUCCESS) {
        ctx->status = GEN_ERROR;
        return s;
    }

    if (saw_eos) {
        ctx->status = GEN_COMPLETE;
        return APR_SUCCESS;
    }
    else
        return APR_INCOMPLETE;
}


struct xml_ctx {
    apr_xml_doc                 *doc;
    apr_xml_parser              *xml_parser;
    enum {
        XML_INCOMPLETE,
        XML_COMPLETE,
        XML_ERROR
    }                            status;
};


APREQ_DECLARE_HOOK(apreq_hook_apr_xml_parser)
{
    apr_pool_t *pool = hook->pool;
    struct xml_ctx *ctx = hook->ctx;
    apr_status_t s = APR_SUCCESS;
    apr_bucket *e;

    if (ctx == NULL) {
        hook->ctx = ctx = apr_palloc(pool, sizeof *ctx);
        ctx->doc = NULL;
        ctx->xml_parser = apr_xml_parser_create(pool);
        ctx->status = XML_INCOMPLETE;
    }

    PARSER_STATUS_CHECK(XML);

    for (e = APR_BRIGADE_FIRST(bb); e != APR_BRIGADE_SENTINEL(bb);
         e = APR_BUCKET_NEXT(e))
    {
        const char *data;
        apr_size_t dlen;

        if (APR_BUCKET_IS_EOS(e)) {
            s = apr_xml_parser_done(ctx->xml_parser, &ctx->doc);
            if (s == APR_SUCCESS) {
                ctx->status = XML_COMPLETE;
                if (hook->next)
                    s = APREQ_RUN_HOOK(hook->next, param, bb);
            }
            else {
                ctx->status = XML_ERROR;
            }
           return s;
        }
        else if (APR_BUCKET_IS_METADATA(e)) {
            continue;
        }

        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);

        if (s != APR_SUCCESS) {
            ctx->status = XML_ERROR;
            return s;
        }

        s = apr_xml_parser_feed(ctx->xml_parser, data, dlen);

        if (s != APR_SUCCESS) {
            ctx->status = XML_ERROR;
            return s;
        }

    }

    if (hook->next)
        return APREQ_RUN_HOOK(hook->next, param, bb);

    return APR_SUCCESS;
}

