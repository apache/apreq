#ifndef APREQ_H
#define APREQ_H

#include "apr_tables.h" 
#include <stddef.h>

#ifdef  __cplusplus
 extern "C" {
#endif 


#define APREQ_DECLARE(d)                APR_DECLARE(d)
#define APREQ_DECLARE_NONSTD(d)         APR_DECLARE_NONSTD(d)
#define APREQ_URL_ENCTYPE               "application/x-www-form-urlencoded"
#define APREQ_MFD_ENCTYPE               "multipart/form-data"
#define APREQ_XML_ENCTYPE               "application/xml"

#define APREQ_NELTS                     8

typedef struct apreq_value_t {
    const char          *name;
    apr_status_t         status;
    apr_size_t           size;
    char                 data[1];
} apreq_value_t;

typedef apreq_value_t *(apreq_value_merge_t)(apr_pool_t *p,
                                             const apr_array_header_t *a);
typedef apreq_value_t *(apreq_value_copy_t)(apr_pool_t *p,
                                            const apreq_value_t *v);

#define apreq_attr_to_type(T,A,P) ( (T*) ((char*)(P)-offsetof(T,A)) )
#define apreq_char_to_value(ptr)  apreq_attr_to_type(apreq_value_t, data, ptr)

#define apreq_strtoval(ptr)  apreq_char_to_value(ptr)
#define apreq_strlen(ptr) (apreq_strtoval(ptr)->size)

APREQ_DECLARE(apreq_value_t *) apreq_make_value(apr_pool_t *p, 
                                                const char *name,
                                                const apr_ssize_t nlen,
                                                const char *val, 
                                                const apr_ssize_t vlen);

APREQ_DECLARE(apreq_value_t *) apreq_copy_value(apr_pool_t *p, 
                                                const apreq_value_t *val);

APREQ_DECLARE(apreq_value_t *) apreq_merge_values(apr_pool_t *p, 
                                            const apr_array_header_t *arr);


typedef enum { AS_IS, ENCODE, DECODE, QUOTE } apreq_join_t;

APREQ_DECLARE(const char *) apreq_join(apr_pool_t *p, 
                                       const char *sep, 
                                       const apr_array_header_t *arr, 
                                       apreq_join_t mode);


typedef enum {FULL, PARTIAL} apreq_match_t;

char *apreq_memmem(char* hay, apr_size_t haylen, 
                   const char* ndl, apr_size_t ndllen, 
                   const apreq_match_t type);

apr_ssize_t apreq_index(const char* hay, apr_size_t haylen, 
                        const char* ndl, apr_size_t ndllen, 
                        const apreq_match_t type);

/* url-escapes non-alphanumeric characters */
apr_size_t apreq_quote(char *dest, const char *src, const apr_size_t slen);
apr_size_t apreq_encode(char *dest, const char *src, const apr_size_t slen);
apr_ssize_t apreq_decode(char *dest, const char *src, const apr_size_t slen);

APREQ_DECLARE(char *) apreq_escape(apr_pool_t *p, 
                                   const char *src, const apr_size_t slen);

APREQ_DECLARE(apr_ssize_t) apreq_unescape(char *str);
#define apreq_unescape(str) apreq_decode(str,str,strlen(str))

/**
 * Returns an RFC-822 formatted time string. Similar to ap_gm_timestr_822.
 *
 * @param req The current apreq_request_t object.
 * @param ts  YMDhms time units (from now) until expiry.
 *            Understands "now".
 */

/* returns date string, (time_str is offset from "now") formatted
 * either as an ::NSCOOKIE or ::HTTP date */

typedef enum {HTTP, NSCOOKIE} apreq_expires_t;

APREQ_DECLARE(char *) apreq_expires(apr_pool_t *p, const char *time_str, 
                                    const apreq_expires_t type);

/* file sizes (KMG) to bytes */
apr_int64_t apreq_atoi64(const char *s);

/* "time" strings (YMDhms) to seconds */
long apreq_atol(const char *s);

#ifdef __cplusplus
 }
#endif

#endif /* APREQ_H */

