AUTOMAKE_OPTIONS = foreign
SUBDIRS = src env t
EXTRA_DIST = INSTALL
APUDIR=`@APU_CONFIG@ --srcdir`
APRDIR=`@APR_CONFIG@ --srcdir`
APUDOX = (cd $(APUDIR); cat - docs/doxygen.conf | doxygen -)
APRDOX = (cd $(APRDIR); cat - docs/doxygen.conf | doxygen -)
COOKIE_POD=glue/perl/blib/lib/Apache/Cookie.pod
REQUEST_POD=glue/perl/blib/lib/Apache/Request.pod

docs:	src env glue build/doxygen.conf
	doxygen build/doxygen.conf

pod2html: Request.html Cookie.html

Cookie.html: Makefile.am $(COOKIE_POD)
	pod2html <$(COOKIE_POD) | perl -0777 -pe 's(^[\S\s]+<body.+|</body>[\S\s]+$$)()gi' > Cookie.html
Request.html: Makefile.am $(REQUEST_POD)
	pod2html <$(REQUEST_POD)| perl -0777 -pe 's(^[\S\s]+<body.+|</body>[\S\s]+$$)()gi' > Request.html

apr_tags: 
	-mkdir docs
	echo GENERATE_TAGFILE=`pwd`/docs/apu.tag | $(APUDOX)
	echo GENERATE_TAGFILE=`pwd`/docs/apr.tag | $(APRDOX)

test: lib_test env_test

env_test:
	cd env; make test

lib_test: check
	t/testall -v

perl_test:
	cd glue/perl; make test

perl_glue:
	cd glue/perl; @PERL@ ../../build/xsbuilder.pl run
	cd glue/perl; @PERL@ Makefile.PL -apxs @APACHE2_APXS@
	cd glue/perl; make
