INCLUDES = -I../src -I@APACHE2_INCLUDES@

TEST_VERBOSITY_LEVEL = 0
TEST_CONFIG_SCRIPT  = ../build/test_config.pl

lib_LIBRARIES = libapreq_cgi.a
noinst_LTLIBRARIES = mod_apreq.la

mod_apreq_la_SOURCES = mod_apreq.c
mod_apreq_la_LDFLAGS = -export-dynamic -module -avoid-version -rpath @APACHE2_MODULES@
mod_apreq_la_LIBADD = ../src/libapreq.la

libapreq_cgi_a_SOURCES = libapreq_cgi.c
libapreq_cgi_a_LIBADD = ../src/libapreq.la

libdir = @APACHE2_LIBS@
includedir = @APACHE2_INCLUDES@

install-exec-local: mod_apreq.la
	@APACHE2_APXS@ -i -a -n apreq mod_apreq.la

t/TEST test_config : Makefile $(TEST_CONFIG_SCRIPT)
	@PERL@ $(TEST_CONFIG_SCRIPT) -apxs @APACHE2_APXS@

run_tests : cmodules
	@PERL@ t/TEST -verbose=$(TEST_VERBOSITY_LEVEL) $(TEST_FILES)

test :: all run_tests

test_clean :
	@PERL@ t/TEST -clean

cmodules: test_config
	cd c-modules && $(MAKE) all

cmodules_clean:
	cd c-modules && $(MAKE) clean

clean-local: test_clean cmodules_clean
