AUTOMAKE_OPTIONS = foreign
SUBDIRS = src env glue t
EXTRA_DIST = README INSTALL CHANGES MANIFEST.SKIP Makefile.PL build win32
APU_SRCDIR=`@APU_CONFIG@ --srcdir`
APR_SRCDIR=`@APR_CONFIG@ --srcdir`
APU_DOX = (cd $(APU_SRCDIR); cat - docs/doxygen.conf | doxygen -)
APR_DOX = (cd $(APR_SRCDIR); cat - docs/doxygen.conf | doxygen -)
COOKIE_POD=glue/perl/blib/lib/Apache/Cookie.pod
REQUEST_POD=glue/perl/blib/lib/Apache/Request.pod
SED_BODY_TAG= s(^[\S\s]+<body.+|</body>[\S\s]+$$)()gi
EUM=ExtUtils::Manifest

bin_SCRIPTS = apreq2_config

if BUILD_PERL_GLUE
PERL_TEST = perl_test
else
PERL_TEST = 
endif

release: distdir
	@PERL@ build/version_check.pl > $(distdir)/PREREQUISITES
	echo MANIFEST > $(distdir)/MANIFEST
	cd $(distdir); ./buildconf
	@cd $(distdir); @PERL@ -M$(EUM) -e "$(EUM)::mkmanifest"
	@PERL@ -ple '$$_="$(distdir)/$$_"' $(distdir)/MANIFEST | tar -czf $(distdir).tar.gz -T -
	rm -rf $(distdir)
	@echo "Made $(distdir).tar.gz"

release_test:
	-rm -rf $(distdir)
	tar xzvf $(distdir).tar.gz
	@cd $(distdir); @PERL@ -M$(EUM) -e "die 'Bogus MANIFEST' if $(EUM)::manicheck"
	@cd $(distdir); @PERL@ Makefile.PL -apxs @APACHE2_APXS@ && make test
	rm -rf $(distdir)
	@echo "$(distdir).tar.gz is kosher."

apreq2_config:

docs:	src env glue build/doxygen.conf
	doxygen build/doxygen.conf

pod2html: Request.html Cookie.html

Cookie.html: Makefile.am $(COOKIE_POD)
	pod2html <$(COOKIE_POD) | perl -0777 -pe '$(SED_BODY_TAG)' > Cookie.html
Request.html: Makefile.am $(REQUEST_POD)
	pod2html <$(REQUEST_POD)| perl -0777 -pe '$(SED_BODY_TAG)' > Request.html

apr_tags: 
	-mkdir docs
	echo GENERATE_TAGFILE=`pwd`/docs/apu.tag | $(APU_DOX)
	echo GENERATE_TAGFILE=`pwd`/docs/apr.tag | $(APR_DOX)

test: lib_test env_test $(PERL_TEST)

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

install-exec-local: httpd_install

httpd_install:
	cd @APACHE2_SRC@; make install

endif
