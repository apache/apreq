#include "apache_request.h"

typedef struct {
    request_rec *r;
    pool *subp;
    long length;
    long total;
    long boundary_length;
    char *boundary;
    char *boundary_end;
    char *buffer;
    long buffer_len;
} multipart_buffer;

#define multipart_buffer_eof(self) \
(((self->buffer == NULL) || (*self->buffer == '\0')) && (self->length <= 0))

char *multipart_buffer_read_body(multipart_buffer *self); 
table *multipart_buffer_headers(multipart_buffer *self);
void multipart_buffer_fill(multipart_buffer *self, long bytes);
char *multipart_buffer_read(multipart_buffer *self, long bytes, int *blen);
multipart_buffer *multipart_buffer_new(char *boundary, long length, request_rec *r);

#define MPB_ERROR APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, self->r
