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

APREQ_DECLARE(apreq_parser_t *) apreq_make_parser(apr_pool_t *pool,
                                                  const char *enctype,
                                                  APREQ_DECLARE_PARSER(*parser),
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

APREQ_DECLARE(apreq_hook_t *) apreq_make_hook(apr_pool_t *pool,
                                              APREQ_DECLARE_HOOK(*hook),
                                              apreq_hook_t *next,
                                              void *ctx)
{
    apreq_hook_t *h = apr_palloc(pool, sizeof *h);
    h->hook = hook;
    h->next = next;
    h->ctx = ctx;
    return h;
}


APREQ_DECLARE(apr_status_t)apreq_add_hook(apreq_parser_t *p, 
                                          apreq_hook_t *h)
{
    apreq_hook_t *last = h;

    if (p == NULL || h == NULL)
        return APR_EINIT;

    while (last->next)
        last = last->next;

    last->next = p->hook;
    p->hook = h;
    return APR_SUCCESS;
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
    apr_size_t total, off;
    apreq_value_t *v = &param->v;

    param->bb = NULL;
    param->info = NULL;

    v->name = v->data + vlen + 1;

    off = 0;
    total = 0;
    while (total < nlen) {
        apr_size_t dlen;
        const char *data;
        apr_bucket *f = APR_BRIGADE_FIRST(bb);
        apr_status_t s = apr_bucket_read(f, &data, &dlen, APR_BLOCK_READ);
        apr_ssize_t decoded_len;

        if ( s != APR_SUCCESS )
            return s;

        total += dlen;

        if (total >= nlen) {
            dlen -= total - nlen;
            apr_bucket_split(f, dlen);
            if (data[dlen-1] == '=')
                --dlen;
        }

        decoded_len = apreq_decode((char *)v->name + off, data, dlen);

        if (decoded_len < 0) {
            return APR_BADARG;
        }

        off += decoded_len;
        apr_bucket_delete(f);
    }

    ((char *)v->name)[off] = 0;

    off = 0;
    total = 0;
    while (total < vlen) {
        apr_size_t dlen;
        const char *data;
        apr_bucket *f = APR_BRIGADE_FIRST(bb);
        apr_status_t s = apr_bucket_read(f, &data, &dlen, APR_BLOCK_READ);
        apr_ssize_t decoded_len;

        if ( s != APR_SUCCESS )
            return s;

        total += dlen;

        if (total >= vlen) {
            dlen -= total - vlen;
            apr_bucket_split(f, dlen);
            if (data[dlen-1] == '&' || data[dlen-1] == ';')
                --dlen;
        }

        decoded_len = apreq_decode(v->data + off, data, dlen);

        if (decoded_len < 0) {
            return APR_BADCH;
        }

        off += decoded_len;
        apr_bucket_delete(f);
    }

    v->data[off] = 0;
    v->size = off;
    apr_table_addn(t, v->name, v->data);
    return v->status = APR_SUCCESS;
}

struct url_ctx {
    apr_bucket_brigade *bb;
    enum {
        URL_NAME, 
        URL_VALUE
    }                   status;
};

APREQ_DECLARE_PARSER(apreq_parse_urlencoded)
{
    apr_pool_t *pool = apr_table_elts(t)->pool;
    apr_ssize_t nlen, vlen;
    apr_bucket *e;
    struct url_ctx *ctx;

    if (parser->ctx == NULL) {
        parser->ctx = apr_pcalloc(pool, sizeof *ctx);
        ctx = parser->ctx;
        ctx->bb = apr_brigade_create(pool, bb->bucket_alloc);
    }
    else
        ctx = parser->ctx;

    APR_BRIGADE_CONCAT(ctx->bb, bb);
    bb = ctx->bb;

 parse_url_brigade:

    ctx->status = URL_NAME;

    for (e  =  APR_BRIGADE_FIRST(bb), nlen = vlen = 0;
         e !=  APR_BRIGADE_SENTINEL(bb);
         e  =  APR_BUCKET_NEXT(e))
    {
        apr_size_t off = 0, dlen;
        const char *data;
        apr_status_t s;

        if (APR_BUCKET_IS_EOS(e)) {
            s = (ctx->status == URL_NAME) ? APR_SUCCESS : 
                split_urlword(t, bb, nlen+1, vlen);
            return s;
        }
        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);
        if ( s != APR_SUCCESS )
            return s;

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
                    s = split_urlword(t, bb, nlen+1, vlen+1);
                    if (s != APR_SUCCESS)
                        return s;
                    goto parse_url_brigade;
                default:
                    ++vlen;
                }
            }
            break;
        }
    }

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

    v->status = APR_SUCCESS;
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
   enum {
       HDR_NAME, 
       HDR_GAP, 
       HDR_VALUE, 
       HDR_NEWLINE, 
       HDR_CONTINUE
   }                    status;
};

APREQ_DECLARE_PARSER(apreq_parse_headers)
{
    apr_pool_t *pool = apreq_env_pool(env);
    apr_ssize_t nlen, glen, vlen;
    apr_bucket *e;
    struct hdr_ctx *ctx;

    if (parser->ctx == NULL)
        parser->ctx = apr_pcalloc(pool, sizeof *ctx);

    ctx = parser->ctx;


 parse_hdr_brigade:

    /* parse the brigade for CRLF_CRLF-terminated header block, 
     * each time starting from the front of the brigade.
     */
    ctx->status = HDR_NAME;

    for (e = APR_BRIGADE_FIRST(bb), nlen = glen = vlen = 0;
         e != APR_BRIGADE_SENTINEL(bb); e = APR_BUCKET_NEXT(e))
    {
        apr_size_t off = 0, dlen;
        const char *data;
        apr_status_t s;
        if (APR_BUCKET_IS_EOS(e))
            return ctx->status = APR_EOF;

        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);

        if ( s != APR_SUCCESS )
            return s;

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
                        apr_bucket *f = APR_BRIGADE_FIRST(bb);
                        apr_bucket_delete(f); 
                    } while (e != APR_BRIGADE_FIRST(bb));

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
                    s = split_header(t, bb, nlen, glen, vlen);
                    if (s != APR_SUCCESS)
                        return s;

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


        }
    }

    return APR_INCOMPLETE;
}


/********************* multipart/form-data *********************/

struct mfd_ctx {
    void                        *hook_data;
    apr_table_t                 *info;
    apr_bucket_brigade          *bb;
    apreq_parser_t              *hdr_parser;
    const apr_strmatch_pattern  *pattern;
    char                        *bdry;
    enum {
        MFD_INIT,  
        MFD_NEXTLINE, 
        MFD_HEADER, 
        MFD_PARAM, 
        MFD_UPLOAD, 
        MFD_ERROR 
    }                            status;
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
            /* prior (partial) strncmp failed, restart */
            do {
                apr_bucket *f = APR_BRIGADE_FIRST(in);
                APR_BUCKET_REMOVE(f);
                APR_BRIGADE_INSERT_TAIL(out, f);
            } while (e != APR_BRIGADE_FIRST(in));
            off = 0;
        }

        if (pattern != NULL && len >= blen) {
            const char *match = apr_strmatch(pattern, buf, len);
            if (match != NULL)
                idx = match - buf;
            else {
                idx = apreq_index(buf + len-blen, blen, bdry, blen, PARTIAL);
                if (idx >= 0)
                    idx += len-blen;
            }
        }
        else
            idx = apreq_index(buf, len, bdry, blen, PARTIAL);

        if (idx > 0)
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
    struct iovec v[APREQ_NELTS];
    apr_bucket_file *f;
    apr_bucket *e;
    apr_off_t wlen;
    int n = 0;

    if (APR_BUCKET_IS_EOS(last))
        return APR_EOF;

    if (! APR_BUCKET_IS_FILE(last)) {
        apr_bucket_brigade *bb;
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

        s = apreq_brigade_fwrite(file, &wlen, apreq_brigade_copy(out));

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

    s = apreq_brigade_fwrite(f->fd, &wlen, in);
    if (s == APR_SUCCESS)
        last->length += wlen;
    return s;
}


APREQ_DECLARE_PARSER(apreq_parse_multipart)
{
    apr_pool_t *pool = apreq_env_pool(env);
    struct mfd_ctx *ctx = parser->ctx;
    apr_off_t off;
    apr_status_t s;


    if (parser->ctx == NULL) {
        char *ct;
        apr_size_t blen;

        ctx = apr_pcalloc(pool, sizeof *ctx);

        ct = strchr(parser->enctype, ';');
        if (ct == NULL) {
            return APR_EINIT;
        }
        *ct++ = 0;

        s = apreq_header_attribute(ct, "boundary", 8,
                                   (const char **)&ctx->bdry, &blen);
        if (s != APR_SUCCESS)
            return s;
        
        ctx->bdry[blen] = 0;

        *--ctx->bdry = '-';
        *--ctx->bdry = '-';
        *--ctx->bdry = '\n';
        *--ctx->bdry = '\r';

        ctx->pattern = apr_strmatch_precompile(pool,ctx->bdry,1);
        ctx->hdr_parser = apreq_make_parser(pool, "", apreq_parse_headers,
                                            NULL,NULL);
        ctx->info = NULL;
        ctx->bb = apr_brigade_create(pool, bb->bucket_alloc);

        ctx->status = MFD_INIT;
        parser->ctx = ctx;
    }

 mfd_parse_brigade:

    switch (ctx->status) {

    case MFD_INIT:
        {
            s = split_on_bdry(ctx->bb, bb, NULL, ctx->bdry + 2);
            if (s != APR_SUCCESS) {
                return s;
            }
            ctx->status = MFD_NEXTLINE;
        }
        /* fall through */

    case MFD_NEXTLINE:
        {
            apr_status_t s;
            s = split_on_bdry(ctx->bb, bb, NULL, CRLF);
            if (s != APR_SUCCESS) {
                return s;
            }
            ctx->status = MFD_HEADER;
            apr_brigade_cleanup(ctx->bb);
            ctx->info = NULL;
        }
        /* fall through */

    case MFD_HEADER:
        {
            apr_status_t s;
            const char *cd, *name, *filename;
            apr_size_t nlen, flen;

            if (ctx->info == NULL)
                ctx->info = apr_table_make(pool, APREQ_NELTS);

            s = apreq_run_parser(ctx->hdr_parser, env, ctx->info, bb);

            if (s != APR_SUCCESS)
                return (s == APR_EOF) ? APR_SUCCESS : s;

            cd = apr_table_get(ctx->info, "Content-Disposition");

            if (cd == NULL) {
                ctx->status = MFD_ERROR;
                return APR_BADARG;
            }

            s = apreq_header_attribute(cd, "name", 4, &name, &nlen);

            if (s != APR_SUCCESS) {
                ctx->status = MFD_ERROR;
                return APR_BADARG;
            }

            s = apreq_header_attribute(cd, "filename", 8, &filename, &flen);

            if (s != APR_SUCCESS) {
                apr_bucket *e;
                name = apr_pstrmemdup(pool, name, nlen);
                e = apr_bucket_transient_create(name, nlen,
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
                param->v.status = APR_INCOMPLETE;
                apr_table_addn(t, param->v.name, param->v.data);
                ctx->status = MFD_UPLOAD;
            }
            /* XXX must handle special case of missing CRLF (mainly
               coming from empty file uploads). See RFC2065 S5.1.1:

                 body-part = MIME-part-header [CRLF *OCTET]

               So the CRLF we already matched may have been part of
               the boundary string! Both Konqueror (v??) and Mozilla-0.97
               are known to emit such blocks.
            */
        }
        goto mfd_parse_brigade;

    case MFD_PARAM:
        {
            apr_status_t s = split_on_bdry(ctx->bb, bb, 
                                           ctx->pattern, ctx->bdry);
            apr_bucket *e;
            apreq_param_t *param;
            apreq_value_t *v;
            const char *name;
            apr_size_t len;
            apr_off_t off;

            switch (s) {

            case APR_INCOMPLETE:
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
                v->status = APR_SUCCESS;
                apr_brigade_flatten(ctx->bb, v->data, &len);
                v->size = len;
                v->data[v->size] = 0;
                apr_table_addn(t, v->name, v->data);
                ctx->status = MFD_NEXTLINE;
                goto mfd_parse_brigade;

            default:
                ctx->status = MFD_ERROR;
                return s;
            }


        }
        break;  /* not reached */

    case MFD_UPLOAD:
        {
            apr_status_t s = split_on_bdry(ctx->bb, bb, 
                                           ctx->pattern, ctx->bdry);
            apreq_param_t *param;
            const apr_array_header_t *arr;
            apr_table_entry_t *e;

            arr = apr_table_elts(t);
            e = (apr_table_entry_t *)arr->elts;
            param = apreq_value_to_param(apreq_strtoval(e[arr->nelts-1].val));

            switch (s) {

            case APR_INCOMPLETE:
                if (parser->hook) {
                    s = apreq_run_hook(parser->hook, env, param, ctx->bb);
                    if (s != APR_INCOMPLETE && s != APR_SUCCESS)
                        return s;
                }
                s = apreq_brigade_concat(env, param->bb, ctx->bb);
                return (s == APR_SUCCESS) ? APR_INCOMPLETE : s;

            case APR_SUCCESS:
                if (parser->hook) {
                    apr_bucket *eos = 
                        apr_bucket_eos_create(ctx->bb->bucket_alloc);
                    APR_BRIGADE_INSERT_TAIL(ctx->bb, eos);
                    s = apreq_run_hook(parser->hook, env, param, ctx->bb);
                    apr_bucket_delete(eos);
                    if (s != APR_SUCCESS)
                        return s;
                }

                param->v.status = apreq_brigade_concat(env,
                                                       param->bb, ctx->bb);

                if (param->v.status != APR_SUCCESS)
                    return s;

                ctx->status = MFD_NEXTLINE;
                goto mfd_parse_brigade;

            default:
                ctx->status = MFD_ERROR;
                return s;
            }

        }
        break;  /* not reached */

    case MFD_ERROR:
        return APR_EGENERAL;
    }

    return APR_INCOMPLETE;
}
