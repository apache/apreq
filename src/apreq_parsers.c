/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
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
 *
 * Portions of this software are based upon public domain software
 * originally written at the National Center for Supercomputing Applications,
 * University of Illinois, Urbana-Champaign.
 */

#include "apreq_parsers.h"
#include "apreq_env.h"

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
                                                  APREQ_PARSER(*parser),
                                                  APREQ_HOOK(*hook),
                                                  void *out)
{
    apreq_parser_t *p = apr_palloc(pool, APREQ_CTX_MAXSIZE + sizeof *p);

    p->v.name = apr_pstrdup(pool, enctype);
    p->v.size = 0;
    p->v.status = APR_SUCCESS;

    p->parser = parser;
    p->hook = hook;
    p->out = out;
    return p;
}


APREQ_DECLARE(apr_status_t) apreq_register_parser(apreq_request_t *req,
                                                  const char *enctype,
                                                  APREQ_PARSER(*parser),
                                                  APREQ_HOOK(*hook),
                                                  void *out)
{
    apreq_parser_t *p = apreq_make_parser(req->pool, enctype, 
                                          parser, hook, out);
    return apreq_env_add_parser(req->ctx, &p->v);
}


static apr_status_t split_urlword(apr_pool_t *pool, apreq_table_t *t,
                                  apr_bucket_brigade *bb, 
                                  const apr_size_t nlen,
                                  const apr_size_t vlen)
{
    apreq_param_t *param = apr_palloc(pool, nlen + vlen + 1 + sizeof *param);
    apr_size_t total, off;
    const apr_size_t glen = 1;
    apreq_value_t *v = &param->v;

    param->bb = NULL; /* XXX: could use to retain original encoded data */
    param->info = NULL;
    param->charset = UTF_8;
    param->language = NULL;

    v->name = v->data + vlen;

    off = 0;
    total = 0;
    while (total < nlen) {
        apr_size_t dlen;
        const char *data;
        apr_bucket *f = APR_BRIGADE_FIRST(bb);
        apr_status_t s = apr_bucket_read(f, &data, &dlen, APR_BLOCK_READ);
        apr_ssize_t rv;

        if ( s != APR_SUCCESS )
            return s;

        if (dlen > nlen - total) {
            apr_bucket_split(f, nlen - total);
            dlen = nlen - total;
        }

        rv = apreq_decode((char *)v->name + off, data, dlen);

        if (rv < 0) {
            return APR_BADARG;
        }

        total += dlen;
        off += rv;
        apr_bucket_delete(f);
    }

    ((char *)v->name)[off] = 0;

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


    off = 0;
    total = 0;
    while (total < vlen) {
        apr_size_t dlen;
        const char *data;
        apr_bucket *f = APR_BRIGADE_FIRST(bb);
        apr_status_t s = apr_bucket_read(f, &data, &dlen, APR_BLOCK_READ);
        apr_ssize_t rv;

        if ( s != APR_SUCCESS )
            return s;

        if (dlen > vlen - off) {
            apr_bucket_split(f, vlen - total);
            dlen = vlen - total;
        }

        rv = apreq_decode(v->data + off, data, dlen);

        if (rv < 0) {
            return APR_BADCH;
        }

        total += dlen;
        off += rv;
        apr_bucket_delete(f);
    }

    v->data[off] = 0;
    v->size = off;
    v->status = APR_SUCCESS;
    return apreq_table_add(t, v);

}


APREQ_DECLARE(apr_status_t) apreq_parse_urlencoded(apr_pool_t *pool,
                                                   apr_bucket_brigade *bb,
                                                   apreq_parser_t *parser)
{
    apreq_request_t *req = (apreq_request_t *)parser->out;
    apreq_table_t *t = req->body;

    apr_ssize_t nlen, vlen;
    apr_bucket *e;

/* use parser->v.status to maintain state */
#define URL_NAME        0
#define URL_VALUE       1


 parse_url_brigade:

    /* parse the brigade for CRLF_CRLF-terminated header block, 
     * each time starting from the front of the brigade.
     */
    parser->v.status = URL_NAME;

    for (e  =  APR_BRIGADE_FIRST(bb), nlen = vlen = 0;
         e !=  APR_BRIGADE_SENTINEL(bb);
         e  =  APR_BUCKET_NEXT(e))
    {
        apr_size_t off = 0, dlen;
        const char *data;
        apr_status_t s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);

        if (APR_BUCKET_IS_EOS(e))
            return nlen == 0 ? APR_SUCCESS : 
                split_urlword(pool, t, bb, nlen, vlen);

        if ( s != APR_SUCCESS )
            return s;


    parse_url_bucket:

        switch (parser->v.status) {


        case URL_NAME:

            while (off < dlen) {
                switch (data[off++]) {
                case '=':
                    parser->v.status = URL_VALUE;
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
                    s = split_urlword(pool, t, bb, nlen, vlen + 1);
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

static apr_status_t split_header(apr_pool_t *pool, apreq_table_t *t, 
                                 apr_bucket_brigade *bb,
                                 const apr_size_t nlen, 
                                 const apr_size_t glen,
                                 const apr_size_t vlen)
{
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

    return apreq_table_add(t, v);

}


APREQ_DECLARE(apr_status_t) apreq_parse_headers(apr_pool_t *pool,
                                                apr_bucket_brigade *bb,
                                                apreq_parser_t *parser)
{
    apreq_table_t *t = (apreq_table_t *) parser->out;
    apr_ssize_t nlen, glen, vlen;
    apr_bucket *e;

/* use parser->v.status to maintain state */
#define HDR_NAME        0
#define HDR_GAP         1
#define HDR_VALUE       2
#define HDR_NEWLINE     3
#define HDR_CONTINUE    4


 parse_hdr_brigade:

    /* parse the brigade for CRLF_CRLF-terminated header block, 
     * each time starting from the front of the brigade.
     */
    parser->v.status = HDR_NAME;

    for (e = APR_BRIGADE_FIRST(bb), nlen = glen = vlen = 0;
         ! APR_BUCKET_IS_EOS(e) && e != APR_BRIGADE_SENTINEL(bb);
         e = APR_BUCKET_NEXT(e))
    {
        apr_size_t off = 0, dlen;
        const char *data;
        apr_status_t s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);

        if ( s != APR_SUCCESS )
            return s;


    parse_hdr_bucket:

        /*              gap           nlen = 13
         *              vvv           glen =  3
         * Sample-Header:  grape      vlen =  5
         * ^^^^^^^^^^^^^   ^^^^^
         *     name        value
         */

        switch (parser->v.status) {


        case HDR_NAME:

            while (off < dlen) {
                switch (data[off++]) {

                case '\n':
                    if (off < dlen) {
                        apr_bucket_split(e, off);                       
                    }

                    e = APR_BUCKET_NEXT(e);

                    do apr_bucket_delete( APR_BRIGADE_FIRST(bb) ); 
                    while (e != APR_BRIGADE_FIRST(bb));

                    return APR_SUCCESS;

                case ':':
                    ++glen; 
                    ++off;
                    parser->v.status = HDR_GAP;
                    goto parse_hdr_bucket;

                default:
                    ++nlen;
                    ++off;
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
                    parser->v.status = HDR_NEWLINE;
                    goto parse_hdr_bucket;

                default:
                    parser->v.status = HDR_VALUE;
                    ++vlen;
                    goto parse_hdr_bucket;
                }
            }
            break;


        case HDR_VALUE:

            while (off < dlen) {
                if (data[off++] != '\n')
                    ++vlen;
                else {
                    --vlen;
                    parser->v.status = HDR_NEWLINE;
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
                    parser->v.status = HDR_CONTINUE;
                    ++off;
                    vlen += 2;
                    break;

                default:
                    /* can parse brigade now */
                    if (off > 0) {
                        apr_bucket_split(e, off - 1);
                        e = APR_BUCKET_NEXT(e);
                    }

                    s = split_header(pool, t, bb, nlen, glen, vlen);
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
                    parser->v.status = HDR_NEWLINE;
                    goto parse_hdr_bucket;

                default:
                    parser->v.status = HDR_VALUE;
                    ++vlen;
                    goto parse_hdr_bucket;
                }
            }
            break;


        }
    }

    return APR_INCOMPLETE;
}

struct mfd_ctx {
    void                *hook_data;
    apreq_table_t       *t;
    apr_bucket_brigade  *bb;
    char                bdry[1];
};


static apr_status_t split_on_bdry(apr_pool_t *pool, 
                                  apr_bucket_brigade *out,
                                  apr_bucket_brigade *in,
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
            e = APR_BUCKET_NEXT(e);
            continue;
        }

        if (strncmp(bdry + off, buf, MIN(len, blen - off)) == 0) {
            if ( len >= blen - off ) {
                /* complete match */
                apr_bucket_split(e, blen - off);
                e = APR_BUCKET_NEXT(e);

                do apr_bucket_delete( APR_BRIGADE_FIRST(in) );
                while (APR_BRIGADE_FIRST(in) != e);

                return APR_SUCCESS;
            }
            /* partial match */
            off += len;
            e = APR_BUCKET_NEXT(e);
            continue;
        }
        else if (off > 0) {
            /* prior (partial) strncmp failed, restart */
            apr_bucket *f;
            do {
                f = APR_BRIGADE_FIRST(in);
                APR_BUCKET_REMOVE(f);
                APR_BRIGADE_INSERT_TAIL(out, f);
            } while (e != f);
            off = 0;
        }

        idx = apreq_index(buf, len, bdry, blen, PARTIAL);

        if (idx > 0)
            apr_bucket_split(e, idx);

        APR_BUCKET_REMOVE(e);
        APR_BRIGADE_INSERT_TAIL(out, e);
        e = APR_BRIGADE_FIRST(in);
    }

    return APR_INCOMPLETE;
}

static apr_status_t getval(const char **line, const char *name,
                           const char **val, apr_size_t *vlen)
{
    const char *v = strstr(*line, name);
    int in_quotes = 0;

    if (v == NULL)
        return APR_NOTFOUND;

    v += strlen(name);

    while (*v) {
        if (*v == '=' || apr_isspace(*v))
            ++v;
        else
            break;
    }

    if (*v == '"') {
        ++v;
        in_quotes = 1;
    }
    
    for(*val = v; *v; ++v) {
        switch (*v) {
        case ';':
            if (in_quotes)
                continue;
            /* else fall through */
        case '"':
            break;
        case '\\':
            if (in_quotes && v[1] != 0)
                ++v;
            break;
        default:
            break;
        }
    }

    *vlen = v - *val;
    *line = (*v == 0) ? v : v + 1;
    return APR_SUCCESS;
}

static const char crlf[] = CRLF; 

APREQ_DECLARE(apr_status_t) apreq_parse_multipart(apr_pool_t *pool,
                                                  apr_bucket_brigade *bb,
                                                  apreq_parser_t *parser)
{
    apreq_request_t *req = (apreq_request_t *)parser->out;
    struct mfd_ctx *ctx = (struct mfd_ctx *) apreq_parser_ctx(parser);

#define MAX_BLEN    80

#define MFD_INIT     0
#define MFD_NEXTLINE 1
#define MFD_HEADER   2
#define MFD_PARAM    3
#define MFD_UPLOAD   4
#define MFD_ERROR   -1

    if (parser->v.size == 0) {
        const char *bdry, *ct = apreq_env_content_type(req->ctx);
        apr_size_t blen;
        apr_status_t s;

        memcpy(ctx->bdry, CRLF "--", 4);

        s = getval(&ct, "boundary", &bdry, &blen);
        
        if (s != APR_SUCCESS)
            return s;

        if (blen > MAX_BLEN)
            return APR_ENAMETOOLONG;

        memcpy(ctx->bdry + 4, bdry, blen);
        ctx->bdry[4 + blen] = 0;

        APR_BRIGADE_INSERT_HEAD(bb,
                  apr_bucket_immortal_create(crlf,2,bb->bucket_alloc));

        if (ctx->bb == NULL)
            ctx->bb = apr_brigade_create(pool, bb->bucket_alloc);

        parser->v.status = MFD_INIT;
        parser->v.size = sizeof *ctx;
    }


 mfd_parse_brigade:

    switch (parser->v.status) {

    case MFD_INIT:
        {
            apr_status_t s;
            s = split_on_bdry(pool, ctx->bb, bb, ctx->bdry);
            if (s != APR_SUCCESS)
                return s;

            parser->v.status = MFD_NEXTLINE;
        }
        /* fall through */

    case MFD_NEXTLINE:
        {
            apr_status_t s;
            s = split_on_bdry(pool, ctx->bb, bb, crlf);
            if (s != APR_SUCCESS)
                return s;

            parser->v.status = MFD_HEADER;
            apr_brigade_cleanup(ctx->bb);
            ctx->t = NULL;
        }
        /* fall through */

    case MFD_HEADER:
        {
            apreq_parser_t par = {0};
            apr_status_t s;
            const char *cd, *name, *filename;
            apr_size_t nlen, flen;

            if (ctx->t == NULL)
                ctx->t = apreq_make_table(pool, APREQ_NELTS);

            par.out = (void *)ctx->t;
            s = apreq_parse_headers(pool, bb, &par);

            if (s != APR_SUCCESS)
                return s;

            cd = apreq_table_get(ctx->t, "Content-Disposition");

            if (cd == NULL) {
                parser->v.status = MFD_ERROR;
                return APR_BADARG;
            }

            s = getval(&cd, "name", &name, &nlen);
            if (s != APR_SUCCESS) {
                parser->v.status = MFD_ERROR;
                return APR_BADARG;
            }

            s = getval(&cd, "filename", &filename, &flen);

            if (s != APR_SUCCESS) {
                apr_bucket *e;
                name = apr_pstrmemdup(pool, name, nlen);
                e = apr_bucket_transient_create(name, nlen,
                                                ctx->bb->bucket_alloc);
                APR_BRIGADE_INSERT_HEAD(ctx->bb,e);
                parser->v.status = MFD_PARAM;
            } 
            else {
                apreq_param_t *param = apreq_make_param(pool, name, nlen, 
                                                        filename, flen);
                param->info = ctx->t;
                param->bb = apr_brigade_create(pool, 
                                               apr_bucket_alloc_create(pool));

                apreq_table_add(req->body, &param->v);
                parser->v.status = MFD_UPLOAD;
            }
        }
        goto mfd_parse_brigade;

    case MFD_PARAM:
        {
            apr_status_t s = split_on_bdry(pool, ctx->bb, bb, ctx->bdry);
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
                    parser->v.status = MFD_ERROR;
                    return s;
                }

                param = apr_palloc(pool, len + sizeof *param);
                param->charset = APREQ_CHARSET;
                param->language = NULL;
                param->bb = NULL;
                param->info = ctx->t;

                v = &param->v;
                v->name = name;
                v->status = APR_SUCCESS;
                apr_brigade_flatten(ctx->bb, v->data, &len);
                v->size = len;
                v->data[v->size] = 0;

                parser->v.status = MFD_NEXTLINE;
                goto mfd_parse_brigade;

            default:
                parser->v.status = MFD_ERROR;
                return s;
            }


        }
        break;

    case MFD_UPLOAD:
        {
            apr_status_t s = split_on_bdry(pool, ctx->bb, bb, ctx->bdry);
            apreq_param_t *param;
            const apreq_value_t *v;
            int dummy;

            switch (s) {

            case APR_INCOMPLETE:
                apreq_table_last(req->body, &v, &dummy);
                param = apreq_value_to_param(v);

                if (parser->hook)
                    return parser->hook(pool, param->bb, ctx->bb, parser);
                else {
                    APR_BRIGADE_CONCAT(param->bb, ctx->bb);
                    return s;
                }

            case APR_SUCCESS:
                apreq_table_last(req->body, &v, &dummy);
                param = apreq_value_to_param(v);

                /* XXX: push an eos bucket onto ctx->bb */

                if (parser->hook) {
                    do s = parser->hook(pool, param->bb, ctx->bb, parser);
                    while (s == APR_INCOMPLETE);
                }
                else
                    APR_BRIGADE_CONCAT(param->bb, ctx->bb);

                parser->v.status = MFD_NEXTLINE;
                goto mfd_parse_brigade;

            default:
                parser->v.status = MFD_ERROR;
                return s;
            }

        }
        break;

    case MFD_ERROR:
        return APR_EGENERAL;
    }

    return APR_INCOMPLETE;
}
