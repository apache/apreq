INCLUDES = -I../src @APACHE2_INCLUDES@ @APR_INCLUDES@ @APU_INCLUDES@
LIBS = ../src/lib@APREQ_LIBNAME@.la @APR_LTLIBS@ @APU_LTLIBS@
LDFLAGS = @APR_LDFLAGS@ @APU_LDFLAGS@
TEST_CONFIG_SCRIPT = package Apache::TestMM; filter_args(); generate_script("t/TEST")
EXTRA_DIST = t c-modules
pkgincludedir = $(includedir)/@APREQ_LIBNAME@
pkginclude_HEADERS = apreq_env_apache2.h
mod_apreq_la_LDFLAGS = -export-dynamic -module -avoid-version -rpath `@APACHE2_APXS@ -q LIBEXECDIR`

check_PROGRAMS = test_cgi

if MAINTAINER_MODE
    AM_CPPFLAGS= -DAP_HAVE_DESIGNATED_INITIALIZER
endif

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

noinst_LTLIBRARIES = mod_apreq.la
mod_apreq_la_SOURCES = mod_apreq.c

install-exec-local: mod_apreq.la
	@APACHE2_APXS@ -i -a -n apreq mod_apreq.la

t/TEST : Makefile.am t/TEST.PL
	@PERL@ -MApache::TestMM -e '$(TEST_CONFIG_SCRIPT)' -- -apxs @APACHE2_APXS@

endif

if HAVE_APACHE_TEST

run_tests : t/TEST
	-cp -f test_cgi t/cgi-bin
	-cp -Rp .libs t
	MAKE=$(MAKE) @PERL@ t/TEST
else

run_tests:
	@echo "Apache::Test not available, skipping env/ tests."

endif

test :: all check run_tests

test_clean : cmodules_clean
	-MAKE=$(MAKE) @PERL@ t/TEST -clean
	-rm -rf t/htdocs t/logs t/modules t/TEST t/core t/core.* t/cgi-bin/test_cgi t/cgi-bin/.libs t/.libs t/conf/extra.conf t/conf/ssl/ca t/conf/ssl/httpd-passphrase.pl

cmodules_clean:
	-cd c-modules && $(MAKE) clean
	-rm c-modules/Makefile c-modules/*/Makefile c-modules/apache_httpd_test.h

clean-local: test_clean
