extern module AP_MODULE_DECLARE_DATA apreq_module;
#define APREQ_FILTER_NAME "apreq2"

struct dir_config {
    const char         *temp_dir;
    apr_uint64_t        read_limit;
    apr_size_t          brigade_limit;
};

/* The "warehouse", stored in r->request_config */
struct apache2_handle {
    apreq_handle_t      env;
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
    apr_uint64_t        bytes_read;   /* Total bytes read into this filter. */
    apr_uint64_t        read_limit;   /* Max bytes the filter may show to parser */
    apr_size_t          brigade_limit;
    const char         *temp_dir;
};


apr_status_t apreq_filter(ap_filter_t *f,
                          apr_bucket_brigade *bb,
                          ap_input_mode_t mode,
                          apr_read_type_e block,
                          apr_off_t readbytes);

void apreq_filter_make_context(ap_filter_t *f);
void apreq_filter_init_context(ap_filter_t *f);

APR_INLINE
static void apreq_filter_relocate(ap_filter_t *f)
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
static ap_filter_t *get_apreq_filter(apreq_handle_t *env)
{
    struct apache2_handle *handle = (struct apache2_handle *)env;

    if (handle->f == NULL) {
        handle->f = ap_add_input_filter(APREQ_FILTER_NAME, NULL, 
                                        handle->r, 
                                        handle->r->connection);
        /* ap_add_input_filter does not guarantee cfg->f == r->input_filters,
         * so we reposition the new filter there as necessary.
         */
        apreq_filter_relocate(handle->f); 
    }

    return handle->f;
}

APR_INLINE
static apr_status_t apreq_filter_read(ap_filter_t *f, apr_off_t bytes)
{
    struct filter_ctx *ctx = f->ctx;
    apr_status_t s;

    if (ctx->status == APR_EINIT)
        apreq_filter_init_context(f);

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

