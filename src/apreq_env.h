#ifndef APREQ_ENV_H
#define APREQ_ENV_H

#ifdef  __cplusplus
 extern "C" {
#endif 

#include "apreq.h"
/* ctx ~ request_rec */

extern const struct apreq_env {
    const char             *name;
    apr_pool_t *(*pool)(void *ctx);

    /* header access */
    const char *(*in)(void *ctx, char *name);
    apr_status_t(*out)(void *ctx, const char *name, char *value);

    /* cached objects */
    void        *(*jar)(void *ctx, void *j);
    void       *(*request)(void *ctx);

    /* request confuration */
    apr_status_t (*configure)(void *ctx, apreq_cfg_t *cfg);
    apr_status_t (*add_parser)(void *ctx, apreq_value_t *parser);

    /* XXX: the brass tacks */
    apr_status_t (*parse)(void *ctx);

    void (*log)(const char *file, int line, int level, 
                apr_status_t status, void *ctx, const char *fmt, ...);
} APREQ_ENV;



#define apreq_env_add_parser(c,p) APREQ_ENV.add_parser(c,p)

APREQ_DECLARE(void) apreq_log(const char *file, int line, int level, 
                              apr_status_t status, void *ctx, const char *fmt, ...);

APREQ_DECLARE(void *) apreq_env_request(void *ctx, void *r);
APREQ_DECLARE(char *) apreq_env_args(void *ctx);
APREQ_DECLARE(apr_status_t) apreq_env_parse(void *req);

APREQ_DECLARE(void *) apreq_env_jar(void *ctx, void *j);

#define apreq_env_name APREQ_ENV.name
#define apreq_env_jar(c,j) APREQ_ENV.jar(c,j)
#define apreq_env_request(c,r) APREQ_ENV.jar(c,r)
#define apreq_env_pool(c) APREQ_ENV.pool(c)
#define apreq_env_cookie(c) APREQ_ENV.in(c, "Cookie")
#define apreq_env_cookie2(c) APREQ_ENV.in(c, "Cookie2")
#define apreq_env_set_cookie(c,s) APREQ_ENV.out(c,"Set-Cookie",s)
#define apreq_env_set_cookie2(c,s) APREQ_ENV.out(c,"Set-Cookie2",s)

#define AP_LOG_MARK      __FILE__,__LINE__

#define AP_LOG_EMERG     0       /* system is unusable */
#define AP_LOG_ALERT     1       /* action must be taken immediately */
#define AP_LOG_CRIT      2       /* critical conditions */
#define AP_LOG_ERR       3       /* error conditions */
#define AP_LOG_WARNING   4       /* warning conditions */
#define AP_LOG_NOTICE    5       /* normal but significant condition */
#define AP_LOG_INFO      6       /* informational */
#define AP_LOG_DEBUG     7       /* debug-level messages */


#define APREQ_LOG_DEBUG(r)  AP_LOG_MARK, AP_LOG_DEBUG,0,(r)
#define APREQ_LOG_ERROR(r)  AP_LOG_MARK, AP_LOG_ERR,0,(r)
#define APREQ_LOG_WARN(r)   AP_LOG_MARK, AP_LOG_WARNING,0,(r)

APREQ_DECLARE(void) apreq_debug(void *ctx, apr_status_t status,
                               const char *fmt, ...);
APREQ_DECLARE(void) apreq_warn(void *ctx, apr_status_t status,
                               const char *fmt, ...);
APREQ_DECLARE(void) apreq_error(void *ctx, apr_status_t status,
                                const char *fmt, ...);

#ifdef __cplusplus
 }
#endif

#endif
