INCLUDES = -I@APACHE2_INCLUDES@
LIBS = -L@APACHE2_LIBS@

lib_LTLIBRARIES = libapreq.la
nobase_include_HEADERS = apreq.h apreq_cookie.h apreq_params.h \
                         apreq_tables.h apreq_env.h apreq_parsers.h
libapreq_la_SOURCES = apreq.c apreq_tables.c apreq_cookie.c \
                      apreq_params.c apreq_parsers.c
libapreq_la_LDFLAGS = -lapr-0 -laprutil-0 -version-info 2:0 

libdir=@APACHE2_LIBS@
includedir=@APACHE2_INCLUDES@
