dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT(libapreq - Apache Request Library, 2.01-dev, apreq-dev@httpd.apache.org, libapreq2)

dnl this:
dnl AC_CONFIG_AUX_DIR(build)
dnl causes problems on certain platforms
dnl aclocal.m4:817: required file `build/ltmain.sh' not found

AM_INIT_AUTOMAKE

AC_PROG_MAKE_SET

dnl Checks for programs.
AC_PROG_CC
AM_PROG_LIBTOOL
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LN_S

AM_MAINTAINER_MODE

dnl Checks for header files.
AM_CONFIG_HEADER(src/apreq_config.h)
dnl Checks for typedefs, structures, and compiler characteristics.
dnl Checks for library functions.

AC_APREQ
AC_CONFIG_FILES([Makefile src/Makefile env/Makefile glue/Makefile t/Makefile])
AC_CONFIG_FILES([apreq2-config], [chmod +x apreq2-config])
AC_OUTPUT
