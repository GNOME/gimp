AC_CHECK_FUNCS(vsnprintf,AC_DEFINE(HAVE_VSNPRINTF),[
   AC_MSG_WARN(vsnprintf not found.. I hope you are using gcc...)
])

AC_MSG_CHECKING(for intelligent life)
AC_MSG_RESULT(not found)

dnl disable some warnings I don't want to see
dnl disabled, since $GCC does not correspond to perl's $(CC)
dnl if test "x$GCC" = xyes; then
dnl    nowarn="-Wno-parentheses -Wno-unused -Wno-uninitialized"
dnl    GIMP_CFLAGS="$GIMP_CFLAGS $nowarn"
dnl    GIMP_CFLAGS_NOUI="$GIMP_CFLAGS_NOUI $nowarn"
dnl fi

AC_SUBST(EXTENSIVE_TESTS)dnl from Makefile.PL

AC_SUBST(CPPFLAGS)
AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(prefix)

AC_SUBST(IN_GIMP)
AC_SUBST(PERL_COMPAT_CRUFT)

AC_SUBST(GIMP_CFLAGS)
AC_SUBST(GIMP_CFLAGS_NOUI)
AC_SUBST(GIMP_LIBS)
AC_SUBST(GIMP_LIBS_NOUI)
AC_SUBST(PERL)
AC_SUBST(GIMP)
AC_SUBST(GIMPTOOL)
AC_SUBST(GIMP_CFLAGS)
AC_SUBST(GIMP_LIBS)
AC_CHECK_FUNCS(_exit)


