/* ====================================================================
 * Copyright (c) 1995-1999 The Apache Group.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "apache_multipart_buffer.h"

#define FILLUNIT (1024 * 5)
#ifndef CRLF
#define CRLF "\015\012"
#endif
#define CRLF_CRLF "\015\012\015\012"

static char *my_join(pool *p, char *one, int lenone, char *two, int lentwo)
{
    char *res; 
    int len = lenone+lentwo;
    res = (char *)ap_palloc(p, len + 1); 
    memcpy(res, one, lenone);  
    memcpy(res+lenone, two, lentwo);
    return res;
}

static char * 
my_ninstr(register char *big, register char *bigend, char *little, char *lend) 
{ 
    register char *s, *x; 
    register int first = *little; 
    register char *littleend = lend; 
 
    if (!first && little >= littleend) {
        return big; 
    }
    if (bigend - big < littleend - little) {
        return NULL; 
    }
    bigend -= littleend - little++; 
    while (big <= bigend) { 
	if (*big++ != first) {
	    continue; 
	}
	for (x=big,s=little; s < littleend; /**/ ) { 
	    if (*s++ != *x++) { 
		s--; 
		break; 
	    }
	}
	if (s >= littleend) {
	    return big-1; 
	}
    }
    return NULL; 
} 

/*
 * This fills up our internal buffer in such a way that the
 * boundary is never split between reads
 */
void multipart_buffer_fill(multipart_buffer *self, long bytes)
{
    int len_read, length, bytes_to_read;
    int buffer_len = self->buffer_len;

    if (!self->length) {
	return;
    }
    length = (bytes - buffer_len) + self->boundary_length + 2;
    if (self->length < length) {
	length = self->length;
    }
    bytes_to_read = length;

    ap_hard_timeout("multipart_buffer_fill", self->r);
    while (bytes_to_read > 0) {
	/*XXX: we can optimize this loop*/ 
	char *buff = (char *)ap_pcalloc(self->subp,
					sizeof(char) * bytes_to_read + 1);
	len_read = ap_get_client_block(self->r, buff, bytes_to_read);

	if (len_read <= 0) {
	    ap_log_rerror(MPB_ERROR,
			  "[libapreq] client dropped connection during read");
	    self->length = 0;
	    self->buffer = NULL;
	    self->buffer_len = 0;
	    break;
	}

	self->buffer = self->buffer ? 
	    my_join(self->r->pool, 
		    self->buffer, self->buffer_len, 
		    buff, len_read) :
	    ap_pstrndup(self->r->pool, buff, len_read);

	self->total      += len_read;
	self->buffer_len += len_read;
	self->length     -= len_read;
	bytes_to_read    -= len_read;

	ap_reset_timeout(self->r);
    }
    ap_kill_timeout(self->r);
    ap_clear_pool(self->subp);
}

char *multipart_buffer_read(multipart_buffer *self, long bytes, int *blen)
{
    int start = -1;
    char *retval, *str;

    /* default number of bytes to read */
    if (!bytes) {
	bytes = FILLUNIT;       
    }

    /*
     * Fill up our internal buffer in such a way that the boundary
     * is never split between reads.
     */
    multipart_buffer_fill(self, bytes);

    /* Find the boundary in the buffer (it may not be there). */
    if (self->buffer) {
	if ((str = my_ninstr(self->buffer, 
			    self->buffer+self->buffer_len, 
			    self->boundary, 
			    self->boundary+self->boundary_length))) 
	{
	    start = str - self->buffer;
	}
    }

    /* protect against malformed multipart POST operations */
    if (!(start >= 0 || self->length > 0)) {
	ap_log_rerror(MPB_ERROR, 
		      "[libapreq] malformed upload: start=%d, self->length=%d", 
		      start, (int)self->length);
	return NULL;
    }

    /*
     * If the boundary begins the data, then skip past it
     * and return NULL.  The +2 here is a fiendish plot to
     * remove the CR/LF pair at the end of the boundary.
     */
    if (start == 0) {
        /* clear us out completely if we've hit the last boundary. */
	if (strEQ(self->buffer, self->boundary_end)) {
	    self->buffer = NULL;
	    self->buffer_len = 0;
	    self->length = 0;
	    return NULL;
	}

        /* otherwise just remove the boundary. */
	self->buffer += (self->boundary_length + 2);
	self->buffer_len -= (self->boundary_length + 2);
	return NULL;
    }

    if (start > 0) {           /* read up to the boundary */
	*blen = start > bytes ? bytes : start;
    } 
    else {    /* read the requested number of bytes */
	/*
	 * leave enough bytes in the buffer to allow us to read
	 * the boundary.  Thanks to Kevin Hendrick for finding
	 * this one.
	 */
	*blen = bytes - (self->boundary_length + 1);
    }
    
    retval = ap_pstrndup(self->r->pool, self->buffer, *blen); 
    
    self->buffer += *blen;
    self->buffer_len -= *blen;

    /* If we hit the boundary, remove the CRLF from the end. */
    if (start > 0) {
	*blen -= 2;
	retval[*blen] = '\0';
    }

    return retval;
}

table *multipart_buffer_headers(multipart_buffer *self)
{
    int end=0, ok=0, bad=0;
    table *tab;
    char *header;

    do {
	char *str;
	multipart_buffer_fill(self, FILLUNIT);
	if ((str = strstr(self->buffer, CRLF_CRLF))) {
	    ++ok;
	    end = str - self->buffer;
	}
	if (self->buffer == NULL || *self->buffer == '\0') {
	    ++ok;
	}
	if (!ok && self->length <= 0) {
	    ++bad;
	}
    } while (!ok && !bad);

    if (bad) {
	return NULL;
    }

    header = ap_pstrndup(self->r->pool, self->buffer, end+2);
    /*XXX: need to merge continuation lines here?*/

    tab = ap_make_table(self->r->pool, 10);
    self->buffer += end+4;
    self->buffer_len -= end+4;

    {
	char *entry;
	while ((entry = ap_getword_nc(self->r->pool, &header, '\r')) && *header) {
	    char *key;
	    key = ap_getword_nc(self->r->pool, &entry, ':');
	    while (ap_isspace(*entry)) {
		++entry;
	    }
	    if (*header == '\n') {
		++header;
	    }
	    ap_table_add(tab, key, entry);
	}
    }

    return tab;
}

char *multipart_buffer_read_body(multipart_buffer *self) 
{
    char *data, *retval=NULL;
    int blen = 0, old_len = 0;

    while ((data = multipart_buffer_read(self, 0, &blen))) {
	retval = retval ?
	    my_join(self->r->pool, retval, old_len, data, blen) :
	    ap_pstrndup(self->r->pool, data, blen);
	old_len = blen;
    }

    return retval;
}

multipart_buffer *multipart_buffer_new(char *boundary, long length, request_rec *r)
{
    int blen;
    multipart_buffer *self = (multipart_buffer *)
	ap_pcalloc(r->pool, sizeof(multipart_buffer));

    self->r = r;
    self->length = length;
    self->boundary = ap_pstrcat(r->pool, "--", boundary, NULL);
    self->boundary_length = strlen(self->boundary);
    self->boundary_end = ap_pstrcat(r->pool, self->boundary, "--", NULL);
    self->buffer = NULL;
    self->buffer_len = 0;
    self->subp = ap_make_sub_pool(self->r->pool);

    /* Read the preamble and the topmost (boundary) line plus the CRLF. */
    (void)multipart_buffer_read(self, 0, &blen);

    if (multipart_buffer_eof(self)) {
	return NULL;
    }

    return self;
}
