INCLUDES = -I../../include @APR_INCLUDES@ @APU_INCLUDES@
LDFLAGS = @APU_LTFLAGS@ @APR_LTFLAGS@
LIBS  = ../lib@APREQ_LIBNAME@.la @APU_LIBS@ @APR_LIBS@
noinst_LIBRARIES = libapache_test.a
libapache_test_a_SOURCES = at.h at.c

check_PROGRAMS = version cookie params parsers error util
LDADD  = libapache_test.a

check_SCRIPTS = version.t cookie.t params.t parsers.t error.t util.t
TESTS = $(check_SCRIPTS)
TESTS_ENVIRONMENT = @PERL@ -MTest::Harness -e 'runtests(@ARGV)'
CLEANFILES = $(check_SCRIPTS)

%.t: %
	echo "#!perl" > $@
	echo "exec './$*'" >> $@

test: $(check_SCRIPTS)
	$(TESTS_ENVIRONMENT) $(check_SCRIPTS)
