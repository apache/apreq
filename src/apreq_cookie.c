/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
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
 *
 * Portions of this software are based upon public domain software
 * originally written at the National Center for Supercomputing Applications,
 * University of Illinois, Urbana-Champaign.
 */

#include "apreq_cookie.h"
#include "apreq_env.h"
#include "apr_strings.h"
#include "apr_lib.h"


APREQ_DECLARE(int) (apreq_jar_items)(apreq_jar_t *jar)
{
    return apreq_jar_items(jar);
}

APREQ_DECLARE(apreq_cookie_t *) (apreq_cookie)(const apreq_jar_t *jar, 
                                               const char *name)
{
    return apreq_cookie(jar,name);
}

APREQ_DECLARE(apr_status_t) (apreq_add_cookie)(apreq_jar_t *jar, 
                                       const apreq_cookie_t *c)
{
    return apreq_add_cookie(jar,c);
}

APREQ_DECLARE(void) apreq_cookie_expires(apr_pool_t *p,
                                         apreq_cookie_t *c, 
                                         const char *time_str)
{
    if ( c->version == NETSCAPE )
        c->time.expires = apreq_expires(p, time_str, NSCOOKIE);
    else
        c->time.max_age = apreq_atol(time_str);
}

static int has_rfc_cookie(void *ctx, const char *key, const char *val)
{
    const apreq_cookie_t *c = apreq_value_to_cookie(
                      apreq_char_to_value(val));

    return c->version == NETSCAPE; /* 0 -> non-netscape cookie found, stop.
                                      1 -> not found, keep going. */
}

APREQ_DECLARE(apreq_cookie_version_t) apreq_ua_cookie_version(void *ctx)
{
    if (apreq_env_cookie2(ctx) == NULL) {
        apreq_jar_t *j = apreq_jar(ctx, NULL);

        if (j == NULL || apreq_jar_nelts(j) == 0) 
            return APREQ_COOKIE_VERSION;

        else if (apreq_table_do(has_rfc_cookie, NULL, j->cookies) == 1)
            return NETSCAPE;

        else
            return RFC;
    }
    else
        return RFC;
}

static apr_status_t apreq_cookie_attr(apr_pool_t *p,
                                      apreq_cookie_t *c, 
                                      char *attr,
                                      char *val)
{
    dAPREQ_LOG;

    if ( attr[0] ==  '-' || attr[0] == '$' )
        ++attr;

    switch (apr_tolower(*attr)) {

    case 'n': /* name */
        c->v.name = val;
        return APR_SUCCESS;

    case 'v': /* version */
        while (*val && !apr_isdigit(*val))
            ++val;
        if (val) {
            c->version = *val - '0';
            return APR_SUCCESS;
        }
        else {
            return APR_BADARG;
        }

    case 'e': case 'm': /* expires, max-age */
        apreq_cookie_expires(p, c, val);
        return APR_SUCCESS;

    case 'd':
        c->domain = val;
        return APR_SUCCESS;

    case 'p':
        if (strcasecmp("port", attr)==0) {
            c->port = val;
            return APR_SUCCESS;
        }
        else if (strcasecmp("path", attr)==0) {
            c->path = val;
            return APR_SUCCESS;
        }
        break;

    case 'c':
        if (strcasecmp("comment", attr)==0) {
            c->comment = val;
            return APR_SUCCESS;
        } 
        else if (strcasecmp("commentURL", attr)==0) {
            c->commentURL = val;
            return APR_SUCCESS;
        }
        break;

    case 's':
        c->secure = ( strcasecmp(val,"off")!=0 && *val != '0' );
        return APR_SUCCESS;

    };

    return APR_ENOTIMPL;
}

APREQ_DECLARE(apreq_cookie_t *) apreq_make_cookie(apr_pool_t *p, 
                                  const apreq_cookie_version_t version,
                                  const char *name, const apr_size_t nlen,
                                  const char *value, const apr_size_t vlen)
{
    apreq_cookie_t *c = apr_palloc(p, vlen + sizeof *c);
    apreq_value_t *v = &c->v;

    v->size = vlen;
    v->name = apr_pstrmemdup(p, name, nlen);
    memcpy(v->data, value, vlen);
    v->data[vlen] = 0;
    
    c->version = version;

    /* session cookie is the default */

    if (c->version == NETSCAPE)
        c->time.expires = NULL;
    else
        c->time.max_age = -1;

    c->path = NULL;
    c->domain = NULL;
    c->port = NULL;
    c->secure = 0; 

    c->comment = NULL;
    c->commentURL = NULL;

    return c;
}

static APR_INLINE apr_status_t get_pair(const char **data,
                                        const char **n, apr_size_t *nlen,
                                        const char **v, apr_size_t *vlen)
{
    const char *d = *data;
    unsigned char in_quotes = 0;

    *n = d;

    while( *d != '=' && !apr_isspace(*d) )
        if (*d++ == 0)  {
            /*error: no '=' sign detected */
            *data = d-1;
            return APR_NOTFOUND;
        }
    *nlen = d - *n;

    do ++d; while ( *d == '=' || apr_isspace(*d) );

    *v = d;

    for (;;++d) {
        switch (*d) {

        case ';': case ',':
            if (in_quotes)
                break;
            /* else fall through */
        case 0:
            goto pair_result;

        case '\\':
            if (*++d) {
                break;
            }
            else {  /* shouldn't end on a sour note */
                *vlen = d - *v;
                *data = d;
                return APR_BADCH;
            }

        case '"':
            in_quotes = ! in_quotes;

        }
    }

 pair_result:

    *vlen = d - *v;
    *data = d;

    if (in_quotes)
        return APR_BADARG;

    return APR_SUCCESS;
}

APREQ_DECLARE(apreq_jar_t *) apreq_jar(void *ctx, 
                                       const char *data)
{
    apr_pool_t *p = apreq_env_pool(ctx);

    apreq_cookie_version_t version;
    apreq_jar_t *j = NULL;
    apreq_cookie_t *c;

    const char *origin;
    const char *name, *value; 
    apr_size_t nlen, vlen;

    dAPREQ_LOG;

    /* initialize jar */
    
    if (data == NULL) {
        /* use the environment's cookie data */

        /* fetch ctx->jar (cached jar) */
        j = apreq_env_jar(ctx, NULL);
        if ( j != NULL )
            return j;

        j = apr_palloc(p, sizeof *j);
        j->pool = p;
        j->env = ctx;
        j->cookies = apreq_table_make(p, APREQ_NELTS);

        data = apreq_env_cookie(ctx);

        /* XXX: potential race condition here 
           between env_jar fetch and env_jar set.  */

        apreq_env_jar(ctx,j);      /* set (cache) ctx->jar */

        if (data == NULL)
            return j;
    }
    else {
        j = apr_palloc(p, sizeof *j);
        j->pool = p;
        j->env = ctx;
        j->cookies = apreq_table_make(p, APREQ_NELTS);
    }

    origin = data;

    apreq_log(APREQ_DEBUG 0, ctx, "parsing cookie data: %s", data);

    /* parse data */

 parse_cookie_header:

    c = NULL;
    version = NETSCAPE;

    while (apr_isspace(*data))
        ++data;

    /* XXX cheat: assume "$..." => "$Version" => RFC Cookie header */

    if (*data == '$') { 
        version = RFC;
        while (*data && !apr_isspace(*data))
            ++data;
    }

    for (;;) {
        apr_status_t status;

        while (*data == ';' || apr_isspace(*data))
            ++data;

        switch (*data) {

        case 0:
            /* this is the normal exit point for apreq_jar */
            return j;

        case ',':
            ++data;
            goto parse_cookie_header;

        case '$':
            if (c == NULL) {
                apreq_log(APREQ_ERROR APR_BADCH, ctx,
                      "Saw attribute, expecting NAME=VALUE cookie pair: %s",
                          data);
                return j;
            }
            else if (version == NETSCAPE) {
                c->v.status = APR_EMISMATCH;
                apreq_log(APREQ_ERROR c->v.status, ctx, 
                          "Saw attribute in a Netscape Cookie header: %s", 
                          data);
                return j;
            }

            status = get_pair(&data, &name, &nlen, &value, &vlen);

            if (status == APR_SUCCESS)
                apreq_cookie_attr(p, c, apr_pstrmemdup(p, name, nlen),
                                        apr_pstrmemdup(p,value, vlen));    
            else {
                c->v.status = status;
                apreq_log(APREQ_WARN c->v.status, ctx,
                           "Ignoring bad attribute pair: %s", data);
            }
            break;

        default:
            status = get_pair(&data, &name, &nlen, &value, &vlen);

            if (status == APR_SUCCESS) {
                c = apreq_make_cookie(p, version, name, nlen, 
                                      value, vlen);
                apreq_log(APREQ_DEBUG status, ctx, 
                          "adding cookie: %s => %s", c->v.name, c->v.data);
                apreq_add_cookie(j, c);
            }
            else {
                apreq_log(APREQ_WARN status, ctx,
                          "Skipping bad NAME=VALUE pair: %s", data);
            }
        }
    }

    /* NOT REACHED */
    return j;
}


APREQ_DECLARE(int) apreq_serialize_cookie(char *buf, apr_size_t len,
                                          const apreq_cookie_t *c)
{
    /*  The format string must be large enough to accomodate all
     *  of the cookie attributes.  The current attributes sum to 
     *  ~80 characters (w/ 6 padding chars per attr), so anything 
     *  over that should number be fine.
     */

    char format[128] = "%s=%s";
    char *f = format + strlen(format);

    /* XXX protocol enforcement (for debugging, anyway) ??? */

    if (c->v.name == NULL)
        return -1;
    
#define ADD_ATTR(name) do { strcpy(f,c->name ? "; " #name "=%s" : \
                                    "%.0s"); f+= strlen(f); } while (0)
#define NONNULL(attr) (attr ? attr : "")


    if (c->version == NETSCAPE) {
        
        ADD_ATTR(path);
        ADD_ATTR(domain);

        strcpy(f, c->time.expires ? "; expires=%s" : "");
        f += strlen(f);

        if (c->secure)
            strcpy(f, "; secure");

        return apr_snprintf(buf, len, format, c->v.name, c->v.data,
            NONNULL(c->path), NONNULL(c->domain), c->time.expires);
    }

    /* c->version == RFC */

    strcpy(f,"; Version=%d");
    f += strlen(f);

    ADD_ATTR(path);
    ADD_ATTR(domain);
    ADD_ATTR(port);
    ADD_ATTR(comment);
    ADD_ATTR(commentURL);

#undef ADD_ATTR

    strcpy(f, c->time.max_age >= 0 ? "; max-age=%ld" : "");

    f += strlen(f);

    if (c->secure)
        strcpy(f, "; secure");

    return apr_snprintf(buf, len, format, c->v.name, c->v.data,
                        c->version, NONNULL(c->path), NONNULL(c->domain), 
                        NONNULL(c->port), NONNULL(c->comment), 
                        NONNULL(c->commentURL), c->time.max_age);
}


APREQ_DECLARE(char*) apreq_cookie_as_string(apr_pool_t *p,
                                            const apreq_cookie_t *c)

{
    char s[APREQ_COOKIE_LENGTH];
    int n = apreq_serialize_cookie(s, APREQ_COOKIE_LENGTH,c);

    if ( n < APREQ_COOKIE_LENGTH )
        return apr_pstrmemdup(p, s, n);
    else
        return NULL;
}

APREQ_DECLARE(apr_status_t) apreq_bake_cookie(const apreq_cookie_t *c,
                                              void *ctx)
{
    char *s = apreq_cookie_as_string(apreq_env_pool(ctx),c);
    dAPREQ_LOG;

    if (s == NULL) {
        apreq_log(APREQ_ERROR APR_ENAMETOOLONG, ctx, 
                  "Serialized cookie exceeds APREQ_COOKIE_LENGTH = %d", 
                    APREQ_COOKIE_LENGTH);
        return APR_ENAMETOOLONG;
    }

    return apreq_env_set_cookie(ctx, s);
}

APREQ_DECLARE(apr_status_t) apreq_bake2_cookie(const apreq_cookie_t *c,
                                               void *ctx)
{
    char *s = apreq_cookie_as_string(apreq_env_pool(ctx),c);
    dAPREQ_LOG;

    if ( s == NULL ) {
        apreq_log(APREQ_ERROR APR_ENAMETOOLONG, ctx,
                  "Serialized cookie exceeds APREQ_COOKIE_LENGTH = %d", 
                    APREQ_COOKIE_LENGTH);
        return APR_ENAMETOOLONG;
    }
    else if ( c->version == NETSCAPE ) {
        apreq_log(APREQ_ERROR APR_EMISMATCH, ctx,
                  "Cannot bake2 a Netscape cookie: %s", s);
        return APR_EMISMATCH;
    }

    return apreq_env_set_cookie2(ctx, s);
}



/* The functions below belong somewhere else, since they
   generally make use of "common conventions" for cookie values. 
   (whereas the cookie specs regard values as opaque) */

#ifdef NEEDS_A_NEW_HOME

void (apreq_cookie_push)(apreq_cookie_t *c, apreq_value_t *v)
{
    apreq_cookie_push(c,v);
}

void apreq_cookie_addn(apreq_cookie_t *c, char *val, int len)
{
    apreq_value_t *v = (apreq_value_t *)apr_array_push(c->values.data);
    v->data = val;
    v->size = len;
}

void apreq_cookie_add(apreq_cookie_t *c, char *val, int len)
{
    apr_array_header_t *a = (apr_array_header_t *)c->values.data;
    apreq_cookie_addn(c, apr_pstrndup(a->pool, val, len), len);
}

APREQ_dDECODE(apreq_cookie_decode)
{
    apr_array_header_t *a;
    apreq_value_t *v;
    apr_off_t len;
    char *word;

    word = apr_pstrdup(p,key);
    len  = apreq_unescape(word);
    if (len <= 0)       /* key size must be > 0 */
        return NULL;

    a = apr_array_make(p, APREQ_NELTS, sizeof(apreq_value_t));
    v = (apreq_value_t *)apr_array_push(a);
    v->data = word;
    v->size = len;

    while ( *val && (word = apr_getword(p, &val, '&')) ) {
        len = apreq_unescape(word);
        if (len < 0)
            return NULL; /* bad escape data */
        v = (apreq_value_t *)apr_array_push(a);
        v->data = word;
        v->size = len;
    }

    return a;
}

static const char c2x_table[] = "0123456789abcdef";
APREQ_dENCODE(apreq_cookie_encode)
{
    apr_size_t len = 0;
    char *res, *data;
    int i;

    if (s == 0)
        return apr_pcalloc(p,1);

    for (i = 0; i < s; ++i)
        len += a[i].size;

    res = data = apr_palloc(p, 3*len + a->nelts);

    for (i = 0; i < s; ++i) {
        apr_size_t n = a[i].size;
        const unsigned char *s = (const unsigned char *)a[i].data;

        while (n--) {
            unsigned c = *s;
            if (apr_isalnum(c))
                *data++ = c;
            else if (c == ' ') 
                *data++ = '+';
            else {
#if APR_CHARSET_EBCDIC
                c = apr_xlate_conv_byte(ap_hdrs_to_ascii, (unsigned char)c);
#endif
                *data++ = '%';
                *data++ = c2x_table[c >> 4];
                *data++ = c2x_table[c & 0xf];
            }
            ++s;
        }
        *data++ = (i == 0) ? '=' : '&';
    }

    if (s == 1)
        data[0] = 0;    /* no value: name= */
    else
        data[-1] = 0;   /* replace final '&' with '\0' */

    return res;
}

#endif /* NEEDS_A_NEW_HOME */
