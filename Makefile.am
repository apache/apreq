AUTOMAKE_OPTIONS = foreign
SUBDIRS = src env glue t
EXTRA_DIST = README INSTALL CHANGES FAQ MANIFEST.SKIP Makefile.PL buildconf build win32 docs
APU_SRCDIR=`@APU_CONFIG@ --srcdir`
APR_SRCDIR=`@APR_CONFIG@ --srcdir`
APU_DOX = (cd $(APU_SRCDIR); cat - docs/doxygen.conf | doxygen -)
APR_DOX = (cd $(APR_SRCDIR); cat - docs/doxygen.conf | doxygen -)
POD_PREFIX=glue/perl/xsbuilder/Apache
COOKIE_POD=$(POD_PREFIX)/Cookie/Cookie_pod
REQUEST_POD=$(POD_PREFIX)/Request/Request_pod
UPLOAD_POD=$(POD_PREFIX)/Upload/Upload_pod
SED_BODY_TAG= s(^[\S\s]+<body.+|</body>[\S\s]+$$)()gi
EUM=ExtUtils::Manifest

bin_SCRIPTS = apreq2_config

if BUILD_PERL_GLUE
PERL_TEST = perl_test
else
PERL_TEST = 
endif

release: reconfig docs_clean docs distdir
	@PERL@ build/version_check.pl > $(distdir)/PREREQUISITES
	@PERL@ build/version_check.pl -version=@PACKAGE_VERSION@ > $(distdir)/META.yml
	echo MANIFEST > $(distdir)/MANIFEST
	@cd $(distdir); @PERL@ -M$(EUM) -e "$(EUM)::mkmanifest"
	@PERL@ -ple '$$_="$(distdir)/$$_"' $(distdir)/MANIFEST | tar -czf $(distdir).tar.gz -T -
	rm -rf $(distdir)
	@echo "Made $(distdir).tar.gz"

release_test:
	-rm -rf $(distdir)
	tar xzvf $(distdir).tar.gz
	@cd $(distdir); @PERL@ -M$(EUM) -e "die 'Bogus MANIFEST' if $(EUM)::manicheck"
	@cd $(distdir); @PERL@ Makefile.PL -apxs @APACHE2_APXS@ && $(MAKE) test
	rm -rf $(distdir)
	@echo "$(distdir).tar.gz is kosher."


apreq2_config:

reconfig:
	./buildconf
	./config.nice

docs_clean:
	-rm -rf docs

docs:	src env glue build/doxygen.conf apr_tags pod2html feather.gif
	@PERL@ build/version_check.pl doxygen
	doxygen build/doxygen.conf
	cp feather.gif docs/html

feather.gif:
	wget http://httpd.apache.org/docs-2.0/images/feather.gif


docs_install: docs
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)
	cp -a docs $(DESTDIR)$(pkgdatadir)

pod2html: Request.html Cookie.html Upload.html

Cookie.html: Makefile.am $(COOKIE_POD)
	pod2html <$(COOKIE_POD) | perl -0777 -pe '$(SED_BODY_TAG)' > Cookie.html
Request.html: Makefile.am $(REQUEST_POD)
	pod2html <$(REQUEST_POD)| perl -0777 -pe '$(SED_BODY_TAG)' > Request.html
Upload.html: Makefile.am $(UPLOAD_POD)
	pod2html <$(UPLOAD_POD)| perl -0777 -pe '$(SED_BODY_TAG)' > Upload.html

apr_tags: docs/apr.tag docs/apu.tag

docs/apu.tag:
	-mkdir docs
	echo GENERATE_TAGFILE=`pwd`/docs/apu.tag | $(APU_DOX)
docs/apr.tag:
	-mkdir docs
	echo GENERATE_TAGFILE=`pwd`/docs/apr.tag | $(APR_DOX)

test: lib_test env_test $(PERL_TEST)

env_test:
	cd env; $(MAKE) test

lib_test: check
	t/testall -v

perl_install:
	cd glue/perl; $(MAKE) install

perl_test:
	cd glue/perl; $(MAKE) test

perl_glue:
	cd glue/perl; @PERL@ ../../build/xsbuilder.pl run
	cd glue/perl; @PERL@ Makefile.PL -apxs @APACHE2_APXS@
	cd glue/perl; $(MAKE)

if BUILD_HTTPD

install-exec-local: httpd_install

httpd_install:
	cd @APACHE2_SRC@; $(MAKE) install

endif
