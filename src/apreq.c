/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
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

#include "apreq.h"
#include "apr_time.h"
#include "apr_strings.h"
#include "apr_lib.h"

#define MIN(a,b) ( (a) < (b) ? (a) : (b) )
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )

APREQ_DECLARE(apreq_value_t *)apreq_make_value(apr_pool_t  *p, 
                                               const char  *name,
                                               const apr_ssize_t nlen,
                                               const char  *val, 
                                               const apr_ssize_t vlen)
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


APREQ_DECLARE(apreq_value_t *)apreq_copy_value(apr_pool_t *p, 
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

APREQ_DECLARE(apreq_value_t *)apreq_merge_values(apr_pool_t *p,
                                           const apr_array_header_t *arr)
{
    apreq_value_t *a = *(apreq_value_t **)(arr->elts);
    apreq_value_t *v = apreq_char_to_value( apreq_join(p, ", ", arr, AS_IS) );
    if (arr->nelts > 0)
        v->name = a->name;
    return v;
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
    if ( strcasecmp(time_str,"now") != 0 ) 
        when += apreq_atol(time_str);

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

APREQ_DECLARE(apr_int64_t) apreq_atoi64(const char *s)
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

APREQ_DECLARE(long) apreq_atol(const char *s) 
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
    digit = ((what[0] >= 'A') ? ((what[0] & 0xdf) - 'A') + 10 : (what[0] - '0'));
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

APREQ_DECLARE(apr_ssize_t) apreq_decode(char *d, const char *s, 
                                       const apr_ssize_t slen)
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
            else if (s[1] == 'u' && apr_isxdigit(s[2]) &&
                     apr_isxdigit(s[3]) && apr_isxdigit(s[4]) &&
                     apr_isxdigit(s[5]))
            {
                /* XXX: Need to decode oddball
                 * javascript unicode escapes: %uXXXX
                 * For now we'll just give up.
                 */
                badesc = 1;
                *d = '%';
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

APREQ_DECLARE(apr_size_t) apreq_quote(char *dest, const char *src, 
                                      const apr_size_t slen) 
{
    char *d = dest;
    const char *s = src;

    if (slen == 0) {
        *d = 0;
        return 0;
    }

    if (src[0] == '"' && src[slen-1] == '"') { /* src is already quoted */
        memcpy(dest, src, slen);
        return slen;
    }

    *d++ = '"';

    while (s < src + slen) {

        switch (*s) {

        case '\\': 
            if (s < src + slen - 1) {
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
        d += apreq_quote(d, a[0]->data, a[0]->size);

        for (j = 1; j < n; ++j) {
            memcpy(d, sep, slen);
            d += slen;
            d += apreq_quote(d, a[j]->data, a[j]->size);
        }
        break;


    case AS_IS:
        memcpy(d,a[0]->data,a[0]->size);
        d += a[0]->size;

        for (j = 0; j < n ; ++j) {
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
    if (src == NULL || slen == 0)
        return NULL;

    rv = apr_palloc(p, 3 * slen + sizeof *rv);
    rv->name = NULL;
    rv->status = APR_SUCCESS;
    rv->size = apreq_encode(rv->data, src, slen);
    return rv->data;
}

