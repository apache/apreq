INCLUDES = -I../src -I@APR_INCLUDES@ -I@APU_INCLUDES@
LIBS = -L../src @APR_LIBS@ @APU_LIBS@

noinst_LIBRARIES = libapreq_tests.a
libapreq_tests_a_SOURCES = CuTest.c env.c cookie.c params.c parsers.c
libapreq_tests_a_LIBADD = ../src/libapreq.la

check_PROGRAMS = testall
testall_LDADD =  libapreq_tests.a
testall_LINK = @APACHE2_APXS@ -c -p -L../src -lapreq -o testall

test:	check
	./testall -v
