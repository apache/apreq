TEST_CONFIG_SCRIPT = package Apache::TestMM; filter_args(); generate_script("t/TEST")
pkginclude_HEADERS = apreq_module_apache2.h
mod_apreq2_la_LDFLAGS = -export-dynamic -module -avoid-version @APR_LTFLAGS@ \
                        `@APREQ_CONFIG@ --link-libtool --libs`
mod_apreq2_la_SOURCES = apreq_private_apache2.h handle.c filter.c


if MAINTAINER_MODE

AM_CPPFLAGS = -DAP_HAVE_DESIGNATED_INITIALIZER @APACHE2_INCLUDES@ @APR_INCLUDES@

else

AM_CPPFLAGS = @APACHE2_INCLUDES@ @APR_INCLUDES@

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

pkgincludedir = `@APACHE2_APXS@ -q INCLUDEDIR`/@APREQ_LIBNAME@
pkglibdir = `@APACHE2_APXS@ -q LIBEXECDIR`
pkglib_LTLIBRARIES = mod_apreq2.la

endif
