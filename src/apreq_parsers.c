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
#include "apreq_params.h"
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

APREQ_DECLARE(apreq_parser_t *) apreq_make_parser(
                                                  apr_pool_t *p,
                                                  const char *enctype,
                                                  APREQ_PARSER(*parser),
                                                  void *out)
{
    apreq_parser_t *p = apr_palloc(pool, APREQ_CTX_MAXSIZE + sizeof *p);

    p->v.name = enctype;
    p->v.size = 0;
    apreq_parser_ctx(p) = NULL;
    p->v.status = APR_SUCCESS;

    p->parser = parser;
    p->ext = NULL;
    p->out = out;
    return p;
}


APREQ_DECLARE(apr_status_t) apreq_register_parser(apreq_request_t *req,
                                                  const char *enctype,
                                                  APREQ_PARSER(*parser),
                                                  void *out)
{
    apreq_parser_t *p = apreq_make_parser(req->pool, enctype, parser, out);
    return apreq_env_add_parser(req->ctx, &p->v);
}


static apr_status_t split_urlword(apr_pool_t *p, apreq_table_t *t,
                                  apr_bucket_brigade *bb, 
                                  const apr_size_t nlen,
                                  const apr_size_t vlen)
{
    apreq_param_t *param = apr_palloc(p, nlen + vlen + 1 + sizeof *param);
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
        char *data;
        apr_bucket_t *f = APR_BRIGADE_FIRST(bb);
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

    (char *)v->name[off] = 0;

    /* skip gap */

    off = 0;
    while (off < glen) {
        apr_size_t dlen;
        char *data;
        apr_bucket_t *f = APR_BRIGADE_FIRST(bb);
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
        char *data;
        apr_bucket_t *f = APR_BRIGADE_FIRST(bb);
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


APREQ_DECLARE(apr_status_t) apreq_parse_urlencoded(apr_pool_t *p,
                                                   apr_bucket_brigade *bb,
                                                   apreq_parser_t *parser)
{
    apreq_request_t *req = (apreq_request_t *)parser->out;
    apreq_table_t *t = req->body;

    apr_ssize_t nlen, glen, vlen;
    apr_bucket *e;

/* use parser->v.status to maintain state */
#define URL_NAME        0
#define URL_VALUE       1


 parse_url_brigade:

    /* parse the brigade for CRLF_CRLF-terminated header block, 
     * each time starting from the front of the brigade.
     */
    parser->v.status = URL_NAME;

    for (e  =  APR_BUCKET_FIRST(bb), nlen = vlen = 0;
         e !=  APR_BRIGADE_SENTINEL(bb);
         e  =  APR_BUCKET_NEXT(e))
    {
        apr_size_t off = 0, dlen;
        const char *name, *data;
        apr_status_t s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);

        if (APR_BUCKET_IS_EOS(e))
            return nlen == 0 ? APR_SUCCESS : 
                split_urlword(p, t, bb, nlen, vlen);

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
                    s = split(urlword p, t, bb, nlen, vlen + 1);
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

static apr_status_t split_header(apr_pool_t *p, apreq_table_t *t, 
                                 apr_bucket_brigade *bb,
                                 const apr_size_t nlen, 
                                 const apr_size_t glen,
                                 const apr_size_t vlen)
{
    apreq_value_t *v = apr_palloc(p, nlen + vlen + sizeof *v);
    apr_size_t off;

    v->name = v->data + vlen;

    /* copy name */

    off = 0;
    while (off < nlen) {
        apr_size_t dlen;
        char *data;
        apr_bucket_t *f = APR_BRIGADE_FIRST(bb);
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
        char *data;
        apr_bucket_t *f = APR_BRIGADE_FIRST(bb);
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
        char *data;
        apr_bucket_t *f = APR_BRIGADE_FIRST(bb);
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
    (char *)v->name[nlen] = 0;

    /* remove trailing (CR)LF from value */
    v->size = vlen - 1;
    if ( v->size > 0 && v->data[v->size-1] == '\r')
        v->size--;

    v->data[v->size] = 0;

    return apreq_table_add(t, v);

}


APREQ_DECLARE(apr_status_t) apreq_parse_headers(apr_pool_t *p,
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

    for (e = APR_BUCKET_FIRST(bb), nlen = glen = vlen = 0, 
         e != APR_BUCKET_IS_EOS(e) && e != APR_BRIGADE_SENTINEL(bb);
         e = APR_BUCKET_NEXT(e))
    {
        apr_size_t off = 0, dlen;
        const char *name, *data;
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
                    /* No more headers: split, shift & bail out */
                    if (off < dlen)
                        apr_bucket_split(e, off);

                    e = APR_BUCKET_NEXT(e);

                    do apr_bucket_delete( APR_BUCKET_FIRST(bb) ); 
                    while (e != APR_BUCKET_FIRST(bb));

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
                        apr_split_bucket(e, off - 1);
                        e = APR_BUCKET_NEXT(e);
                    }

                    s = split_header(p, t, bb, nlen, glen, vlen);
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
    apreq_table_t       *t;
    apr_bucket_brigade  *bb;
    char                bdry[1];
};


static apr_status_t split_on_bdry(apr_pool_t *pool, 
                                  apr_bucket_brigade *out,
                                  apr_bucket_brigade *in,
                                  char *bdry, apreq_parser_t *parser)
{
    apr_bucket_t *e = APR_BRIGADE_FIRST(in);
    apr_size_t blen = strlen(bdry);

    while ( e != APR_BRIGADE_SENTINEL(in) ) {
        apr_ssize_t *idx;
        apr_size_t len;
        char *buf;
        apr_status_t s;


        if (APR_BUCKET_IS_EOS(e))
            return APR_EOS;

        s = apr_bucket_read(e, &buf, &len);
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
            apr_bucket_t *f;
            do {
                f = APR_BRIGADE_FIRST(in);
                APR_BUCKET_REMOVE(f);
                APR_BRIGADE_INSERT_TAIL(out, f);
            } while (e != f);
            off = 0;
        }

        idx = apreq_index(buf, len, bdry, blen);

        if (idx > 0)
            apr_bucket_split(e, idx);

        APR_BUCKET_REMOVE(e);
        APR_BRIGADE_INSERT_TAIL(out, e);
        e = APR_BRIGADE_FIRST(e);

    }

    return APR_INCOMPLETE;
}

APREQ_DECLARE(apr_status_t) apreq_parse_multipart(apr_pool_t *pool,
                                                  apr_bucket_brigade *bb,
                                                  apreq_parser_t *parser)
{
    apr_bucket *e;
    apreq_request_t *req = (apreq_request_t *)parser->out;
    struct mfd_ctx *ctx = (struct mfd_ctx *) apreq_parser_ctx(parser);

#define MAX_BLEN  100

#define MFD_HEADER  0
#define MFD_PARAM   1
#define MFD_UPLOAD  2
#define MFD_ERROR  -1

    if (parser->v.size == 0) {
        apr_ssize_t total = 0;
        char blen = 0;

        ctx->bdry[blen++] = '\r';
        ctx->bdry[blen++] = '\n';

        /* parse first line for mfd boundary */
        while (1) {
            apr_size_t dlen;
            char *data, *newline;
            apr_bucket_t *f = APR_BRIGADE_FIRST(bb);
            apr_status_t s;

            if (f == APR_BRIGADE_SENTINEL(bb))
                return APR_EAGAIN;

            if (APR_BUCKET_IS_EOS(f))
                return APR_EOS;

            s = apr_bucket_read(f, &data, &dlen, APR_BLOCK_READ);
            if (s != APR_SUCCESS)
                return s;

            newline = memchr(data, '\n', dlen);

            if (newline != NULL) {
                if (newline + 1 < data + dlen)
                    apr_bucket_split(f, newline + 1 - data);
                else
                    dlen = newline - data;

                if (blen + dlen >= MAX_BLEN)
                    return APR_BADARG;
                memcpy(ctx->bdry + blen, data, dlen);
                apr_bucket_delete(f);
                blen += dlen;
                break;
            }

            if (blen + dlen >= MAX_BLEN)
                return APR_BADARG;
            memcpy(ctx->bdry + blen, data, dlen);
            apr_bucket_delete(f);
            blen += dlen;
        }

        blen -= 2; /* ignore trailing CRLF */
        ctx->bdry[blen] = 0;

        parser->v.size = blen + sizeof *ctx;
        parser->v.status = MFD_HEADER;

    }

    switch (parser->v.status) {

    case MFD_HEADER:
    {
        apreq_parser_t par = {0};
        apr_status_t s;
        char *cd;

        if (ctx->t == NULL)
            ctx->t = apreq_make_table(pool, APREQ_NELTS);

        par.out = (void *)ctx->t;
        
        s = apreq_parse_headers(pool, bb, &par);

        if (s != APR_SUCCESS)
            return s;

        parser->v.status = MFD_VALUE;
        ctx->bb = apr_brigade_create(pool, apr_bucket_alloc_create(pool));

        /* parse Content-Disposition header to determine value type */
        cd = apreq_table_get(ctx->t, "Content-Disposition");



        /* fall thru */
    }

    case MFD_PARAM:
    case MFD_UPLOAD:
    {
        apr_status_t s = split_on_bdry(pool, ctx->bb, bb, 
                                       ctx->bdry, parser);
        if (s != APR_SUCCESS)
            return s;

        if (parser->v.status == MFD_PARAM) {
            apr_size_t len;
            apreq_param_t *param;
            apreq_value_t *v;
            apr_status_t s;

            s = apr_brigade_length(ctx->bb, 1, &len);

            if (s != APR_SUCCESS) {
                parser->v.status = MFD_ERROR;
                return s;
            }


            param = apr_palloc(pool, len + sizeof *param);
            v = &param->v;
            param->charset = UTF_8;
            param->language = NULL;
            param->info = ctx->t;
            param->bb = ctx->bb;
            v->size = len;
            v->status = apr_brigade_flatten(ctx->bb, v->data, &v->size);

            apreq_table_add(req->body, v);
        }


        parser->v.status = MFD_VALUE;
        ctx->t = NULL;
        ctx->bb = NULL;
    }
    break;

    case MFD_ERROR:
        return APR_EGENERAL;
    }

    return APR_INCOMPLETE;
}
