INCLUDES = -I../src @APR_INCLUDES@ @APU_INCLUDES@
LIBS = ../src/libapreq2.la @APR_LTLIBS@ @APU_LTLIBS@

noinst_LIBRARIES = libapreq2_tests.a
libapreq2_tests_a_SOURCES = CuTest.h test_apreq.h CuTest.c cookie.c params.c parsers.c

check_PROGRAMS = testall
testall_LDADD  = libapreq2_tests.a

test:	check
	./testall -v
