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

#include "apreq.h"
#include "apreq_env.h"
#include "apr_time.h"
#include "apr_strings.h"
#include "apr_lib.h"

#define MIN(a,b) ( (a) < (b) ? (a) : (b) )
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )

APREQ_DECLARE(apreq_value_t *)apreq_make_value(apr_pool_t  *p, 
                                               const char  *name,
                                               const apr_size_t nlen,
                                               const char  *val, 
                                               const apr_size_t vlen)
{
    apreq_value_t *v = apr_palloc(p, vlen + nlen + 1 + sizeof *v);
    if (v == NULL)
        return NULL;

    v->size = vlen;
    memcpy(v->data, val, vlen);
    v->data[vlen] = 0;

    v->name = v->data + vlen + 1;
    memcpy((char *)v->name, name, nlen);
    ((char *)v->name)[nlen] = 0;
    v->status = APR_SUCCESS;

    return v;
}


apreq_value_t * apreq_copy_value(apr_pool_t *p, 
                                 const apreq_value_t *val)
{
    apreq_value_t *v;
    if (val == NULL)
        return NULL;

    v = apr_palloc(p, val->size + sizeof *v);

    *v = *val;

    if ( v->size > 0 )
        memcpy(v->data, val->data, v->size);

    return v;
}

apreq_value_t * apreq_merge_values(apr_pool_t *p,
                                   const apr_array_header_t *arr)
{
    apreq_value_t *a = *(apreq_value_t **)(arr->elts);
    apreq_value_t *v = apreq_char_to_value( apreq_join(p, ", ", arr, AS_IS) );
    if (arr->nelts > 0)
        v->name = a->name;
    return v;
}

APREQ_DECLARE(const char *)apreq_enctype(void *env)
{
    char *enctype;
    const char *ct = apreq_env_content_type(env);

    if (ct == NULL)
        return NULL;
    else {
        const char *semicolon = strchr(ct, ';');
        if (semicolon) {
            enctype = apr_pstrdup(apreq_env_pool(env), ct);
            enctype[semicolon - ct] = 0;
            return enctype;
        }
        else
            return ct;
    }
}

APREQ_DECLARE(char *) apreq_expires(apr_pool_t *p, const char *time_str,
                                          const apreq_expires_t type)
{
    apr_time_t when;
    apr_time_exp_t tms;
    char sep = (type == HTTP) ? ' ' : '-';

    if (time_str == NULL) {
	return NULL;
    }

    when = apr_time_now();
    if (strcasecmp(time_str,"now")) 
        when += apr_time_from_sec(apreq_atoi64t(time_str));

    if ( apr_time_exp_gmt(&tms, when) != APR_SUCCESS )
        return NULL;

    return apr_psprintf(p,
		       "%s, %.2d%c%s%c%.2d %.2d:%.2d:%.2d GMT",
		       apr_day_snames[tms.tm_wday],
		       tms.tm_mday, sep, apr_month_snames[tms.tm_mon], sep,
		       tms.tm_year + 1900,
		       tms.tm_hour, tms.tm_min, tms.tm_sec);
}

/* used for specifying file & byte sizes */

APREQ_DECLARE(apr_int64_t) apreq_atoi64f(const char *s)
{
    apr_int64_t n = 0;
    char *p;
    if (s == NULL)
        return 0;

    n = apr_strtoi64(s, &p, 0);

    if (p == NULL)
        return n;
    while (apr_isspace(*p))
        ++p;

    switch (apr_tolower(*p)) {
      case 'g': return n * 1024*1024*1024;
      case 'm': return n * 1024*1024;
      case 'k': return n * 1024;
    }

    return n;
}


/* converts date offsets (e.g. "+3M") to seconds */

APREQ_DECLARE(apr_int64_t) apreq_atoi64t(const char *s) 
{
    apr_int64_t n = 0;
    char *p;
    if (s == NULL)
        return 0;
    n = apr_strtoi64(s, &p, 0); /* XXX: what about overflow? */

    if (p == NULL)
        return n;
    while (apr_isspace(*p))
        ++p;

    switch (*p) {
      case 'Y': /* fall thru */
      case 'y': return n * 60*60*24*365;
      case 'M': return n * 60*60*24*30;
      case 'D': /* fall thru */
      case 'd': return n * 60*60*24;
      case 'H': /* fall thru */
      case 'h': return n * 60*60;
      case 'm': return n * 60;
      case 's': /* fall thru */
      default:
          return n;
    }
    /* should never get here */
    return -1;
}


/*
  search for a string in a fixed-length byte string.
  if partial is true, partial matches are allowed at the end of the buffer.
  returns NULL if not found, or a pointer to the start of the first match.
*/

/* XXX: should we drop this and replace it with apreq_index ? */
APREQ_DECLARE(char *) apreq_memmem(char* hay, apr_size_t hlen, 
                                   const char* ndl, apr_size_t nlen,
                                   const apreq_match_t type)
{
    apr_size_t len = hlen;
    char *end = hay + hlen;

    while ( (hay = memchr(hay, ndl[0], len)) ) {
	len = end - hay;

	/* done if matches up to capacity of buffer */
	if ( memcmp(hay, ndl, MIN(nlen, len)) == 0 ) {
            if (type == FULL && len < nlen)
                hay = NULL;     /* insufficient room for match */
	    break;
        }
        --len;
        ++hay;
    }

    return hay;
}

APREQ_DECLARE(apr_ssize_t ) apreq_index(const char* hay, apr_size_t hlen, 
                                        const char* ndl, apr_size_t nlen, 
                                        const apreq_match_t type)
{
    apr_size_t len = hlen;
    const char *end = hay + hlen;
    const char *begin = hay;

    while ( (hay = memchr(hay, ndl[0], len)) ) {
	len = end - hay;

	/* done if matches up to capacity of buffer */
	if ( memcmp(hay, ndl, MIN(nlen, len)) == 0 ) {
            if (type == FULL && len < nlen)
                hay = NULL;     /* insufficient room for match */
	    break;
        }
        --len;
        ++hay;
    }

    return hay ? hay - begin : -1;
}

static const char c2x_table[] = "0123456789abcdef";
static APR_INLINE char x2c(const char *what)
{
    register char digit;

#if !APR_CHARSET_EBCDIC
    digit  = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A') + 10 : (what[0] - '0'));
    digit *= 16;
    digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A') + 10 : (what[1] - '0'));
#else /*APR_CHARSET_EBCDIC*/
    char xstr[5];
    xstr[0]='0';
    xstr[1]='x';
    xstr[2]=what[0];
    xstr[3]=what[1];
    xstr[4]='\0';
    digit = apr_xlate_conv_byte(ap_hdrs_from_ascii, 0xFF & strtol(xstr, NULL, 16));
#endif /*APR_CHARSET_EBCDIC*/
    return (digit);
}

static APR_INLINE unsigned int x2ui(const char *what) {
    register unsigned int digit = 0;

#if !APR_CHARSET_EBCDIC
    digit  = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A') + 10 : (what[0] - '0'));
    digit *= 16;
    digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A') + 10 : (what[1] - '0'));
    digit *= 16;
    digit += (what[2] >= 'A' ? ((what[2] & 0xdf) - 'A') + 10 : (what[2] - '0'));
    digit *= 16;
    digit += (what[3] >= 'A' ? ((what[3] & 0xdf) - 'A') + 10 : (what[3] - '0'));

#else /*APR_CHARSET_EBCDIC*/
    char xstr[7];
    xstr[0]='0';
    xstr[1]='x';
    xstr[2]=what[0];
    xstr[3]=what[1];
    xstr[4]=what[2];
    xstr[5]=what[3];
    xstr[6]='\0';
    digit = apr_xlate_conv_byte(ap_hdrs_from_ascii, 0xFFFF & strtol(xstr, NULL, 16));
#endif /*APR_CHARSET_EBCDIC*/
    return (digit);
}

APREQ_DECLARE(apr_ssize_t) apreq_decode(char *d, const char *s, 
                                       const apr_size_t slen)
{
    register int badesc = 0;
    char *start = d;
    const char *end = s + slen;

    if (s == NULL || d == NULL)
        return -1;

    if (s == (const char *)d) {     /* optimize for src = dest case */
        for ( ; s < end; ++s) {
            if (*s == '%' || *s == '+')
                break;
            else if (*s == 0)
                return s - (const char *)d;
        }
        d = (char *)s;
    }

    for (; s < end; ++d, ++s) {
        switch (*s) {

        case '+':
            *d = ' ';
            break;

        case '%':
	    if (apr_isxdigit(s[1]) && apr_isxdigit(s[2]))
            {
		*d = x2c(s + 1);
		s += 2;
	    }
            else if ((s[1] == 'u' || s[1] == 'U') &&
                     apr_isxdigit(s[2]) && apr_isxdigit(s[3]) && 
                     apr_isxdigit(s[4]) && apr_isxdigit(s[5]))
            {
                unsigned int c = x2ui(s+2);
                if (c < 0x80) {
                    *d = c;
                }
                else if (c < 0x800) {
                    *d++ = 0xc0 | (c >> 6);
                    *d   = 0x80 | (c & 0x3f);
                }
                else if (c < 0x10000) {
                    *d++ = 0xe0 | (c >> 12);
                    *d++ = 0x80 | ((c >> 6) & 0x3f);
                    *d   = 0x80 | (c & 0x3f);
                }
                else if (c < 0x200000) {
                    *d++ = 0xf0 | (c >> 18);
                    *d++ = 0x80 | ((c >> 12) & 0x3f);
                    *d++ = 0x80 | ((c >> 6) & 0x3f);
                    *d   = 0x80 | (c & 0x3f);
                }
                else if (c < 0x4000000) {
                    *d++ = 0xf8 | (c >> 24);
                    *d++ = 0x80 | ((c >> 18) & 0x3f);
                    *d++ = 0x80 | ((c >> 12) & 0x3f);
                    *d++ = 0x80 | ((c >> 6) & 0x3f);
                    *d   = 0x80 | (c & 0x3f);
                }
                else if (c < 0x8000000) {
                    *d++ = 0xfe | (c >> 30);
                    *d++ = 0x80 | ((c >> 24) & 0x3f);
                    *d++ = 0x80 | ((c >> 18) & 0x3f);
                    *d++ = 0x80 | ((c >> 12) & 0x3f);
                    *d++ = 0x80 | ((c >> 6) & 0x3f);
                    *d   = 0x80 | (c & 0x3f);
                }
                s += 5;
            }
	    else {
		badesc = 1;
		*d = '%';
	    }
            break;

        case 0:
            *d = 0;
            return -1;

        default:
            *d = *s;
        }
    }

    *d = 0;

    if (badesc)
	return -2;
    else
	return d - start;
}


APREQ_DECLARE(apr_size_t) apreq_encode(char *dest, const char *src, 
                                       const apr_size_t slen) 
{
    char *d = dest;
    const unsigned char *s = (const unsigned char *)src;
    unsigned c;

    for ( ;s < (const unsigned char *)src + slen; ++s) {
        c = *s;
        if ( apr_isalnum(c) )
            *d++ = c;

        else if ( c == ' ' )
            *d++ = '+';

        else {
#if APR_CHARSET_EBCDIC
            c = apr_xlate_conv_byte(ap_hdrs_to_ascii, (unsigned char)c);
#endif
            *d++ = '%';
            *d++ = c2x_table[c >> 4];
            *d++ = c2x_table[c & 0xf];
        }
    }
    *d = 0;

    return d - dest;
}

APREQ_DECLARE(apr_size_t) apreq_quote_once(char *dest, const char *src, 
                                           const apr_size_t slen) 
{
    if (slen > 1 && src[0] == '"' && src[slen-1] == '"') {
        /* looks like src is already quoted */        
        memcpy(dest, src, slen);
        return slen;
    }
    else
        return apreq_quote(dest, src, slen);
}

APREQ_DECLARE(apr_size_t) apreq_quote(char *dest, const char *src, 
                                      const apr_size_t slen) 
{
    char *d = dest;
    const char *s = src;
    const char *const last = src + slen - 1;

    if (slen == 0) {
        *d = 0;
        return 0;
    }

    *d++ = '"';

    while (s <= last) {

        switch (*s) {

        case '\\': 
            if (s < last) {
                *d++ = *s++;
                break;
            }
            /* else fall through */

        case '"':
            *d++ = '\\';
        }

        *d++ = *s++;
    }

    *d++ = '"';
    *d = 0;

    return d - dest;
}


APREQ_DECLARE(const char *) apreq_join(apr_pool_t *p, 
                                       const char *sep, 
                                       const apr_array_header_t *arr,
                                       apreq_join_t mode)
{
    apr_ssize_t len, slen;
    apreq_value_t *rv;
    const apreq_value_t **a = (const apreq_value_t **)arr->elts;
    char *d;
    const int n = arr->nelts;
    int j;

    slen = sep ? strlen(sep) : 0;

    if (n == 0)
        return NULL;

    for (j=0, len=0; j < n; ++j)
        len += a[j]->size + slen;

    /* Allocated the required space */

    switch (mode) {
    case ENCODE:
        len += 2 * len;
        break;
    case QUOTE:
        len = 2 * (len + n);
        break;
    default:
        /* nothing special required, just here to keep noisy compilers happy */
        break;
    }

    rv = apr_palloc(p, len + sizeof *rv);
    rv->name = 0;
    rv->size = 0;
    rv->status = APR_SUCCESS;
    rv->data[0] = 0;

    if (n == 0)
        return rv->data;

    /* Pass two --- copy the argument strings into the result space */

    d = rv->data;

    switch (mode) {

    case ENCODE:
        d += apreq_encode(d, a[0]->data, a[0]->size);

        for (j = 1; j < n; ++j) {
                memcpy(d, sep, slen);
                d += slen;
                d += apreq_encode(d, a[j]->data, a[j]->size);
        }
        break;

    case DECODE:
        len = apreq_decode(d, a[0]->data, a[0]->size);

        if (len < 0) {
            rv->status = APR_BADCH;
            break;
        }
        else
            d += len;

        for (j = 1; j < n; ++j) {
            memcpy(d, sep, slen);
            d += slen;

            len = apreq_decode(d, a[j]->data, a[j]->size);

            if (len < 0) {
                rv->status = APR_BADCH;
                break;
            }
            else
                d += len;
        }
        break;


    case QUOTE:

        d += apreq_quote_once(d, a[0]->data, a[0]->size);

        for (j = 1; j < n; ++j) {
            memcpy(d, sep, slen);
            d += slen;
            d += apreq_quote_once(d, a[j]->data, a[j]->size);
        }
        break;


    case AS_IS:
        memcpy(d,a[0]->data,a[0]->size);
        d += a[0]->size;

        for (j = 1; j < n ; ++j) {
            memcpy(d, sep, slen);
            d += slen;
            memcpy(d, a[j]->data, a[j]->size);
            d += a[j]->size;
        }
        break;

    default:
        break;
    }

    *d = 0;
    rv->size = d - rv->data;
    return rv->data;
}

APREQ_DECLARE(char *) apreq_escape(apr_pool_t *p, 
                                   const char *src, const apr_size_t slen)
{
    apreq_value_t *rv;
    if (src == NULL)
        return NULL;

    rv = apr_palloc(p, 3 * slen + sizeof *rv);
    rv->name = NULL;
    rv->status = APR_SUCCESS;
    rv->size = apreq_encode(rv->data, src, slen);
    return rv->data;
}

APREQ_DECLARE(apr_ssize_t) apreq_unescape(char *str)
{
    return apreq_decode(str,str,strlen(str));
}

APR_INLINE
static apr_status_t apreq_fwritev(apr_file_t *f, struct iovec *v, 
                                  int *nelts, apr_size_t *bytes_written)
{
    apr_size_t len, bytes_avail = 0;
    int n = *nelts;
    apr_status_t s = apr_file_writev(f, v, n, &len);

    *bytes_written = len;

    if (s != APR_SUCCESS)
        return s;

    while (--n >= 0)
        bytes_avail += v[n].iov_len;


    if (bytes_avail > len) {
        /* incomplete write: must shift v */
        n = 0;
        while (v[n].iov_len <= len) {
            len -= v[n].iov_len;
            ++n;
        }
        v[n].iov_len -= len;
        v[n].iov_base = (char *)(v[n].iov_base) + len;

        if (n > 0) {
            struct iovec *dest = v;
            do {
                *dest++ = v[n++];
            }  while (n < *nelts);
            *nelts = dest - v;
        }
        else {
            s = apreq_fwritev(f, v, nelts, &len);
            *bytes_written += len;
        }
    }
    else
        *nelts = 0;

    return s;
}


APREQ_DECLARE(apr_status_t) apreq_brigade_fwrite(apr_file_t *f, 
                                                 apr_off_t *wlen,
                                                 apr_bucket_brigade *bb)
{
    struct iovec v[APREQ_NELTS];
    apr_status_t s;
    apr_bucket *e;
    int n = 0;
    *wlen = 0;

    for (e = APR_BRIGADE_FIRST(bb); e != APR_BRIGADE_SENTINEL(bb);
         e = APR_BUCKET_NEXT(e)) 
    {
        apr_size_t len;
        if (n == APREQ_NELTS) {
            s = apreq_fwritev(f, v, &n, &len);
            if (s != APR_SUCCESS)
                return s;
            *wlen += len;
        }
        s = apr_bucket_read(e, (const char **)&(v[n].iov_base), 
                            &len, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            return s;

        v[n++].iov_len = len;
    }

    while (n > 0) {
        apr_size_t len;
        s = apreq_fwritev(f, v, &n, &len);
        if (s != APR_SUCCESS)
            return s;
        *wlen += len;
    }
    return APR_SUCCESS;
}

APREQ_DECLARE(apr_status_t) apreq_file_mktemp(apr_file_t **fp, 
                                              apr_pool_t *pool,
                                              const char *path)
{
    apr_status_t rc = APR_SUCCESS;
    char *tmpl;
    if (path) {
        rc = apr_filepath_merge(&tmpl, path, "apreqXXXXXX",
                                APR_FILEPATH_NOTRELATIVE, pool);
        if (rc != APR_SUCCESS)
            return rc;
    }
    else {
        const char *tdir;
        rc = apr_temp_dir_get(&tdir, pool);
        if (rc != APR_SUCCESS)
            return rc;
        tmpl = apr_pstrcat(pool, tdir, "XXXXXX", NULL);
    }
    
    return apr_file_mktemp(fp, tmpl,
                           APR_CREATE | APR_READ | APR_WRITE
                           | APR_EXCL | APR_DELONCLOSE | APR_BINARY, pool);
}

/* circumvents linker issue (perl's CCFLAGS != apr's CCFLAGS) on linux */
APREQ_DECLARE(apr_file_t *)apreq_brigade_spoolfile(apr_bucket_brigade *bb)
{
    apr_bucket *last = APR_BRIGADE_LAST(bb);
    apr_bucket_file *f;
    if (APR_BUCKET_IS_FILE(last)) {
        f = last->data;
        return f->fd;
    }
    else
        return NULL;
}


APREQ_DECLARE(apr_status_t)
    apreq_header_attribute(const char *hdr,
                           const char *name, const apr_size_t nlen,
                           const char **val, apr_size_t *vlen)
{
    const char *loc = strchr(hdr, '='), *v;

    if (loc == NULL)
        return APR_NOTFOUND;

    v = loc + 1;
    --loc;

    while (apr_isspace(*loc) && loc - hdr > nlen)
        --loc;

    loc -= nlen - 1;

    while (apr_isspace(*v))
            ++v;

    if (*v == '"') {
        ++v;
        /* value is inside quotes */
        for (*val = v; *v; ++v) {
            switch (*v) {
            case '"':
                goto finish;
            case '\\':
                if (v[1] != 0)
                    ++v;
            default:
                break;
            }
        }
    }
    else {
        /* value is not wrapped in quotes */
        for (*val = v; *v; ++v) {
            switch (*v) {
            case ' ':
            case ';':
            case ',':
            case '\t':
            case '\r':
            case '\n':
                goto finish;
            default:
                break;
            }
        }
    }

 finish:
    if (strncasecmp(loc,name,nlen) != 0
        || (loc > hdr && apr_isalpha(loc[-1])))
        return apreq_header_attribute(v, name, nlen, val, vlen);

    *vlen = v - *val;
    return APR_SUCCESS;


}
