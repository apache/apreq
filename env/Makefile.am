lib_LTLIBRARIES = mod_apreq.la libapreq_cgi.la
INCLUDES = -I@APACHE2_INCLUDES@ -I../src

mod_apreq_la_SOURCES = mod_apreq.c
mod_apreq_la_LDFLAGS = -rpath @APACHE2_LIBS@ -dynamic -module
mod_apreq_la_LIBADD = ../src/libapreq.la
mod_apreq_ls_CPPFLAGS = -DLIBDIR=@APACHE2_MODULES@


libapreq_cgi_la_SOURCES = libapreq_cgi.c
libapreq_cgi_la_LDFLAGS = -rpath @APACHE2_LIBS@ -version-info 2:0
libapreq_cgi_la_LIBADD = ../src/libapreq.la
libapreq_cgi_la_CPPFLAGS = -DLIBDIR=@APACHE2_LIBS@
