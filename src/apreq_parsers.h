#ifndef APREQ_PARSERS_H
#define APREQ_PARSERS_H


#ifdef  __cplusplus
extern "C" {
#endif 

#include "apreq_params.h"
#include "apr_buckets.h"

/* parsers: single copy paradigm */

/***********************************************************
 * API: apreq_parsers.c
 */

typedef struct apreq_parser_t apreq_parser_t;

#define APREQ_PARSER(f) apr_status_t(f)(apr_pool_t *p, apr_bucket_brigade *bb,\
                                        apreq_parser_t *parser)
#define APREQ_HOOK(f) apr_status_t(f)(apr_pool_t *p, apr_bucket_brigade *out, \
                                      apr_bucket_brigade *in, \
                                      apreq_parser_t *parser)

#define APREQ_CTX_MAXSIZE  128

struct apreq_parser_t {
    APREQ_PARSER   (*parser);
    APREQ_HOOK     (*hook);
    void            *out;
    apreq_value_t    v; /* maintains (internal) parser state */
};


#define apreq_value_to_parser(ptr) apreq_attr_to_type(apreq_parser_t,v,ptr)
#define apreq_ctx_to_parser(ptr) apreq_value_to_parser(apreq_strtoval(ptr))

#define apreq_parser_enctype(p)  ((p)->v.name)
#define apreq_parser_ctx(p)     ((p)->v.data)

APREQ_DECLARE(apr_status_t) apreq_parse_headers(apr_pool_t *p,
                                                apr_bucket_brigade *bb,
                                                apreq_parser_t *parser);

APREQ_DECLARE(apr_status_t) apreq_parse_urlencoded(apr_pool_t *pool,
                                                   apr_bucket_brigade *bb,
                                                   apreq_parser_t *parser);

APREQ_DECLARE(apr_status_t) apreq_parse_multipart(apr_pool_t *pool,
                                                  apr_bucket_brigade *bb,
                                                  apreq_parser_t *parser);


APREQ_DECLARE(apreq_parser_t *) apreq_make_parser(apr_pool_t *pool,
                                                  const char *enctype,
                                                  APREQ_PARSER(*parser),
                                                  APREQ_HOOK(*hook),
                                                  void *out);

APREQ_DECLARE(apr_status_t) apreq_register_parser(apreq_request_t *req,
                                                  apreq_parser_t *parser);

APREQ_DECLARE(apr_status_t) apreq_copy_parser(apr_pool_t *p, 
                                              const apreq_value_t *v);

APREQ_DECLARE(apr_status_t) apreq_merge_parsers(apr_pool_t *p,
                                                const apr_array_header_t *a);

APREQ_DECLARE(apr_status_t)apreq_parse(apreq_request_t *req, 
                                       apr_bucket_brigade *bb);


#ifdef __cplusplus
 }
#endif

#endif /*APREQ_PARSERS_H*/
