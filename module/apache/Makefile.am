if MAINTAINER_MODE
APACHE1_INCLUDES = -isystem `@APACHE1_APXS@ -q INCLUDEDIR`
else
APACHE1_INCLUDES = -I `@APACHE1_APXS@ -q INCLUDEDIR`
endif

INCLUDES = $(APACHE1_INCLUDES) @APR_INCLUDES@ @APU_INCLUDES@
LIBS = ../../library/lib@APREQ_LIBNAME@.la @APU_LIBS@ @APR_LIBS@
LDFLAGS = @APR_LTFLAGS@ @APU_LTFLAGS@
TEST_CONFIG_SCRIPT = package Apache::TestMM; filter_args(); generate_script("t/TEST")
pkgincludedir = $(includedir)/@APREQ_LIBNAME@
pkginclude_HEADERS = apreq_module_apache.h
mod_apreq2_la_LDFLAGS = -export-dynamic -module -avoid-version -rpath `@APACHE1_APXS@ -q LIBEXECDIR`

if HAVE_APACHE1

noinst_LTLIBRARIES = mod_apreq2.la
mod_apreq2_la_SOURCES = apreq_private_apache.h handle.c command.c

install-exec-local: mod_apreq2.la
	$(LIBTOOL) --mode=install cp mod_apreq2.la `@APACHE1_APXS@ -q LIBEXECDIR`

run_tests : t/TEST
	-cp -Rp .libs t
	MAKE=$(MAKE) @PERL@ t/TEST

t/TEST : Makefile.am t/TEST.PL
	@PERL@ -MApache::TestMM -e '$(TEST_CONFIG_SCRIPT)' -- -apxs @APACHE1_APXS@

test :: all check run_tests

test_clean : cmodules_clean
	-MAKE=$(MAKE) @PERL@ t/TEST -clean
	-rm -rf t/htdocs t/logs t/modules t/TEST t/core t/core.* t/cgi-bin/test_cgi t/cgi-bin/.libs t/.libs t/conf/extra.conf t/conf/ssl/ca t/conf/ssl/httpd-passphrase.pl

cmodules_clean:
	-cd c-modules && $(MAKE) clean
	-rm c-modules/Makefile c-modules/*/Makefile c-modules/apache_httpd_test.h

clean-local: test_clean


endif
