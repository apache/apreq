#include "apreq_parser.h"
#include "apreq_util.h"
#include "apreq_error.h"

#define PARSER_STATUS_CHECK(PREFIX)   do {         \
    if (ctx->status == PREFIX##_ERROR)             \
        return APR_EGENERAL;                       \
    else if (ctx->status == PREFIX##_COMPLETE)     \
        return APR_SUCCESS;                        \
    else if (bb == NULL)                           \
        return APR_INCOMPLETE;                     \
} while (0);



struct url_ctx {
    apr_bucket_brigade *bb;
    enum {
        URL_NAME, 
        URL_VALUE,
        URL_COMPLETE,
        URL_ERROR
    }                   status;
};


/******************** application/x-www-form-urlencoded ********************/

static apr_status_t split_urlword(apreq_param_t **p, apr_pool_t *pool,
                                  apr_bucket_brigade *bb,
                                  apr_size_t nlen,
                                  apr_size_t vlen)
{
    apreq_param_t *param;
    apreq_value_t *v;
    apr_bucket *e, *end;
    apr_status_t s;
    struct iovec vec[APREQ_DEFAULT_NELTS];
    apr_array_header_t arr;

    if (nlen == 0)
        return APR_EBADARG;

    param = apreq_param_make(pool, NULL, nlen, NULL, vlen);
    *(const apreq_value_t **)&v = &param->v;

    arr.pool     = pool;
    arr.elt_size = sizeof(struct iovec);
    arr.nelts    = 0;
    arr.nalloc   = APREQ_DEFAULT_NELTS;
    arr.elts     = (char *)vec;

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

    s = apreq_decodev(v->name, &nlen,
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
        ((struct iovec *)arr.elts)[arr.nelts - 1].iov_len--; /* drop [&;] */

    s = apreq_decodev(v->data, &vlen,
                      (struct iovec *)arr.elts, arr.nelts);

    if (s != APR_SUCCESS)
        return s;

    while ((e = APR_BRIGADE_FIRST(bb)) != end)
        apr_bucket_delete(e);

    APREQ_FLAGS_ON(param->flags, APREQ_TAINT);
    *p = param;
    return APR_SUCCESS;
}

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
                    s = apreq_hook_run(parser->hook, param, NULL);

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
                        s = apreq_hook_run(parser->hook, param, NULL);

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
    apreq_brigade_setaside(ctx->bb, pool);
    return APR_INCOMPLETE;
}


