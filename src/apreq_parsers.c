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
#include "apreq_env.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "apr_strmatch.h"

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
                      const char *enctype,
                      apr_status_t (*parser) (APREQ_PARSER_ARGS),
                      apreq_hook_t *hook,
                      void *ctx)
{
    apreq_parser_t *p = apr_palloc(pool, sizeof *p);
    p->enctype = apr_pstrdup(pool,enctype);
    p->parser = parser;
    p->hook = hook;
    p->ctx = ctx;
    return p;
}

APREQ_DECLARE(apreq_hook_t *)
    apreq_make_hook(apr_pool_t *pool,
                    apr_status_t (*hook) (APREQ_HOOK_ARGS),
                    apreq_hook_t *next,
                    void *ctx)
{
    apreq_hook_t *h = apr_palloc(pool, sizeof *h);
    h->hook = hook;
    h->next = next;
    h->ctx = ctx;
    return h;
}


APREQ_DECLARE(void) apreq_add_hook(apreq_parser_t *p, 
                                   apreq_hook_t *h)
{
    apreq_hook_t *last = h;

    while (last->next)
        last = last->next;

    last->next = p->hook;
    p->hook = h;
}

APREQ_DECLARE(apreq_parser_t *)apreq_parser(void *env, apreq_hook_t *hook)
{
    apr_pool_t *pool = apreq_env_pool(env);
    const char *type = apreq_env_content_type(env);

    if (type == NULL)
        return NULL;

    if (!strncasecmp(type, APREQ_URL_ENCTYPE,strlen(APREQ_URL_ENCTYPE)))
        return apreq_make_parser(pool, type, 
                                 apreq_parse_urlencoded, hook, NULL);

    else if (!strncasecmp(type,APREQ_MFD_ENCTYPE,strlen(APREQ_MFD_ENCTYPE)))
        return apreq_make_parser(pool, type, 
                                 apreq_parse_multipart, hook, NULL);

    else
        return NULL;

}



/******************** application/x-www-form-urlencoded ********************/

static apr_status_t split_urlword(apr_table_t *t,
                                  apr_bucket_brigade *bb, 
                                  const apr_size_t nlen,
                                  const apr_size_t vlen)
{
    apr_pool_t *pool = apr_table_elts(t)->pool;
    apreq_param_t *param = apr_palloc(pool, nlen + vlen + 1 + sizeof *param);
    apreq_value_t *v = &param->v;
    apr_bucket *e, *end;
    apr_status_t s;
    struct iovec vec[APREQ_NELTS];
    apr_array_header_t arr = { pool, sizeof(struct iovec), 0,
                               APREQ_NELTS, (char *)vec };

    param->bb = NULL;
    param->info = NULL;

    v->name = v->data + vlen + 1;

    apr_brigade_partition(bb, nlen+1, &end);

    for (e = APR_BRIGADE_FIRST(bb); e != end; e = APR_BUCKET_NEXT(e)) {
        apr_size_t dlen;
        const char *data;
        struct iovec *iov;
        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            return s;

        iov = apr_array_push(&arr);
        iov->iov_base = (char *)data;
        iov->iov_len  = dlen;
    }

    ((struct iovec *)arr.elts)[arr.nelts - 1].iov_len--; /* drop '=' sign */

    s = apreq_decodev((char *)v->name, &v->size,
                      (struct iovec *)arr.elts, arr.nelts);
    if (s != APR_SUCCESS)
        return s;

    while ((e = APR_BRIGADE_FIRST(bb)) != end)
        apr_bucket_delete(e);

    arr.nelts = 0;
    apr_brigade_partition(bb, vlen + 1, &end);

    for (e = APR_BRIGADE_FIRST(bb); e != end; e = APR_BUCKET_NEXT(e)) {
        apr_size_t dlen;
        const char *data;
        struct iovec *vec;
        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            return s;

        vec = apr_array_push(&arr);
        vec->iov_base = (char *)data;
        vec->iov_len  = dlen;
    }

    if (end != APR_BRIGADE_SENTINEL(bb))
        ((struct iovec *)arr.elts)[arr.nelts - 1].iov_len--; /* drop '=' sign */

    s = apreq_decodev(v->data, &v->size,
                      (struct iovec *)arr.elts, arr.nelts);
    if (s != APR_SUCCESS)
        return s;

    while ((e = APR_BRIGADE_FIRST(bb)) != end)
        apr_bucket_delete(e);

    apr_table_addn(t, v->name, v->data);
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
    apr_pool_t *pool = apr_table_elts(t)->pool;
    apr_ssize_t nlen, vlen;
    apr_bucket *e;
    struct url_ctx *ctx;

    if (parser->ctx == NULL) {
        apr_bucket_alloc_t *bb_alloc = apr_bucket_alloc_create(pool);
        ctx = apr_palloc(pool, sizeof *ctx);
        ctx->bb = apr_brigade_create(pool, bb_alloc);
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
        apr_size_t off = 0, dlen;
        const char *data;
        apr_status_t s;

        if (APR_BUCKET_IS_EOS(e)) {
            s = (ctx->status == URL_NAME) ? APR_SUCCESS : 
                split_urlword(t, ctx->bb, nlen, vlen);
            APR_BRIGADE_CONCAT(bb, ctx->bb);
            ctx->status = (s == APR_SUCCESS) ? URL_COMPLETE : URL_ERROR;
            apreq_log(APREQ_DEBUG s, env, "url parser saw EOS");
            return s;
        }
        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);
        if ( s != APR_SUCCESS ) {
            ctx->status = URL_ERROR;
            apreq_log(APREQ_ERROR s, env, "apr_bucket_read failed");
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
                    s = split_urlword(t, ctx->bb, nlen, vlen);
                    if (s != APR_SUCCESS) {
                        ctx->status = URL_ERROR;
                        apreq_log(APREQ_ERROR s, env, "split_urlword failed");
                        return s;
                    }
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


static apr_status_t split_header(apr_table_t *t, 
                                 apr_bucket_brigade *bb,
                                 const apr_size_t nlen, 
                                 const apr_size_t glen,
                                 const apr_size_t vlen)
{
    apr_pool_t *pool = apr_table_elts(t)->pool;
    apreq_value_t *v = apr_palloc(pool, nlen + vlen + sizeof *v);
    apr_size_t off;

    v->name = v->data + vlen;

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

        memcpy((char *)v->name + off, data, dlen);
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

    ((char *)v->name)[nlen] = 0;

    /* remove trailing (CR)LF from value */
    v->size = vlen - 1;
    if ( v->size > 0 && v->data[v->size-1] == '\r')
        v->size--;

    v->data[v->size] = 0;

    apr_table_addn(t, v->name, v->data);
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
    apr_pool_t *pool = apreq_env_pool(env);
    apr_ssize_t nlen, glen, vlen;
    apr_bucket *e;
    struct hdr_ctx *ctx;

    if (parser->ctx == NULL) {
        apr_bucket_alloc_t *bb_alloc = apr_bucket_alloc_create(pool);
        ctx = apr_pcalloc(pool, sizeof *ctx);
        ctx->bb = apr_brigade_create(pool, bb_alloc);
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
                    s = split_header(t, ctx->bb, nlen, glen, vlen);
                    if (s != APR_SUCCESS) {
                        ctx->status = HDR_ERROR;
                        return s;
                    }
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
    void                        *hook_data;
    apr_table_t                 *info;
    apr_bucket_brigade          *in;
    apr_bucket_brigade          *bb;
    apreq_parser_t              *hdr_parser;
    const apr_strmatch_pattern  *pattern;
    char                        *bdry;
    enum {
        MFD_INIT,  
        MFD_NEXTLINE, 
        MFD_HEADER,
        MFD_POST_HEADER,
        MFD_PARAM, 
        MFD_UPLOAD,
        MFD_COMPLETE,
        MFD_ERROR 
    }                            status;
    apr_bucket                   *eos;
    apreq_param_t                *upload;
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

#define MAX_FILE_BUCKET_LENGTH ((apr_off_t) 1 << (6 * sizeof(apr_size_t)))

APREQ_DECLARE(apr_status_t) apreq_brigade_concat(void *env,
                                                 apr_bucket_brigade *out, 
                                                 apr_bucket_brigade *in)
{
    apr_bucket *last = APR_BRIGADE_LAST(out);
    apr_status_t s;
    apr_bucket_file *f;
    apr_bucket *e;
    apr_off_t wlen;

    if (APR_BUCKET_IS_EOS(last))
        return APR_EOF;

    if (! APR_BUCKET_IS_FILE(last)) {
        apr_file_t *file;
        apr_off_t len, max_brigade = apreq_env_max_brigade(env,-1);

        s = apr_brigade_length(out, 1, &len);
        if (s != APR_SUCCESS)
            return s;

        if (max_brigade >= 0 && len < max_brigade) {
            APR_BRIGADE_CONCAT(out, in);
            return APR_SUCCESS;
        }

        s = apreq_file_mktemp(&file, apreq_env_pool(env), 
                              apreq_env_temp_dir(env,NULL));
        if (s != APR_SUCCESS)
            return s;

        s = apreq_brigade_fwrite(file, &wlen, out);

        if (s != APR_SUCCESS)
            return s;

        last = apr_bucket_file_create(file, wlen, 0, out->p, out->bucket_alloc);
        APR_BRIGADE_INSERT_TAIL(out, last);
    }

    f = last->data;

    if (last->length > MAX_FILE_BUCKET_LENGTH) {
        apr_bucket_copy(last, &e);
        APR_BRIGADE_INSERT_TAIL(out, e);
        e->length = 0;
        e->start = last->length + 1;
        last = e;
    }

    e = APR_BRIGADE_LAST(in);
    if (APR_BUCKET_IS_EOS(e)) {
        APR_BUCKET_REMOVE(e);
        APR_BRIGADE_INSERT_TAIL(out, e);
    }

    /* Write the remaining buckets to disk, then delete them. */
    s = apreq_brigade_fwrite(f->fd, &wlen, in);
    apr_brigade_cleanup(in);

    if (s == APR_SUCCESS)
        last->length += wlen;
    return s;
}


APREQ_DECLARE_PARSER(apreq_parse_multipart)
{
    apr_pool_t *pool = apreq_env_pool(env);
    struct mfd_ctx *ctx = parser->ctx;
    apr_status_t s;

    if (ctx == NULL) {
        char *ct;
        apr_size_t blen;
        apr_bucket_alloc_t *bucket_alloc = apr_bucket_alloc_create(pool);

        ctx = apr_pcalloc(pool, sizeof *ctx);
        parser->ctx = ctx;
        ctx->status = MFD_INIT;

        ct = strchr(parser->enctype, ';');
        if (ct == NULL) {
            ctx->status = MFD_ERROR;
            apreq_log(APREQ_ERROR APR_EGENERAL, env, 
                      "mfd parser cannot find required "
                      "semicolon in Content-Type header");
            return APR_EGENERAL;
        }
        *ct++ = 0;

        s = apreq_header_attribute(ct, "boundary", 8,
                                   (const char **)&ctx->bdry, &blen);
        if (s != APR_SUCCESS) {
            ctx->status = MFD_ERROR;
            apreq_log(APREQ_ERROR APR_EGENERAL, env,
                      "mfd parser cannot find boundary "
                      "attribute in Content-Type header");
            return s;
        }
        ctx->bdry[blen] = 0;

        *--ctx->bdry = '-';
        *--ctx->bdry = '-';
        *--ctx->bdry = '\n';
        *--ctx->bdry = '\r';

        ctx->pattern = apr_strmatch_precompile(pool,ctx->bdry,1);
        ctx->hdr_parser = apreq_make_parser(pool, "", apreq_parse_headers,
                                            NULL,NULL);
        ctx->info = NULL;
        ctx->bb = apr_brigade_create(pool, bucket_alloc);
        ctx->in = apr_brigade_create(pool, bucket_alloc);
        ctx->eos = apr_bucket_eos_create(bucket_alloc);
    }
    else if (ctx->status == MFD_COMPLETE) {
        apreq_log(APREQ_DEBUG APR_SUCCESS, env, 
                  "mfd parser is already complete- "
                  "all further input brigades will be ignored.");
        return APR_SUCCESS;
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
                ctx->info = apr_table_make(pool, APREQ_NELTS);
                /* flush out old header parser internal structs for reuse */
                ctx->hdr_parser->ctx = NULL;
            }
            s = APREQ_RUN_PARSER(ctx->hdr_parser, env, ctx->info, ctx->in);
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

            if (cd == NULL) {
                ctx->status = MFD_ERROR;
                return APR_EGENERAL;
            }

            s = apreq_header_attribute(cd, "name", 4, &name, &nlen);

            if (s != APR_SUCCESS) {
                ctx->status = MFD_ERROR;
                return APR_EGENERAL;
            }

            s = apreq_header_attribute(cd, "filename", 8, &filename, &flen);

            if (s != APR_SUCCESS) {
                name = apr_pstrmemdup(pool, name, nlen);
                e = apr_bucket_immortal_create(name, nlen,
                                                ctx->bb->bucket_alloc);
                APR_BRIGADE_INSERT_HEAD(ctx->bb,e);
                ctx->status = MFD_PARAM;
            } 
            else {
                apreq_param_t *param = apreq_make_param(pool, name, nlen, 
                                                        filename, flen);
                param->info = ctx->info;
                param->bb = apr_brigade_create(pool, 
                                               apr_bucket_alloc_create(pool));
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
            const char *name;
            apr_size_t len;
            apr_off_t off;
            apr_bucket *e;
            
            s = split_on_bdry(ctx->bb, ctx->in, ctx->pattern, ctx->bdry);

            switch (s) {

            case APR_INCOMPLETE:
                APREQ_BRIGADE_SETASIDE(ctx->in, pool);
                APREQ_BRIGADE_SETASIDE(ctx->bb,pool);
                return s;

            case APR_SUCCESS:
                e = APR_BRIGADE_FIRST(ctx->bb);
                apr_bucket_read(e, &name, &len, APR_BLOCK_READ);
                apr_bucket_delete(e);

                s = apr_brigade_length(ctx->bb, 1, &off);
                if (s != APR_SUCCESS) {
                    ctx->status = MFD_ERROR;
                    return s;
                }
                len = off;
                param = apr_palloc(pool, len + sizeof *param);
                param->bb = NULL;
                param->info = ctx->info;

                v = &param->v;
                v->name = name;
                apr_brigade_flatten(ctx->bb, v->data, &len);
                v->size = len;
                v->data[v->size] = 0;
                apr_table_addn(t, v->name, v->data);
                ctx->status = MFD_NEXTLINE;
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
                if (parser->hook) {
                    s = APREQ_RUN_HOOK(parser->hook, env, param, ctx->bb);
                    if (s != APR_INCOMPLETE && s != APR_SUCCESS) {
                        ctx->status = MFD_ERROR;
                        return s;
                    }
                }
                APREQ_BRIGADE_SETASIDE(ctx->bb, pool);
                APREQ_BRIGADE_SETASIDE(ctx->in, pool);
                s = apreq_brigade_concat(env, param->bb, ctx->bb);
                return (s == APR_SUCCESS) ? APR_INCOMPLETE : s;

            case APR_SUCCESS:
                if (parser->hook) {
                    APR_BRIGADE_INSERT_TAIL(ctx->bb, ctx->eos);
                    s = APREQ_RUN_HOOK(parser->hook, env, param, ctx->bb);
                    APR_BUCKET_REMOVE(ctx->eos);
                    if (s != APR_SUCCESS) {
                        ctx->status = MFD_ERROR;
                        return s;
                    }
                }
                apr_table_addn(t, param->v.name, param->v.data);
                APREQ_BRIGADE_SETASIDE(ctx->bb, pool);
                s = apreq_brigade_concat(env, param->bb, ctx->bb);

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

    default:
        return APR_EGENERAL;
    }

    return APR_INCOMPLETE;
}

APREQ_DECLARE_HOOK(apreq_hook_disable_uploads)
{
    apreq_log(APREQ_ERROR APR_EGENERAL, env, 
              "Uploads are disabled for this request.");
    return APR_EGENERAL;
}
