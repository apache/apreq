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

#include "apreq_cookie.h"
#include "apreq_env.h"
#include "apr_strings.h"
#include "apr_lib.h"

#define RFC      APREQ_COOKIE_VERSION_RFC
#define NETSCAPE APREQ_COOKIE_VERSION_NETSCAPE
#define DEFAULT  APREQ_COOKIE_VERSION_DEFAULT

APREQ_DECLARE(apreq_cookie_t *) apreq_cookie(const apreq_jar_t *jar, 
                                             const char *name)
{
    const char *val = apr_table_get(jar->cookies, name);
    if (val == NULL)
        return NULL;
    return apreq_value_to_cookie(apreq_char_to_value(val));
}

APREQ_DECLARE(void) apreq_jar_add(apreq_jar_t *jar, 
                                  const apreq_cookie_t *c)
{
    apr_table_addn(jar->cookies,
                   c->v.name,c->v.data);
}

APREQ_DECLARE(void) apreq_cookie_expires(apreq_cookie_t *c, 
                                         const char *time_str)
{
    if (time_str == NULL) {
        c->max_age = -1;
        return;
    }

    if (!strcasecmp(time_str, "now"))
        c->max_age = 0;
    else
        c->max_age = apr_time_from_sec(apreq_atoi64t(time_str));
}

static int has_rfc_cookie(void *ctx, const char *key, const char *val)
{
    const apreq_cookie_t *c = apreq_value_to_cookie(
                      apreq_char_to_value(val));

    return c->version == NETSCAPE; /* 0 -> non-netscape cookie found, stop.
                                      1 -> not found, keep going. */
}

APREQ_DECLARE(apreq_cookie_version_t) apreq_ua_cookie_version(void *env)
{
    if (apreq_env_cookie2(env) == NULL) {
        apreq_jar_t *j = apreq_jar(env, NULL);

        if (j == NULL || apreq_jar_nelts(j) == 0) 
            return NETSCAPE;

        else if (apr_table_do(has_rfc_cookie, NULL, j->cookies) == 1)
            return NETSCAPE;

        else
            return RFC;
    }
    else
        return RFC;
}


APREQ_DECLARE(apr_status_t) 
    apreq_cookie_attr(apr_pool_t *p, apreq_cookie_t *c, 
                      const char *attr, apr_size_t alen,
                      const char *val, apr_size_t vlen)
{
    if (alen < 2)
        return APR_EGENERAL;

    if ( attr[0] ==  '-' || attr[0] == '$' ) {
        ++attr;
        --alen;
    }

    switch (apr_tolower(*attr)) {

    case 'n': /* name */
        c->v.name = apr_pstrmemdup(p,val,vlen);
        return APR_SUCCESS;

    case 'v': /* version */
        while (!apr_isdigit(*val)) {
            if (vlen == 0)
                return APR_EGENERAL;
            ++val;
            --vlen;
        }
        c->version = *val - '0';
        return APR_SUCCESS;

    case 'e': case 'm': /* expires, max-age */
        apreq_cookie_expires(c, val);
        return APR_SUCCESS;

    case 'd':
        c->domain = apr_pstrmemdup(p,val,vlen);
        return APR_SUCCESS;

    case 'p':
        if (alen != 4)
            break;
        if (!strncasecmp("port", attr, 4)) {
            c->port = apr_pstrmemdup(p,val,vlen);
            return APR_SUCCESS;
        }
        else if (!strncasecmp("path", attr, 4)) {
            c->path = apr_pstrmemdup(p,val,vlen);
            return APR_SUCCESS;
        }
        break;

    case 'c':
        if (!strncasecmp("commentURL", attr, 10)) {
            c->commentURL = apr_pstrmemdup(p,val,vlen);
            return APR_SUCCESS;
        } 
        else if (!strncasecmp("comment", attr, 7)) {
            c->comment = apr_pstrmemdup(p,val,vlen);
            return APR_SUCCESS;
        }
        break;

    case 's':
        c->secure = (!strncasecmp("on",val,vlen));
        return APR_SUCCESS;

    };

    return APR_ENOTIMPL;
}

APREQ_DECLARE(apreq_cookie_t *) apreq_cookie_make(apr_pool_t *p, 
                                  const char *name, const apr_size_t nlen,
                                  const char *value, const apr_size_t vlen)
{
    apreq_cookie_t *c = apr_palloc(p, vlen + sizeof *c);
    apreq_value_t *v = &c->v;

    v->size = vlen;
    v->name = apr_pstrmemdup(p, name, nlen);
    memcpy(v->data, value, vlen);
    v->data[vlen] = 0;
    
    c->version = DEFAULT;

    /* session cookie is the default */
    c->max_age = -1;

    c->path = NULL;
    c->domain = NULL;
    c->port = NULL;
    c->secure = 0; 

    c->comment = NULL;
    c->commentURL = NULL;

    return c;
}

APR_INLINE
static apr_status_t get_pair(const char **data,
                             const char **n, apr_size_t *nlen,
                             const char **v, apr_size_t *vlen)
{
    const char *d = *data;
    unsigned char in_quotes = 0;

    *n = d;

    while (*d != '=' && !apr_isspace(*d)) {
        if (*d++ == 0)  {
            /*error: no '=' sign detected */
            *data = d-1;
            return APR_EGENERAL;
        }
    }

    *nlen = d - *n;

    do ++d;
    while (*d == '=' || apr_isspace(*d));

    *v = d;

    for (;;++d) {
        switch (*d) {

        case ';':
        case ',':
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
                return APR_EGENERAL;
            }

        case '"':
            in_quotes = ! in_quotes;

        }
    }

 pair_result:

    *vlen = d - *v;
    *data = d;

    if (in_quotes)
        return APR_EGENERAL;

    return APR_SUCCESS;
}


APREQ_DECLARE(apreq_jar_t *) apreq_jar(void *env, const char *hdr)
{
    apr_pool_t *p = apreq_env_pool(env);

    apreq_cookie_version_t version;
    apreq_jar_t *j = NULL;
    apreq_cookie_t *c;

    const char *origin;
    const char *name, *value; 
    apr_size_t nlen, vlen;

    /* initialize jar */
    
    if (hdr == NULL) {
        /* use the environment's cookie data */

        j = apreq_env_jar(env, NULL);
        if ( j != NULL )
            return j;

        j = apr_palloc(p, sizeof *j);
        j->env = env;
        j->cookies = apr_table_make(p, APREQ_NELTS);
        j->status = APR_SUCCESS;

        hdr = apreq_env_cookie(env);

        /* XXX: potential race condition here 
           between env_jar fetch and env_jar set.  */

        apreq_env_jar(env,j);

        if (hdr == NULL)
            return j;
    }
    else {
        j = apr_palloc(p, sizeof *j);
        j->env = env;
        j->cookies = apr_table_make(p, APREQ_NELTS);
        j->status = APR_SUCCESS;
    }

    origin = hdr;

    apreq_log(APREQ_DEBUG 0, env, "parsing cookie data: %s", hdr);

    /* parse data */

 parse_cookie_header:

    c = NULL;
    version = NETSCAPE;

    while (apr_isspace(*hdr))
        ++hdr;

    /* XXX cheat: assume "$..." => "$Version" => RFC Cookie header */

    if (*hdr == '$') { 
        version = RFC;
        while (*hdr && !apr_isspace(*hdr))
            ++hdr;
    }

    for (;;) {
        apr_status_t status;

        while (*hdr == ';' || apr_isspace(*hdr))
            ++hdr;

        switch (*hdr) {

        case 0:
            /* this is the normal exit point for apreq_jar */
            if (c != NULL) {
                apreq_log(APREQ_DEBUG j->status, env, 
                          "adding cookie: %s => %s", c->v.name, c->v.data);
                apreq_add_cookie(j, c);
            }
            return j;

        case ',':
            ++hdr;
            if (c != NULL) {
                apreq_log(APREQ_DEBUG j->status, env, 
                          "adding cookie: %s => %s", c->v.name, c->v.data);
                apreq_add_cookie(j, c);
            }
            goto parse_cookie_header;

        case '$':
            if (c == NULL) {
                j->status = APR_EGENERAL;
                apreq_log(APREQ_ERROR j->status, env,
                      "Saw RFC attribute, was expecting NAME=VALUE cookie pair: %s",
                          hdr);
                return j;
            }
            else if (version == NETSCAPE) {
                j->status = APR_EGENERAL;
                apreq_log(APREQ_ERROR j->status, env, 
                          "Saw RFC attribute in a Netscape Cookie header: %s", 
                          hdr);
                return j;
            }

            status = get_pair(&hdr, &name, &nlen, &value, &vlen);
            if (status != APR_SUCCESS) {
                j->status = status;
                apreq_log(APREQ_ERROR status, env,
                              "Bad RFC attribute: %s",
                           apr_pstrmemdup(p, name, hdr-name));
                return j;
            }

            status = apreq_cookie_attr(p, c, name, nlen, value, vlen);
            switch (status) {
            case APR_ENOTIMPL:
                apreq_log(APREQ_WARN status, env, 
                          "Skipping unrecognized RFC attribute pair: %s",
                           apr_pstrmemdup(p, name, hdr-name));
                /* fall through */
            case APR_SUCCESS:
                break;
            default:
                j->status = status;
                apreq_log(APREQ_ERROR status, env,
                          "Bad RFC attribute pair: %s",
                           apr_pstrmemdup(p, name, hdr-name));
                return j;
            }

            break;

        default:
            if (c != NULL) {
                apreq_log(APREQ_DEBUG j->status, env, 
                          "adding cookie: %s => %s", c->v.name, c->v.data);
                apreq_add_cookie(j, c);
            }

            status = get_pair(&hdr, &name, &nlen, &value, &vlen);

            if (status == APR_SUCCESS) {
                c = apreq_make_cookie(p, name, nlen, value, vlen);
                c->version = version;
            }
            else {
                apreq_log(APREQ_WARN status, env, 
                          "Skipping bad NAME=VALUE pair: %s", 
                           apr_pstrmemdup(p, name, hdr-name));
            }
        }
    }

    /* NOT REACHED */
    return j;
}


APREQ_DECLARE(int) apreq_cookie_serialize(const apreq_cookie_t *c,
                                          char *buf, apr_size_t len)
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
#define NULL2EMPTY(attr) (attr ? attr : "")


    if (c->version == NETSCAPE) {
        char expires[APR_RFC822_DATE_LEN] = {0};

        ADD_ATTR(path);
        ADD_ATTR(domain);

        if (c->max_age != -1) {
            strcpy(f, "; expires=%s");
            apr_rfc822_date(expires, c->max_age + apr_time_now());
            expires[7] = '-';
            expires[11] = '-';
        }
        else
            strcpy(f, "");

        f += strlen(f);

        if (c->secure)
            strcpy(f, "; secure");

        return apr_snprintf(buf, len, format, c->v.name, c->v.data,
           NULL2EMPTY(c->path), NULL2EMPTY(c->domain), expires);
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

    strcpy(f, c->max_age != -1 ? "; max-age=%" APR_TIME_T_FMT : "");

    f += strlen(f);

    if (c->secure)
        strcpy(f, "; secure");

    return apr_snprintf(buf, len, format, c->v.name, c->v.data, c->version,
                        NULL2EMPTY(c->path), NULL2EMPTY(c->domain), 
                        NULL2EMPTY(c->port), NULL2EMPTY(c->comment), 
                        NULL2EMPTY(c->commentURL), apr_time_sec(c->max_age));
}


APREQ_DECLARE(char*) apreq_cookie_as_string(const apreq_cookie_t *c,
                                            apr_pool_t *p)
{
    char s[APREQ_COOKIE_MAX_LENGTH];
    int n = apreq_serialize_cookie(s, APREQ_COOKIE_MAX_LENGTH, c);

    if (n < APREQ_COOKIE_MAX_LENGTH)
        return apr_pstrmemdup(p, s, n);
    else
        return NULL;
}

APREQ_DECLARE(apr_status_t) apreq_cookie_bake(const apreq_cookie_t *c,
                                              void *env)
{
    char *s = apreq_cookie_as_string(c,apreq_env_pool(env));

    if (s == NULL) {
        apreq_log(APREQ_ERROR APR_EGENERAL, env, 
                  "Serialized cookie exceeds APREQ_COOKIE_MAX_LENGTH = %d", 
                    APREQ_COOKIE_MAX_LENGTH);
        return APR_EGENERAL;
    }

    return apreq_env_set_cookie(env, s);
}

APREQ_DECLARE(apr_status_t) apreq_cookie_bake2(const apreq_cookie_t *c,
                                               void *env)
{
    char *s = apreq_cookie_as_string(c,apreq_env_pool(env));

    if ( s == NULL ) {
        apreq_log(APREQ_ERROR APR_EGENERAL, env,
                  "Serialized cookie exceeds APREQ_COOKIE_MAX_LENGTH = %d", 
                    APREQ_COOKIE_MAX_LENGTH);
        return APR_EGENERAL;
    }
    else if ( c->version == NETSCAPE ) {
        apreq_log(APREQ_ERROR APR_EGENERAL, env,
                  "Cannot bake2 a Netscape cookie: %s", s);
        return APR_EGENERAL;
    }

    return apreq_env_set_cookie2(env, s);
}




/* XXX: The functions below belong somewhere else, since they
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
