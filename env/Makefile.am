INCLUDES = -I../src -I@APACHE2_INCLUDES@

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
