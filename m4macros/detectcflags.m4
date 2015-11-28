dnl GIMP_DETECT_CFLAGS(RESULT, FLAGSET)
dnl Detect if the compiler supports a set of flags

AC_DEFUN([GIMP_DETECT_CFLAGS],
[
  $1=
  for flag in $2; do
    if test -z "[$]$1"; then
      $1_save_CFLAGS="$CFLAGS"
      CFLAGS="$CFLAGS -Werror $flag"
      AC_MSG_CHECKING([whether [$]CC understands [$]flag])
      AC_TRY_COMPILE([], [], [$1_works=yes], [$1_works=no])
      AC_MSG_RESULT([$]$1_works)
      CFLAGS="[$]$1_save_CFLAGS"
      if test "x[$]$1_works" = "xyes"; then
        $1="$flag"
      fi
    fi
  done
])
