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

#ifndef APREQ_PARSERS_H
#define APREQ_PARSERS_H

#include "apreq.h"
#include "apr_tables.h"
#include "apr_buckets.h"
#include "apr_file_io.h"

#ifdef  __cplusplus
extern "C" {
#endif 


/**
 * Single-copy paradigm.
 *
 * @file apreq_parsers.h
 * @brief Parser and Hook stuff.
 */
/**
 * @defgroup parsers Parsers and Hooks
 * @ingroup LIBRARY
 * @{
 */

/** Request config */
typedef struct apreq_cfg_t {
    apr_off_t      max_len;
    apr_size_t     max_brigade_len; /* in-memory cutoff */
    int            max_fields;
    int            read_ahead; /* prefetch length */
    int            disable_uploads;
    char          *temp_dir;
} apreq_cfg_t;


#define APREQ_DECLARE_PARSER(f) apr_status_t (f)(apreq_parser_t *parser, \
                                         const apreq_cfg_t *cfg,         \
                                         apr_table_t *t,                 \
                                         apr_bucket_brigade *bb)

#define APREQ_DECLARE_HOOK(f) apr_status_t (f)(apreq_hook_t *hook,   \
                                       apr_pool_t *pool,             \
                                       const apreq_cfg_t *cfg,       \
                                       apr_bucket_brigade *bb)

#define apreq_run_parser(psr,cfg,t,bb) (psr)->parser(psr,cfg,t,bb)
#define apreq_run_hook(h,pool,cfg,bb) (h)->hook(h,pool,cfg,bb)

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
APREQ_DECLARE(apr_status_t) apreq_brigade_concat(apr_pool_t *pool, 
                                                 const apreq_cfg_t *cfg,
                                                 apr_bucket_brigade *out, 
                                                 apr_bucket_brigade *in);


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

/** @} */
#ifdef __cplusplus
 }
#endif

#endif /*APREQ_PARSERS_H*/
