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

#include "apreq_util.h"
#include "apreq_error.h"
#include "apr_time.h"
#include "apr_strings.h"
#include "apr_lib.h"
#include <assert.h>
#define MIN(a,b) ( (a) < (b) ? (a) : (b) )
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )

/* used for specifying file sizes */

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
            if (type == APREQ_MATCH_FULL && len < nlen)
                hay = NULL;     /* insufficient room for match */
	    break;
        }
        --len;
        ++hay;
    }

    return hay ? hay - begin : -1;
}

static const char c2x_table[] = "0123456789abcdef"; /* XXX uppercase a-f ? */
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

APR_INLINE
static apr_status_t url_decode(char *d, apr_size_t *dlen, 
                               const char *s, apr_size_t *slen)
{
    const char *src = s;
    char *start = d;
    const char *end = s + *slen;

    for (; s < end; ++d, ++s) {
        switch (*s) {

        case '+':
            *d = ' ';
            break;

        case '%':
	    if (s + 2 < end && apr_isxdigit(s[1]) && apr_isxdigit(s[2]))
            {
		*d = x2c(s + 1);
		s += 2;
	    }
            else if (s + 5 < end && (s[1] == 'u' || s[1] == 'U') &&
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
                *dlen = d - start;
                *slen = s - src;
                if (s + 5 < end || (s + 2 < end && s[1] != 'u' && s[1] != 'U')) {
                    *d = 0;
                    return APREQ_ERROR_BADSEQ;
                }
                memcpy(d, s, end - s);
                d[end - s] = 0;
                return APR_INCOMPLETE;
	    }
            break;

        case 0:
            *d = 0;
            *dlen = d - start;
            *slen = s - src;
            return APREQ_ERROR_BADCHAR;

        default:
            *d = *s;
        }
    }

    *d = 0;
    *dlen = d - start;
    *slen = s - src;
    return APR_SUCCESS;
}

APREQ_DECLARE(apr_status_t) apreq_decode(char *d, apr_size_t *dlen,
                                         const char *s, apr_size_t slen)
{
    apr_size_t len = 0;
    const char *end = s + slen;
    apr_status_t rv;

    if (s == (const char *)d) {     /* optimize for src = dest case */
        for ( ; d < end; ++d) {
            if (*d == '%' || *d == '+')
                break;
            else if (*d == 0) {
                *dlen = (const char *)d - s;
                return APREQ_ERROR_BADCHAR;
            }
        }
        len = (const char *)d - s;
        s = (const char *)d;
        slen -= len;
    }

    rv = url_decode(d, dlen, s, &slen);
    *dlen += len;
    return rv;
}


APREQ_DECLARE(apr_status_t) apreq_decodev(char *d, apr_size_t *dlen,
                                          struct iovec *v, int nelts)
{
    apr_status_t status = APR_SUCCESS;
    int n = 0;

    *dlen = 0;
    for (n = 0; n < nelts; ++n) {
        apr_size_t slen, len;

    start_decodev:

        slen = v[n].iov_len;
        switch (status = url_decode(d,&len,v[n].iov_base, &slen)) {

        case APR_SUCCESS:
            d += len;
            *dlen += len;
            continue;

        case APR_INCOMPLETE:
            d += len;
            *dlen += len;

            if (++n == nelts)
                return APR_INCOMPLETE;

            len = v[n-1].iov_len - slen;
            memcpy(d + len, v[n].iov_base, v[n].iov_len);
            v[n].iov_len += len;
            v[n].iov_base = d;
            goto start_decodev;

        default:
            *dlen += len;
            return status;
        }
    }
    return APR_SUCCESS;
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


#define apreq_value_nlen(v) (v->size - (v->name - v->data))
#define apreq_value_vlen(v) ((v->name - v->data) - 1)

APREQ_DECLARE(char *) apreq_join(apr_pool_t *p, 
                                 const char *sep, 
                                 const apr_array_header_t *arr,
                                 apreq_join_t mode)
{
    apr_ssize_t len, slen;
    char *rv;
    const apreq_value_t **a = (const apreq_value_t **)arr->elts;
    char *d;
    const int n = arr->nelts;
    int j;

    slen = sep ? strlen(sep) : 0;

    if (n == 0)
        return NULL;

    for (j=0, len=0; j < n; ++j)
        len += apreq_value_vlen(a[j]) + slen;

    /* Allocated the required space */

    switch (mode) {
    case APREQ_JOIN_ENCODE:
        len += 2 * len;
        break;
    case APREQ_JOIN_QUOTE:
        len = 2 * (len + n);
        break;
    default:
        /* nothing special required, just here to keep noisy compilers happy */
        break;
    }

    rv = apr_palloc(p, len);

    if (n == 0)
        return rv;

    /* Pass two --- copy the argument strings into the result space */

    d = rv;

    switch (mode) {

    case APREQ_JOIN_ENCODE:
        d += apreq_encode(d, a[0]->data, apreq_value_vlen(a[0]));

        for (j = 1; j < n; ++j) {
                memcpy(d, sep, slen);
                d += slen;
                d += apreq_encode(d, a[j]->data, apreq_value_vlen(a[j]));
        }
        break;

    case APREQ_JOIN_DECODE:
        if (apreq_decode(d, &len, a[0]->data, apreq_value_vlen(a[0])))
            return NULL;
        else
            d += len;

        for (j = 1; j < n; ++j) {
            memcpy(d, sep, slen);
            d += slen;

            if (apreq_decode(d, &len, a[j]->data, apreq_value_vlen(a[j])))
                return NULL;
            else
                d += len;
        }
        break;


    case APREQ_JOIN_QUOTE:

        d += apreq_quote_once(d, a[0]->data, apreq_value_vlen(a[0]));

        for (j = 1; j < n; ++j) {
            memcpy(d, sep, slen);
            d += slen;
            d += apreq_quote_once(d, a[j]->data, apreq_value_vlen(a[j]));
        }
        break;


    case APREQ_JOIN_AS_IS:
        memcpy(d,a[0]->data,apreq_value_vlen(a[0]));
        d += apreq_value_vlen(a[0]);

        for (j = 1; j < n ; ++j) {
            memcpy(d, sep, slen);
            d += slen;
            memcpy(d, a[j]->data, apreq_value_vlen(a[j]));
            d += apreq_value_vlen(a[j]);
        }
        break;

    default:
        break;
    }

    *d = 0;
    return rv;
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
    struct iovec v[APREQ_DEFAULT_NELTS];
    apr_status_t s;
    apr_bucket *e;
    int n = 0;
    *wlen = 0;

    for (e = APR_BRIGADE_FIRST(bb); e != APR_BRIGADE_SENTINEL(bb);
         e = APR_BUCKET_NEXT(e)) 
    {
        apr_size_t len;
        if (n == APREQ_DEFAULT_NELTS) {
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


struct cleanup_data {
    const char *fname;
    apr_pool_t *pool;
};

static apr_status_t apreq_file_cleanup(void *d)
{
    struct cleanup_data *data = d;
    return apr_file_remove(data->fname, data->pool);
}

/*
 * The reason we need the above cleanup is because on Windows, APR_DELONCLOSE
 * forces applications to open the file with FILE_SHARED_DELETE
 * set, which is, unfortunately, a property that is preserved 
 * across NTFS "hard" links.  This breaks apps that link() the temp 
 * file to a permanent location, and subsequently expect to open it 
 * before the original tempfile is closed+deleted. In fact, even
 * Apache::Upload does this, so it is a common enough event that the
 * apreq_file_cleanup workaround is necessary.
 */

APREQ_DECLARE(apr_status_t) apreq_file_mktemp(apr_file_t **fp, 
                                              apr_pool_t *pool,
                                              const char *path)
{
    apr_status_t rc;
    char *tmpl;
    struct cleanup_data *data;

    if (path == NULL) {
        rc = apr_temp_dir_get(&path, pool);
        if (rc != APR_SUCCESS)
            return rc;
    }
    rc = apr_filepath_merge(&tmpl, path, "apreqXXXXXX",
                            APR_FILEPATH_NOTRELATIVE, pool);

    if (rc != APR_SUCCESS)
        return rc;
    
    data = apr_palloc(pool, sizeof *data);
    /* cleanups are LIFO, so this one will run just after 
       the cleanup set by mktemp */
    apr_pool_cleanup_register(pool, data, 
                              apreq_file_cleanup, apreq_file_cleanup);

    rc = apr_file_mktemp(fp, tmpl, /* NO APR_DELONCLOSE! see comment above */
                           APR_CREATE | APR_READ | APR_WRITE
                           | APR_EXCL | APR_BINARY, pool);

    if (rc == APR_SUCCESS) {
        apr_file_name_get(&data->fname, *fp);
        data->pool = pool;
    }
    else {
        apr_pool_cleanup_kill(pool, data, apreq_file_cleanup);
    }

    return rc;
}


APREQ_DECLARE(apr_status_t)
    apreq_header_attribute(const char *hdr,
                           const char *name, const apr_size_t nlen,
                           const char **val, apr_size_t *vlen)
{
    const char *key, *v;

    /*Must ensure first char isn't '=', so we can safely backstep. */
    while (*hdr == '=')
        ++hdr;

    while ((key = strchr(hdr, '=')) != NULL) {

        v = key + 1;
        --key;

        while (apr_isspace(*key) && key > hdr + nlen)
            --key;

        key -= nlen - 1;

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
            /* bad token: no terminating quote found */
            return APREQ_ERROR_BADTOKEN;
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
        if (strncasecmp(key, name, nlen) == 0) {
            *vlen = v - *val;
            if (key > hdr) {
                /* ensure preceding character isn't a token, per rfc2616, s2.2 */
                switch (key[-1]) {
                case '(': case ')': case '<': case '>': case '@':
                case ',': case ';': case ':': case '\\': case '"':
                case '/': case '[': case ']': case '?': case '=':
                case '{': case '}': case ' ': case '\t':
                    return APR_SUCCESS;
                default:
                    if (apr_iscntrl(key[-1]))
                        return APR_SUCCESS;
                }
            }
            else
                return APR_SUCCESS;
        }
        hdr = v;
    }

    return APREQ_ERROR_NOATTR;
}



#define BUCKET_IS_SPOOL(e) ((e)->type == &spool_bucket_type)
#define FILE_BUCKET_LIMIT      ((apr_size_t)-1 - 1)

static
void spool_bucket_destroy(void *data)
{
    apr_bucket_type_file.destroy(data);
}

static
apr_status_t spool_bucket_read(apr_bucket *e, const char **str,
                                   apr_size_t *len, apr_read_type_e block)
{
    return apr_bucket_type_file.read(e, str, len, block);
}

static
apr_status_t spool_bucket_setaside(apr_bucket *data, apr_pool_t *reqpool)
{
    return apr_bucket_type_file.setaside(data, reqpool);
}

static
apr_status_t spool_bucket_split(apr_bucket *a, apr_size_t point)
{
    apr_status_t rv = apr_bucket_shared_split(a, point);
    a->type = &apr_bucket_type_file;
    return rv;
}

static const apr_bucket_type_t spool_bucket_type = {
    "APREQ_SPOOL", 5, APR_BUCKET_DATA,
    spool_bucket_destroy,
    spool_bucket_read,
    spool_bucket_setaside,
    spool_bucket_split,
    apr_bucket_copy_notimpl,
};


APREQ_DECLARE(apr_status_t) apreq_brigade_concat(apr_pool_t *pool,
                                                 const char *temp_dir,
                                                 apr_size_t heap_limit,
                                                 apr_bucket_brigade *out, 
                                                 apr_bucket_brigade *in)
{
    apr_status_t s;
    apr_bucket_file *f;
    apr_off_t wlen;
    apr_file_t *file;
    apr_off_t in_len, out_len;
    apr_bucket *last_in, *last_out;

    last_out = APR_BRIGADE_LAST(out);

    if (APR_BUCKET_IS_EOS(last_out))
        return APR_EOF;

    s = apr_brigade_length(out, 0, &out_len);
    if (s != APR_SUCCESS)
        return s;

    /* This cast, when out_len = -1, is intentional */
    if ((apr_uint64_t)out_len < heap_limit) {

        s = apr_brigade_length(in, 0, &in_len);
        if (s != APR_SUCCESS)
            return s;

        /* This cast, when in_len = -1, is intentional */
        if ((apr_uint64_t)in_len < heap_limit - out_len) {
            APR_BRIGADE_CONCAT(out, in);
            return APR_SUCCESS;
        }
    }
    
    if (!BUCKET_IS_SPOOL(last_out)) {

        s = apreq_file_mktemp(&file, pool, temp_dir);
        if (s != APR_SUCCESS)
            return s;

        /* This cast, when out_len = -1, is intentional */
        if ((apr_uint64_t)out_len < heap_limit)
            s = apreq_brigade_fwrite(file, &wlen, out);

        if (s != APR_SUCCESS)
            return s;

        last_out = apr_bucket_file_create(file, wlen, 0, 
                                          out->p, out->bucket_alloc);
        last_out->type = &spool_bucket_type;
        APR_BRIGADE_INSERT_TAIL(out, last_out);
        f = last_out->data;
    }
    else {
        f = last_out->data;
        /* Need to seek here, just in case our spool bucket 
         * was read from between apreq_brigade_concat calls. 
         */
        wlen = last_out->start + last_out->length;
        s = apr_file_seek(f->fd, APR_SET, &wlen);
        if (s != APR_SUCCESS)
            return s;
    }

    last_in = APR_BRIGADE_LAST(in);

    if (APR_BUCKET_IS_EOS(last_in))
        APR_BUCKET_REMOVE(last_in);

    s = apreq_brigade_fwrite(f->fd, &wlen, in);

    if (s == APR_SUCCESS) {

        /* We have to deal with the possibility that the new 
         * data may be too large to be represented by a single
         * temp_file bucket.
         */

        while ((apr_uint64_t)wlen > FILE_BUCKET_LIMIT - last_out->length) {
            apr_bucket *e;

            apr_bucket_copy(last_out, &e);
            e->length = 0;
            e->start = last_out->start + FILE_BUCKET_LIMIT;
            wlen -= FILE_BUCKET_LIMIT - last_out->length;
            last_out->length = FILE_BUCKET_LIMIT;
            last_out->type = &apr_bucket_type_file;

            APR_BRIGADE_INSERT_TAIL(out, e);
            last_out = e;
        }

        last_out->length += wlen;

        if (APR_BUCKET_IS_EOS(last_in))
            APR_BRIGADE_INSERT_TAIL(out, last_in);

    }
    else if (APR_BUCKET_IS_EOS(last_in))
        APR_BRIGADE_INSERT_TAIL(in, last_in);

    apr_brigade_cleanup(in);
    return s;
}
