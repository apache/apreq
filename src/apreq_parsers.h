#ifndef APREQ_PARSERS_H
#define APREQ_PARSERS_H


#ifdef  __cplusplus
extern "C" {
#endif 


/* parsers: single copy paradigm */

/***********************************************************
 * API: apreq_parsers.c
 */

typedef struct apreq_parser_t apreq_parser_t;

#define APREQ_PARSER(f) apr_status_t (f)(apr_pool_t *p, apr_bucket_brigade *bb, 
                                         apreq_parser_t *parser)

#define APREQ_CTX_MAXSIZE  128

struct apreq_parser_t {
    APREQ_PARSER   (*parser);
    void            *out;
    apreq_parser_t  *ext;
    apreq_value_t    v; /* maintains (internal) parser state */
};


#define apreq_value_to_parser(ptr) apreq_attr_to_type(apreq_parser_t, \
                                                      v, ptr)
#define apreq_ctx_to_parser(ptr) apreq_value_to_parser(apreq_strtoval(ptr))

#define apreq_parser_hook(p)     ((p)->next)
#define apreq_parser_enctype(p)  ((p)->v.name)
#define apreq_parser_data(p)     ((p)->v.data)

#define apreq_parse(pool, bb, p) (p)->parser(pool, bb, p)

APREQ_DECLARE(apr_status_t) apreq_parse_headers(apr_pool_t *p,
                                                apr_bucket_brigade *bb,
                                                apreq_parser_t *parser);

APREQ_DECLARE(apreq_parser_t *) apreq_make_parser(const char *enctype,
                                                  APREQ_PARSER(*parser),
                                                  apreq_parser_t *ext);

APREQ_DECLARE(apr_status_t) apreq_add_parser(apreq_table_t *t, 
                                             apreq_parser_t *parser);

#define apreq_add_parser(t,p) apreq_table_add(t, &(p)->v)

APREQ_DECLARE(apr_status_t) apreq_copy_parser(apr_pool_t *p, 
                                              const apreq_value_t *v);

APREQ_DECLARE(apr_status_t) apreq_merge_parsers(apr_pool_t *p,
                                                const apr_array_header_t *a);


#ifdef __cplusplus
 }
#endif

#endif /*APREQ_PARSERS_H*/
