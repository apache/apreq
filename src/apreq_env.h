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

extern const char apreq_env[];

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


APREQ_DECLARE(apr_status_t)apreq_env_header_out(void *env, 
                                                const char *name,
                                                char *val);

#define apreq_env_set_cookie(e,s) apreq_env_header_out(e,"Set-Cookie",s)
#define apreq_env_set_cookie2(e,s) apreq_env_header_out(e,"Set-Cookie2",s)



APREQ_DECLARE(apr_status_t) apreq_env_read(void *env,
                                           apr_read_type_e block,
                                           apr_off_t bytes);


#ifdef __cplusplus
 }
#endif

#endif
