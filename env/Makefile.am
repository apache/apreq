INCLUDES = -I../src -I@APACHE2_INCLUDES@ -I@APR_INCLUDES@ -I@APU_INCLUDES@
LIBS = ../src/libapreq.la @APR_LIBS@ @APU_LIBS@
TEST_CONFIG_SCRIPT = package Apache::TestMM; filter_args(); generate_script("t/TEST")

lib_LIBRARIES = libapreq_cgi.a
libapreq_cgi_a_SOURCES = libapreq_cgi.c

if BUILD_HTTPD

# mod_apreq.c needs to be built from httpd-2.X, e.g.
#
# % cd ../httpd-2.X;
# % ./configure --with-module=filters:../httpd-apreq-2/env/mod_apreq.c ...
#
# See the INSTALL file for details.

@APACHE2_HTTPD@:
	cd @APACHE2_SRC@ && make

all-local: @APACHE2_HTTPD@

t/TEST : Makefile.am
	@PERL@ -MApache::TestMM -e '$(TEST_CONFIG_SCRIPT)' -- -httpd @APACHE2_SRC@/.libs/httpd

else

APACHE2_MODULES=`@APACHE2_APXS@ -q LIBEXECDIR`

noinst_LTLIBRARIES = mod_apreq.la
mod_apreq_la_SOURCES = mod_apreq.c
mod_apreq_la_LDFLAGS = -export-dynamic -module -avoid-version -rpath $(APACHE2_MODULES)

install-exec-local: mod_apreq.la
	@APACHE2_APXS@ -i -a -n apreq mod_apreq.la

t/TEST : Makefile.am
	@PERL@ -MApache::TestMM -e '$(TEST_CONFIG_SCRIPT)' -- -apxs @APACHE2_APXS@

endif

run_tests : t/TEST
	@PERL@ t/TEST

test :: all run_tests

test_clean : cmodules_clean
	-@PERL@ t/TEST -clean
	-rm -rf t/conf t/htdocs t/logs t/modules t/TEST

cmodules_clean:
	-cd c-modules && $(MAKE) clean
	-rm c-modules/Makefile c-modules/*/Makefile c-modules/apache_httpd_test.h

clean-local: test_clean
