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

#include "apreq_parser.h"
#include "apreq_error.h"
#include "apreq_util.h"

#define PARSER_STATUS_CHECK(PREFIX)   do {         \
    if (ctx->status == PREFIX##_ERROR)             \
        return APR_EGENERAL;                       \
    else if (ctx->status == PREFIX##_COMPLETE)     \
        return APR_SUCCESS;                        \
    else if (bb == NULL)                           \
        return APR_INCOMPLETE;                     \
} while (0);


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

    param = apreq_param_make(pool, NULL, nlen, NULL, vlen);
    *(const apreq_value_t **)&v = &param->v;

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
    APREQ_FLAGS_ON(param->flags, APREQ_TAINT);
    *p = param;
    return APR_SUCCESS;

}


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
                        s = apreq_hook_run(parser->hook, param, NULL);

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
    apreq_brigade_setaside(ctx->bb,pool);
    return APR_INCOMPLETE;
}
