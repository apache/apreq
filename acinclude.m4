AC_DEFUN(AC_APREQ, [
	AC_ARG_WITH(apache2,
		[  --with-apache2  the apache-2 server_root directory],
		[APACHE2=$withval],
		[APACHE2="/usr/local/apache2"])
	APACHE2_INCLUDES="$APACHE2/include"
        APACHE2_MODULES="$APACHE2/modules"
        APACHE2_LIBS="$APACHE2/lib"
	AC_SUBST(APACHE2_INCLUDES)
        AC_SUBST(APACHE2_MODULES)
        AC_SUBST(APACHE2_LIBS)
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

