INCLUDES = -I../src -I@APACHE2_INCLUDES@
LIBS = -L../src

noinst_LIBRARIES = libapreq_tests.a
libapreq_tests_a_SOURCES = CuTest.c env.c tables.c cookie.c
libapreq_tests_a_LIBADD = ../src/libapreq.la

check_PROGRAMS = testall
testall_LDADD =  libapreq_tests.a
testall_LINK = @APACHE2_APXS@ -c -p -l apreq -o testall

test:	check
	./testall