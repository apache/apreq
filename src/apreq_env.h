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
#include <stdarg.h>

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
 * @ingroup libapreq2
 */

/**
 * Analog of Apache's ap_log_rerror().
 * @param file Filename to list in the log message.
 * @param line Line number from the file.
 * @param level Log level.
 * @param status Status code.
 * @param env Current environment.
 * @param fmt Format string for the log message.
 */

APREQ_DECLARE_NONSTD(void) apreq_log(const char *file, int line,
                                     int level, apr_status_t status,
                                     apreq_env_handle_t *env,
                                     const char *fmt, ...);

/**
 * Pool associated with the environment.
 * @param env The current environment
 * @return The associated pool.
 */

APREQ_DECLARE(apr_pool_t *) apreq_env_pool(apreq_env_handle_t *env);

/**
 * Bucket allocator associated with the environment.
 * @param env The current environment
 * @return The associated bucket allocator.
 */

APREQ_DECLARE(apr_bucket_alloc_t *) apreq_env_bucket_alloc(apreq_env_handle_t *env);

/**
 * Get/set the jar currently associated to the environment.
 * @param env The current environment.
 * @param jar New Jar to associate.
 * @return The previous jar associated to the environment.
 * jar == NULL gets the current jar, which will remain associated
 * after the call.
 */
APREQ_DECLARE(apreq_jar_t *) apreq_env_jar(apreq_env_handle_t *env,
                                           apreq_jar_t *jar);

/**
 * Get/set the request currently associated to the environment.
 * @param env The current environment.
 * @param req New request to associate.
 * @return The previous request associated to the environment.
 * req == NULL gets the current request, which will remain associated
 * after the call.
 */
APREQ_DECLARE(apreq_request_t *) apreq_env_request(apreq_env_handle_t *env,
                                                   apreq_request_t *req);

/**
 * Fetch the query string.
 * @param env The current environment.
 * @return The query string.
 */
APREQ_DECLARE(const char *) apreq_env_query_string(apreq_env_handle_t *env);

/**
 * Fetch the header value (joined by ", " if there are multiple headers)
 * for a given header name.
 * @param env The current environment.
 * @param name The header name.
 * @return The value of the header, NULL if not found.
 */
APREQ_DECLARE(const char *) apreq_env_header_in(apreq_env_handle_t *env,
                                                const char *name);


/**
 * Fetch the environment's "Content-Type" header.
 * @param env The current environment.
 * @return The value of the Content-Type header, NULL if not found.
 */
#define apreq_env_content_type(env) apreq_env_header_in(env, "Content-Type")


/**
 * Fetch the environment's "Cookie" header.
 * @param env The current environment.
 * @return The value of the "Cookie" header, NULL if not found.
 */
#define apreq_env_cookie(env) apreq_env_header_in(env, "Cookie")

/**
 * Fetch the environment's "Cookie2" header.
 * @param env The current environment.
 * @return The value of the "Cookie2" header, NULL if not found.
 */
#define apreq_env_cookie2(env) apreq_env_header_in(env, "Cookie2")

/**
 * Add a header field to the environment's outgoing response headers
 * @param env The current environment.
 * @param name The name of the outgoing header.
 * @param val Value of the outgoing header.
 * @return APR_SUCCESS on success, error code otherwise.
 */
APREQ_DECLARE(apr_status_t)apreq_env_header_out(apreq_env_handle_t *env, 
                                                const char *name,
                                                char *val);

/**
 * Add a "Set-Cookie" header to the outgoing response headers.
 * @param e The current environment.
 * @param s The cookie string.
 * @return APR_SUCCESS on success, error code otherwise.
 */
#define apreq_env_set_cookie(e,s) apreq_env_header_out(e,"Set-Cookie",s)

/**
 * Add a "Set-Cookie2" header to the outgoing response headers.
 * @param e The current environment.
 * @param s The cookie string.
 * @return APR_SUCCESS on success, error code otherwise.
 */
#define apreq_env_set_cookie2(e,s) apreq_env_header_out(e,"Set-Cookie2",s)

/**
 * Read data from the environment and into the current active parser.
 * @param env The current environment.
 * @param block Read type (APR_READ_BLOCK or APR_READ_NONBLOCK).
 * @param bytes Maximum number of bytes to read.
 * @return APR_INCOMPLETE if there's more data to read,
 *         APR_SUCCESS if everything was read & parsed successfully,
 *         error code otherwise.
 */
APREQ_DECLARE(apr_status_t) apreq_env_read(apreq_env_handle_t *env,
                                           apr_read_type_e block,
                                           apr_off_t bytes);

/**
 * Get/set the current temporary directory.
 * @param env The current environment.
 * @param path The full pathname of the new directory.
 * @return The path of the previous temporary directory.  Note: a call using
 * path==NULL fetches the current directory without resetting it to NULL.
 */

APREQ_DECLARE(const char *) apreq_env_temp_dir(apreq_env_handle_t *env,
                                               const char *path);

/**
 * Get/set the current max_body setting.  This is the maximum
 * amount of bytes that will be read into the environment's parser.
 * @param env The current environment.
 * @param bytes The new max_body setting.
 * @return The previous max_body setting.  Note: a call using
 * bytes == -1 fetches the current max_body setting without modifying it.
 *
 */

APREQ_DECLARE(apr_off_t) apreq_env_max_body(apreq_env_handle_t *env,
                                            apr_off_t bytes);

/**
 * Get/set the current max_brigade setting.  This is the maximum
 * amount of heap-allocated buckets libapreq2 will use for its brigades.  
 * If additional buckets are necessary, they will be created from a temporary file.
 * @param env The current environment.
 * @param bytes The new max_brigade setting.
 * @return The previous max_brigade setting.  Note: a call using
 * bytes == -1 fetches the current max_brigade setting without modifying it.
 *
 */
APREQ_DECLARE(apr_ssize_t) apreq_env_max_brigade(apreq_env_handle_t *env,
                                                 apr_ssize_t bytes);

/**
 * This must be fully defined for libapreq2 to operate properly 
 * in a given environment. Normally it is set once, with an apreq_env_module() 
 * call during process initialization, and should remain fixed thereafter.
 * @brief Vtable describing the necessary environment functions.
 */

typedef struct apreq_env_module_t {
    const char *name;
    apr_uint32_t magic_number;
    void (*log)(const char *,int,int,apr_status_t,apreq_env_handle_t *,
                const char *,va_list);
    apr_pool_t *(*pool)(apreq_env_handle_t *);
    apr_bucket_alloc_t *(*bucket_alloc)(apreq_env_handle_t *);
    apreq_jar_t *(*jar)(apreq_env_handle_t *,apreq_jar_t *);
    apreq_request_t *(*request)(apreq_env_handle_t *,apreq_request_t *);
    const char *(*query_string)(apreq_env_handle_t *);
    const char *(*header_in)(apreq_env_handle_t *,const char *);
    apr_status_t (*header_out)(apreq_env_handle_t *, const char *,char *);
    apr_status_t (*read)(apreq_env_handle_t *,apr_read_type_e,apr_off_t);
    const char *(*temp_dir)(apreq_env_handle_t *, const char *);
    apr_off_t (*max_body)(apreq_env_handle_t *,apr_off_t);
    apr_ssize_t (*max_brigade)(apreq_env_handle_t *, apr_ssize_t);
} apreq_env_module_t;

/**
 * Convenience macro for defining an environment module by mapping
 * a function prefix to an associated environment structure.
 * @param pre Prefix to define new environment.  All attributes of
 * the apreq_env_module_t struct are defined with this as their prefix. The
 * generated struct is named by appending "_module" to the prefix.
 * @param name Name of this environment.
 * @param mmn Magic number (i.e. version number) of this environment.
 */
#define APREQ_ENV_MODULE(pre, name, mmn) const apreq_env_module_t pre##_module = { \
  name, mmn, pre##_log, pre##_pool, pre##_bucket_alloc, pre##_jar,          \
  pre##_request, pre##_query_string, pre##_header_in, pre##_header_out,     \
  pre##_read, pre##_temp_dir, pre##_max_body, pre##_max_brigade }

/**
 * Create an apreq handle which is suitable for a CGI program. It
 * reads input from stdin and writes output to stdout.
 */
APREQ_DECLARE(apreq_env_handle_t*) apreq_env_make_cgi(apr_pool_t *pool);

/**
 * Create a custom apreq handle which knows only some static
 * values. Useful if you want to test the parser code or if you have
 * got data from a custom source (neither Apache 2 nor CGI).
 * @param pool the APR pool
 * @param query_string the query string
 * @param cookie value of the request "Cookie" header
 * @param cookie2 value of the request "Cookie2" header
 * @param content_type content type of the request body
 * @param in a bucket brigade containing the request body
 */
APREQ_DECLARE(apreq_env_handle_t*) apreq_env_make_custom(apr_pool_t *pool,
                                                         const char *query_string,
                                                         const char *cookie,
                                                         const char *cookie2,
                                                         const char *content_type,
                                                         apr_bucket_brigade *in);

#ifdef __cplusplus
 }
#endif

#endif
