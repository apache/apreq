#ifndef _APACHE_REQUEST_H

#define _APACHE_REQUEST_H

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "util_script.h"

typedef struct ApacheUpload ApacheUpload;

typedef struct {
    table *parms;
    ApacheUpload *upload;
    int status;
    int parsed;
    int post_max;
    int disable_uploads;
    request_rec *r;
} ApacheRequest;

struct ApacheUpload {
    ApacheUpload *next;
    char *filename;
    char *name;
    table *info;
    FILE *fp;
    long size;
    ApacheRequest *req;
};

#ifndef strEQ
#define strEQ(s1,s2) (!strcmp(s1,s2))
#endif

#ifndef strEQN
#define strEQN(s1,s2,n) (!strncmp(s1,s2,n))
#endif

#ifndef strcaseEQ
#define strcaseEQ(s1,s2) (!strcasecmp(s1,s2))
#endif

#ifndef strcaseEQN
#define strcaseEQN(s1,s2,n) (!strncasecmp(s1,s2,n))
#endif

#define DEFAULT_TABLE_NELTS 10

#define DEFAULT_ENCTYPE "application/x-www-form-urlencoded"
#define DEFAULT_ENCTYPE_LENGTH 33

#define MULTIPART_ENCTYPE "multipart/form-data"
#define MULTIPART_ENCTYPE_LENGTH 19

#ifdef  __cplusplus
 extern "C" {
#endif 

ApacheRequest *ApacheRequest_new(request_rec *r);
int ApacheRequest_parse_multipart(ApacheRequest *req);
int ApacheRequest_parse_urlencoded(ApacheRequest *req);
char *ApacheRequest_script_name(ApacheRequest *req);
char *ApacheRequest_script_path(ApacheRequest *req);
const char *ApacheRequest_param(ApacheRequest *req, const char *key);
array_header *ApacheRequest_params(ApacheRequest *req, const char *key);
char *ApacheRequest_params_as_string(ApacheRequest *req, const char *key);
int ApacheRequest___parse(ApacheRequest *req);
#define ApacheRequest_parse(req) \
    (req->status = req->parsed ? req->status : ApacheRequest___parse(req)) 

FILE *ApacheRequest_tmpfile(ApacheRequest *req);
ApacheUpload *ApacheUpload_new(ApacheRequest *req);
ApacheUpload *ApacheUpload_find(ApacheUpload *upload, char *name);

#define ApacheRequest_upload(req) \
    ((req->parsed || (ApacheRequest_parse(req) == OK)) ? req->upload : NULL)

#define ApacheUpload_info(upload, key) \
ap_table_get(upload->info, key)

#define ApacheUpload_type(upload) \
ApacheUpload_info(upload, "Content-Type")

char *ApacheUtil_expires(pool *p, char *time_str, int type);
#define EXPIRES_HTTP   1
#define EXPIRES_COOKIE 2
char *ApacheRequest_expires(ApacheRequest *req, char *time_str);

#ifdef __cplusplus
 }
#endif

#define REQ_ERROR APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, req->r

#ifdef REQDEBUG
#define REQ_DEBUG(a) a
#else
#define REQ_DEBUG(a)
#endif

#endif // _APACHE_REQUEST_H
