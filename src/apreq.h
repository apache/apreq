#ifndef APREQ_H
#define APREQ_H

#ifdef  __cplusplus
 extern "C" {
#endif 

#include "apr_tables.h" 
#include "apr_lib.h"
#include "apr_pools.h"
#include "apr_strings.h"

#define APREQ_DECLARE(d)        d

/* default enctypes */
#define APREQ_URL_ENCTYPE               "application/x-www-form-urlencoded"
#define APREQ_URL_LENGTH                33
#define APREQ_MFD_ENCTYPE               "multipart/form-data"
#define APREQ_MFD_ENCTYPE_LENGTH        19
#define APREQ_XML_ENCTYPE               "text/xml"
#define APREQ_XML_ENCTYPE_LENGTH        8

#define APREQ_EXPIRES_HTTP              1
#define APREQ_EXPIRES_NSCOOKIE          2

#define APREQ_DEFAULT_POST_MAX         -1 /* XXX: unsafe default ??? */
#define APREQ_DEFAULT_NELTS             8

/* string match modes:
 * full match: full needle must be found 
 * partial match: allow for partial matches at the end of haystack
 */
#define APREQ_MATCH_FULL                0
#define APREQ_MATCH_PART                1

/* join modes */
#define APREQ_ESCAPE                    1
#define APREQ_URLENCODE                 2
#define APREQ_QUOTE                     3

/* status codes */
#define APREQ_OK                  0
#define APREQ_CONTINUE          100
#define APREQ_ERROR             500

typedef struct apreq_value_t {
    apr_ssize_t          size;
    unsigned char        flags; /* ??? */
    char                 data[1];
} apreq_value_t;


typedef apreq_value_t *(apreq_value_merge_t)(apr_pool_t *p,
                                             const apr_array_header_t *a);
typedef apreq_value_t *(apreq_value_copy_t)(apr_pool_t *p,
                                            const apreq_value_t *v);

#define apreq_attr_to_type(T,A,P) ( (T*) ((char*)(P)-offsetof(T,A)) )
#define apreq_char_to_value(ptr)  apreq_attr_to_type(apreq_value_t, data, ptr)

apreq_value_t *apreq_value_make(apr_pool_t *p, const char *str, 
                                const apr_ssize_t size);
apreq_value_t *apreq_value_copy(apr_pool_t *p, const apreq_value_t *val);
apreq_value_t *apreq_value_merge(apr_pool_t *p, const apr_array_header_t *arr);

/* apr_array_pstrcat, w/ string separator and APREQ_ESCAPE mode */
char *apreq_pvcat(apr_pool_t *p, 
                  const apr_array_header_t *arr, 
                  const char *sep, 
                  int mode);

char *apreq_memmem(char* h, apr_off_t hlen, 
                   const char* n, apr_off_t nlen, int part);

apr_off_t apreq_index(char* h, apr_off_t hlen, 
                      const char* n, apr_off_t nlen, int part);

/* url-escapes non-alphanumeric characters */
char *apreq_escape(apr_pool_t *p, char *s);
apr_off_t apreq_unescape(char *s);

/* returns date string, (time_str is offset from "now") formatted
 * either as an NSCOOKIE or HTTP date */
char *apreq_expires(apr_pool_t *p, char *time_str, int type);

/* file sizes (KMG) to bytes */
apr_int64_t apreq_atol(const char *s);
/* "duration" strings (YMDhms) to seconds */
apr_int64_t apreq_atod(const char *s);

/* r->uri, omitting path info */


/*
char *apreq_script_name(void *r);
char *(apreq_script_path)(void *r);
#define apreq_script_path(r) ap_make_dirstr_parent(r->pool,apreq_script_name(r))
*/
#ifdef __cplusplus
 }
#endif

#endif
