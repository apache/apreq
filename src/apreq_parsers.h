#ifndef APREQ_PARSERS_H
#define APREQ_PARSERS_H

#include "apreq.h"
#include "apr_tables.h"
#include "apr_buckets.h"

#ifdef  __cplusplus
extern "C" {
#endif 

/* parsers: single copy paradigm */

/***********************************************************
 * API: apreq_parsers.c
 */

typedef struct apreq_cfg_t {
    char          *temp_dir;
    apr_off_t      max_len;
    int            read_bytes;
    int            disable_uploads;
} apreq_cfg_t;


#define APREQ_DECLARE_PARSER(f) apr_status_t (f)(apreq_parser_t *parser, \
                                         const apreq_cfg_t *cfg,         \
                                         apr_table_t *t,                 \
                                         apr_bucket_brigade *bb)

#define APREQ_DECLARE_HOOK(f) apr_status_t (f)(apreq_hook_t *hook,   \
                                       apr_pool_t *pool,             \
                                       const apreq_cfg_t *cfg,       \
                                       apr_bucket_brigade *out,      \
                                       apr_bucket_brigade *in)

#define APREQ_CTX_MAXSIZE  128
#define apreq_run_parser(psr,cfg,t,bb) psr->parser(psr,cfg,t,bb)
#define apreq_run_hook(h,pool,cfg,out,in) h->hook(h,pool,cfg,out,in)

typedef struct apreq_hook_t apreq_hook_t;
typedef struct apreq_parser_t apreq_parser_t;

struct apreq_hook_t {
    APREQ_DECLARE_HOOK    (*hook);
    apreq_hook_t           *next;
    void                   *ctx;
};

struct apreq_parser_t {
    APREQ_DECLARE_PARSER  (*parser);
    const char             *content_type;
    apreq_hook_t           *hook;
    void                   *ctx;
};


APREQ_DECLARE_PARSER(apreq_parse_headers);
APREQ_DECLARE_PARSER(apreq_parse_urlencoded);
APREQ_DECLARE_PARSER(apreq_parse_multipart);

APREQ_DECLARE(apreq_parser_t *) apreq_make_parser(apr_pool_t *pool,
                                                  const char *enctype,
                                                  APREQ_DECLARE_PARSER(*parser),
                                                  apreq_hook_t *hook,
                                                  void *ctx);

APREQ_DECLARE(apreq_hook_t *) apreq_make_hook(apr_pool_t *pool,
                                              APREQ_DECLARE_HOOK(*hook),
                                              apreq_hook_t *next,
                                              void *ctx);

APREQ_DECLARE(apr_status_t)apreq_add_hook(apreq_parser_t *p, 
                                          apreq_hook_t *h);

APREQ_DECLARE(apreq_parser_t *)apreq_parser(void *env, apreq_hook_t *hook);

#ifdef __cplusplus
 }
#endif

#endif /*APREQ_PARSERS_H*/
