lib_LTLIBRARIES = mod_apreq.la
INCLUDES = -I@APACHE2_INCLUDES@ -I../src
mod_apreq_la_SOURCES = mod_apreq.c
mod_apreq_la_LDFLAGS = -rpath @APACHE2_LIBS@ -dynamic -module
mod_apreq_la_LIBADD = ../src/libapreq.la
