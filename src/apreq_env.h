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
#ifndef APREQ_ENV_H
#define APREQ_ENV_H

#include "apreq_params.h"
#include "apreq_cookie.h"
#include "apreq_parsers.h"

#ifdef HAVE_SYSLOG
#include <syslog.h>

#ifndef LOG_PRIMASK
#define LOG_PRIMASK 7
#endif


#define APREQ_LOG_EMERG     LOG_EMERG     /* system is unusable */
#define APREQ_LOG_ALERT     LOG_ALERT     /* action must be taken immediately */
#define APREQ_LOG_CRIT      LOG_CRIT      /* critical conditions */
#define APREQ_LOG_ERR       LOG_ERR       /* error conditions */
#define APREQ_LOG_WARNING   LOG_WARNING   /* warning conditions */
#define APREQ_LOG_NOTICE    LOG_NOTICE    /* normal but significant condition */
#define APREQ_LOG_INFO      LOG_INFO      /* informational */
#define APREQ_LOG_DEBUG     LOG_DEBUG     /* debug-level messages */

#define APREQ_LOG_LEVELMASK LOG_PRIMASK   /* mask off the level value */

#else

#define	APREQ_LOG_EMERG	    0	/* system is unusable */
#define	APREQ_LOG_ALERT	    1	/* action must be taken immediately */
#define	APREQ_LOG_CRIT	    2	/* critical conditions */
#define	APREQ_LOG_ERR	    3	/* error conditions */
#define	APREQ_LOG_WARNING   4	/* warning conditions */
#define	APREQ_LOG_NOTICE    5	/* normal but significant condition */
#define	APREQ_LOG_INFO	    6	/* informational */
#define	APREQ_LOG_DEBUG	    7   /* debug-level messages */

#define	APREQ_LOG_LEVELMASK	7	/* mask off the level value */

#endif

#define APREQ_LOG_MARK	__FILE__ , __LINE__

#define APREQ_DEBUG  APREQ_LOG_MARK, APREQ_LOG_DEBUG,
#define APREQ_WARN   APREQ_LOG_MARK, APREQ_LOG_WARNING,
#define APREQ_ERROR  APREQ_LOG_MARK, APREQ_LOG_ERR,

#ifdef  __cplusplus
 extern "C" {
#endif 

/**
 * @file apreq_env.h
 * @brief Logging and environment (module) declarations.
 */
/**
 * @defgroup ENV  Environment declarations
 * @ingroup LIBRARY
 * @{
 */

#ifndef WIN32
extern const char apreq_env[];
#else
#if defined(MOD_APREQ_EXPORTS) || defined(LIBAPREQ_CGI_EXPORTS)
__declspec(dllexport) const char apreq_env[];
#else
__declspec(dllimport) const char apreq_env[];
#endif
#endif

/** logger */
#define APREQ_DECLARE_LOG(f) APREQ_DECLARE_NONSTD(void)(f)(const char *file, \
                             int line,  int level, apr_status_t status, \
                             void *env, const char *fmt, ...)


APREQ_DECLARE_LOG(apreq_log);
APREQ_DECLARE(apr_pool_t *) apreq_env_pool(void *env);


APREQ_DECLARE(apreq_jar_t *) apreq_env_jar(void *env, apreq_jar_t *jar);
APREQ_DECLARE(apreq_request_t *) apreq_env_request(void *env,
                                                   apreq_request_t *req);


APREQ_DECLARE(const char *) apreq_env_query_string(void *env);
APREQ_DECLARE(const char *) apreq_env_header_in(void *env, const char *name);


#define apreq_env_content_type(env) apreq_env_header_in(env, "Content-Type")
#define apreq_env_cookie(env) apreq_env_header_in(env, "Cookie")
#define apreq_env_cookie2(env) apreq_env_header_in(env, "Cookie2")

/** header out */
APREQ_DECLARE(apr_status_t)apreq_env_header_out(void *env, 
                                                const char *name,
                                                char *val);

#define apreq_env_set_cookie(e,s) apreq_env_header_out(e,"Set-Cookie",s)
#define apreq_env_set_cookie2(e,s) apreq_env_header_out(e,"Set-Cookie2",s)

APREQ_DECLARE(apr_status_t) apreq_env_read(void *env,
                                           apr_read_type_e block,
                                           apr_off_t bytes);

/** @} */
#ifdef __cplusplus
 }
#endif

#endif
