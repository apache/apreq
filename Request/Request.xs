#include "apache_request.h"
#include "mod_perl.h"

typedef ApacheRequest * Apache__Request;
typedef ApacheUpload  * Apache__Upload;

#define ApacheUpload_fh(upload)       upload->fp
#define ApacheUpload_size(upload)     upload->size
#define ApacheUpload_name(upload)     upload->name
#define ApacheUpload_filename(upload) upload->filename
#define ApacheUpload_next(upload)     upload->next

#ifndef PerlLIO_dup
#define PerlLIO_dup(fd)   dup((fd)) 
#endif
#ifndef PerlLIO_close
#define PerlLIO_close(fd) close((fd)) 
#endif

static char *r_keys[] = { "_r", "r", NULL };

static SV *r_key_sv(SV *in)
{
    SV *sv;
    int i;

    for (i=0; r_keys[i]; i++) {
	int klen = strlen(r_keys[i]);
	if(hv_exists((HV*)SvRV(in), r_keys[i], klen) &&
	   (sv = *hv_fetch((HV*)SvRV(in), 
			   r_keys[i], klen, FALSE)))
	{
	    return sv;
	}
    }

    return Nullsv;
}

static ApacheRequest *sv_2apreq(SV *sv)
{
    if (SvROK(sv) && sv_derived_from(sv, "Apache::Request")) { 
	SV *obj;

	switch (SvTYPE(SvRV(sv))) {
	case SVt_PVHV :
	    obj = r_key_sv(sv);
	    break;
	default:
	    obj = sv;
	};
	return (ApacheRequest *)SvIV((SV*)SvRV(obj)); 
    }
    else {
	return ApacheRequest_new(perl_request_rec(NULL));
    }
} 

static SV *upload_bless(ApacheUpload *upload) 
{ 
    SV *sv = newSV(0);  
    sv_setref_pv(sv, "Apache::Upload", (void*)upload);  
    return sv; 
} 

#define upload_push(upload) \
    XPUSHs(sv_2mortal(upload_bless(upload))) 

static void apreq_add_magic(SV *sv, ApacheRequest *req)
{
    sv_magic(SvRV(sv), Nullsv, '~', "dummy", -1);
    SvMAGIC(SvRV(sv))->mg_ptr = (char *)req->r;
}

static void apreq_close_handle(void *data)
{
    GV *handle = (GV*)data;
    (void)hv_delete(GvSTASH(handle),
		    GvNAME(handle), GvNAMELEN(handle), G_DISCARD);
}

#ifdef CGI_COMPAT
static void register_uploads (ApacheRequest *req) {
    ApacheUpload *upload;

    for (upload = req->upload; upload; upload = upload->next) {
	if(upload->fp && upload->filename) {
	    GV *gv = gv_fetchpv(upload->filename, TRUE, SVt_PVIO);
	    if (do_open(gv, "<&", 2, FALSE, 0, 0, upload->fp)) { 
		ap_register_cleanup(req->r->pool, (void*)gv, 
				    apreq_close_handle, ap_null_cleanup);
	    } 
	}
    }
}
#else
#define register_uploads(req)
#endif

MODULE = Apache::Request    PACKAGE = Apache::Request   PREFIX = ApacheRequest_

PROTOTYPES: DISABLE 

BOOT:
    av_push(perl_get_av("Apache::Request::ISA",TRUE), newSVpv("Apache",6));

Apache::Request
ApacheRequest_new(class, r, ...)
    SV *class
    Apache r

    PREINIT:
    int i;

    CODE:
    class = class; /* -Wall */ 
    RETVAL = ApacheRequest_new(r->main ? r->main : r);
    register_uploads(RETVAL);

    for (i=2; i<items; i+=2) { 
        char *key = SvPV(ST(i),na); 
	switch (toLOWER(*key)) {
	case 'd':
	    if (strcasecmp(key, "disable_uploads") == 0) {
		RETVAL->disable_uploads = (int)SvIV(ST(i+1));
		break;
	    }
	case 'p':
	    if (strcasecmp(key, "post_max") == 0) {
		RETVAL->post_max = (int)SvIV(ST(i+1));
		break;
	    }
	default:
	    croak("[libapreq] unknown attribute: `%s'", key);
	}
    }

    OUTPUT:
    RETVAL

    CLEANUP:
    apreq_add_magic(ST(0), RETVAL);

char *
ApacheRequest_script_name(req)
    Apache::Request req

int
ApacheRequest_parse(req)
    Apache::Request req

void
ApacheRequest_parms(req)
    Apache::Request req

    CODE:
    ApacheRequest_parse(req);
    ST(0) = mod_perl_tie_table(req->parms);

void
ApacheRequest_upload(req, name=NULL)
    Apache::Request req
    char *name

    PREINIT:
    ApacheUpload *uptr;

    PPCODE:
    ApacheRequest_parse(req);
    if (GIMME == G_SCALAR) {
	if (name) {
	    uptr = ApacheUpload_find(req->upload, name);
	    if (!uptr) {
		XSRETURN_UNDEF;
	    }
	}
	else {
	    uptr = req->upload;
	}
	upload_push(uptr);
    }
    else {
	for (uptr = req->upload; uptr; uptr = uptr->next) {
	    upload_push(uptr);
	}
    }

char *
ApacheRequest_expires(req, time_str)
    Apache::Request req
    char *time_str

MODULE = Apache::Request    PACKAGE = Apache::Upload   PREFIX = ApacheUpload_

PROTOTYPES: DISABLE 

FILE *
ApacheUpload_fh(upload)
    Apache::Upload upload

    CODE:
    if (!(RETVAL = ApacheUpload_fh(upload))) {
	XSRETURN_UNDEF;
    }

    OUTPUT:
    RETVAL

    CLEANUP:
    if (ST(0) != &sv_undef) {
	IO *io = GvIOn((GV*)SvRV(ST(0))); 
	int fd = PerlIO_fileno(IoIFP(io));
	FILE *fp;

	fd = PerlLIO_dup(fd); 
	if (!(fp = PerlIO_fdopen(fd, "r"))) { 
	    PerlLIO_close(fd);
	    croak("fdopen failed!");
	} 
	fseek(fp, 0, 0); 
	IoIFP(GvIOn((GV*)SvRV(ST(0)))) = fp;
	ap_register_cleanup(upload->req->r->pool, (void*)SvRV(ST(0)), 
			    apreq_close_handle, ap_null_cleanup);    
    }

long
ApacheUpload_size(upload)
    Apache::Upload upload

char *
ApacheUpload_name(upload)
    Apache::Upload upload

char *
ApacheUpload_filename(upload)
    Apache::Upload upload

Apache::Upload
ApacheUpload_next(upload)
    Apache::Upload upload 

const char *
ApacheUpload_type(upload)
    Apache::Upload upload 

void
ApacheUpload_info(upload, key=NULL)
    Apache::Upload upload 
    char *key

    CODE:
    if (key) {
	const char *val = ApacheUpload_info(upload, key);
	if (!val) {
	    XSRETURN_UNDEF;
	}
	ST(0) = sv_2mortal(newSVpv((char *)val,0));
    }   
    else {
        ST(0) = mod_perl_tie_table(upload->info);
    }
