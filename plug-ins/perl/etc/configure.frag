AC_CHECK_FUNCS(vsnprintf,AC_DEFINE(HAVE_VSNPRINTF),[
   AC_MSG_WARN(vsnprintf not found.. I hope you are using gcc...)
])

dnl disable some warnings I don't want to see
if test "x$GCC" = xyes; then
   nowarn="-Wno-parentheses -Wno-unused -Wno-uninitialized"
   GIMP_CFLAGS="$GIMP_CFLAGS $nowarn"
   GIMP_CFLAGS_NOUI="$GIMP_CFLAGS"
fi

AC_SUBST(EXTENSIVE_TESTS)dnl from Makefile.PL

AC_SUBST(CPPFLAGS)
AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(prefix)

AC_SUBST(IN_GIMP)

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


