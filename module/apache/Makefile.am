if MAINTAINER_MODE
APACHE1_INCLUDES = -isystem `@APACHE1_APXS@ -q INCLUDEDIR`
else
APACHE1_INCLUDES = -I `@APACHE1_APXS@ -q INCLUDEDIR`
endif

INCLUDES = $(APACHE1_INCLUDES) @APR_INCLUDES@ @APU_INCLUDES@
LIBS = ../../library/lib@APREQ_LIBNAME@.la @APR_LTLIBS@ @APU_LTLIBS@
LDFLAGS = @APR_LDFLAGS@ @APU_LDFLAGS@
TEST_CONFIG_SCRIPT = package Apache::TestMM; filter_args(); generate_script("t/TEST")
pkgincludedir = $(includedir)/@APREQ_LIBNAME@
pkginclude_HEADERS = apreq_module_apache.h
mod_apreq2_la_LDFLAGS = -export-dynamic -module -avoid-version -rpath `@APACHE1_APXS@ -q LIBEXECDIR`

if HAVE_APACHE1

noinst_LTLIBRARIES = mod_apreq2.la
mod_apreq2_la_SOURCES = apreq_private_apache.h handle.c command.c

install-exec-local: mod_apreq2.la
	$(LIBTOOL) --mode=install cp mod_apreq2.la `@APACHE1_APXS@ -q LIBEXECDIR`

endif
