AUTOMAKE_OPTIONS = foreign
SUBDIRS = src env t
EXTRA_DIST = INSTALL

docs:	src
	doxygen build/doxygen.conf

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
