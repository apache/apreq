INCLUDES = -I@APR_INCLUDES@ -I@APU_INCLUDES@
LIBS = @APR_LIBS@ @APU_LIBS@
lib_LTLIBRARIES = libapreq.la
nobase_include_HEADERS = apreq.h apreq_cookie.h apreq_params.h \
                         apreq_env.h apreq_parsers.h
libapreq_la_LDFLAGS = -version-info 2:0 

BUILT_SOURCES= @APR_LA@ @APU_LA@
libapreq_la_SOURCES = apreq.c apreq_cookie.c \
                      apreq_params.c apreq_parsers.c
if BUILD_APR

@APR_LA@:
	cd `@APR_CONFIG@ --srcdir` && make

endif

if BUILD_APU

@APU_LA@: @APR_LA@
	cd `@APU_CONFIG@ --srcdir` && make

endif
