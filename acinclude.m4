AC_DEFUN([AC_APREQ], [

        AC_ARG_ENABLE(perl_glue,
                AC_HELP_STRING([--enable-perl-glue],[build perl modules Apache::Request and Apache::Cookie]),
                [PERL_GLUE=$enableval],[PERL_GLUE="no"])
        AC_ARG_WITH(perl,
                AC_HELP_STRING([--with-perl],[path to perl executable]),
                [PERL=$withval],[PERL="perl"])
        AC_ARG_WITH(apache2-apxs,
                AC_HELP_STRING([--with-apache2-apxs],[path to apache2's apxs]),
                [APACHE2_APXS=$withval],[APACHE2_APXS="apxs"])
        AC_ARG_WITH(apache2-src,
                AC_HELP_STRING([--with-apache2-src],[path to httpd-2 source]),
                [APACHE2_SRC=$withval],[APACHE2_SRC=""])

        prereq_check="$PERL build/version_check.pl"

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
                APACHE2_HTTPD=`$APACHE2_APXS -q SBINDIR`/httpd
                APR_CONFIG=`$APACHE2_APXS -q APR_BINDIR`/apr-config
                APU_CONFIG=`$APACHE2_APXS -q APU_BINDIR`/apu-config

                if test -z "`$prereq_check apache2 $APACHE2_HTTPD`"; then
                    AC_MSG_ERROR([Bad apache2 version])
                fi
        fi

        AC_CHECK_FILE([$APR_CONFIG],,
            AC_MSG_ERROR([invalid apr-config location- did you forget to configure apr?]))

        if test -z "`$prereq_check apr $APR_CONFIG`"; then
            AC_MSG_ERROR([Bad libapr version])
        fi

        AC_CHECK_FILE([$APU_CONFIG],,
            AC_MSG_ERROR([invalid apu-config location- did you forget to configure apr-util?]))

        if test -z "`$prereq_check apu $APU_CONFIG`"; then
            AC_MSG_ERROR([Bad libaprutil version])
        fi

        if test "x$PERL_GLUE" != "xno"; then
            if test -z "`$prereq_check perl $PERL`"; then
                AC_MSG_ERROR([Bad perl version])
            fi
            if test -z "`$prereq_check ExtUtils::XSBuilder`"; then
                AC_MSG_ERROR([Bad ExtUtils::XSBuilder version])
            fi
            if test -z "`$prereq_check mod_perl`"; then
                AC_MSG_ERROR([Bad mod_perl version])
            fi
            if test -z "`$prereq_check Apache::Test`"; then
                AC_MSG_ERROR([Bad Apache::Test version])
            fi
        fi

        AC_CONFIG_COMMANDS_POST([test "x$PERL_GLUE" != "xno" && 
           (cd glue/perl && $PERL ../../build/xsbuilder.pl run)],
                [PERL=$PERL;PERL_GLUE=$PERL_GLUE;APACHE2_APXS=$APACHE2_APXS])


        AM_CONDITIONAL(BUILD_PERL_GLUE, test "x$PERL_GLUE" != "xno")
        AM_CONDITIONAL(HAVE_APACHE_TEST, test -n "`$prereq_check Apache::Test`")
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

        dnl AC_SUBST(top_builddir)
        dnl AC_SUBST(abs_srcdir)
        dnl AC_SUBST(abs_builddir)

        get_version="$abs_srcdir/build/get-version.sh"
        version_hdr="$abs_srcdir/src/apreq_version.h"

        APREQ_LIBTOOL_VERSION=`$get_version libtool $version_hdr APREQ`
        APREQ_MAJOR_VERSION=`$get_version major $version_hdr APREQ`
        APREQ_DOTTED_VERSION=`$get_version all $version_hdr APREQ`

        APREQ_LIBNAME="apreq$APREQ_MAJOR_VERSION"
        APREQ_INCLUDES=""
        APREQ_LDFLAGS=""
        APREQ_EXPORT_LIBS=""

        echo "lib$APREQ_LIBNAME Version: $APREQ_DOTTED_VERSION"

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

AC_DEFUN([APR_ADDTO],[
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

dnl Iteratively interpolate the contents of the second argument
dnl until interpolation offers no new result. Then assign the
dnl final result to $1.
dnl
dnl Example:
dnl
dnl foo=1
dnl bar='${foo}/2'
dnl baz='${bar}/3'
dnl APR_EXPAND_VAR(fraz, $baz)
dnl   $fraz is now "1/2/3"
dnl 
AC_DEFUN(APR_EXPAND_VAR,[
ap_last=
ap_cur="$2"
while test "x${ap_cur}" != "x${ap_last}";
do
  ap_last="${ap_cur}"
  ap_cur=`eval "echo ${ap_cur}"`
done
$1="${ap_cur}"
])


dnl APR_CONFIG_NICE(filename)
dnl
dnl Saves a snapshot of the configure command-line for later reuse
dnl
AC_DEFUN(APR_CONFIG_NICE,[
  rm -f $1
  cat >$1<<EOF
#! /bin/sh
#
# Created by configure

EOF
  if test -n "$CC"; then
    echo "CC=\"$CC\"; export CC" >> $1
  fi
  if test -n "$CFLAGS"; then
    echo "CFLAGS=\"$CFLAGS\"; export CFLAGS" >> $1
  fi
  if test -n "$CPPFLAGS"; then
    echo "CPPFLAGS=\"$CPPFLAGS\"; export CPPFLAGS" >> $1
  fi
  if test -n "$LDFLAGS"; then
    echo "LDFLAGS=\"$LDFLAGS\"; export LDFLAGS" >> $1
  fi
  if test -n "$LTFLAGS"; then
    echo "LTFLAGS=\"$LTFLAGS\"; export LTFLAGS" >> $1
  fi
  if test -n "$LIBS"; then
    echo "LIBS=\"$LIBS\"; export LIBS" >> $1
  fi
  if test -n "$INCLUDES"; then
    echo "INCLUDES=\"$INCLUDES\"; export INCLUDES" >> $1
  fi
  if test -n "$NOTEST_CFLAGS"; then
    echo "NOTEST_CFLAGS=\"$NOTEST_CFLAGS\"; export NOTEST_CFLAGS" >> $1
  fi
  if test -n "$NOTEST_CPPFLAGS"; then
    echo "NOTEST_CPPFLAGS=\"$NOTEST_CPPFLAGS\"; export NOTEST_CPPFLAGS" >> $1
  fi
  if test -n "$NOTEST_LDFLAGS"; then
    echo "NOTEST_LDFLAGS=\"$NOTEST_LDFLAGS\"; export NOTEST_LDFLAGS" >> $1
  fi
  if test -n "$NOTEST_LIBS"; then
    echo "NOTEST_LIBS=\"$NOTEST_LIBS\"; export NOTEST_LIBS" >> $1
  fi

  for arg in [$]0 "[$]@"; do
    APR_EXPAND_VAR(arg, $arg)
    echo "\"[$]arg\" \\" >> $1
  done
  echo '"[$]@"' >> $1
  chmod +x $1
])dnl
