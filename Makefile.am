AUTOMAKE_OPTIONS = foreign
SUBDIRS = src env t
EXTRA_DIST = INSTALL

test: lib_test env_test

env_test:
	cd env; make test

lib_test: check
	t/testall -v

perl_test:
	cd glue/perl; make test

perl_glue:
	cd glue/perl; @PERL@ ../../build/xsbuilder.pl run && @PERL@ Makefile.PL && make
