#include "apreq.h"
#include "apr_time.h"

#define MIN(a,b) ( (a) < (b) ? (a) : (b) )
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )

apreq_value_t *apreq_value_make(apr_pool_t *p, const char *n,
                                const char *d, const apr_ssize_t s)
{
    apreq_value_t *v = apr_palloc(p, s + sizeof *v);
    memcpy(v->data, d, s);
    v->flags = 0;
    v->name = n;
    v->size = s;
    v->data[s] = 0;
    return v;
}

apreq_value_t *apreq_value_copy(apr_pool_t *p, 
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

apreq_value_t *apreq_value_merge(apr_pool_t *p,
                                 const apr_array_header_t *arr)
{
    apr_ssize_t s;
    apreq_value_t *rv;
    apreq_value_t **a = (apreq_value_t **)arr->elts;
    int j, n = arr->nelts;

    if (n == 0)
        return NULL;

    while (*a == NULL && n > 0) {
        ++a;
        --n;
    }
    while (n > 0 && a[n-1] == NULL)
        --n;

    if (n == 0)
        return NULL;

    for (j=0, s=0; j < n; ++j)
        if (a[j] != NULL)
            s += a[j]->size + 2;

    rv = apr_palloc(p, s + sizeof *rv);
    rv->size = 0;

    memcpy(rv->data, a[0]->data, a[0]->size);
    rv->size = a[0]->size;

    for (j = 1; j < n; ++j) {
        rv->data[rv->size++] = ',';
        rv->data[rv->size++] = ' ';
        memcpy(rv->data, a[j]->data, a[j]->size);
        rv->size += a[j]->size;
    }
    rv->flags = 0;              /* XXX */
    rv->name = a[0]->name;
    rv->data[rv->size] = 0;     /* nul-terminate rv->data */
    return rv;
}

char *apreq_expires(apr_pool_t *p, const char *time_str, const int type)
{
    apr_time_t when;
    apr_time_exp_t tms;
    char sep = (type == APREQ_EXPIRES_HTTP) ? ' ' : '-';

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

apr_int64_t apreq_atoi64(const char *s) {
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

long apreq_atol(const char *s) 
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
char *apreq_memmem(char* hay, apr_off_t hlen, 
                   const char* ndl, apr_off_t nlen, int part)
{
    apr_off_t len = hlen;
    char *end = hay + hlen;

    while ( (hay = memchr(hay, ndl[0], len)) ) {
	len = end - hay;

	/* done if matches up to capacity of buffer */
	if ( memcmp(hay, ndl, MIN(nlen, len)) == 0 ) {
            if (part == 0 && len < nlen)
                hay = NULL;     /* insufficient room for match */
	    break;
        }
        --len;
        ++hay;
    }

    return hay;
}

apr_off_t apreq_index(const char* hay, apr_off_t hlen, 
                      const char* ndl, apr_off_t nlen, int part)
{
    apr_off_t len = hlen;
    const char *end = hay + hlen;
    const char *begin = hay;

    while ( (hay = memchr(hay, ndl[0], len)) ) {
	len = end - hay;

	/* done if matches up to capacity of buffer */
	if ( memcmp(hay, ndl, MIN(nlen, len)) == 0 ) {
            if (part == 0 && len < nlen)
                hay = NULL;     /* insufficient room for match */
	    break;
        }
        --len;
        ++hay;
    }

    return hay ? hay - begin : -1;
}

static const char c2x_table[] = "0123456789abcdef";
static char x2c(const char *what)
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

apr_off_t apreq_unescape(char *s)
{
    register int badesc, badpath;
    char *x, *y;

    badesc = 0;
    badpath = 0;
    /* Initial scan for first '%'. Don't bother writing values before
     * seeing a '%' */
    y = strchr(s, '%');
    if (y == NULL) {
        return -1;
    }

    for (x = y; *y; ++x, ++y) {
        switch (*y) {
        case '+':
            *x = ' ';
            break;
        case '%':
	    if (!apr_isxdigit(*(y + 1)) || !apr_isxdigit(*(y + 2))) {
		badesc = 1;
		*x = '%';
	    }
	    else {
		*x = x2c(y + 1);
		y += 2;
	    }
            break;
        default:
            *x = *y;
        }
    }
    *x = '\0';
    if (badesc)
	return -1;
    else
	return x - s;
}


char *apreq_escape(apr_pool_t *p, char *str) 
{
    char *esc = apr_palloc(p, 3 * strlen(str) + 1);
    char *d = esc;
    const unsigned char *s = (const unsigned char *)str;
    unsigned c;
    while((c = *s)) {
        if( apr_isalnum(c) ) {
            *d++ = c;
        } 
        else if ( c == ' ' ) {
            *d++ = '+';
        }
        else {
#if APR_CHARSET_EBCDIC
            c = apr_xlate_conv_byte(ap_hdrs_to_ascii, (unsigned char)c);
#endif
            *d++ = '%';
            *d++ = c2x_table[c >> 4];
            *d++ = c2x_table[c & 0xf];
        }
        ++s;
    }
    *d = 0;
    return esc;
}

static char *apreq_array_pstrcat(apr_pool_t *p,
                                 const apr_array_header_t *arr,
                                 const char *sep, int mode)
{
    char *cp, *res, **strpp;
    apr_size_t len, size;
    int i;

    size = sep ? strlen(sep) : 0;

    if (arr->nelts <= 0 || arr->elts == NULL) {    /* Empty table? */
        return (char *) apr_pcalloc(p, 1);
    }

    /* Pass one --- find length of required string */

    len = 0;
    for (i = 0, strpp = (char **) arr->elts; ; ++strpp) {
        if (strpp && *strpp != NULL) {
            len += strlen(*strpp);
        }
        if (++i >= arr->nelts) {
            break;
        }

        len += size;
    }

    /* Allocate the required string */

    if (mode == APREQ_ESCAPE)
        len += 2 * len;
    else if(mode == APREQ_QUOTE)
        len += 2 * arr->nelts;

    res = (char *) apr_palloc(p, len + 1);
    cp = res;

    /* Pass two --- copy the argument strings into the result space */

    for (i = 0, strpp = (char **) arr->elts; ; ++strpp) {
        if (strpp && *strpp != NULL) {
            len = strlen(*strpp);

            if (mode == APREQ_ESCAPE) {
                const unsigned char *s = (const unsigned char *)*strpp;
                unsigned c;
                while((c = *s)) {
                    if( apr_isalnum(c) ) {
                        *cp++ = c;
                    } else if ( c == ' ' ) {
                        *cp++ = '+';
                    }
                    else {
#if APR_CHARSET_EBCDIC
                        c = apr_xlate_conv_byte(ap_hdrs_to_ascii, 
                                                (unsigned char)c);
#endif
                        *cp++ = '%';
                        *cp++ = c2x_table[c >> 4];
                        *cp++ = c2x_table[c & 0xf];
                    }
                    ++s;
                }
            } 
            else if (mode == APREQ_QUOTE) {
                *cp++ = '"';
                memcpy(cp, *strpp, len);
                cp += len;
                *cp++ = '"';

            }

            else {
                memcpy(cp, *strpp, len);
                cp += len;
            }

        }
        if (++i >= arr->nelts) {
            break;
        }
        if (sep) {
            memcpy(cp,sep,size);
            cp += size;
        }

    }

    *cp = '\0';

    return res;
}
