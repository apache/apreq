INCLUDES = -I../src @APACHE2_INCLUDES@ @APR_INCLUDES@ @APU_INCLUDES@
LIBS = ../src/lib@APREQ_LIBNAME@.la @APR_LTLIBS@ @APU_LTLIBS@
TEST_CONFIG_SCRIPT = package Apache::TestMM; filter_args(); generate_script("t/TEST")
EXTRA_DIST = t c-modules

# XXX: should name libapreq2_cgi.a w/ source file libapreq2_cgi.c
lib_LIBRARIES = lib@APREQ_LIBNAME@_cgi.a
lib@APREQ_LIBNAME@_cgi_a_SOURCES = libapreq_cgi.c
#
#check_PROGRAMS = test_cgi
#test_cgi_LDADD = lib@APREQ_LIBNAME@_cgi.a

if BUILD_HTTPD

# mod_apreq.c needs to be built from httpd-2.X, e.g.
#
# % cd ../httpd-2.X;
# % ./configure --with-module=filters:../httpd-apreq-2/env/mod_apreq.c ...
#
# See the INSTALL file for details.

@APACHE2_HTTPD@:
	cd @APACHE2_SRC@ && $(MAKE)

all-local: @APACHE2_HTTPD@

t/TEST : Makefile.am t/TEST.PL
	@PERL@ -MApache::TestMM -e '$(TEST_CONFIG_SCRIPT)' -- -httpd @APACHE2_HTTPD@

else

APACHE2_MODULES=`@APACHE2_APXS@ -q LIBEXECDIR`

noinst_LTLIBRARIES = mod_apreq.la
mod_apreq_la_SOURCES = mod_apreq.c
mod_apreq_la_LDFLAGS = -export-dynamic -module -avoid-version -rpath $(APACHE2_MODULES)

install-exec-local: mod_apreq.la
	@APACHE2_APXS@ -i -a -n apreq mod_apreq.la

t/TEST : Makefile.am t/TEST.PL
	@PERL@ -MApache::TestMM -e '$(TEST_CONFIG_SCRIPT)' -- -apxs @APACHE2_APXS@

endif

run_tests : t/TEST
	@PERL@ t/TEST

test :: all check run_tests

test_clean : cmodules_clean
	-@PERL@ t/TEST -clean
	-rm -rf t/htdocs t/logs t/modules t/TEST

cmodules_clean:
	-cd c-modules && $(MAKE) clean
	-rm c-modules/Makefile c-modules/*/Makefile c-modules/apache_httpd_test.h

clean-local: test_clean
