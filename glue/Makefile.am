EXTRA_DIST = perl

dist-hook:
	rm -rf `find $(distdir)/glue -name CVS`
	rm -rf `find $(distdir)/glue -name '*~'`

perl_config:
	cd perl; @PERL@ ../../build/xsbuilder.pl run

perl/Makefile: perl/xsbuilder perl/Makefile.PL
	cd perl; @PERL@ Makefile.PL -apxs @APACHE2_APXS@

perl_glue: perl/Makefile
	cd perl; make

perl_test: perl/Makefile
	cd perl; make test

perl_install:
	cd perl; make install

perl_clean:
	cd perl; make clean

if BUILD_PERL_GLUE

all-local: perl_glue

install-exec-local: perl_install

clean-local: perl_clean

test: perl_test

endif
