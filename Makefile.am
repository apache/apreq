AUTOMAKE_OPTIONS = foreign
SUBDIRS = src env t
EXTRA_DIST = INSTALL
APU_SRCDIR=`@APU_CONFIG@ --srcdir`
APR_SRCDIR=`@APR_CONFIG@ --srcdir`
APU_DOX = (cd $(APU_SRCDIR); cat - docs/doxygen.conf | doxygen -)
APR_DOX = (cd $(APR_SRCDIR); cat - docs/doxygen.conf | doxygen -)
COOKIE_POD=glue/perl/blib/lib/Apache/Cookie.pod
REQUEST_POD=glue/perl/blib/lib/Apache/Request.pod

bin_SCRIPTS = apreq2_config

apreq2_config:

docs:	src env glue build/doxygen.conf
	doxygen build/doxygen.conf

pod2html: Request.html Cookie.html

Cookie.html: Makefile.am $(COOKIE_POD)
	pod2html <$(COOKIE_POD) | perl -0777 -pe 's(^[\S\s]+<body.+|</body>[\S\s]+$$)()gi' > Cookie.html
Request.html: Makefile.am $(REQUEST_POD)
	pod2html <$(REQUEST_POD)| perl -0777 -pe 's(^[\S\s]+<body.+|</body>[\S\s]+$$)()gi' > Request.html

apr_tags: 
	-mkdir docs
	echo GENERATE_TAGFILE=`pwd`/docs/apu.tag | $(APU_DOX)
	echo GENERATE_TAGFILE=`pwd`/docs/apr.tag | $(APR_DOX)

test: lib_test env_test

env_test:
	cd env; make test

lib_test: check
	t/testall -v

perl_install:
	cd glue/perl; make install

perl_test:
	cd glue/perl; make test

perl_glue:
	cd glue/perl; @PERL@ ../../build/xsbuilder.pl run
	cd glue/perl; @PERL@ Makefile.PL -apxs @APACHE2_APXS@
	cd glue/perl; make

if BUILD_HTTPD

httpd_install:
	cd @APACHE2_SRC@; make install

endif
