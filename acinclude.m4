AC_DEFUN(AC_APREQ, [
        AC_ARG_WITH(perl,
                AC_HELP_STRING([--with-perl],[path to perl executable]),
                [PERL=$withval],[PERL="perl"])
        AC_ARG_WITH(apache2-apxs,
                AC_HELP_STRING([--with-apache2-apxs],[path to apache2's apxs]),
                [APACHE2_APXS=$withval],[APACHE2_APXS="apxs"])
        AC_ARG_WITH(apache2-src,
                AC_HELP_STRING([--with-apache2-src],[path to httpd-2 source]),
                [APACHE2_SRC=$withval],[APACHE2_SRC=""])

        if test -n "$APACHE2_SRC"; then
                APACHE2_SRC=`cd $APACHE2_SRC;pwd`

                AC_CHECK_FILE([$APACHE2_SRC/include/httpd.h],,
                    AC_MSG_ERROR([invalid Apache2 source directory]))

                APACHE2_INCLUDES=-I$APACHE2_SRC/include
                APACHE2_HTTPD=$APACHE2_SRC/httpd
                AC_ARG_WITH(apr-config,
                    AC_HELP_STRING([  --with-apr-config],[path to apr-config (requires --with-apache2-src)]),
                    [APR_CONFIG=$withval],[APR_CONFIG="$APACHE2_SRC/srclib/apr/apr-config"])
                AC_ARG_WITH(apu-config,
                    AC_HELP_STRING([  --with-apu-config],[path to apu-config (requires --with-apache2-src)]),
                    [APU_CONFIG=$withval],[APU_CONFIG="$APACHE2_SRC/srclib/apr-util/apu-config"])

        else
                APACHE2_INCLUDES=-I`$APACHE2_APXS -q INCLUDEDIR`
                APACHE2_HTTPD=`$APACHE2_APXS -q BINDIR`/httpd
                APR_CONFIG=`$APACHE2_APXS -q APR_BINDIR`/apr-config
                APU_CONFIG=`$APACHE2_APXS -q APU_BINDIR`/apu-config
 
        fi

        AC_CHECK_FILE([$APR_CONFIG],,
            AC_MSG_ERROR([invalid apr-config location- did you forget to configure apr?]))
        AC_CHECK_FILE([$APU_CONFIG],,
            AC_MSG_ERROR([invalid apu-config location- did you forget to configure apr-util?]))

        AM_CONDITIONAL(BUILD_HTTPD, test -n "$APACHE2_SRC")
        AM_CONDITIONAL(BUILD_APR, test "x$APR_CONFIG" = x`$APR_CONFIG --srcdir`/apr-config)
        AM_CONDITIONAL(BUILD_APU, test "x$APU_CONFIG" = x`$APU_CONFIG --srcdir`/apu-config)

        dnl Reset the default installation prefix to be the same as apu's
        ac_default_prefix=`$APU_CONFIG --prefix`

        APR_INCLUDES=`$APR_CONFIG --includes`
        APU_INCLUDES=`$APU_CONFIG --includes`
        APR_LA=`$APR_CONFIG --link-libtool`
        APU_LA=`$APU_CONFIG --link-libtool`
        APR_LTLIBS=`$APR_CONFIG --link-libtool --libs`
        APU_LTLIBS=`$APU_CONFIG --link-libtool --libs`
        dnl perl glue/tests do not use libtool: need ld linker flags
        APR_LDLIBS=`$APR_CONFIG --link-ld --libs`
        APU_LDLIBS=`$APU_CONFIG --link-ld --libs`

        dnl Absolute source/build directory
        abs_srcdir=`(cd $srcdir && pwd)`
        abs_builddir=`pwd`
        top_builddir="$abs_srcdir"

        if test "$abs_builddir" != "$abs_srcdir"; then
          USE_VPATH=1
        fi

        AC_SUBST(top_builddir)
        AC_SUBST(abs_srcdir)
        AC_SUBST(abs_builddir)

        get_version="$abs_srcdir/build/get-version.sh"
        version_hdr="$abs_srcdir/src/apreq_version.h"

        APREQ_LIBTOOL_VERSION=`$get_version libtool $version_hdr APREQ`
        APREQ_MAJOR_VERSION=`$get_version major $version_hdr APREQ`
        APREQ_DOTTED_VERSION=`$get_version all $version_hdr APREQ`

        echo "libapreq Version: $APREQ_DOTTED_VERSION"

        APREQ_LIBNAME="apreq$APREQ_MAJOR_VERSION"
        APREQ_INCLUDES=""
        APREQ_LDFLAGS=""
        APREQ_EXPORT_LIBS=""

        AC_SUBST(APREQ_LIBNAME)
        AC_SUBST(APREQ_LDFLAGS)
        AC_SUBST(APREQ_INCLUDES)
        AC_SUBST(APREQ_EXPORT_LIBS)
        AC_SUBST(APREQ_LIBTOOL_VERSION)
        AC_SUBST(APREQ_MAJOR_VERSION)
        AC_SUBST(APREQ_DOTTED_VERSION)

        AC_SUBST(APACHE2_APXS)
        AC_SUBST(APACHE2_SRC)
        AC_SUBST(APACHE2_INCLUDES)
        AC_SUBST(APACHE2_HTTPD)

        AC_SUBST(APU_CONFIG)
        AC_SUBST(APR_CONFIG)
        AC_SUBST(APR_INCLUDES)
        AC_SUBST(APU_INCLUDES)
        AC_SUBST(APR_LTLIBS)
        AC_SUBST(APU_LTLIBS)
        AC_SUBST(APR_LDLIBS)
        AC_SUBST(APU_LDLIBS)
        AC_SUBST(APR_LA)
        AC_SUBST(APU_LA)
        AC_SUBST(PERL)
])

AC_DEFUN(APR_ADDTO,[
  if test "x$$1" = "x"; then
    echo "  setting $1 to \"$2\""
    $1="$2"
  else
    apr_addto_bugger="$2"
    for i in $apr_addto_bugger; do
      apr_addto_duplicate="0"
      for j in $$1; do
        if test "x$i" = "x$j"; then
          apr_addto_duplicate="1"
          break
        fi
      done
      if test $apr_addto_duplicate = "0"; then
        echo "  adding \"$i\" to $1"
        $1="$$1 $i"
      fi
    done
  fi
])
