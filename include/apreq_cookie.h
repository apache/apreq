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

#ifndef APREQ_COOKIE_H
#define APREQ_COOKIE_H

#include "apreq_util.h"
#include "apr_tables.h"

#ifdef  __cplusplus
extern "C" {
#endif 

/**
 * @file apreq_cookie.h
 * @brief Cookies and Jars.
 * @ingroup libapreq2
 *
 * apreq_cookie.h describes a common server-side API for request (incoming)
 * and response (outgoing) cookies.  It aims towards compliance with the 
 * standard cookie specifications listed below.
 *
 * @see http://wp.netscape.com/newsref/std/cookie_spec.html
 * @see http://www.ietf.org/rfc/rfc2109.txt
 * @see http://www.ietf.org/rfc/rfc2964.txt
 * @see http://www.ietf.org/rfc/rfc2965.txt
 *
 */


/**
 * Cookie Version.  libapreq does not distinguish between
 * rfc2109 and its successor rfc2965; both are referred to
 * as APREQ_COOKIE_VERSION_RFC.  Users can distinguish between
 * them in their outgoing cookies by using apreq_cookie_bake()
 * for sending rfc2109 cookies, or apreq_cookie_bake2() for rfc2965.
 * The original Netscape cookie spec is still preferred for its
 * greater portability, it is named APREQ_COOKIE_VERSION_NETSCAPE.
 *
 */
typedef enum { APREQ_COOKIE_VERSION_NETSCAPE, 
               APREQ_COOKIE_VERSION_RFC } apreq_cookie_version_t;


/** Default version, used when creating a new cookie. See apreq_cookie_make().*/
#define APREQ_COOKIE_VERSION_DEFAULT       APREQ_COOKIE_VERSION_NETSCAPE

/** Maximum length of a single Set-Cookie(2) header */
#define APREQ_COOKIE_MAX_LENGTH            4096

/** @brief Cookie type, supporting both Netscape and RFC cookie specifications.
 */
typedef struct apreq_cookie_t {

    apreq_cookie_version_t version; /**< RFC or Netscape compliant cookie */

    char           *path;        /**< Restricts url path */
    char           *domain;      /**< Restricts server domain */
    char           *port;        /**< Restricts server port */
    unsigned        secure;      /**< Notify browser of "https" requirement */
    char           *comment;     /**< RFC cookies may send a comment */
    char           *commentURL;  /**< RFC cookies may place an URL here */
    apr_time_t      max_age;     /**< total duration of cookie: -1 == session */
    unsigned char   flags;       /**< charsets, taint marks, app-specific bits */
    const apreq_value_t   v;     /**< "raw" cookie value */

} apreq_cookie_t;

/** Upgrades cookie jar table values to apreq_cookie_t structs. */
static APR_INLINE
apreq_cookie_t *apreq_value_to_cookie(const char *val)
{
    union { const char *in; char *out; } deconst;

    deconst.in = val;
    return apreq_attr_to_type(apreq_cookie_t, v, 
           apreq_attr_to_type(apreq_value_t, data, deconst.out));
}

static APR_INLINE
const char *apreq_cookie_name(const apreq_cookie_t *c) {
    return c->v.name;
}

static APR_INLINE
const char *apreq_cookie_value(const apreq_cookie_t *c) {
    return c->v.data;
}

APREQ_DECLARE(apr_status_t)apreq_parse_cookie_header(apr_pool_t *pool,
                                                     apr_table_t *jar,
                                                     const char *header);

/**
 * Returns a new cookie, made from the argument list.
 *
 * @param pool  Pool which allocates the cookie.
 * @param name  The cookie's name.
 * @param nlen  Length of name.
 * @param value The cookie's value.
 * @param vlen  Length of value.
 */
APREQ_DECLARE(apreq_cookie_t *) apreq_cookie_make(apr_pool_t *pool, 
                                  const char *name, const apr_size_t nlen, 
                                  const char *value, const apr_size_t vlen);

#define apreq_make_cookie(p,n,nl,v,vl) apreq_cookie_make(p,n,nl,v,vl)

/**
 * Sets the associated cookie attribute.
 * @param p    Pool for allocating the new attribute.
 * @param c    Cookie.
 * @param attr Name of attribute- leading '-' or '$' characters
 *             are ignored.
 * @param alen Length of attr.
 * @param val  Value of new attribute.
 * @param vlen Length of new attribute.
 * @remarks    Ensures cookie version & time are kept in sync.
 */
APREQ_DECLARE(apr_status_t) 
    apreq_cookie_attr(apr_pool_t *p, apreq_cookie_t *c, 
                      const char *attr, apr_size_t alen,
                      const char *val, apr_size_t vlen);


/**
 * Returns a string that represents the cookie as it would appear 
 * in a valid "Set-Cookie*" header.
 *
 * @param c The cookie.
 * @param p The pool.
 */
APREQ_DECLARE(char*) apreq_cookie_as_string(const apreq_cookie_t *c,
                                            apr_pool_t *p);


/**
 * Same functionality as apreq_cookie_as_string.  Stores the string
 * representation in buf, using up to len bytes in buf as storage.
 * The return value has the same semantics as that of apr_snprintf,
 * including the special behavior for a "len = 0" argument.
 *
 * @param c The cookie.
 * @param buf Storage location for the result.
 * @param len Size of buf's storage area. 
 */

APREQ_DECLARE(int) apreq_cookie_serialize(const apreq_cookie_t *c,
                                          char *buf, apr_size_t len);

#define apreq_serialize_cookie(buf,len,c) apreq_cookie_serialize(c,buf,len)

/**
 * Set the Cookie's expiration date.
 * 
 * @param c The cookie.
 * @param time_str If NULL, the Cookie's expiration date is unset,
 * making it a session cookie.  This means no "expires" or "max-age" 
 * attribute will appear in the cookie's serialized form. If time_str
 * is not NULL, the expiration date will be reset to the offset (from now)
 * represented by time_str.  The time_str should be in a format that 
 * apreq_atoi64t() can understand, namely /[+-]?\d+\s*[YMDhms]/.
 */
APREQ_DECLARE(void) apreq_cookie_expires(apreq_cookie_t *c, 
                                         const char *time_str);

#ifdef __cplusplus
 }
#endif

#endif /*APREQ_COOKIE_H*/

