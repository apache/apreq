#ifndef APREQ_PARSERS_H
#define APREQ_PARSERS_H
/* These structs are defined below */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct apreq_hook_t apreq_hook_t;
typedef struct apreq_parser_t apreq_parser_t;

/** Parser arguments. */
#define APREQ_PARSER_ARGS  apreq_parser_t *parser,     \
                           apr_table_t *t,             \
                           apr_bucket_brigade *bb

/** Hook arguments */
#define APREQ_HOOK_ARGS    apreq_hook_t *hook,         \
                           apreq_param_t *param,       \
                           apr_bucket_brigade *bb

typedef apr_status_t (*apreq_parser_function_t)(APREQ_PARSER_ARGS);
typedef apr_status_t (*apreq_hook_function_t)(APREQ_HOOK_ARGS);

/**
 * Declares a API parser.
 */
#define APREQ_DECLARE_PARSER(f) APREQ_DECLARE_NONSTD(apr_status_t) \
                                (f) (APREQ_PARSER_ARGS)

/**
 * Declares an API hook.
 */
#define APREQ_DECLARE_HOOK(f)   APREQ_DECLARE_NONSTD(apr_status_t) \
                                (f) (APREQ_HOOK_ARGS)

/**
 * Singly linked list of hooks.
 *
 */
struct apreq_hook_t {
    apreq_hook_function_t hook;
    apreq_hook_t         *next;
    apr_pool_t           *pool;
    void                 *ctx;
};

/**
 * Request parser with associated enctype and hooks. 
 *
 */
struct apreq_parser_t {
    apreq_parser_function_t parser;
    const char             *content_type;
    apr_pool_t             *pool;
    apr_bucket_alloc_t     *bucket_alloc;
    apr_size_t              brigade_limit;
    const char             *temp_dir;
    apreq_hook_t           *hook;
    void                   *ctx;
};




/**
 * Parse the incoming brigade into a table.  Parsers normally
 * consume all the buckets of the brigade during parsing. However
 * parsers may leave "rejected" data in the brigade, even during a
 * successful parse, so callers may need to clean up the brigade
 * themselves (in particular, rejected buckets should not be 
 * passed back to the parser again).
 * @remark  bb == NULL is valid: the parser should return its 
 * public status: APR_INCOMPLETE, APR_SUCCESS, or an error code.
 */
static APR_INLINE
apr_status_t apreq_run_parser(struct apreq_parser_t *psr, apr_table_t *t,
                              apr_bucket_brigade *bb) {
    return psr->parser(psr, t, bb);
}

/**
 * Run the hook with the current parameter and the incoming 
 * bucket brigade.  The hook may modify the brigade if necessary.
 * Once all hooks have completed, the contents of the brigade will 
 * be added to the parameter's bb attribute.
 * @return APR_SUCCESS on success. All other values represent errors.
 */
static APR_INLINE
apr_status_t apreq_run_hook(struct apreq_hook_t *h, apreq_param_t *param,
                            apr_bucket_brigade *bb) {
    return h->hook(h, param, bb);
}


/**
 * Rfc822 Header parser. It will reject all data
 * after the first CRLF CRLF sequence (an empty line).
 * See #apreq_run_parser for more info on rejected data.
 */
APREQ_DECLARE_PARSER(apreq_parse_headers);

/**
 * Rfc2396 application/x-www-form-urlencoded parser.
 */
APREQ_DECLARE_PARSER(apreq_parse_urlencoded);

/**
 * Rfc2388 multipart/form-data (and XForms 1.0 multipart/related)
 * parser. It will reject any buckets representing preamble and 
 * postamble text (this is normal behavior, not an error condition).
 * See #apreq_run_parser for more info on rejected data.
 */
APREQ_DECLARE_PARSER(apreq_parse_multipart);

/**
 * Generic parser.  No table entries will be added to
 * the req->body table by this parser.  The parser creates
 * a dummy apreq_param_t to pass to any configured hooks.  If
 * no hooks are configured, the dummy param's bb slot will
 * contain a copy of the request body.  It can be retrieved
 * by casting the parser's ctx pointer to (apreq_param_t **).
 */
APREQ_DECLARE_PARSER(apreq_parse_generic);

/**
 * apr_xml_parser hook. It will parse until EOS appears.
 * The parsed document isn't available until parsing has
 * completed successfully.  The hook's ctx pointer may 
 * be cast as (apr_xml_doc **) to retrieve the 
 * parsed document.
 */
APREQ_DECLARE_HOOK(apreq_hook_apr_xml_parser);

/**
 * Construct a parser.
 *
 * @param pool Pool used to allocate the parser.
 * @param enctype Content-type that this parser can deal with.
 * @param parser The parser function.
 * @param hook Hooks to asssociate this parser with.
 * @param ctx Parser's internal scratch pad.
 * @return New parser.
 */
APREQ_DECLARE(apreq_parser_t *)
        apreq_make_parser(apr_pool_t *pool,
                          apr_bucket_alloc_t *bucket_alloc,
                          const char *content_type,
                          apreq_parser_function_t parser,
                          apr_size_t brigade_limit,
                          const char *temp_dir,
                          apreq_hook_t *hook,
                          void *ctx);

/**
 * Construct a hook.
 *
 * @param pool used to allocate the hook.
 * @param hook The hook function.
 * @param next List of other hooks for this hook to call on.
 * @param ctx Hook's internal scratch pad.
 * @return New hook.
 */
APREQ_DECLARE(apreq_hook_t *)
        apreq_make_hook(apr_pool_t *pool,
                        apreq_hook_function_t hook,
                        apreq_hook_t *next,
                        void *ctx);


/**
 * Add a new hook to the end of the parser's hook list.
 *
 * @param p Parser.
 * @param h Hook to append.
 */
APREQ_DECLARE(apr_status_t) apreq_parser_add_hook(apreq_parser_t *p, 
                                                  apreq_hook_t *h);


/**
 * Create the default parser associated with the
 * current request's Content-Type (if possible).
 * @param env The current environment.
 * @param hook Hook(s) to add to the parser.
 * @return New parser, NULL if the Content-Type is
 * unrecognized.
 *
 * @param env The current environment.
 * @param hook Additional hooks to supply the parser with.
 * @return The parser; NULL if the environment's
 * Content-Type is unrecognized.
 */
APREQ_DECLARE(apreq_parser_function_t)apreq_parser(const char *enctype);


/**
 * Register a new parsing function with a MIME enctype.
 * Registered parsers are added to apreq_parser()'s
 * internal lookup table.
 *
 * @param enctype The MIME type.
 * @param parser  The function to use during parsing. Setting
 *                parser == NULL will remove an existing parser.
 * @remark This is not a thread-safe operation, so applications 
 * should only call this during process startup,
 * or within a request-thread mutex.
 */

APREQ_DECLARE(apr_status_t) apreq_register_parser(const char *enctype,
                                                  apreq_parser_function_t parser);


/**
 * Returns APR_EGENERAL.  Effectively disables mfd parser
 * if a file-upload field is present.
 *
 */
APREQ_DECLARE_HOOK(apreq_hook_disable_uploads);

/**
 * Calls apr_brigade_cleanup on the incoming brigade
 * after passing the brigade to any subsequent hooks.
 */
APREQ_DECLARE_HOOK(apreq_hook_discard_brigade);


#ifdef __cplusplus
}

#endif
#endif /* APREQ_PARSERS_H */


