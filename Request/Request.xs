#include "apache_request.h"

#ifdef WIN32

#ifdef uid_t
#define apache_uid_t uid_t
#undef uid_t
#endif
#define uid_t apache_uid_t

#ifdef gid_t
#define apache_gid_t gid_t
#undef gid_t
#endif
#define gid_t apache_gid_t

#ifdef stat
#define apache_stat stat
#undef stat
#endif

#ifdef lstat
#define apache_lstat lstat
#undef lstat
#endif

#ifdef sleep
#define apache_sleep sleep
#undef sleep
#endif

#endif /* WIN32 */

#undef __attribute__
#include "mod_perl.h"

#ifdef WIN32

#undef uid_t
#ifdef apache_uid_t
#define uid_t apache_uid_t
#undef apache_uid_t
#endif

#undef gid_t
#ifdef apache_gid_t
#define gid_t apache_gid_t
#undef apache_gid_t
#endif

#ifdef apache_lstat
#undef lstat
#define lstat apache_lstat
#undef apache_lstat
#endif

#ifdef apache_stat
#undef stat
#define stat apache_stat
#undef apache_stat
#endif

#ifdef apache_sleep
#undef sleep
#define sleep apache_sleep
#undef apache_sleep
#endif

#endif /* WIN32 */

typedef ApacheRequest * Apache__Request;
typedef ApacheUpload  * Apache__Upload;

#define ApacheUpload_fh(upload)       upload->fp
#define ApacheUpload_size(upload)     upload->size
#define ApacheUpload_name(upload)     upload->name
#define ApacheUpload_filename(upload) upload->filename
#define ApacheUpload_next(upload)     upload->next
#define ApacheUpload_tempname(upload)   upload->tempname

#ifndef PerlLIO_dup
#define PerlLIO_dup(fd)   dup((fd)) 
#endif
#ifndef PerlLIO_close
#define PerlLIO_close(fd) close((fd)) 
#endif

#ifdef PerlIO
typedef PerlIO * InputStream;
#else
typedef FILE * InputStream;
#define PerlIO_importFILE(fp,flags) fp
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
	SV *obj = sv;

	switch (SvTYPE(SvRV(obj))) {
	case SVt_PVHV :
            do {
                obj = r_key_sv(obj);
            } while (SvROK(obj) && (SvTYPE(SvRV(obj)) == SVt_PVHV));
	    break;
	default:
	    break;
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

static void apreq_add_magic(SV *sv, SV *obj, ApacheRequest *req)
{
    sv_magic(SvRV(sv), obj, '~', "dummy", -1);
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
    SV *robj;

    CODE:
    class = class; /* -Wall */ 
    robj = ST(1);
    RETVAL = ApacheRequest_new(r);
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
	case 't':
	    if (strcasecmp(key, "temp_dir") == 0) {
		RETVAL->temp_dir = (char *)SvPV(ST(i+1), PL_na);
		break;
	    }
	default:
	    croak("[libapreq] unknown attribute: `%s'", key);
	}
    }

    OUTPUT:
    RETVAL

    CLEANUP:
    apreq_add_magic(ST(0), robj, RETVAL);

char *
ApacheRequest_script_name(req)
    Apache::Request req

int
ApacheRequest_parse(req)
    Apache::Request req

void
ApacheRequest_parms(req, parms=NULL)
    Apache::Request req
    Apache::Table parms

    CODE:
    if (parms) {
        req->parms = parms->utable;
        req->parsed = 1;
    }
    else {
        ApacheRequest_parse(req);
    }
    ST(0) = mod_perl_tie_table(req->parms);

void
ApacheRequest_param(req, key=NULL, sv=Nullsv)
    Apache::Request req	
    char *key
    SV *sv

    PPCODE:
    if ( !req->parsed ) ApacheRequest_parse(req);

    if (key) {

	if (sv != Nullsv) {

	    if (SvROK(sv) && SvTYPE(SvRV(sv)) == SVt_PVAV) {
	    	I32 i;
	    	AV *av = (AV*)SvRV(sv);
	    	const char *val;

            	ap_table_unset(req->parms, key);
	    	for (i=0; i<=AvFILL(av); i++) {
		    val = (const char *)SvPV(*av_fetch(av, i, FALSE),PL_na);
	            ap_table_add(req->parms, key, val);
	    	}
	    }
            else ap_table_set(req->parms, key, (const char *)SvPV(sv, PL_na));
	}

	switch (GIMME_V) {

        case G_SCALAR:			/* return (first) parameter value */
	    {
	    	const char *val = ap_table_get(req->parms, key);
	    	if (val) XPUSHs(sv_2mortal(newSVpv((char*)val,0)));
	    	else XSRETURN_UNDEF;
	    }
	    break;

	case G_ARRAY:			/* return list of parameter values */
	    {
  	        I32 i;
	        array_header *arr  = ap_table_elts(req->parms);
	        table_entry *elts = (table_entry *)arr->elts;
	        for (i = 0; i < arr->nelts; ++i) {
	            if (elts[i].key && !strcasecmp(elts[i].key, key))
	            	XPUSHs(sv_2mortal(newSVpv(elts[i].val,0)));
	        }
	    }
	    break;

	default:
            XSRETURN_UNDEF;
	} 
    } 
    else {		

	switch (GIMME_V) {

	case G_SCALAR:	    		/* like $apr->parms */
	    ST(0) = mod_perl_tie_table(req->parms);
	    XSRETURN(1); 
	    break;

	case G_ARRAY:			/* return list of unique keys */
            {
            	I32 i;
	    	array_header *arr  = ap_table_elts(req->parms);
	    	table_entry *elts = (table_entry *)arr->elts;
	    	for (i = 0; i < arr->nelts; ++i) {
		    I32 j;
	           if (!elts[i].key) continue;
		    /* simple but inefficient uniqueness check */
		    for (j = 0; j < i; ++j) { 
		        if (!strcasecmp(elts[i].key, elts[j].key))
			    break;
		    }
	            if ( i == j )
	                XPUSHs(sv_2mortal(newSVpv(elts[i].key,0)));
	        }
            }
	    break;

	default:
	    XSRETURN_UNDEF;
 	}
    }

void
ApacheRequest_upload(req, sv=Nullsv)
    Apache::Request req
    SV *sv

    PREINIT:
    ApacheUpload *uptr;

    PPCODE:
    if (sv && SvOBJECT(sv) && sv_isa(sv, "Apache::Upload")) {
        req->upload = (ApacheUpload *)SvIV((SV*)SvRV(sv));
        XSRETURN_EMPTY;
    }
    ApacheRequest_parse(req);
    if (GIMME == G_SCALAR) {
        STRLEN n_a;
        char *name = sv ? SvPV(sv, n_a) : NULL;

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

InputStream
ApacheUpload_fh(upload)
    Apache::Upload upload

    CODE:
    if (!(RETVAL = PerlIO_importFILE(ApacheUpload_fh(upload),0))) {
	XSRETURN_UNDEF;
    }

    OUTPUT:
    RETVAL

    CLEANUP:
    if (ST(0) != &sv_undef) {
	IO *io = GvIOn((GV*)SvRV(ST(0))); 
	int fd = PerlIO_fileno(IoIFP(io));
	PerlIO *fp;

	fd = PerlLIO_dup(fd); 
	if (!(fp = PerlIO_fdopen(fd, "r"))) { 
	    PerlLIO_close(fd);
	    croak("fdopen failed!");
	} 
	PerlIO_seek(fp, 0, 0); 
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

char *
ApacheUpload_tempname(upload)
    Apache::Upload upload

Apache::Upload
ApacheUpload_next(upload)
    Apache::Upload upload 

const char *
ApacheUpload_type(upload)
    Apache::Upload upload 

char *
ApacheUpload_link(upload, name)
    Apache::Upload upload
    char *name

	CODE:
	RETVAL = (link(upload->tempname, name)) ? NULL : name;
	
	OUTPUT:
	RETVAL	

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
