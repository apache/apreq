INCLUDES = @APR_INCLUDES@ @APU_INCLUDES@
LIBS = @APR_LTLIBS@ @APU_LTLIBS@
BUILT_SOURCES= @APR_LA@ @APU_LA@
lib_LTLIBRARIES = libapreq2.la
libapreq2_la_SOURCES = apreq.c apreq_version.c apreq_cookie.c \
                       apreq_params.c apreq_parsers.c apreq_env.c
pkginclude_HEADERS = apreq.h apreq_version.h apreq_cookie.h \
                  apreq_params.h  apreq_env.h apreq_parsers.h
libapreq2_la_LDFLAGS = -version-info @APREQ_LIBTOOL_VERSION@

pkgincludedir = $(includedir)/@APREQ_LIBNAME@

if BUILD_APR

@APR_LA@:
	cd `@APR_CONFIG@ --srcdir` && make

endif

if BUILD_APU

@APU_LA@: @APR_LA@
	cd `@APU_CONFIG@ --srcdir` && make

endif
