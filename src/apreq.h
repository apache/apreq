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

#ifndef APREQ_H
#define APREQ_H

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

#define APREQ_URL_ENCTYPE               "application/x-www-form-urlencoded"
#define APREQ_MFD_ENCTYPE               "multipart/form-data"
#define APREQ_XML_ENCTYPE               "application/xml"

#define APREQ_NELTS                     8
#define APREQ_READ_AHEAD                (64 * 1024)
/**
 * Maximum amount of heap space a brigade may use before switching to file
 * buckets
*/
#define APREQ_MAX_BRIGADE_LEN           (256 * 1024) 
                     

/** @brief libapreq's pre-extensible string type */
typedef struct apreq_value_t {
    const char    *name;    /**< value's name */
    apr_size_t     size;    /**< Size of data.*/
    unsigned char  flags;   /**< reserved (for future charset support) */
    char           data[1]; /**< Actual data bytes.*/
} apreq_value_t;

typedef apreq_value_t *(apreq_value_merge_t)(apr_pool_t *p,
                                             const apr_array_header_t *a);
typedef apreq_value_t *(apreq_value_copy_t)(apr_pool_t *p,
                                            const apreq_value_t *v);


#define apreq_attr_to_type(T,A,P) ( (T*) ((char*)(P)-offsetof(T,A)) )

/**
 * Converts (char *) to (apreq_value_t *).  The char * is assumed
 * to point at the data attribute of an apreq_value_t struct.
 *
 * @param ptr   points at the data field of an apreq_value_t struct.
 */

#define apreq_char_to_value(ptr)  apreq_attr_to_type(apreq_value_t, data, ptr)

/** convert a const pointer into a non-const. WARNING: this is
    dangerous. Use only if you really know what you're doing. Only for
    Dirty Hacks (TM) */
static APR_INLINE void *apreq_deconst(const void *p) {
    /* go around the gcc warning */
    /* FIXME: does this work on all platforms? */
    long v = (long)p;
    return (void*)v;
}

static APR_INLINE apreq_value_t *apreq_strtoval(const char *name) {
    return (apreq_value_t*)apreq_char_to_value(apreq_deconst(name));
}

/**
 * Computes the length of the string, but unlike strlen(),
 * it permits embedded null characters.
 *
 * @param ptr  points at the data field of an apreq_value_t struct.
 * 
 */

#define apreq_strlen(ptr) (apreq_strtoval(ptr)->size)

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

/**
 * Makes a pool-allocated copy of the value.
 * @param p  Pool.
 * @param val Original value to copy.
 */
APREQ_DECLARE(apreq_value_t *) apreq_copy_value(apr_pool_t *p, 
                                                const apreq_value_t *val);

/**
 * Merges an array of values into one.
 * @param p   Pool from which the new value is generated.
 * @param arr Array of apr_value_t *.
 */
apreq_value_t * apreq_merge_values(apr_pool_t *p, 
                                   const apr_array_header_t *arr);

/**
 * Fetches the enctype from the environment.
 * @param env Environment.
 */
APREQ_DECLARE(const char *)apreq_enctype(void *env);

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
 * @remark     Return string can be upgraded to an apreq_value_t 
 *             with apreq_stroval.
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
 * Return a pointer to the match string, or NULL if no match is found.
 * @param hay  Location of bytes to scan.
 * @param hlen Number of bytes available for scanning.
 * @param ndl  Search string
 * @param nlen Length of search string.
 * @param type Match type.
 *
 */
APREQ_DECLARE(char *) apreq_memmem(char* hay, apr_size_t hlen, 
                                   const char* ndl, apr_size_t nlen, 
                                   const apreq_match_t type);

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
 * @param src  Original string.
 * @param slen Length of original string.
 * @return Length of url-decoded string in dest, or < 0 on decoding (bad data) error.
 */

APREQ_DECLARE(apr_ssize_t) apreq_decode(char *dest, const char *src, apr_size_t slen);


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

APREQ_DECLARE(char *) apreq_escape(apr_pool_t *p, 
                                   const char *src, const apr_size_t slen);

/**
 * An \e in-situ url-decoder.
 * @param str  The string to decode
 * @return  Length of decoded string, or < 0 on error.
 * @remark Equivalent to apreq_decode(str,str,strlen(str)).
 */

APREQ_DECLARE(apr_ssize_t) apreq_unescape(char *str);


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
 * Gets the spoolfile associated to a brigade, if any.
 * @param bb Brigade, usually associated to a file upload (apreq_param_t).
 * @return If the last bucket in the brigade is a file bucket,
 *         this function will return its associated file.  Otherwise,
 *         this function returns NULL.
 */

APREQ_DECLARE(apr_file_t *) apreq_brigade_spoolfile(apr_bucket_brigade *bb);

/**
 * Set aside all buckets in the brigade.
 * @param bb Brigade.
 * @param p  Setaside buckets into this pool.
 */

static APR_INLINE void APREQ_BRIGADE_SETASIDE(apr_bucket_brigade *bb, apr_pool_t *p) {
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

#define APREQ_BRIGADE_COPY(d,s) do {                                \
    apr_bucket *e;                                                  \
    for (e = APR_BRIGADE_FIRST(s); e != APR_BRIGADE_SENTINEL(s);    \
         e = APR_BUCKET_NEXT(e))                                    \
    {                                                               \
        apr_bucket *c;                                              \
        apr_bucket_copy(e, &c);                                     \
        APR_BRIGADE_INSERT_TAIL(d, c);                              \
    }                                                               \
} while (0)


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


#ifdef __cplusplus
 }
#endif

#endif /* APREQ_H */
