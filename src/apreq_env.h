#ifndef APREQ_ENV_H
#define APREQ_ENV_H

#ifdef  __cplusplus
 extern "C" {
#endif 

#include "apreq.h"
#include "apr_buckets.h"

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

#define dAPREQ_LOG  APREQ_LOG(*apreq_log) = APREQ_ENV.log

typedef struct apreq_cfg_t {
    apr_off_t max_len;
    char     *temp_dir;
    int       disable_uploads;
} apreq_cfg_t;

#define APREQ_LOG(f) void (f)(const char *file, int line, int level, \
                        apr_status_t status, void *ctx, const char *fmt, ...)

extern const struct apreq_env {
    const char          *name;
    apr_pool_t          *(*pool)(void *ctx);

    /* header access */
    const char          *(*in)(void *ctx, const char *name);
    apr_status_t         (*out)(void *ctx, const char *name, char *value);

    /* raw (unparsed) query_string access */
    const char          *(*args)(void *ctx);

    /* (get/set) cached core objects */
    void                *(*jar)(void *ctx, void *j);
    void                *(*request)(void *ctx, void *r);

    /* environment configuration */
    apreq_cfg_t         *(*config)(void *ctx);

    /* the brass tacks */
    apr_status_t  (*get_brigade)(void *ctx, apr_bucket_brigade **bb);

    /* core logging function */
    APREQ_LOG            (*log);

} APREQ_ENV;

#define apreq_env_name() APREQ_ENV.name
#define apreq_env_pool(c) APREQ_ENV.pool(c)

#define apreq_env_jar(c,j) APREQ_ENV.jar(c,j)
#define apreq_env_request(c,r) APREQ_ENV.request(c,r)

#define apreq_env_config(c) APREQ_ENV.config(c)

#define apreq_env_content_type(c) APREQ_ENV.in(c, "Content-Type");
#define apreq_env_cookie(c) APREQ_ENV.in(c, "Cookie")
#define apreq_env_cookie2(c) APREQ_ENV.in(c, "Cookie2")
#define apreq_env_set_cookie(c,s) APREQ_ENV.out(c,"Set-Cookie",s)
#define apreq_env_set_cookie2(c,s) APREQ_ENV.out(c,"Set-Cookie2",s)
#define apreq_env_args(c) APREQ_ENV.args(c)

#define apreq_env_get_brigade(c,bb) APREQ_ENV.get_brigade(c,bb)


#ifdef __cplusplus
 }
#endif

#endif
