INCLUDES = -I../../include @APACHE2_INCLUDES@ @APR_INCLUDES@ @APU_INCLUDES@
LIBS = ../../library/lib@APREQ_LIBNAME@.la @APR_LTLIBS@ @APU_LTLIBS@
LDFLAGS = @APR_LDFLAGS@ @APU_LDFLAGS@
TEST_CONFIG_SCRIPT = package Apache::TestMM; filter_args(); generate_script("t/TEST")
pkgincludedir = $(includedir)/@APREQ_LIBNAME@
pkginclude_HEADERS = apreq_module_apache2.h
mod_apreq2_la_LDFLAGS = -export-dynamic -module -avoid-version -rpath `@APACHE2_APXS@ -q LIBEXECDIR`

if MAINTAINER_MODE
    AM_CPPFLAGS= -DAP_HAVE_DESIGNATED_INITIALIZER
endif

if BUILD_HTTPD

# mod_apreq.c needs to be built from httpd-2.X, e.g.
#
# % cd ../httpd-2.X;
# % ./configure --with-module=filters:../httpd-apreq-2/module/apache2/mod_apreq2.c ...
#
# See the INSTALL file for details.

@APACHE2_HTTPD@:
	cd @APACHE2_SRC@ && $(MAKE)

all-local: @APACHE2_HTTPD@

else

noinst_LTLIBRARIES = mod_apreq2.la
mod_apreq2_la_SOURCES = apreq_private_apache2.h handle.c filter.c

install-exec-local: mod_apreq.la
	@APACHE2_APXS@ -i -a -n apreq2 mod_apreq2.la

endif
