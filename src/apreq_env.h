#ifndef APREQ_ENV_H
#define APREQ_ENV_H

#include "apreq_cookie.h"

/*
#include "apreq_request.h"
*/

#ifdef  __cplusplus
 extern "C" {
#endif 

/* XXX: Per-process initialization */

struct apreq_request_env {
    int foo;
/*
    apr_status_t (*init)(void *ctx, apreq_parser_cfg_t *cfg);
    apreq_request_t *(*make)(void *ctx);
    apr_status_t (*parse)(apreq_request_t *req);
*/
};

struct apreq_cookie_env {
    const char *(*in)(void *ctx);
    const char *(*in2)(void *ctx);

    apr_status_t (*jar)(void *ctx, apreq_jar_t **j);
    apr_status_t (*out)(void *ctx, const char *c);
    apr_status_t (*out2)(void *ctx, const char *c);
};

extern const struct apreq_env {
    struct apreq_request_env r;
    struct apreq_cookie_env  c;

    /* uri components (cookies need to know the host/domain and path) */
    const char *(*uri)(void *ctx);
    const char *(*uri_path)(void *ctx);

    apr_pool_t *(*pool)(void *ctx);
    void (*log)(const char *file, int line, int level, 
                apr_status_t status, void *ctx, const char *fmt, ...);
} APREQ_ENV;

APREQ_DECLARE(void) apreq_log(const char *file, int line, int level, 
                              apr_status_t status, void *ctx, const char *fmt, ...);

APREQ_DECLARE(apr_status_t) apreq_env_jar(void *ctx, apreq_jar_t **j);
APREQ_DECLARE(const char *) apreq_env_cookie(void *ctx);
APREQ_DECLARE(const char *) apreq_env_cookie2(void *ctx);
APREQ_DECLARE(apr_status_t) apreq_env_set_cookie(void *ctx, const char *s);
APREQ_DECLARE(apr_status_t) apreq_env_set_cookie2(void *ctx, const char *s);
APREQ_DECLARE(apr_status_t) apreq_env_cookie_jar(void *ctx, 
                                                 apreq_jar_t **j);

#define apreq_env_pool(r) APREQ_ENV.pool(r)
#define apreq_env_cookie(r) APREQ_ENV.c.in(r)
#define apreq_env_cookie2(r) APREQ_ENV.c.in2(r)
#define apreq_env_set_cookie(r,s) APREQ_ENV.c.out(r,s)
#define apreq_env_set_cookie2(r,s) APREQ_ENV.c.out2(r,s)

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

APREQ_DECLARE(void) apreq_warn(void *ctx, const char *fmt, ...);

#ifdef __cplusplus
 }
#endif

#endif
