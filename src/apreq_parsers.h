/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2003 The Apache Software Foundation.  All rights
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
