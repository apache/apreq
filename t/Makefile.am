INCLUDES = -I../src -I@APR_INCLUDES@ -I@APU_INCLUDES@
LIBS = ../src/libapreq.la @APR_LIBS@ @APU_LIBS@

noinst_LIBRARIES = libapreq_tests.a
libapreq_tests_a_SOURCES = CuTest.c env.c cookie.c params.c parsers.c

check_PROGRAMS = testall
testall_LDADD  = libapreq_tests.a

test:	check
	./testall -v
