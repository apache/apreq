#ifndef APREQ_COOKIE_H
#define APREQ_COOKIE_H

#include "apreq_tables.h"

#ifdef  __cplusplus
extern "C" {
#endif 

typedef struct apreq_table_t apreq_jar_t;

typedef enum { NETSCAPE, RFC } apreq_cookie_version_t;

#define APREQ_COOKIE_DEFAULT_VERSION       NETSCAPE

typedef struct apreq_cookie_t {

    apreq_cookie_version_t version;

    char           *path;
    char           *domain;
    char           *port;
    unsigned        secure;

    char           *comment;
    char           *commentURL;

    union {
        long        max_age; 
        const char *expires; 
    } time;

    void           *ctx;

    apreq_value_t   v;           /* "raw" value (extended struct) */

} apreq_cookie_t;


#define apreq_value_to_cookie(ptr) apreq_attr_to_type(apreq_cookie_t, \
                                                      v, ptr)
#define apreq_cookie_name(c)  ((c)->v.name)
#define apreq_cookie_value(c) ((c)->v.data)

/**
 * Returns the number of cookies in the jar.
 *
 * @param jar The cookie jar.
 * @remark    Shouldn't this be called apreq_jar_nelts?
 */

int apreq_jar_items(apreq_jar_t *jar);
#define apreq_jar_items(j) apreq_table_nelts(j)

/**
 * Fetches a cookie from the jar
 *
 * @param jar   The cookie jar.
 * @param name  The name of the desired cookie.
 */

apreq_cookie_t *apreq_jar_get(apreq_jar_t *jar, char *name);
#define apreq_jar_get(j,k) apreq_value_to_cookie(apreq_char_to_value( \
                              apreq_table_get(j,k)))

/**
 * Adds a cookie by pushing it to the bottom of the jar.
 *
 * @param jar The cookie jar.
 * @param c The cookie to add.
 */

void apreq_jar_add(apreq_jar_t *jar, apreq_cookie_t *c);
#define apreq_jar_add(jar,c)  apreq_table_add(jar, &(c)->v)

/**
 * Parse the incoming "Cookie:" headers into a cookie jar.
 * 
 * @param ctx The current context.
 * @param data  String to parse as a HTTP-merged "Cookie" header.
 * @remark "data = NULL" has special semantics.  In this case,
 * apreq_jar_parse will attempt to fetch a cached jar from the
 * environment ctx via apreq_env_jar.  Failing that, it replace
 * data with the result of apreq_env_cookie(ctx), parse that,
 * and store the resulting jar back within the environment.
 * This Orcish maneuver is designed to mimimize parsing work,
 * since generating the cookie jar is fairly expensive.
 *
 */


apreq_jar_t *apreq_jar_parse(void *ctx, const char *data);

/**
 * Returns a new cookie, made from the argument list.
 * The cookie is allocated from the ctx pool.
 *
 * @param ctx   The current context.
 * @param v     The cookie version, currently one of  ::NETSCAPE or ::RFC.
 * @param name  The cookie's name.
 * @param nlen  Length of name.
 * @param value The cookie's value.
 * @param vlen  Length of value.
 */
apreq_cookie_t *apreq_cookie_make(void *ctx, const apreq_cookie_version_t v,
                                  const char *name, const apr_ssize_t nlen, 
                                  const char *value, const apr_ssize_t vlen);


APREQ_DECLARE(apr_status_t) apreq_cookie_attr(apreq_cookie_t *c, 
                                              char *attr, char *val);

/**
 * Returns a string that represents the cookie as it would appear 
 * in a valid "Set-Cookie*" header.
 *
 * @param c The cookie.
 * @param p The pool.
 */
APREQ_DECLARE(const char*) apreq_cookie_as_string(const apreq_cookie_t *c,
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

/**
 * Get/set the "expires" string.  For NETSCAPE cookies, this returns 
 * the date (properly formatted) when the cookie is to expire.
 * For RFC cookies, this function returns NULL.
 * 
 * @param c The cookie.
 * @param time_str If NULL, return the current expiry date. Otherwise
 * replace with this value instead.  The time_str should be in a format
 * that apreq_atod() can understand, namely /[+-]?\d+\s*[YMDhms]/.
 */
APREQ_DECLARE(void) apreq_cookie_expires(apreq_cookie_t *c, 
                                         const char *time_str);

/**
 * Add the cookie to the outgoing "Set-Cookie" headers.
 *
 * @param c The cookie.
 */
apr_status_t apreq_cookie_bake(apreq_cookie_t *c);

/* XXX: how about baking whole cookie jars, too ??? */

/**
 * Add the cookie to the outgoing "Set-Cookie2" headers.
 *
 * @param c The cookie.
 */
apr_status_t apreq_cookie_bake2(apreq_cookie_t *c);

#ifdef __cplusplus
 }
#endif

#endif /*APREQ_COOKIE_H*/


