dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.13)
AC_INIT("libapreq - Apache Request Library", 2.0, apreq-dev@httpd.apache.org, libapreq)
AM_INIT_AUTOMAKE
dnl AC_CONFIG_AUX_DIR(./build)

AC_PROG_MAKE_SET

dnl Checks for programs.
AC_PROG_CC
AM_PROG_LIBTOOL
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LN_S

dnl AC_ARG_ENABLE(maintainer-mode,[  --enable-maintainer-mode  Turn on debugging 
dnl and compile time warnings],
dnl   [APR_ADDTO(CFLAGS,-g)
dnl   if test "$GCC" = "yes"; then
dnl     APR_ADDTO(CFLAGS,[-Wall -Wmissing-prototypes -Wstrict-prototypes 
dnl                       -Wmissing-declarations])
dnl   elif test "$AIX_XLC" = "yes"; then
dnl     APR_ADDTO(CFLAGS,-qfullpath -qinitauto=FE -qwarn64 -qcheck=all)
dnl   fi
dnl ])dnl

AM_MAINTAINER_MODE

dnl Checks for header files.

dnl Checks for typedefs, structures, and compiler characteristics.

dnl Checks for library functions.

AC_APREQ
AC_OUTPUT([Makefile src/Makefile env/Makefile t/Makefile])
