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

#include "assert.h"

#include "httpd.h"
#include "http_config.h"
#include "http_log.h"
#include "util_filter.h"
#include "apr_tables.h"
#include "apr_buckets.h"
#include "http_request.h"
#include "apr_strings.h"

#include "apreq.h"
#include "apreq_env.h"
#include "apreq_params.h"
#include "apreq_cookie.h"
#include "apreq_env_apache2.h"

static const char filter_name[] = "apreq";

struct dir_config {
    const char         *temp_dir;
    apr_uint64_t        read_limit;
    apr_size_t          brigade_limit;
};

/* The "warehouse", stored in r->request_config */
struct apache2_handle {
    apreq_env_handle_t  env;
    request_rec        *r;
    apr_table_t        *jar, *args;
    apr_status_t        jar_status, args_status;
    ap_filter_t        *f;
};

/* Tracks the apreq filter state */
struct filter_ctx {
    apr_bucket_brigade *bb;    /* input brigade that's passed to the parser */
    apr_bucket_brigade *spool; /* copied prefetch data for downstream filters */
    apr_table_t        *body;
    apreq_parser_t     *parser;
    apreq_hook_t       *hook_queue;
    apr_status_t        status;
    unsigned            saw_eos;      /* Has EOS bucket appeared in filter? */
    apr_uint64_t           bytes_read;   /* Total bytes read into this filter. */
    apr_uint64_t           read_limit;   /* Max bytes the filter may show to parser */
    apr_size_t          brigade_limit;
    const char         *temp_dir;
};

static apr_status_t apreq_filter(ap_filter_t *f,
                                 apr_bucket_brigade *bb,
                                 ap_input_mode_t mode,
                                 apr_read_type_e block,
                                 apr_off_t readbytes);

static void apreq_filter_make_context(ap_filter_t *f);



/**
 * @defgroup mod_apreq Apache 2.X Filter Module
 * @ingroup apreq_env
 * @brief mod_apreq - DSO that ties libapreq2 to Apache 2.X.
 *
 * mod_apreq provides the "APREQ" input filter for using libapreq2
 * (and allow its parsed data structures to be shared) within
 * the Apache 2.X webserver.  Using it, libapreq2 works properly
 * in every phase of the HTTP request, from translation handlers 
 * to output filters, and even for subrequests / internal redirects.
 *
 * <hr>
 *
 * <h2>Activating mod_apreq in Apache 2.X</h2>
 *
 * Normally the installation process triggered by
 * <code>% make install</code>
 * will make the necessary changes to httpd.conf for you. In any case,
 * after installing the mod_apreq.so module, be sure your webserver's
 * httpd.conf activates it on startup with a LoadModule directive, e.g.
 * @code
 *
 *     LoadModule    modules/mod_apreq.so
 *
 * @endcode
 *
 * The mod_apreq filter is named "APREQ", and may be used in Apache's
 * input filter directives, e.g.
 * @code
 *
 *     AddInputFilter APREQ         # or
 *     SetInputFilter APREQ
 *
 * @endcode
 *
 * However, this is not required because libapreq2 will add the filter (only)
 * if it's necessary.  You just need to ensure that your module instantiates
 * an apreq_request_t using apreq_request() <em>before the content handler
 * ultimately reads from the input filter chain</em>.  It is important to
 * recognize that no matter how the input filters are initially arranged,
 * the APREQ filter will attempt to reposition itself to be the last input filter
 * to read the data.
 *
 * If you want to use other input filters to transform the incoming HTTP
 * request data, is important to register those filters with Apache
 * as having type AP_FTYPE_CONTENT_SET or AP_FTYPE_RESOURCE.  Due to the 
 * limitations of Apache's current input filter design, types higher than 
 * AP_FTYPE_CONTENT_SET may not work properly whenever the apreq filter is active.
 *
 * This is especially true when a content handler uses libapreq2 to parse 
 * some of the post data before doing an internal redirect.  Any input filter
 * subsequently added to the redirected request will bypass the original apreq 
 * filter (and therefore lose access to some of the original post data), unless 
 * its type is less than the type of the apreq filter (currently AP_FTYPE_PROTOCOL-1).
 *
 *
 * <h2>Server Configuration Directives</h2>
 *
 * <TABLE class="qref"><CAPTION>Per-directory commands for mod_apreq</CAPTION>
 * <TR><TH>Directive</TH><TH>Context</TH><TH>Default</TH><TH>Description</TH></TR>
 * <TR class="odd"><TD>APREQ_ReadLimit</TD><TD>directory</TD><TD>-1 (Unlimited)</TD><TD>
 * Maximum number of bytes mod_apreq will send off to libapreq for parsing.  
 * mod_apreq will log this event and remove itself from the filter chain.
 * The APR_EGENERAL error will be reported to libapreq2 users via the return 
 * value of apreq_env_read().
 * </TD></TR>
 * <TR><TD>APREQ_BrigadeLimit</TD><TD>directory</TD><TD> #APREQ_MAX_BRIGADE_LEN </TD><TD>
 * Maximum number of bytes apreq will allow to accumulate
 * within a brigade.  Excess data will be spooled to a
 * file bucket appended to the brigade.
 * </TD></TR>
 * <TR class="odd"><TD>APREQ_TempDir</TD><TD>directory</TD><TD>NULL</TD><TD>
 * Sets the location of the temporary directory apreq will use to spool
 * overflow brigade data (based on the APREQ_BrigadeLimit setting).
 * If left unset, libapreq2 will select a platform-specific location via apr_temp_dir_get().
 * </TD></TR>
 * </TABLE>
 *
 * <h2>Implementation Details</h2>
 * <pre>
 * XXX apreq as a normal input filter
 * XXX apreq as a "virtual" content handler.
 * XXX apreq as a transparent "tee".
 * XXX apreq parser registration in post_config
 * </pre>
 * @{
 */

APR_INLINE
static void filter_relocate(ap_filter_t *f)
{
    request_rec *r = f->r;

    if (f != r->input_filters) {
        ap_filter_t *top = r->input_filters;
        ap_remove_input_filter(f);
        r->input_filters = f;
        f->next = top;
    }
}

APR_INLINE
static ap_filter_t *get_apreq_filter(apreq_env_handle_t *env)
{
    struct apache2_handle *handle = (struct apache2_handle *)env;

    if (handle->f == NULL) {
        handle->f = ap_add_input_filter(filter_name, NULL, 
                                        handle->r, 
                                        handle->r->connection);
        /* ap_add_input_filter does not guarantee cfg->f == r->input_filters,
         * so we reposition the new filter there as necessary.
         */
        filter_relocate(handle->f); 
    }

    return handle->f;
}


APR_INLINE
static void init_filter_context(ap_filter_t *f)
{
    request_rec *r = f->r;
    struct filter_ctx *ctx = f->ctx;
    apr_bucket_alloc_t *ba = r->connection->bucket_alloc;
    const char *cl_header = apr_table_get(r->headers_in, "Content-Length");

    if (cl_header != NULL) {
        char *dummy;
        apr_uint64_t content_length = apr_strtoi64(cl_header,&dummy,0);

        if (dummy == NULL || *dummy != 0) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r,
                          "Invalid Content-Length header (%s)", cl_header);
            ctx->status = APREQ_ERROR_BADHEADER;
            return;
        }
        else if (content_length > ctx->read_limit) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r,
                          "Content-Length header (%s) exceeds configured "
                          "max_body limit (%" APR_UINT64_T_FMT ")", 
                          cl_header, ctx->read_limit);
            ctx->status = APREQ_ERROR_OVERLIMIT;
            return;
        }
    }

    if (ctx->parser == NULL) {
        const char *ct_header = apr_table_get(r->headers_in, "Content-Type");

        if (ct_header != NULL) {
            apreq_parser_function_t pf = apreq_parser(ct_header);

            if (pf != NULL) {
                ctx->parser = apreq_make_parser(r->pool, ba, ct_header, pf,
                                                ctx->brigade_limit,
                                                ctx->temp_dir,
                                                ctx->hook_queue,
                                                NULL);
            }
            else {
                ctx->status = APREQ_ERROR_NOPARSER;
                return;
            }
        }
        else {
            ctx->status = APREQ_ERROR_NOPARSER;
            return;
        }
    }
    else {
        if (ctx->parser->brigade_limit > ctx->brigade_limit)
            ctx->parser->brigade_limit = ctx->brigade_limit;
        if (ctx->temp_dir != NULL)
            ctx->parser->temp_dir = ctx->temp_dir;
        if (ctx->hook_queue != NULL)
            apreq_parser_add_hook(ctx->parser, ctx->hook_queue);
    }

    ctx->hook_queue = NULL;
    ctx->bb    = apr_brigade_create(r->pool, ba);
    ctx->spool = apr_brigade_create(r->pool, ba);
    ctx->body  = apr_table_make(r->pool, APREQ_DEFAULT_NELTS);
    ctx->status = APR_INCOMPLETE;
}


static apr_status_t apreq_filter_read(ap_filter_t *f, apr_off_t bytes)
{
    struct filter_ctx *ctx = f->ctx;
    apr_status_t s;

    if (ctx->status == APR_EINIT)
        init_filter_context(f);

    if (ctx->status != APR_INCOMPLETE || bytes == 0)
        return ctx->status;

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, f->r,
                  "prefetching %" APR_OFF_T_FMT " bytes", bytes);
    s = ap_get_brigade(f, NULL, AP_MODE_READBYTES, APR_BLOCK_READ, bytes);
    if (s != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, s, f->r, 
                      "apreq filter error detected during prefetch");
        return s;
    }
    return ctx->status;
}


static apr_status_t apache2_jar(apreq_env_handle_t *env, const apr_table_t **t)
{
    struct apache2_handle *handle = (struct apache2_handle*)env;
    request_rec *r = handle->r;

    if (handle->jar_status == APR_EINIT) {
        const char *cookies = apr_table_get(r->headers_in, "Cookie");
        if (cookies != NULL) {
            handle->jar = apr_table_make(handle->r->pool, APREQ_DEFAULT_NELTS);
            handle->jar_status = 
                apreq_parse_cookie_header(r->pool, handle->jar, cookies);
        }
        else
            handle->jar_status = APREQ_ERROR_NODATA;
    }

    *t = handle->jar;
    return handle->jar_status;
}

static apr_status_t apache2_args(apreq_env_handle_t *env, const apr_table_t **t)
{
    struct apache2_handle *handle = (struct apache2_handle*)env;
    request_rec *r = handle->r;

    if (handle->args_status == APR_EINIT) {
        if (r->args != NULL) {
            handle->args = apr_table_make(handle->r->pool, APREQ_DEFAULT_NELTS);
            handle->args_status = 
                apreq_parse_query_string(r->pool, handle->args, r->args);
        }
        else
            handle->args_status = APREQ_ERROR_NODATA;
    }

    *t = handle->args;
    return handle->args_status;
}




static apreq_cookie_t *apache2_jar_get(apreq_env_handle_t *env, const char *name)
{
    struct apache2_handle *handle = (struct apache2_handle *)env;
    const apr_table_t *t;
    const char *val;

    if (handle->jar_status == APR_EINIT)
        apache2_jar(env, &t);
    else
        t = handle->jar;

    if (t == NULL)
        return NULL;

    val = apr_table_get(t, name);
    if (val == NULL)
        return NULL;

    return apreq_value_to_cookie(val);
}

static apreq_param_t *apache2_args_get(apreq_env_handle_t *env, const char *name)
{
    struct apache2_handle *handle = (struct apache2_handle *)env;
    const apr_table_t *t;
    const char *val;

    if (handle->args_status == APR_EINIT)
        apache2_args(env, &t);
    else
        t = handle->args;

    if (t == NULL)
        return NULL;

    val = apr_table_get(t, name);
    if (val == NULL)
        return NULL;

    return apreq_value_to_param(val);
}



static apr_status_t apache2_body(apreq_env_handle_t *env, const apr_table_t **t)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;

    switch (ctx->status) {

    case APR_EINIT:
        init_filter_context(f);
        if (ctx->status != APR_INCOMPLETE)
            break;

    case APR_INCOMPLETE:
        while (apreq_filter_read(f, APREQ_DEFAULT_READ_BLOCK_SIZE) == APR_INCOMPLETE)
            ;   /*loop*/
    }

    *t = ctx->body;
    return ctx->status;
}
static apreq_param_t *apache2_body_get(apreq_env_handle_t *env, const char *name)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;
    const char *val;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;

    switch (ctx->status) {

    case APR_EINIT:

        init_filter_context(f);
        if (ctx->status != APR_INCOMPLETE)
            return NULL;
        apreq_filter_read(f, APREQ_DEFAULT_READ_BLOCK_SIZE);

    case APR_INCOMPLETE:

        val = apr_table_get(ctx->body, name);
        if (val != NULL)
            return apreq_value_to_param(val);

        do {
            /* riff on Duff's device */
            apreq_filter_read(f, APREQ_DEFAULT_READ_BLOCK_SIZE);

    default:

            val = apr_table_get(ctx->body, name);
            if (val != NULL)
                return apreq_value_to_param(val);

        } while (ctx->status == APR_INCOMPLETE);

    }

    return NULL;
}

static
apr_status_t apache2_parser_get(apreq_env_handle_t *env, 
                                  const apreq_parser_t **parser)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx = f->ctx;

    if (ctx == NULL) {
        *parser = NULL;
        return APR_EINIT;
    }
    *parser = ctx->parser;
    return ctx->status;
}

static
apr_status_t apache2_parser_set(apreq_env_handle_t *env, 
                                apreq_parser_t *parser)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;

    if (ctx->parser == NULL) {
        ctx->parser = parser;
        return APR_SUCCESS;
    }
    else
        return APREQ_ERROR_CONFLICT;
}



static
apr_status_t apache2_hook_add(apreq_env_handle_t *env,
                              apreq_hook_t *hook)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;

    if (ctx->parser != NULL) {
        return apreq_parser_add_hook(ctx->parser, hook);
    }
    else if (ctx->hook_queue != NULL) {
        apreq_hook_t *h = ctx->hook_queue;
        while (h->next != NULL)
            h = h->next;
        h->next = hook;
    }
    else {
        ctx->hook_queue = hook;
    }
    return APR_SUCCESS;

}

static
apr_status_t apache2_brigade_limit_set(apreq_env_handle_t *env,
                                       apr_size_t bytes)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;

    if (ctx->status == APR_EINIT || ctx->brigade_limit > bytes) {
        ctx->brigade_limit = bytes;
        return APR_SUCCESS;
    }

    return APREQ_ERROR_CONFLICT;
}

static
apr_status_t apache2_brigade_limit_get(apreq_env_handle_t *env,
                                       apr_size_t *bytes)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;
    *bytes = ctx->brigade_limit;
    return APR_SUCCESS;
}

static
apr_status_t apache2_read_limit_set(apreq_env_handle_t *env,
                                    apr_uint64_t bytes)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;

    if (ctx->read_limit > bytes && ctx->bytes_read < bytes) {
        ctx->read_limit = bytes;
        return APR_SUCCESS;
    }

    return APREQ_ERROR_CONFLICT;
}

static
apr_status_t apache2_read_limit_get(apreq_env_handle_t *env,
                                    apr_uint64_t *bytes)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;
    *bytes = ctx->read_limit;
    return APR_SUCCESS;
}

static
apr_status_t apache2_temp_dir_set(apreq_env_handle_t *env,
                                  const char *path)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;
    // init vs incomplete state?
    if (ctx->temp_dir == NULL && ctx->bytes_read == 0) {
        if (path != NULL)
            ctx->temp_dir = apr_pstrdup(f->r->pool, path);
        return APR_SUCCESS;
    }

    return APREQ_ERROR_CONFLICT;
}

static
apr_status_t apache2_temp_dir_get(apreq_env_handle_t *env,
                                  const char **path)
{
    ap_filter_t *f = get_apreq_filter(env);
    struct filter_ctx *ctx;

    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;
    *path = ctx->parser ? ctx->parser->temp_dir : ctx->temp_dir;
    return APR_SUCCESS;
}


/*
 * Situations to contend with:
 *
 * 1) Often the filter will be added by the content handler itself,
 *    so the apreq_filter_init hook will not be run.
 * 2) If an auth handler uses apreq, the apreq_filter will ensure
 *    it's part of the protocol filters.  apreq_filter_init does NOT need
 *    to notify the protocol filter that it must not continue parsing,
 *    the apreq filter can perform this check itself.  apreq_filter_init
 *    just needs to ensure cfg->f does not point at it.
 * 3) If req->proto_input_filters and req->input_filters are apreq
 *    filters, and req->input_filters->next == req->proto_input_filters,
 *    it is safe for apreq_filter to "steal" the proto filter's context 
 *    and subsequently drop it from the chain.
 */


/* Examines the input_filter chain and moves the apreq filter(s) around
 * before the filter chain is stacked by ap_get_brigade.
 */

static apr_status_t apreq_filter_init(ap_filter_t *f)
{
    request_rec *r = f->r;
    struct filter_ctx *ctx = f->ctx;
    struct apache2_handle *handle = 
        (struct apache2_handle *)apreq_handle_apache2(r);

    if (ctx == NULL || ctx->status == APR_EINIT) {
        if (f == r->input_filters) {
            handle->f = f;
        }
        else if (r->input_filters->frec->filter_func.in_func == apreq_filter) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, r,
                          "removing intermediate apreq filter");
            if (handle->f == f)
                handle->f = r->input_filters;
            ap_remove_input_filter(f);
        }
        else {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, r,
                          "relocating intermediate apreq filter");
            filter_relocate(f);
            handle->f = f;
        }
        return APR_SUCCESS;
    }

    /* else this is a protocol filter which may still be active.
     * if it is, we must deregister it now.
     */
    if (handle->f == f) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, r, 
                     "disabling stale protocol filter");
        handle->f = NULL;
    }
    return APR_SUCCESS;
}

static apr_status_t apreq_filter(ap_filter_t *f,
                                 apr_bucket_brigade *bb,
                                 ap_input_mode_t mode,
                                 apr_read_type_e block,
                                 apr_off_t readbytes)
{
    request_rec *r = f->r;
    struct filter_ctx *ctx;
    apr_status_t rv;

    switch (mode) {
    case AP_MODE_READBYTES:
    case AP_MODE_EXHAUSTIVE:
        /* only the modes above are supported */
        break;
    case AP_MODE_GETLINE: /* punt- chunks are b0rked in ap_http_filter */
        return ap_get_brigade(f->next, bb, mode, block, readbytes);
    default:
        return APR_ENOTIMPL;
    }


    if (f->ctx == NULL)
        apreq_filter_make_context(f);

    ctx = f->ctx;

    switch (ctx->status) {

    case APR_EINIT:
        init_filter_context(f);
        if (ctx->status == APR_INCOMPLETE)
            break;

    case APREQ_ERROR_NOPARSER:
        assert(bb != NULL);
        rv = ap_get_brigade(f->next, bb, mode, block, readbytes);
        ap_remove_input_filter(f);
        return rv;
    }


    if (bb != NULL) {

        if (!ctx->saw_eos) {
 
            if (ctx->status == APR_INCOMPLETE) {
                apr_off_t len;
                rv = ap_get_brigade(f->next, bb, mode, block, readbytes);
            
                if (rv != APR_SUCCESS) {
                    ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r, 
                                 "ap_get_brigade failed");
                    return rv;
                }

                apreq_brigade_copy(ctx->bb, bb);
                apr_brigade_length(bb, 1, &len);
                ctx->bytes_read += len;

                if (ctx->bytes_read > ctx->read_limit) {
                    ctx->status = APREQ_ERROR_OVERLIMIT;
                    ap_log_rerror(APLOG_MARK, APLOG_ERR, ctx->status, r, 
                                 "Bytes read (%" APR_UINT64_T_FMT
                                 ") exceeds configured max_body limit (%"
                                 APR_UINT64_T_FMT ")",
                                 ctx->bytes_read, ctx->read_limit);
                }
            }
            if (APR_BUCKET_IS_EOS(APR_BRIGADE_LAST(ctx->bb)))
                ctx->saw_eos = 1;
        }

        if (!APR_BRIGADE_EMPTY(ctx->spool)) {
            APR_BRIGADE_PREPEND(bb, ctx->spool);
            if (mode == AP_MODE_READBYTES) {
                apr_bucket *e;
                rv = apr_brigade_partition(bb, readbytes, &e);
                if (rv != APR_SUCCESS && rv != APR_INCOMPLETE) {
                    ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r, 
                                 "partition failed");
                    return rv;
                }
                if (APR_BUCKET_IS_EOS(e))
                    e = APR_BUCKET_NEXT(e);
                ctx->spool = apr_brigade_split(bb, e);
                apreq_brigade_setaside(ctx->spool, r->pool);
            }
        }

        if (ctx->status != APR_INCOMPLETE) {

            if (APR_BRIGADE_EMPTY(ctx->spool)) {
                ap_filter_t *next = f->next;

                ap_remove_input_filter(f);
                if (APR_BRIGADE_EMPTY(bb))
                    return ap_get_brigade(next, bb, mode, block, readbytes);
            }
            return APR_SUCCESS;
        }
    }
    else if (!ctx->saw_eos) {
        /* bb == NULL, so this is a prefetch read! */
        apr_off_t total_read = 0;

        /* XXX cache this thing in somewhere */
        bb = apr_brigade_create(ctx->bb->p, ctx->bb->bucket_alloc);

        while (total_read < readbytes) {
            apr_off_t len;
            apr_bucket *last = APR_BRIGADE_LAST(ctx->spool);

            if (APR_BUCKET_IS_EOS(last)) {
                ctx->saw_eos = 1;
                break;
            }

            rv = ap_get_brigade(f->next, bb, mode, block, readbytes);
            if (rv != APR_SUCCESS) {
                /*XXX how should we handle this filter-chain error? */
                return rv;
            }
            apreq_brigade_setaside(bb, r->pool);
            apreq_brigade_copy(ctx->bb, bb);

            apr_brigade_length(bb, 1, &len);
            total_read += len;

            rv = apreq_brigade_concat(r->pool, ctx->temp_dir, ctx->brigade_limit,
                                      ctx->spool, bb);
            if (rv != APR_SUCCESS && rv != APR_EOF) {
                ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                          "apreq_brigade_concat failed; TempDir problem?");
                /*XXX how should we handle this apreq-based error? */
                return ctx->status = rv;
            }
        }

        ctx->bytes_read += total_read;

        if (ctx->bytes_read > ctx->read_limit) {
            ctx->status = APREQ_ERROR_OVERLIMIT;
            ap_log_rerror(APLOG_MARK, APLOG_ERR, ctx->status, r, 
                         "Bytes read (%" APR_UINT64_T_FMT
                         ") exceeds configured read limit (%" APR_UINT64_T_FMT ")",
                         ctx->bytes_read, ctx->read_limit);
        }

        /* Adding "f" to the protocol filter chain ensures the 
         * spooled data is preserved across internal redirects.
         */
        if (f != r->proto_input_filters) {
            ap_filter_t *in;
            for (in = r->input_filters; in != r->proto_input_filters; 
                 in = in->next)
            {
                if (f == in) {
                    r->proto_input_filters = f;
                    break;
                }
            }
        }
    }
    else
        return APR_SUCCESS;

    if (ctx->status == APR_INCOMPLETE) {
        ctx->status = apreq_run_parser(ctx->parser, ctx->body, ctx->bb);
        apr_brigade_cleanup(ctx->bb);
    }

    return APR_SUCCESS;
}



static const char *apache2_header_in(apreq_env_handle_t *env, const char *name)
{
    struct apache2_handle *handle = (struct apache2_handle *)env;
    return apr_table_get(handle->r->headers_in, name);
}

static apr_status_t apache2_header_out(apreq_env_handle_t *env,
                                       const char *name, char *value)
{
    struct apache2_handle *handle = (struct apache2_handle *)env;
    apr_table_add(handle->r->err_headers_out, name, value);
    return APR_SUCCESS;
}

static int apreq_post_config(apr_pool_t *p, apr_pool_t *plog,
                             apr_pool_t *ptemp, server_rec *base_server) {
    apr_status_t status;

    status = apreq_initialize(p);
    if (status != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_STARTUP|APLOG_ERR, status, base_server,
                     "Failed to initialize libapreq2");
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    return OK;
}

static void register_hooks (apr_pool_t *p)
{
    /* APR_HOOK_FIRST because we want other modules to be able to
       register parsers in their post_config hook */
    ap_hook_post_config(apreq_post_config, NULL, NULL, APR_HOOK_FIRST);

    ap_register_input_filter(filter_name, apreq_filter, apreq_filter_init,
                             AP_FTYPE_PROTOCOL-1);
}



/* Server configuration directives */

static void *apreq_create_dir_config(apr_pool_t *p, char *d)
{
    /* d == OR_ALL */
    struct dir_config *dc = apr_palloc(p, sizeof *dc);
    dc->temp_dir      = NULL;
    dc->read_limit    = (apr_uint64_t)-1;
    dc->brigade_limit = APREQ_DEFAULT_BRIGADE_LIMIT;
    return dc;
}

static void *apreq_merge_dir_config(apr_pool_t *p, void *a_, void *b_)
{
    struct dir_config *a = a_, *b = b_, *c = apr_palloc(p, sizeof *c);

    c->temp_dir      = (b->temp_dir != NULL)            /* overrides ok */
                      ? b->temp_dir : a->temp_dir;

    c->brigade_limit = (b->brigade_limit == (apr_size_t)-1) /* overrides ok */
                      ? a->brigade_limit : b->brigade_limit;

    c->read_limit    = (b->read_limit < a->read_limit)  /* why min? */
                      ? b->read_limit : a->read_limit;

    return c;
}

static const char *apreq_set_temp_dir(cmd_parms *cmd, void *data,
                                      const char *arg)
{
    struct dir_config *conf = data;
    const char *err = ap_check_cmd_context(cmd, NOT_IN_LIMIT);

    if (err != NULL)
        return err;

    conf->temp_dir = arg;
    return NULL;
}

static const char *apreq_set_read_limit(cmd_parms *cmd, void *data,
                                        const char *arg)
{
    struct dir_config *conf = data;
    const char *err = ap_check_cmd_context(cmd, NOT_IN_LIMIT);

    if (err != NULL)
        return err;

    conf->read_limit = apreq_atoi64f(arg);
    return NULL;
}

static const char *apreq_set_brigade_limit(cmd_parms *cmd, void *data,
                                           const char *arg)
{
    struct dir_config *conf = data;
    const char *err = ap_check_cmd_context(cmd, NOT_IN_LIMIT);

    if (err != NULL)
        return err;

    conf->brigade_limit = apreq_atoi64f(arg);
    return NULL;
}


static const command_rec apreq_cmds[] =
{
    AP_INIT_TAKE1("APREQ_TempDir", apreq_set_temp_dir, NULL, OR_ALL,
                  "Default location of temporary directory"),
    AP_INIT_TAKE1("APREQ_ReadLimit", apreq_set_read_limit, NULL, OR_ALL,
                  "Maximum amount of data that will be fed into a parser."),
    AP_INIT_TAKE1("APREQ_BrigadeLimit", apreq_set_brigade_limit, NULL, OR_ALL,
                  "Maximum in-memory bytes a brigade may use."),
    { NULL }
};

/** @} */


module AP_MODULE_DECLARE_DATA apreq_module = {
	STANDARD20_MODULE_STUFF,
	apreq_create_dir_config,
	apreq_merge_dir_config,
	NULL,
	NULL,
	apreq_cmds,
	register_hooks,
};


static void apreq_filter_make_context(ap_filter_t *f)
{
    request_rec *r;
    struct filter_ctx *ctx;
    struct dir_config *d;

    r = f->r;
    d = ap_get_module_config(r->per_dir_config, &apreq_module);

    if (f == r->input_filters 
        && r->proto_input_filters == f->next
        && f->next->frec->filter_func.in_func == apreq_filter) 
    {

        ctx = f->next->ctx;

        switch (ctx->status) {
        case APR_INCOMPLETE:
        case APR_SUCCESS:

            if (d != NULL) {
                ctx->temp_dir      = d->temp_dir;
                ctx->read_limit    = d->read_limit;
                ctx->brigade_limit = d->brigade_limit;

                if (ctx->parser != NULL) {
                    ctx->parser->temp_dir = d->temp_dir;
                    ctx->parser->brigade_limit = d->brigade_limit;
                }

            }

            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, r,
                          "stealing filter context");
            f->ctx = ctx;
            r->proto_input_filters = f;
            ap_remove_input_filter(f->next);

            return;

        default:
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, ctx->status, r, 
                          "cannot steal context: bad filter status");
        }
    }


    ctx = apr_pcalloc(r->pool, sizeof *ctx);
    ctx->status = APR_EINIT;

    if (d == NULL) {
        ctx->read_limit    = (apr_uint64_t)-1;
        ctx->brigade_limit = APREQ_DEFAULT_BRIGADE_LIMIT;
    } else {
        ctx->temp_dir      = d->temp_dir;
        ctx->read_limit    = d->read_limit;
        ctx->brigade_limit = d->brigade_limit;
    }

    f->ctx = ctx;
}



static APREQ_MODULE(apache2, 20050131);

APREQ_DECLARE(apreq_env_handle_t *) apreq_handle_apache2(request_rec *r)
{
    struct apache2_handle *handle =
        ap_get_module_config(r->request_config, &apreq_module);

    if (handle != NULL) {
        if (handle->f == NULL)
            get_apreq_filter(&handle->env);
        return &handle->env;
    }
    handle = apr_palloc(r->pool, sizeof *handle);
    ap_set_module_config(r->request_config, &apreq_module, handle);

    handle->env.module = &apache2_module;
    handle->r = r;

    handle->args_status = handle->jar_status = APR_EINIT;
    handle->args = handle->jar = NULL;

    handle->f = NULL;

    if (0) {
        apreq_param_t *hack;
        handle->args = apr_table_make(r->pool, APREQ_DEFAULT_NELTS);
        hack = apreq_make_param(r->pool, "foo", 3, "bar", 3);
        apr_table_addn(handle->args, hack->v.name, hack->v.data);
        handle->args_status = APR_SUCCESS;
    }
    get_apreq_filter(&handle->env);
    return &handle->env;

}
