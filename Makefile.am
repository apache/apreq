AUTOMAKE_OPTIONS = foreign
SUBDIRS = src env t
EXTRA_DIST = INSTALL
APUDIR=`@APU_CONFIG@ --srcdir`
APRDIR=`@APR_CONFIG@ --srcdir`
APUDOX = (cd $(APUDIR); cat - docs/doxygen.conf | doxygen -)
APRDOX = (cd $(APRDIR); cat - docs/doxygen.conf | doxygen -)

docs:	src env glue build/doxygen.conf
	doxygen build/doxygen.conf

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
