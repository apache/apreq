INCLUDES = -I@APACHE2_INCLUDES@ -I../src
LIBS = -L@APACHE2_LIBS@

noinst_LIBRARIES = libapreq_tests.a
libapreq_tests_a_SOURCES = CuTest.c env.c tables.c
libapreq_tests_a_LIBADD = ../src/libapreq.la

check_PROGRAMS = testall
testall_LDADD =  libapreq_tests.a
testall_LINK = @APACHE2_APXS@ -c -p -o testall

test:	check
	./testall