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

#ifndef APREQ_ENV_H
#define APREQ_ENV_H

#include "apreq_params.h"
#include "apreq_cookie.h"

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

APREQ_DECLARE_NONSTD(void) apreq_log(const char *file, int line,
                                     int level, apr_status_t status,
                                     void *env, const char *fmt, ...);

APREQ_DECLARE(apr_pool_t *) apreq_env_pool(void *env);
APREQ_DECLARE(apreq_jar_t *) apreq_env_jar(void *env, apreq_jar_t *jar);
APREQ_DECLARE(apreq_request_t *) apreq_env_request(void *env,
                                                   apreq_request_t *req);

APREQ_DECLARE(const char *) apreq_env_query_string(void *env);
APREQ_DECLARE(const char *) apreq_env_header_in(void *env, const char *name);

#define apreq_env_content_type(env) apreq_env_header_in(env, "Content-Type")
#define apreq_env_cookie(env) apreq_env_header_in(env, "Cookie")
#define apreq_env_cookie2(env) apreq_env_header_in(env, "Cookie2")

APREQ_DECLARE(apr_status_t)apreq_env_header_out(void *env, 
                                                const char *name,
                                                char *val);

#define apreq_env_set_cookie(e,s) apreq_env_header_out(e,"Set-Cookie",s)
#define apreq_env_set_cookie2(e,s) apreq_env_header_out(e,"Set-Cookie2",s)

APREQ_DECLARE(apr_status_t) apreq_env_read(void *env,
                                           apr_read_type_e block,
                                           apr_off_t bytes);

APREQ_DECLARE(const char *) apreq_env_temp_dir(void *env, const char *path);
APREQ_DECLARE(apr_off_t) apreq_env_max_body(void *env, apr_off_t bytes);
APREQ_DECLARE(apr_ssize_t) apreq_env_max_brigade(void *env, apr_ssize_t bytes);

typedef struct apreq_env_t {
    const char *name;
    apr_uint32_t magic_number;
    void (*log)(const char *,int,int,apr_status_t,void *,const char *,va_list);
    apr_pool_t *(*pool)(void *);
    apreq_jar_t *(*jar)(void *,apreq_jar_t *);
    apreq_request_t *(*request)(void *,apreq_request_t *);
    const char *(*query_string)(void *);
    const char *(*header_in)(void *,const char *);
    apr_status_t (*header_out)(void *, const char *,char *);
    apr_status_t (*read)(void *,apr_read_type_e,apr_off_t);
    const char *(*temp_dir)(void *, const char *);
    apr_off_t (*max_body)(void *,apr_off_t);
    apr_ssize_t (*max_brigade)(void *, apr_ssize_t);
} apreq_env_t;

#define APREQ_ENV_MODULE(pre, name, mmn) const apreq_env_t pre##_module = { \
  name, mmn, pre##_log, pre##_pool, pre##_jar, pre##_request,               \
  pre##_query_string, pre##_header_in, pre##_header_out, pre##_read,        \
  pre##_temp_dir, pre##_max_body, pre##_max_brigade }


APREQ_DECLARE(const apreq_env_t *) apreq_env_module(const apreq_env_t *mod);

#define apreq_env_name (apreq_env_module(NULL)->name)
#define apreq_env_magic_number (apreq_env_module(NULL)->magic_number)

/** @} */
#ifdef __cplusplus
 }
#endif

#endif
