/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2003 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#ifndef APREQ_H
#define APREQ_H

#include "apr_tables.h" 
#include "apr_file_io.h"
#include "apr_buckets.h"
#include <stddef.h>

#ifndef apr_table_nelts
#define apr_table_nelts(t) apr_table_elts(t)->nelts
#endif

#ifdef  __cplusplus
 extern "C" {
#endif 

/** @defgroup LIBRARY libapreq2     */
/** @defgroup MODULES Environments  */
/** @defgroup GLUE Language Bindings*/

/**
 * @mainpage
 * Project Website: http://httpd.apache.org/apreq/
 * @verbinclude README
 */

/** 
 * @page LICENSE 
 * @verbinclude LICENSE
 */
/** 
 * @page INSTALL 
 * @verbinclude INSTALL
 */
/**
 * @defgroup XS Perl
 * @ingroup GLUE
 */
/**
 * @defgroup TCL TCL
 * @ingroup GLUE
 */
/**
 * @defgroup PYTHON Python
 * @ingroup GLUE
 */
/**
 * @defgroup PHP PHP
 * @ingroup GLUE
 */
/**
 * @defgroup RUBY Ruby
 * @ingroup GLUE
 */
/** 
 * @defgroup XS_Request Apache::Request
 * @ingroup XS
 * @htmlinclude Request.html
 */
/** 
 * @defgroup XS_Cookie Apache::Cookie
 * @ingroup XS
 * @htmlinclude Cookie.html
 */

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
 */
/**
 * @defgroup Utils Common functions, structures and macros
 * @ingroup LIBRARY
 * @{
 */

#ifndef WIN32
#define APREQ_DECLARE(d)                APR_DECLARE(d)
#define APREQ_DECLARE_NONSTD(d)         APR_DECLARE_NONSTD(d)
#else
#define APREQ_DECLARE(type)             __declspec(dllexport) type __stdcall
#define APREQ_DECLARE_NONSTD(type)      __declspec(dllexport) type
#define APREQ_DECLARE_DATA              __declspec(dllexport)
#endif

#define APREQ_URL_ENCTYPE               "application/x-www-form-urlencoded"
#define APREQ_MFD_ENCTYPE               "multipart/form-data"
#define APREQ_XML_ENCTYPE               "application/xml"

#define APREQ_NELTS                     8

/**
 * libapreq-2's pre-extensible string type 
 */
typedef struct apreq_value_t {
    const char    *name;    /**< value's name */
    apr_status_t   status;  /**< APR status, usually SUCCESS or INCOMPLETE*/
    apr_size_t     size;    /**< Size of data.*/
    char           data[1]; /**< Actual data bytes.*/
} apreq_value_t;

typedef apreq_value_t *(apreq_value_merge_t)(apr_pool_t *p,
                                             const apr_array_header_t *a);
typedef apreq_value_t *(apreq_value_copy_t)(apr_pool_t *p,
                                            const apreq_value_t *v);


#define apreq_attr_to_type(T,A,P) ( (T*) ((char*)(P)-offsetof(T,A)) )
#define apreq_char_to_value(ptr)  apreq_attr_to_type(apreq_value_t, data, ptr)

#define apreq_strtoval(ptr)  apreq_char_to_value(ptr)
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
apreq_value_t * apreq_copy_value(apr_pool_t *p, 
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
    AS_IS,      /**< Join the strings without modification */
    ENCODE,     /**< Url-encode the strings before joining them */
    DECODE,     /**< Url-decode the strings before joining them */
    QUOTE       /**< Quote the strings, backslashing existing quote marks. */
} apreq_join_t;

/**
 * Join an array of values.
 * @param p   Pool to allocate return value.
 * @param sep String that is inserted between the joined values.
 * @param arr Array of values.
 * @remark    Return string can be upgraded to an apreq_value_t 
 *            with apreq_stroval.
 */
APREQ_DECLARE(const char *) apreq_join(apr_pool_t *p, 
                                       const char *sep, 
                                       const apr_array_header_t *arr, 
                                       apreq_join_t mode);


/** @enum apreq_match_t Match type */
typedef enum {
    FULL,       /**< Full match only. */
    PARTIAL     /**< Partial matches are ok. */
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
 *
 */
APREQ_DECLARE(apr_ssize_t) apreq_index(const char* hay, apr_size_t hlen, 
                        const char* ndl, apr_size_t nlen, 
                        const apreq_match_t type);
/**
 * Places a quoted copy of src into dest.
 * @param dest Location of quoted copy.  Must be large enough to hold the copy.
 * @param src  Original string.
 * @param slen Length of original string.
 * @return length of quoted copy in dest.
 */

APREQ_DECLARE(apr_size_t) apreq_quote(char *dest, const char *src, const apr_size_t slen);

/**
 * Url-encodes a string.
 * @param dest Location of url-encoded result string. Caller must ensure dest is
 *             large enough.
 * @param src  Original string.
 * @param slen Length of original string.
 * @return length of url-encoded string in dest.
 */

APREQ_DECLARE(apr_size_t) apreq_encode(char *dest, const char *src, const apr_size_t slen);

/**
 * Url-decodes a string.
 * @param dest Location of url-decoded result string. Caller must ensure dest is
 *             large enough.
 * @param src  Original string.
 * @param slen Length of original string.
 * @return Length of url-decoded string in dest, or < 0 on 
 *         decoding (bad data) error.
 */

APREQ_DECLARE(apr_ssize_t) apreq_decode(char *dest, const char *src, const apr_size_t slen);

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
 */

APREQ_DECLARE(apr_ssize_t) apreq_unescape(char *str);
#define apreq_unescape(str) apreq_decode(str,str,strlen(str))

/** @enum apreq_expires_t Expiration date format */
typedef enum {
    HTTP,       /**< Use date formatting consistent with RFC 2616 */
    NSCOOKIE    /**< Use format consistent with Netscape's Cookie Spec */
} apreq_expires_t;

/**
 * Returns an RFC-822 formatted time string. Similar to ap_gm_timestr_822.
 *
 * @param req       The current apreq_request_t object.
 * @param time_str  YMDhms time units (from now) until expiry.
 *                  Understands "now".
 * @param  type     ::HTTP for RFC822 dates, ::NSCOOKIE for cookie dates.
 * @return      Date string, (time_str is offset from "now") formatted
 *              either as an ::NSCOOKIE or ::HTTP date
 * @deprecated  Use apr_rfc822_date instead.  ::NSCOOKIE strings are formatted
 *              with a '-' (instead of a ' ') character at offsets 7 and 11.
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


APREQ_DECLARE(apr_bucket_brigade *)
         apreq_copy_brigade(const apr_bucket_brigade *bb);


/** @} */

#ifdef __cplusplus
 }
#endif

#endif /* APREQ_H */
