AC_DEFUN(AC_APREQ, [
	AC_ARG_WITH(apache-includes,
		[  --with-apache-includes  where the apache header files live],
		[APACHE_INCLUDES=$withval],
		[APACHE_INCLUDES="/usr/local/apache/include"])
	APREQ_INCLUDES="-I$APACHE_INCLUDES"
	AC_SUBST(APREQ_INCLUDES)
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

