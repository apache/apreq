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

#ifndef APREQ_UTIL_H
#define APREQ_UTIL_H

#include "apr_tables.h" 
#include "apr_file_io.h"
#include "apr_buckets.h"
#include <stddef.h>

#ifdef  __cplusplus
 extern "C" {
#endif 

/**
 * The objects in apreq.h are used in various contexts:
 *
 *    - apreq_value_t - the base struct for params & cookies
 *    - string <-> array converters
 *    - substring search functions
 *    - simple encoders & decoders for urlencoded strings
 *    - simple time, date, & file-size converters
 * @file apreq.h
 * @brief Common functions, structures and macros.
 * @ingroup libapreq2
 */

#ifndef WIN32
#define APREQ_DECLARE(d)                APR_DECLARE(d)
#define APREQ_DECLARE_NONSTD(d)         APR_DECLARE_NONSTD(d)
#define APREQ_DECLARE_DATA
#else
#define APREQ_DECLARE(type)             __declspec(dllexport) type __stdcall
#define APREQ_DECLARE_NONSTD(type)      __declspec(dllexport) type
#define APREQ_DECLARE_DATA              __declspec(dllexport)
#endif


/**
 * Commong Defaults.
 * Maximum amount of heap space a brigade may use before switching to file
 * buckets
*/

#define APREQ_DEFAULT_READ_BLOCK_SIZE   (64  * 1024)
#define APREQ_DEFAULT_READ_LIMIT        (64 * 1024 * 1024)
#define APREQ_DEFAULT_BRIGADE_LIMIT     (256 * 1024)
#define APREQ_DEFAULT_NELTS              8

/**
 * Beginning work on error-codes ...
 *
 *
 */
#ifndef APR_EBADARG
#define APR_EBADARG                APR_BADARG   /* apr's unfixed booboo */
#endif

/* 0's: generic error status codes */
#define APREQ_ERROR_GENERAL        APR_OS_START_USERERR
#define APREQ_ERROR_INTERRUPT      APREQ_ERROR_GENERAL + 1

/* 10's: malformed input */
#define APREQ_ERROR_NODATA         APREQ_ERROR_GENERAL + 10
#define APREQ_ERROR_BADSEQ         APREQ_ERROR_GENERAL + 11
#define APREQ_ERROR_BADCHAR        APREQ_ERROR_GENERAL + 12
#define APREQ_ERROR_BADTOKEN       APREQ_ERROR_GENERAL + 13
#define APREQ_ERROR_NOTOKEN        APREQ_ERROR_GENERAL + 14
#define APREQ_ERROR_BADATTR        APREQ_ERROR_GENERAL + 15
#define APREQ_ERROR_BADHEADER      APREQ_ERROR_GENERAL + 16
#define APREQ_ERROR_NOHEADER       APREQ_ERROR_GENERAL + 17

/* 20's: misconfiguration */
#define APREQ_ERROR_CONFLICT       APREQ_ERROR_GENERAL + 20
#define APREQ_ERROR_NOPARSER       APREQ_ERROR_GENERAL + 21


/* 30's: limit violations */
#define APREQ_ERROR_OVERLIMIT      APREQ_ERROR_GENERAL + 30
#define APREQ_ERROR_UNDERLIMIT     APREQ_ERROR_GENERAL + 31




/** @brief libapreq's pre-extensible string type */
typedef struct apreq_value_t {
    char             *name;    /**< value name */
    apr_size_t        size;    /**< value length (in bytes) */
    char              data[1]; /**< value data  */
} apreq_value_t;


#define apreq_attr_to_type(T,A,P) ( (T*) ((char*)(P)-offsetof(T,A)) )

/**
 * Construcs an apreq_value_t from the name/value info
 * supplied by the arguments.
 *
 * @param p     Pool for allocating the name and value.
 * @param name  Name of value.
 * @param nlen  Length of name.
 * @param val   Value data.
 * @param vlen  Length of val.
 * @return      apreq_value_t allocated from pool, 
 *              with v->data holding a copy of val, v->status = 0, and
 *              v->name pointing to a nul-terminated copy of name.
 */
APREQ_DECLARE(apreq_value_t *) apreq_make_value(apr_pool_t *p, 
                                                const char *name,
                                                const apr_size_t nlen,
                                                const char *val, 
                                                const apr_size_t vlen);

/** @enum apreq_join_t Join type */
typedef enum { 
    APREQ_JOIN_AS_IS,      /**< Join the strings without modification */
    APREQ_JOIN_ENCODE,     /**< Url-encode the strings before joining them */
    APREQ_JOIN_DECODE,     /**< Url-decode the strings before joining them */
    APREQ_JOIN_QUOTE       /**< Quote the strings, backslashing existing quote marks. */
} apreq_join_t;

/**
 * Join an array of values.
 * @param p    Pool to allocate return value.
 * @param sep  String that is inserted between the joined values.
 * @param arr  Array of values.
 * @param mode Join type- see apreq_join_t.
 * @remark     Return string can be upgraded to an apreq_value_t.
 */
APREQ_DECLARE(const char *) apreq_join(apr_pool_t *p, 
                                       const char *sep, 
                                       const apr_array_header_t *arr, 
                                       apreq_join_t mode);


/** @enum apreq_match_t Match type */
typedef enum {
    APREQ_MATCH_FULL,       /**< Full match only. */
    APREQ_MATCH_PARTIAL     /**< Partial matches are ok. */
} apreq_match_t;

/**
 * Returns offset of match string's location, or -1 if no match is found.
 * @param hay  Location of bytes to scan.
 * @param hlen Number of bytes available for scanning.
 * @param ndl  Search string
 * @param nlen Length of search string.
 * @param type Match type.
 * @return Offset of match string, or -1 if mo match is found.
 *
 */
APREQ_DECLARE(apr_ssize_t) apreq_index(const char* hay, apr_size_t hlen, 
                                       const char* ndl, apr_size_t nlen, 
                                       const apreq_match_t type);
/**
 * Places a quoted copy of src into dest.  Embedded quotes are escaped with a
 * backslash ('\').
 * @param dest Location of quoted copy.  Must be large enough to hold the copy
 *             and trailing null byte.
 * @param src  Original string.
 * @param slen Length of original string.
 * @param dest Destination string.
 * @return length of quoted copy in dest.
 */
APREQ_DECLARE(apr_size_t) apreq_quote(char *dest, const char *src, 
                                      const apr_size_t slen);

/**
 * Same as apreq_quote() except when src begins and ends in quote marks. In
 * that case it assumes src is quoted correctly, and just copies src to dest.
 * @param dest Location of quoted copy.  Must be large enough to hold the copy
 *             and trailing null byte.
 * @param src  Original string.
 * @param slen Length of original string.
 * @param dest Destination string.
 * @return length of quoted copy in dest.
 */
APREQ_DECLARE(apr_size_t) apreq_quote_once(char *dest, const char *src, 
                                           const apr_size_t slen);

/**
 * Url-encodes a string.
 * @param dest Location of url-encoded result string. Caller must ensure it
 *             is large enough to hold the encoded string and trailing '\0'.
 * @param src  Original string.
 * @param slen Length of original string.
 * @return length of url-encoded string in dest.
 */
APREQ_DECLARE(apr_size_t) apreq_encode(char *dest, const char *src, 
                                       const apr_size_t slen);

/**
 * Url-decodes a string.
 * @param dest Location of url-encoded result string. Caller must ensure dest is
 *             large enough to hold the encoded string and trailing null character.
 * @param dlen points to resultant length of url-decoded string in dest
 * @param src  Original string.
 * @param slen Length of original string.
 * @return APR_SUCCESS, error otherwise.
 */

APREQ_DECLARE(apr_status_t) apreq_decode(char *dest, apr_size_t *dlen,
                                         const char *src, apr_size_t slen);


/**
 * Url-decodes an iovec array.
 * @param dest Location of url-encoded result string. Caller must ensure dest is
 *             large enough to hold the encoded string and trailing null character.
 * @param dlen  Resultant length of dest.
 * @param v Array of iovecs that represent the source string
 * @param nelts Number of iovecs in the array.
 * @return APR_SUCCESS on success, APR_INCOMPLETE if the iovec ends in the
 * middle of an %XX escape sequence, error otherwise.
 */

APREQ_DECLARE(apr_status_t) apreq_decodev(char *d, apr_size_t *dlen,
                                          struct iovec *v, int nelts);

/**
 * Returns an url-encoded copy of a string.
 * @param p    Pool used to allocate the return value.
 * @param src  Original string.
 * @param slen Length of original string.
 * @remark  Use this function insead of apreq_encode if its caller might otherwise
 *          overflow dest.
 */

static APR_INLINE
char *apreq_escape(apr_pool_t *p,
                   const char *src, const apr_size_t slen)
{
    char *rv;

    if (src == NULL)
        return NULL;

    rv = apr_palloc(p, 3 * slen + 1);
    apreq_encode(rv, src, slen);
    return rv;
}

/**
 * An \e in-situ url-decoder.
 * @param str  The string to decode
 * @return  Length of decoded string, or < 0 on error.
 * @remark Equivalent to apreq_decode(str,str,strlen(str)).
 */

static APR_INLINE
apr_ssize_t apreq_unescape(char *str)
{
    apr_size_t len;
    apr_status_t rv = apreq_decode(str,&len,str,strlen(str));
    if (rv == APR_SUCCESS)
        return (apr_ssize_t)len;
    else
        return -1;
}


/** @enum apreq_expires_t Expiration date format */
typedef enum {
    APREQ_EXPIRES_HTTP,       /**< Use date formatting consistent with RFC 2616 */
    APREQ_EXPIRES_NSCOOKIE    /**< Use format consistent with Netscape's Cookie Spec */
} apreq_expires_t;

/**
 * Returns an RFC-822 formatted time string. Similar to ap_gm_timestr_822.
 *
 * @param p       Pool to allocate return string.
 * @param time_str  YMDhms time units (from now) until expiry.
 *                  Understands "now".
 * @param  type     ::APREQ_EXPIRES_HTTP for RFC822 dates,
 *                  ::APREQ_EXPIRES_NSCOOKIE for Netscape cookie dates.
 * @return      Date string, (time_str is offset from "now") formatted
 *              according to type.
 * @deprecated  Use apr_rfc822_date instead.  ::APREQ_EXPIRES_NSCOOKIE strings
 *              are formatted with a '-' (instead of a ' ') character at
 *              offsets 7 and 11.
 */

APREQ_DECLARE(char *) apreq_expires(apr_pool_t *p, const char *time_str, 
                                    const apreq_expires_t type);

/**
 * Converts file sizes (KMG) to bytes
 * @param s  file size matching m/^\d+[KMG]b?$/i
 * @return 64-bit integer representation of s.
 */

APREQ_DECLARE(apr_int64_t) apreq_atoi64f(const char *s);

/**
 * Converts time strings (YMDhms) to seconds
 * @param s time string matching m/^\+?\d+[YMDhms]$/
 * @return 64-bit integer representation of s as seconds.
 */

APREQ_DECLARE(apr_int64_t) apreq_atoi64t(const char *s);

/**
 * Writes brigade to a file.
 * @param f       File that gets the brigade.
 * @param wlen    On a successful return, wlen holds the length of
 *                the brigade, which is the amount of data written to 
 *                the file.
 * @param bb      Bucket brigade.
 * @remark        In the future, this function may do something 
 *                intelligent with file buckets.
 */

APREQ_DECLARE(apr_status_t) apreq_brigade_fwrite(apr_file_t *f,
                                                 apr_off_t *wlen,
                                                 apr_bucket_brigade *bb);
/**
 * Makes a temporary file.
 * @param fp    Points to the temporary apr_file_t on success.
 * @param pool  Pool to associate with the temp file.  When the
 *              pool is destroyed, the temp file will be closed
 *              and deleted.
 * @param path  The base directory which will contain the temp file.
 *              If param == NULL, the directory will be selected via
 *              tempnam().  See the tempnam manpage for details.
 * @return APR_SUCCESS on success; error code otherwise.
 */

APREQ_DECLARE(apr_status_t) apreq_file_mktemp(apr_file_t **fp, 
                                              apr_pool_t *pool,
                                              const char *path);

/**
 * Set aside all buckets in the brigade.
 * @param bb Brigade.
 * @param p  Setaside buckets into this pool.
 */

static APR_INLINE void
apreq_brigade_setaside(apr_bucket_brigade *bb, apr_pool_t *p)
{
    apr_bucket *e;
    for (e = APR_BRIGADE_FIRST(bb); e != APR_BRIGADE_SENTINEL(bb);
         e = APR_BUCKET_NEXT(e))
    {
        apr_bucket_setaside(e, p);
    }
}


/**
 * Copy a brigade.
 * @param d (destination) Copied buckets are appended to this brigade.
 * @param s (source) Brigade to copy from.
 * @remark s == d produces Undefined Behavior.
 */

static APR_INLINE
void apreq_brigade_copy(apr_bucket_brigade *d, apr_bucket_brigade *s) {
    apr_bucket *e;
    for (e = APR_BRIGADE_FIRST(s); e != APR_BRIGADE_SENTINEL(s);
         e = APR_BUCKET_NEXT(e))
    {
        apr_bucket *c;
        apr_bucket_copy(e, &c);
        APR_BRIGADE_INSERT_TAIL(d, c);
    }
}


/**
 * Search a header string for the value of a particular named attribute.
 * @param hdr Header string to scan.
 * @param name Name of attribute to search for.
 * @param nlen Length of name.
 * @param val Location of (first) matching value.
 * @param vlen Length of matching value.
 * @return APR_SUCCESS if found, otherwise APR_NOTFOUND.
 */
APREQ_DECLARE(apr_status_t)
         apreq_header_attribute(const char *hdr,
                                const char *name, const apr_size_t nlen,
                                const char **val, apr_size_t *vlen);


/**
 * Concatenates the brigades, spooling large brigades into
 * a tempfile bucket according to the environment's max_brigade
 * setting- see apreq_env_max_brigade().
 * @param pool           Pool for creating a tempfile bucket.
 * @param temp_dir       Directory for tempfile creation.
 * @param brigade_limit  If out's length would exceed this value, 
 *                       the appended buckets get written to a tempfile.  
 * @param out            Resulting brigade.
 * @param in             Brigade to append.
 * @return APR_SUCCESS on success, error code otherwise.
 */
APREQ_DECLARE(apr_status_t) apreq_brigade_concat(apr_pool_t *pool,
                                                 const char *temp_dir,
                                                 apr_size_t brigade_limit,
                                                 apr_bucket_brigade *out, 
                                                 apr_bucket_brigade *in);


/**
 * Initialize libapreq2. Applications (except apache modules using
 * mod_apreq) have to call this exactly once before they use
 * libapreq2.
 *
 * @param pool a base pool persisting while libapreq2 is used
 * @remark after you detroyed the pool, you have to call this function again
 *    with a new pool if you still plan to use libapreq2
 */
APREQ_DECLARE(apr_status_t) apreq_initialize(apr_pool_t *pool);


#ifdef __cplusplus
 }
#endif

#endif /* APREQ_UTIL_H */