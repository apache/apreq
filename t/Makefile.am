INCLUDES = -I../src @APR_INCLUDES@ @APU_INCLUDES@
LIBS = ../src/libapreq2.la @APR_LTLIBS@ @APU_LTLIBS@
LDFLAGS = @APR_LDFLAGS@ @APU_LDFLAGS@

noinst_LIBRARIES = libapreq2_test.a
libapreq2_test_a_SOURCES = at.h at.c test_env.c

check_PROGRAMS = version cookie params parsers
LDADD  = libapreq2_test.a

check_SCRIPTS = version.t cookie.t params.t parsers.t
TESTS = $(check_SCRIPTS)
TESTS_ENVIRONMENT = @PERL@ -MTest::Harness -e 'runtests(@ARGV)'
CLEANFILES = $(check_SCRIPTS)

%.t: %
	echo "#!perl" > $@
	echo "exec './$*'" >> $@

test: $(check_SCRIPTS)
	$(TESTS_ENVIRONMENT) $(check_SCRIPTS)
