INCLUDES = -I../src @APR_INCLUDES@ @APU_INCLUDES@
LIBS = ../src/lib@APREQ_LIBNAME@.la @APR_LTLIBS@ @APU_LTLIBS@

noinst_LIBRARIES = lib@APREQ_LIBNAME@_tests.a
lib@APREQ_LIBNAME@_tests_a_SOURCES = CuTest.c env.c cookie.c params.c parsers.c

check_PROGRAMS = testall
testall_LDADD  = lib@APREQ_LIBNAME@_tests.a

test:	check
	./testall -v
