# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.54)

# Making releases on the stable branch:
#   GIMP_MICRO_VERSION += 1;
#   GIMP_INTERFACE_AGE += 1;

m4_define([gimp_major_version], [2])
m4_define([gimp_minor_version], [7])
m4_define([gimp_micro_version], [1])
m4_define([gimp_real_version],
          [gimp_major_version.gimp_minor_version.gimp_micro_version])
m4_define([gimp_version], [gimp_real_version])
m4_define([gimp_interface_age], [0])
m4_define([gimp_binary_age],
          [m4_eval(100 * gimp_minor_version + gimp_micro_version)])

# For overriding the version string. Comment out if not needed.
# m4_define([gimp_version], [2.7.0])

# This is the X.Y used in -lgimp-X.Y
m4_define([gimp_api_version], [2.0])

# Versions used for apps, plugins, tools, pkg-config files, and data,
# as well as global and user prefs
m4_define([gimp_app_version], [2.7])
m4_define([gimp_plugin_version], [2.0])
m4_define([gimp_tool_version], [2.0])
m4_define([gimp_pkgconfig_version], [2.0])
m4_define([gimp_data_version], [2.0])
m4_define([gimp_sysconf_version], [2.0])
m4_define([gimp_user_version], [2.7])

m4_define([gimp_unstable],
          m4_if(m4_eval(gimp_minor_version % 2), [1], [yes], [no]))
m4_define([gimp_stable],
          m4_if(m4_eval(gimp_minor_version % 2), [0], [yes], [no]))

m4_define([gimp_full_name], [GNU Image Manipulation Program])

# required versions of other packages
m4_define([babl_required_version], [0.1.0])
m4_define([gegl_required_version], [0.1.0])
m4_define([glib_required_version], [2.22.0])
m4_define([gtk_required_version], [2.18.1])
m4_define([gdk_pixbuf_required_version], [gtk_required_version])
m4_define([cairo_required_version], [1.6.0])
m4_define([pangocairo_required_version], [1.20.1])
m4_define([fontconfig_required_version], [2.2.0])
m4_define([gtkdoc_required_version], [1.0])
m4_define([webkit_required_version], [1.1.0])
m4_define([alsa_required_version], [1.0.0])
m4_define([rsvg_required_version], [2.14.0])
m4_define([wmf_required_version], [0.2.8])
m4_define([pygtk_required_version], [2.10.4])
m4_define([poppler_required_version], [0.4.1])
m4_define([libcurl_required_version], [7.15.1])
m4_define([dbus_glib_required_version], [0.70])
m4_define([libhal_required_version], [0.5.7])
m4_define([exif_required_version], [0.6.15])
m4_define([lcms_required_version], [1.16])


AC_INIT([GIMP], [gimp_version],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=GIMP],
        [gimp])

AC_CONFIG_SRCDIR([app/core/gimp.c])

AM_INIT_AUTOMAKE(no-define dist-bzip2)
AM_CONFIG_HEADER(config.h)

# Enable silent build rules by default, requires at least
# Automake-1.11. Disable by either passing --disable-silent-rules to
# configure or passing V=1 to make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

GIMP_MAJOR_VERSION=gimp_major_version
GIMP_MINOR_VERSION=gimp_minor_version
GIMP_MICRO_VERSION=gimp_micro_version
GIMP_INTERFACE_AGE=gimp_interface_age
GIMP_BINARY_AGE=gimp_binary_age
GIMP_VERSION=gimp_version
GIMP_REAL_VERSION=gimp_real_version
GIMP_API_VERSION=gimp_api_version
GIMP_APP_VERSION=gimp_app_version
GIMP_PLUGIN_VERSION=gimp_plugin_version
GIMP_TOOL_VERSION=gimp_tool_version
GIMP_PKGCONFIG_VERSION=gimp_pkgconfig_version
GIMP_DATA_VERSION=gimp_data_version
GIMP_SYSCONF_VERSION=gimp_sysconf_version
GIMP_USER_VERSION=gimp_user_version
GIMP_UNSTABLE=gimp_unstable
GIMP_FULL_NAME="gimp_full_name"
AC_SUBST(GIMP_MAJOR_VERSION)
AC_SUBST(GIMP_MINOR_VERSION)
AC_SUBST(GIMP_MICRO_VERSION)
AC_SUBST(GIMP_INTERFACE_AGE)
AC_SUBST(GIMP_BINARY_AGE)
AC_SUBST(GIMP_VERSION)
AC_SUBST(GIMP_REAL_VERSION)
AC_SUBST(GIMP_API_VERSION)
AC_SUBST(GIMP_APP_VERSION)
AC_SUBST(GIMP_PLUGIN_VERSION)
AC_SUBST(GIMP_TOOL_VERSION)
AC_SUBST(GIMP_PKGCONFIG_VERSION)
AC_SUBST(GIMP_DATA_VERSION)
AC_SUBST(GIMP_SYSCONF_VERSION)
AC_SUBST(GIMP_USER_VERSION)
AC_SUBST(GIMP_UNSTABLE)
AC_SUBST(GIMP_FULL_NAME)


# These are used in the .pc files
GLIB_REQUIRED_VERSION=glib_required_version
GTK_REQUIRED_VERSION=gtk_required_version
AC_SUBST(GLIB_REQUIRED_VERSION)
AC_SUBST(GTK_REQUIRED_VERSION)

# The symbol GIMP_UNSTABLE is defined above for substitution in
# Makefiles and conditionally defined here as a preprocessor symbol
# and automake conditional.
if test "x$GIMP_UNSTABLE" = "xyes"; then
  AC_DEFINE(GIMP_UNSTABLE, 1,
            [Define to 1 if this is an unstable version of GIMP])
fi
AM_CONDITIONAL(GIMP_UNSTABLE, test "x$GIMP_UNSTABLE" = "xyes")


# libtool versioning
m4_define([lt_current], [m4_eval(100 * gimp_minor_version + gimp_micro_version - gimp_interface_age)])
m4_define([lt_revision], [gimp_interface_age])
m4_define([lt_age], [m4_eval(gimp_binary_age - gimp_interface_age)])
LT_VERSION_INFO="lt_current:lt_revision:lt_age"
LT_CURRENT_MINUS_AGE=m4_eval(lt_current - lt_age)
AC_SUBST(LT_VERSION_INFO)
AC_SUBST(LT_CURRENT_MINUS_AGE)


# gettext i18n support
GETTEXT_PACKAGE=gimp20
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
                   [The prefix for our gettext translation domains.])

# Determine a C compiler to use
AC_PROG_CC
AM_PROG_CC_C_O

# Initialize libtool
AM_DISABLE_STATIC
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

AM_PROG_AS

# Initialize maintainer mode
AM_MAINTAINER_MODE

# Honor aclocal flags
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"


# Check for pkg-config
PKG_PROG_PKG_CONFIG(0.16)


###########################
# Check target architecture
###########################

AC_MSG_CHECKING([for target architecture])
case x"$target" in
  xNONE | x)
    target_or_host="$host" ;;
  *)
    target_or_host="$target" ;;
esac
AC_MSG_RESULT([$target_or_host])

case "$target_or_host" in
  i*86-*-*)
    have_x86=yes
    AC_DEFINE(ARCH_X86, 1, [Define to 1 if you are compiling for ix86.])
    ;;
  x86_64-*-*)
    have_x86=yes
    AC_DEFINE(ARCH_X86, 1, [Define to 1 if you are compiling for ix86.])
    AC_DEFINE(ARCH_X86_64, 1, [Define to 1 if you are compiling for amd64.])
    ;;
  ppc-*-* | powerpc-*)
    have_ppc=yes
    AC_DEFINE(ARCH_PPC, 1, [Define to 1 if you are compiling for PowerPC.])
    ;;
  ppc64-*-* | powerpc64-*)
    have_ppc=yes
    AC_DEFINE(ARCH_PPC, 1, [Define to 1 if you are compiling for PowerPC.])
    AC_DEFINE(ARCH_PPC64, 1, [Define to 1 if you are compiling for PowerPC64.])
    ;;
  *)
    ;;
esac


#################
# Check for Win32
#################

AC_MSG_CHECKING([for some Win32 platform])
case "$target_or_host" in
  *-*-mingw* | *-*-cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = "yes")

AC_MSG_CHECKING([for native Win32])
case "$target_or_host" in
  *-*-mingw*)
    os_win32=yes
    case "$host" in
      x86_64-*-*)
	;;
      *)
	WIN32_LARGE_ADDRESS_AWARE='-Wl,--large-address-aware'
	;;
    esac
    PATHSEP=';'
    ;;
  *)
    os_win32=no
    PATHSEP=':'
    ;;
esac
AC_MSG_RESULT([$os_win32])
AC_SUBST(WIN32_LARGE_ADDRESS_AWARE)
AC_SUBST(PATHSEP)
AM_CONDITIONAL(OS_WIN32, test "$os_win32" = "yes")
AM_CONDITIONAL(OS_UNIX, test "$os_win32" != "yes")

if test "$os_win32" = "yes"; then
  AC_CHECK_PROG(ms_librarian, lib.exe, yes, no)
  AC_CHECK_TOOL(WINDRES, windres, :)
else
  WINDRES=":"
fi

AM_CONDITIONAL(MS_LIB_AVAILABLE, test "x$ms_librarian" = xyes)
AM_CONDITIONAL(HAVE_WINDRES, test "x$WINDRES" != "x:")

AC_SUBST(WINDRES)


###############
# Generic stuff
###############

# Checks for programs.
AC_ISC_POSIX
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Large file support for the swap file
AC_SYS_LARGEFILE

AC_MSG_CHECKING([whether to turn on debugging])
AC_ARG_ENABLE(debug,
              [  --enable-debug          turn on debugging (default=no)],
              if eval "test x$enable_debug = xyes"; then
                DEBUGFLAG="-g"
              fi)

if test -n "$DEBUGFLAG"; then
  AC_MSG_RESULT([yes])
  CFLAGS="$DEBUGFLAG $CFLAGS"
else
  AC_MSG_RESULT([no])
fi

AC_MSG_CHECKING([whether to turn on profiling])
AC_ARG_ENABLE(profile,
              [  --enable-profile        turn on profiling (default=no)],
              if eval "test x$enable_profile = xyes"; then
                PROFFLAG="-pg"
              fi)

if test -n "$PROFFLAG"; then
  AC_MSG_RESULT([yes])
  CFLAGS="$PROFFLAG $CFLAGS"
else
  AC_MSG_RESULT([no])
fi

AC_ARG_ENABLE(ansi,
              [  --enable-ansi           turn on strict ansi (default=no)],,
              enable_ansi=no)

changequote(,)dnl
if eval "test x$GCC = xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac

  if test "x$enable_ansi" = "xyes"; then
    case " $CFLAGS " in
    *[\ \	]-ansi[\ \	]*) ;;
    *) CFLAGS="$CFLAGS -ansi" ;;
    esac

    case " $CFLAGS " in
    *[\ \	]-pedantic[\ \	]*) ;;
    *) CFLAGS="$CFLAGS -pedantic" ;;
    esac
  fi
fi
changequote([,])dnl


# If possible, warn if C99isms are used
GIMP_DETECT_CFLAGS(extra_warnings, '-Wdeclaration-after-statement')
CFLAGS="$CFLAGS $extra_warnings"

GIMP_DETECT_CFLAGS(extra_warnings, '-Wmissing-prototypes')
CFLAGS="$CFLAGS $extra_warnings"

GIMP_DETECT_CFLAGS(extra_warnings, '-Wstrict-prototypes')
CFLAGS="$CFLAGS $extra_warnings"

GIMP_DETECT_CFLAGS(extra_warnings, '-Wmissing-declarations')
CFLAGS="$CFLAGS $extra_warnings"

GIMP_DETECT_CFLAGS(extra_warnings, '-Winit-self')
CFLAGS="$CFLAGS $extra_warnings"

GIMP_DETECT_CFLAGS(extra_warnings, '-Wpointer-arith')
CFLAGS="$CFLAGS $extra_warnings"

GIMP_DETECT_CFLAGS(extra_warnings, '-Wold-style-definition')
CFLAGS="$CFLAGS $extra_warnings"

GIMP_DETECT_CFLAGS(extra_warnings, '-Wmissing-format-attribute')
CFLAGS="$CFLAGS $extra_warnings"

GIMP_DETECT_CFLAGS(extra_warnings, '-Wformat-security')
CFLAGS="$CFLAGS $extra_warnings"

GIMP_DETECT_CFLAGS(extra_warnings, '-Wformat-non-literal')
CFLAGS="$CFLAGS $extra_warnings"


# Ensure MSVC-compatible struct packing convention is used when
# compiling for Win32 with gcc.
# What flag to depends on gcc version: gcc3 uses "-mms-bitfields", while
# gcc2 uses "-fnative-struct".
if test "x$os_win32" = xyes; then
  if test "x$GCC" = xyes; then
    msnative_struct=''
    AC_MSG_CHECKING([how to get MSVC-compatible struct packing])
    if test -z "$ac_cv_prog_CC"; then
      our_gcc="$CC"
    else
      our_gcc="$ac_cv_prog_CC"
    fi
    case `$our_gcc --version | sed -e 's,\..*,.,' -e q` in
      2.)
	if $our_gcc -v --help 2>/dev/null | grep fnative-struct >/dev/null; then
	  msnative_struct='-fnative-struct'
	fi
	;;
      *)
	if $our_gcc -v --help 2>/dev/null | grep ms-bitfields >/dev/null; then
	  msnative_struct='-mms-bitfields'
	fi
	;;
    esac
    if test "x$msnative_struct" = x ; then
      AC_MSG_RESULT([no way])
      AC_MSG_WARN([build will be incompatible with GTK+ DLLs])
    else
      CFLAGS="$CFLAGS $msnative_struct"
      AC_MSG_RESULT([${msnative_struct}])
    fi
  fi
fi


AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_TIME

AC_CHECK_HEADERS(sys/param.h sys/time.h sys/times.h sys/wait.h unistd.h)

AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF

AC_FUNC_ALLOCA

# check some more funcs
AC_CHECK_FUNCS(fsync)
AC_CHECK_FUNCS(difftime mmap)


AM_BINRELOC


# _NL_MEASUREMENT_MEASUREMENT is an enum and not a define
AC_MSG_CHECKING([for _NL_MEASUREMENT_MEASUREMENT])
AC_TRY_LINK([#include <langinfo.h>], [
char c = *((unsigned char *)  nl_langinfo(_NL_MEASUREMENT_MEASUREMENT));
], nl_ok=yes, nl_ok=no)
AC_MSG_RESULT($nl_ok)
if test "$nl_ok" = "yes"; then
  AC_DEFINE(HAVE__NL_MEASUREMENT_MEASUREMENT, 1,
	    [Define to 1 if _NL_MEASUREMENT_MEASUREMENT is available])
fi


######################
# Internationalisation
######################

IT_PROG_INTLTOOL(0.40.1)
AM_GLIB_GNU_GETTEXT


###########
# iso-codes
###########

AC_MSG_CHECKING([for iso-codes])
PKG_CHECK_EXISTS(iso-codes,
  have_iso_codes="yes"
  AC_DEFINE(HAVE_ISO_CODES, 1,
  	    [Define to 1 if the iso-codes package is available])
  ISO_CODES_PREFIX=`$PKG_CONFIG --variable=prefix iso-codes`
  ISO_CODES_LOCATION="$ISO_CODES_PREFIX/share/xml/iso-codes"
  ISO_CODES_LOCALEDIR="$ISO_CODES_PREFIX/$DATADIRNAME/locale",
  have_iso_codes="no (iso-codes package not found)")
AC_MSG_RESULT($have_iso_codes)

AC_SUBST(ISO_CODES_LOCATION)
AC_SUBST(ISO_CODES_LOCALEDIR)


###############################
# Checks for required libraries
###############################

PKG_CHECK_MODULES(BABL, babl >= babl_required_version)
PKG_CHECK_MODULES(GEGL, gegl >= gegl_required_version)

AM_PATH_GLIB_2_0(glib_required_version, :,
  AC_MSG_ERROR(Test for GLIB failed. See the file 'INSTALL' for help.),
  gobject gthread)

PKG_CHECK_MODULES(GMODULE_NO_EXPORT, gmodule-no-export-2.0)

PKG_CHECK_MODULES(GIO, gio-2.0)

AC_MSG_CHECKING([if GLib is version 2.23.0 or newer])
if $PKG_CONFIG --atleast-version=2.23.0 glib-2.0; then
  have_glib_2_23=yes
else
  have_glib_2_23=no
fi
AC_MSG_RESULT($have_glib_2_21)


# Check for bind_textdomain_codeset, including -lintl if GLib brings it in.
gimp_save_LIBS=$LIBS
LIBS="$LIBS $GLIB_LIBS"
AC_CHECK_FUNCS(bind_textdomain_codeset,,
  AC_MSG_ERROR([
*** Check for bind_textdomain_codeset() failed!  This is required.
]))
LIBS=$gimp_save_LIBS


AC_PATH_XTRA

AM_PATH_GTK_2_0(gtk_required_version, :,
  AC_MSG_ERROR(Test for GTK+ failed. See the file 'INSTALL' for help.),
  gthread)

AC_MSG_CHECKING([if GTK+ is version 2.19.0 or newer])
if $PKG_CONFIG --atleast-version=2.19.0 gtk+-2.0; then
  have_gtk_2_19=yes
else
  have_gtk_2_19=no
fi
AC_MSG_RESULT($have_gtk_2_15)

PKG_CHECK_MODULES(GDK_PIXBUF, gdk-pixbuf-2.0 >= gdk_pixbuf_required_version)

AC_PATH_PROG(GDK_PIXBUF_CSOURCE, gdk-pixbuf-csource, no)
if test "x$GDK_PIXBUF_CSOURCE" = xno; then
  AC_MSG_ERROR(Could not find gdk-pixbuf-csource in your PATH)
fi

PKG_CHECK_MODULES(CAIRO, cairo >= cairo_required_version)

PKG_CHECK_MODULES(PANGOCAIRO,
	          pangocairo >= pangocairo_required_version pangoft2, :,
  AC_MSG_ERROR([Test for PangoCairo failed.
*** We require Pango with the optional support for Cairo compiled in.
*** See the file 'INSTALL' for more help.]))

gimp_save_CFLAGS=$CFLAGS
CFLAGS="$PANGOCAIRO_CFLAGS $CFLAGS"

PKG_CHECK_MODULES(FONTCONFIG, fontconfig >= fontconfig_required_version)

AC_MSG_CHECKING([if Pango is version 1.21.0 or newer])
if $PKG_CONFIG --atleast-version=1.21.0 pango; then
  have_pango_1_21=yes
else
  have_pango_1_21=no
fi
AC_MSG_RESULT($have_pango_1_21)

CFLAGS="$PANGOCAIRO_CFLAGS $CFLAGS"

AC_MSG_CHECKING([if Pango is built with a recent fontconfig])
AC_TRY_COMPILE([#include <fontconfig/fontconfig.h>],
  [FcObjectSet *os; os = FcObjectSetBuild (FC_FAMILY, FC_WIDTH);],
  fontconfig_ok=yes, fontconfig_ok=no)
AC_MSG_RESULT($fontconfig_ok)

CFLAGS=$gimp_save_CFLAGS

if test "x$fontconfig_ok" = xno; then
  AC_MSG_ERROR([
*** You have a fontconfig >= fontconfig_required_version installed on your system, but your
*** Pango library is using an older version. This old version is probably in
*** /usr/X11R6. Look at the above output, and note that the result for
*** FONTCONFIG_CFLAGS is not in the result for PANGOCAIRO_CFLAGS, and that
*** there is likely an extra -I line, other than the ones for GLIB,
*** Freetype, and Pango itself. That's where your old fontconfig files are.
*** Rebuild pango, and make sure that it uses the newer fontconfig. The
*** easiest way be sure of this is to simply get rid of the old fontconfig.
*** When you rebuild pango, make sure the result for FONTCONFIG_CFLAGS is
*** the same as the result here.])
fi


AC_PATH_PROG(FREETYPE_CONFIG, freetype-config, no)
if test "x$FREETYPE_CONFIG" != "xno" ; then
  AC_MSG_CHECKING([for freetype libraries])
  FREETYPE_LIBS=`$FREETYPE_CONFIG --libs`
  AC_MSG_RESULT($FREETYPE_LIBS)
fi
AC_SUBST(FREETYPE_LIBS)


##########################################
# Check for some special functions we need
##########################################

AC_CHECK_HEADERS(math.h ieeefp.h)

# DU4 native cc currently needs -std1 for ANSI mode (instead of K&R)
AC_MSG_CHECKING([for extra flags to get ANSI library prototypes])
gimp_save_LIBS=$LIBS
LIBS="$LIBS -lm"
AC_TRY_RUN([#include <math.h>
             int main (void) { return (log(1) != log(1.)); }],
  AC_MSG_RESULT(none needed),
  gimp_save_CFLAGS=$CFLAGS
  CFLAGS="$CFLAGS -std1"
  AC_TRY_RUN([#include <math.h>
               int main (void) { return (log(1) != log(1.)); }],
    AC_MSG_RESULT(-std1),
    AC_MSG_RESULT()
    CFLAGS=$gimp_save_CFLAGS
    AC_MSG_WARN([No ANSI prototypes found in library. (-std1 didn't work.)]),
    AC_MSG_RESULT),
  AC_MSG_RESULT())

# Check for finite or isfinite
AC_CHECK_FUNCS(finite, , [
  AC_MSG_CHECKING(for finite in <math.h>)
  AC_TRY_LINK([#include <math.h>], [double f = 0.0; finite (f)], [
    AC_DEFINE(HAVE_FINITE, 1)
    AC_MSG_RESULT(yes)], [
    AC_MSG_RESULT(no)
    AC_MSG_CHECKING(for isfinite in <math.h>)
    AC_TRY_LINK([#include <math.h>], [float f = 0.0; isfinite(f)], [
      AC_DEFINE(HAVE_ISFINITE, 1,
                [Define to 1 if you have the isfinite function.])
      AC_MSG_RESULT(yes)], AC_MSG_RESULT(no))])])
AC_SUBST(HAVE_FINITE)
AC_SUBST(HAVE_ISFINITE)

LIBS=$gimp_save_LIBS


# NeXTStep cc seems to need this
AC_MSG_CHECKING([for extra flags for POSIX compliance])
AC_TRY_COMPILE([#include <dirent.h>], [DIR *dir;],
  AC_MSG_RESULT(none needed),
  gimp_save_CFLAGS=$CFLAGS
  CFLAGS="$CFLAGS -posix"
  AC_TRY_COMPILE([#include <dirent.h>], [DIR *dir;],
    AC_MSG_RESULT(-posix),
    AC_MSG_RESULT()
    CFLAGS=$gimp_save_CFLAGS
    AC_MSG_WARN([Could not determine POSIX flag. (-posix didn't work.)])))

# check for rint
AC_CHECK_FUNC(rint, AC_DEFINE(HAVE_RINT, 1,
                              [Define to 1 if you have the rint function.]), [
  AC_CHECK_LIB(m, rint, [
    AC_DEFINE(HAVE_RINT)])])


######################################################
# Check for extra libs needed for inet_ntoa and socket
######################################################

gimp_save_LIBS=$LIBS
LIBS=""

AC_CHECK_FUNCS(inet_ntoa, , AC_CHECK_LIB(nsl, inet_ntoa))
AC_CHECK_LIB(socket, socket)

SOCKET_LIBS="$LIBS"
LIBS=$gimp_save_LIBS

AC_SUBST(SOCKET_LIBS)


########################
# Check for MMX assembly
########################

AC_ARG_ENABLE(mmx,
  [  --enable-mmx            enable MMX support (default=auto)],,
  enable_mmx=$have_x86)

AC_ARG_ENABLE(sse,
  [  --enable-sse            enable SSE support (default=auto)],,
  enable_sse=$enable_mmx)

if test "x$enable_mmx" = xyes; then
  GIMP_DETECT_CFLAGS(MMX_EXTRA_CFLAGS, '-mmmx')
  SSE_EXTRA_CFLAGS=

  AC_MSG_CHECKING(whether we can compile MMX code)

  mmx_save_CFLAGS="$CFLAGS"
  CFLAGS="$mmx_save_CFLAGS $MMX_EXTRA_CFLAGS"

  AC_COMPILE_IFELSE([asm ("movq 0, %mm0");],

    AC_DEFINE(USE_MMX, 1, [Define to 1 if MMX assembly is available.])
    AC_MSG_RESULT(yes)

    if test "x$enable_sse" = xyes; then
      GIMP_DETECT_CFLAGS(sse_flag, '-msse')
      SSE_EXTRA_CFLAGS="$MMX_EXTRA_CFLAGS $sse_flag"

      AC_MSG_CHECKING(whether we can compile SSE code)

      CFLAGS="$CFLAGS $sse_flag"

      AC_COMPILE_IFELSE([asm ("movntps %xmm0, 0");],
        AC_DEFINE(USE_SSE, 1, [Define to 1 if SSE assembly is available.])
        AC_MSG_RESULT(yes)
      ,
        enable_sse=no
        AC_MSG_RESULT(no)
        AC_MSG_WARN([The assembler does not support the SSE command set.])
      )

    fi
  ,
    enable_mmx=no
    AC_MSG_RESULT(no)
    AC_MSG_WARN([The assembler does not support the MMX command set.])
  )

  CFLAGS="$mmx_save_CFLAGS"

  AC_SUBST(MMX_EXTRA_CFLAGS)
  AC_SUBST(SSE_EXTRA_CFLAGS)
fi


############################
# Check for AltiVec assembly
############################

AC_ARG_ENABLE(altivec,
  [  --enable-altivec        enable AltiVec support (default=auto)],,
  enable_altivec=$have_ppc)

if test "x$enable_altivec" = xyes; then

  GIMP_DETECT_CFLAGS(altivec_flag, '-faltivec' '-maltivec -mabi=altivec')

  ALTIVEC_EXTRA_CFLAGS=
  case "$altivec_flag" in
    -maltivec*)
      altivec_save_CPPFLAGS="$CPPFLAGS"
      CPPFLAGS="$altivec_save_CPPFLAGS $altivec_flag"
      AC_CHECK_HEADERS(altivec.h, [ALTIVEC_EXTRA_CFLAGS="$altivec_flag"])
      CPPFLAGS="$altivec_save_CPPFLAGS"
      ;;
    *)
      ALTIVEC_EXTRA_CFLAGS="$altivec_flag"
      ;;
  esac
  AC_SUBST(ALTIVEC_EXTRA_CFLAGS)

  AC_MSG_CHECKING(whether we can compile AltiVec code)

  can_use_altivec=no
  if test -z "$ALTIVEC_EXTRA_CFLAGS"; then
    AC_MSG_RESULT(no)
    AC_MSG_WARN([The compiler does not support the AltiVec command set.])
  else
    case "$target_or_host" in
      *-*-darwin*)
        can_use_altivec=yes
        AC_DEFINE(HAVE_ALTIVEC_SYSCTL, 1,
         [Define to 1 if the altivec runtime test should use a sysctl.])
        ;;
      *)
        AC_COMPILE_IFELSE([asm ("vand %v0, %v0, %v0");],
          can_use_altivec=yes, can_use_altivec=no)
        ;;
    esac
    AC_MSG_RESULT($can_use_altivec)

    if test "x$can_use_altivec" = "xyes"; then
      AC_DEFINE(USE_ALTIVEC, 1, [Define to 1 if AltiVec support is available.])
    else
      enable_altivec=no
      AC_MSG_WARN([The assembler does not support the AltiVec command set.])
    fi
  fi

  enable_altivec="$can_use_altivec"
fi


###################################
# Checks for shared memory handling
###################################

# MacOS X has broken SysV shm
case "$target_or_host" in
  *-*-darwin* | *-*-rhapsody* | *-*-machten*)
    shmdefault=posix
    ;;
  *)
    shmdefault=sysv
    ;;
esac

shmtype=auto
AC_ARG_WITH(shm, [  --with-shm=none|sysv|posix|auto    shared memory transport type (default=auto)], shmtype=$with_shm)

case $shmtype in
  none|sysv|posix) ;;
  auto) shmtype=$shmdefault ;;
  *) AC_MSG_ERROR([Invalid shared memory transport type: use none, sysv, posix, or auto.]);;
esac


if test "x$platform_win32" = "xyes"; then
  shmtype=win32
elif test "x$shmtype" = "xsysv"; then
  AC_CHECK_HEADER(sys/ipc.h,
    [AC_DEFINE(HAVE_IPC_H, 1,
               [Define to 1 if you have the <sys/ipc.h> header file.])],
    no_sys_ipc=yes)
  AC_CHECK_HEADER(sys/shm.h,
    [AC_DEFINE(HAVE_SHM_H, 1,
               [Define to 1 if you have the <sys/shm.h> header file.])],
    no_sys_shm=yes)

  if test "$ac_cv_header_sys_shm_h" = "yes"; then
    AC_MSG_CHECKING(whether shmctl IPC_RMID allowes subsequent attaches)
    AC_TRY_RUN([
	#include <sys/types.h>
	#include <sys/ipc.h>
	#include <sys/shm.h>
	int main()
	{
	  int id;
	  char *shmaddr;
	id = shmget (IPC_PRIVATE, 4, IPC_CREAT | 0600);
	if (id == -1)
	  exit (2);
	  shmaddr = shmat (id, 0, 0);
	  shmctl (id, IPC_RMID, 0);
	  if ((char*) shmat (id, 0, 0) == (char*) -1)
	  {
	    shmdt (shmaddr);
	    exit (1);
	  }
	  shmdt (shmaddr);
	  shmdt (shmaddr);
	  exit (0);
	}
      ],
      AC_DEFINE(IPC_RMID_DEFERRED_RELEASE, 1,
	        [Define to 1 if shared memory segments are released deferred.])
        AC_MSG_RESULT(yes),
      AC_MSG_RESULT(no),
      AC_MSG_RESULT(assuming no))

    AC_DEFINE(USE_SYSV_SHM, 1, [Define to 1 to use SYSV shared memory])
  else
    shmtype=none
  fi
elif test "x$shmtype" = "xposix"; then
  have_shm_open=no

  AC_CHECK_FUNC(shm_open, have_shm_open=yes)

  RT_LIBS=
  if test "$have_shm_open" != "yes"; then
    AC_CHECK_LIB(rt, shm_open, RT_LIBS="-lrt" have_shm_open=yes)
  fi
  AC_SUBST(RT_LIBS)

  AC_CHECK_HEADER(sys/mman.h,
    [AC_DEFINE(HAVE_MMAN_H, 1,
               [Define to 1 if you have the <sys/mman.h> header file.])],
    no_sys_mman=yes)

  if test "$ac_cv_func_mmap" = "yes" &&
     test "$ac_cv_header_sys_mman_h" = "yes" &&
     test "$have_shm_open" = "yes"; then
    AC_DEFINE(USE_POSIX_SHM, 1, [Define to 1 to use POSIX shared memory])
  else
    shmtype=none
  fi
fi

AC_MSG_CHECKING(for shared memory transport type)
AC_MSG_RESULT($shmtype)


########################
# Define a symbol prefix
########################

AC_MSG_CHECKING([whether symbols are prefixed])

case "$host_os" in
  mingw*)
    SYMPREFIX='_'
    ;;
  darwin* | rhapsody* | machten*)
    SYMPREFIX='_'
    ;;
  *)
    ;;
esac

if test -n "$SYMPREFIX"; then
  AC_MSG_RESULT([yes, with a $SYMPREFIX])
else
  AC_MSG_RESULT(no)
fi

AC_SUBST(SYMPREFIX)


############################
# Check for select and fdset
############################

if test "x$os_win32" != xyes; then
  AC_MSG_CHECKING([fd_set and sys/select])
  AC_TRY_COMPILE([#include <sys/types.h>],
	  [fd_set readMask, writeMask;], gimp_ok=yes, gimp_ok=no)
  if test "x$gimp_ok" = xno; then
      AC_EGREP_HEADER(fd_mask, sys/select.h, gimp_ok=yes)
      if test "x$gimp_ok" = xyes; then
	  AC_DEFINE(HAVE_SYS_SELECT_H, 1,
		    [Define to 1 if you have the <sys/select.h> header.])
      fi
  fi
  AC_MSG_RESULT($gimp_ok)
  if test "x$gimp_ok" = xno; then
      AC_DEFINE(NO_FD_SET, 1,
		[Define to 1 if you don't have the fd_set typedef.])
  fi
fi


#############################
# Threads and multi processor
#############################

AC_ARG_ENABLE(mp, [  --disable-mp            disable support for multiple processors])

if test "x$enable_mp" != "xno"; then
  AC_DEFINE(ENABLE_MP, 1,
	    [Define to 1 to enable support for multiple processors.])
fi


###################################################
# gimp-remote is obsolete, but can still be enabled
###################################################

AC_ARG_ENABLE(gimp-remote,
	      [  --enable-gimp-remote    build gimp-remote utility (default=no)],
	      enable_gimp_remote=yes,
	      enable_gimp_remote="no (not enabled)")


################################
# Some plug-ins are X11 specific
################################

# Check if building for GTK+-X11
gdk_target=`$PKG_CONFIG --variable=target gtk+-2.0`

have_libxmu="no (building for $gdk_target)"

if test "$gdk_target" = x11; then

   # doc-shooter is X11 specific
   DOC_SHOOTER=doc-shooter

   # screenshot plug-in needs to link to X11 explicitely
   SCREENSHOT_LIBS="$X_LIBS -lX11"

   gimp_save_CFLAGS="$CFLAGS"
   gimp_save_LDFLAGS="$LDFLAGS"
   CFLAGS="$CFLAGS $GTK_CFLAGS"
   LDFLAGS="$LDFLAGS $X_LIBS"

   # Test for Xmu
   if test -z "$XMU_LIBS"; then
      AC_CHECK_LIB(Xmu, XmuClientWindow,
        [AC_CHECK_HEADERS(X11/Xmu/WinUtil.h,
	  have_libxmu=yes
          XMU_LIBS="$X_LIBS $X_PRE_LIBS -lX11 -lXmu -lXt",
	  [have_libxmu="no (XMU header file not found)"],[#include <gdk/gdkx.h>])],
	[have_libxmu="no (XMU library not found)"], -lXt -lX11)
      SCREENSHOT_LIBS="$XMU_LIBS"
   fi

   # Check for shaped window extension
   AC_CHECK_LIB(Xext, XShapeGetRectangles,
     [AC_CHECK_HEADERS(X11/extensions/shape.h,
        SCREENSHOT_LIBS="$SCREENSHOT_LIBS -lXext",,[#include <gdk/gdkx.h>])],,
     -lX11 -lXext)

   CFLAGS="$gimp_save_CFLAGS"
   LDFLAGS="$gimp_save_LDFLAGS"

   # check for the XFixes extension
   PKG_CHECK_MODULES(XFIXES, xfixes,
      AC_DEFINE(HAVE_XFIXES, 1,
                [Define to 1 to if the XFIXES X extension is available])
      SCREENSHOT_LIBS="$SCREENSHOT_LIBS $XFIXES_LIBS",
      true)
fi

AC_SUBST(XMU_LIBS)
AC_SUBST(SCREENSHOT_LIBS)

AC_SUBST(DOC_SHOOTER)


if test x"$enable_gimp_remote" = xyes; then
  enable_gimp_remote="$have_libxmu"
fi

if test x"$enable_gimp_remote" = xyes; then
  GIMP_REMOTE='gimp-remote-gimp_app_version$(EXEEXT)'
  GIMP_REMOTE_MANPAGE='gimp-remote-gimp_app_version.1'
  GIMP_COMMAND='gimp-remote-gimp_app_version'
else
  GIMP_COMMAND='gimp-gimp_app_version'
fi

AC_SUBST(GIMP_REMOTE)
AC_SUBST(GIMP_REMOTE_MANPAGE)
AC_SUBST(GIMP_COMMAND)


############
# MIME types
############

# The list of MIME types that are supported by plug-ins that are compiled
# unconditionally:
MIME_TYPES="application/postscript;application/pdf;image/bmp;image/g3fax;image/gif;image/x-fits;image/pcx;image/x-portable-anymap;image/x-portable-bitmap;image/x-portable-graymap;image/x-portable-pixmap;image/x-psd;image/x-sgi;image/x-tga;image/x-xbitmap;image/x-xwindowdump;image/x-xcf;image/x-compressed-xcf"


###################
# Check for libtiff
###################

AC_ARG_WITH(libtiff,  [  --without-libtiff       build without TIFF support])

have_libtiff=no
if test "x$with_libtiff" != xno && test -z "$TIFF_LIBS"; then
  have_libtiff=yes
  AC_CHECK_LIB(tiff, TIFFReadScanline,
    [AC_CHECK_HEADER(tiffio.h,
      FILE_TIFF_LOAD='file-tiff-load$(EXEEXT)'; FILE_TIFF_SAVE='file-tiff-save$(EXEEXT)'; TIFF_LIBS='-ltiff',
      [have_libtiff="no (TIFF header files not found)"])],
    [AC_CHECK_LIB(tiff, TIFFWriteScanline,
      [AC_CHECK_HEADER(tiffio.h,
        FILE_TIFF_LOAD='file-tiff-load$(EXEEXT)'; FILE_TIFF_SAVE='file-tiff-save$(EXEEXT)'; TIFF_LIBS='-ltiff -ljpeg -lz',
	[have_libtiff="no (TIFF header files not found)"])],
      [AC_CHECK_LIB(tiff34, TIFFFlushData,
	[AC_CHECK_HEADER(tiffio.h,
          FILE_TIFF_LOAD='file-tiff-load$(EXEEXT)'; FILE_TIFF_SAVE='file-tiff-save$(EXEEXT)'; TIFF_LIBS='-ltiff34 -ljpeg -lz',
	  [have_libtiff="no (TIFF header files not found)"])],
      [have_libtiff="no (TIFF library not found)"], -ljpeg -lz -lm)], -ljpeg -lz -lm)], -lm)
fi

if test "x$with_libtiff" != xno && test -z "$TIFF_LIBS"; then
  AC_MSG_ERROR([
*** Checks for TIFF libary failed. You can build without it by passing
*** --without-libtiff to configure but you won't be able to use TIFFs then.])
fi

if test "x$have_libtiff" = xyes; then
  MIME_TYPES="$MIME_TYPES;image/tiff"
fi

AC_SUBST(FILE_TIFF_LOAD)
AC_SUBST(FILE_TIFF_SAVE)
AC_SUBST(TIFF_LIBS)


###################
# Check for libjpeg
###################

AC_ARG_WITH(libjpeg,  [  --without-libjpeg       build without JPEG support])

jpeg_ok=no
if test "x$with_libjpeg" != xno && test -z "$JPEG_LIBS"; then
  AC_CHECK_LIB(jpeg, jpeg_destroy_decompress,
    jpeg_ok=yes,
    [jpeg_ok="no (JPEG library not found)"
    AC_MSG_WARN(*** XJT plug-in will not be built (JPEG library not found) ***)])
  if test "$jpeg_ok" = yes; then
    AC_MSG_CHECKING([for jpeglib.h])
    AC_TRY_CPP([
#include <stdio.h>
#undef HAVE_STDDEF_H
#undef HAVE_STDLIB_H
#undef PACKAGE
#undef VERSION
#include <jpeglib.h>],
      jpeg_ok=yes,
      jpeg_ok="no (Can't include jpeglib.h)")
    AC_MSG_RESULT($jpeg_ok)
    if test "$jpeg_ok" = yes; then
      AC_CHECK_LIB(jpeg, jpeg_save_markers,
      JPEG='jpeg$(EXEEXT)'; JPEG_LIBS='-ljpeg',
      jpeg_ok="no (JPEG library is too old)")
    else
      jpeg_ok="no (JPEG header file not found)"
    fi
  fi
fi

if test "x$with_libjpeg" != xno && test -z "$JPEG_LIBS"; then
  AC_MSG_ERROR([
*** Checks for JPEG library failed. You can build without it by passing
*** --without-libjpeg to configure but you won't be able to use JPEGs then.])
fi

if test "x$jpeg_ok" = xyes; then
  MIME_TYPES="$MIME_TYPES;image/jpeg"
fi

AM_CONDITIONAL(BUILD_JPEG, test "x$jpeg_ok" = xyes)
AC_SUBST(JPEG_LIBS)


## xjt does build, but it is more confusing than useful on Win32,
## as it wants to run tar, mv and bzip2, which seldom are available
AM_CONDITIONAL(BUILD_XJT, test "x$jpeg_ok" = xyes && test "x$os_win32" = xno)
if test "x$jpeg_ok" != xyes; then
  have_xjt="no (no JPEG library)"
elif test "x$os_win32" = xyes; then
  have_xjt="no (not built on Windows)"
else
  have_xjt=yes
fi


################
# Check for libz
################

have_zlib=yes
if test -z "$Z_LIBS"; then
  AC_CHECK_LIB(z, gzsetparams,
    [AC_CHECK_HEADER(zlib.h,
      FILE_PSP='file-psp$(EXEEXT)'; Z_LIBS='-lz',
      [have_zlib="no (ZLIB header files not found)"])],
    [have_zlib="no (ZLIB library not found)"])
fi

if test "x$have_zlib" = xyes; then
  MIME_TYPES="$MIME_TYPES;image/x-psd"
fi

AC_SUBST(FILE_PSP)
AC_SUBST(Z_LIBS)


##################
# Check for libpng
##################

AC_ARG_WITH(libpng,  [  --without-libpng        build without PNG support])

have_libpng=no
if test "x$with_libpng" != xno; then
  have_libpng=yes
  PKG_CHECK_MODULES(PNG, libpng,
    FILE_PNG='file-png$(EXEEXT)',
    [have_libpng="no (libpng not found)"
     AC_MSG_ERROR([
*** Checks for PNG library failed. You can build without it by passing
*** --without-libpng to configure but you won't be able to use PNGs then.])])
fi

if test "x$have_libpng" = xyes; then
  MIME_TYPES="$MIME_TYPES;image/png;image/x-icon"
fi

AC_SUBST(FILE_PNG)

AM_CONDITIONAL(HAVE_PNG, test "x$have_libpng" = xyes)


##################
# Check for libmng
##################

AC_ARG_WITH(libmng,  [  --without-libmng        build without MNG support])

have_libmng=no
if test "x$with_libmng" != xno && test -z "$MNG_LIBS" &&
   test -n "$PNG_LIBS" && test -n "$JPEG_LIBS" && test -n "$Z_LIBS"; then
  AC_CHECK_LIB(mng, mng_create,
    [AC_CHECK_HEADER(libmng.h, mng_ok=yes)],
    [have_libmng="no (MNG library not found)"], -lz -lm)
  if test "$mng_ok" = yes; then
    have_libmng=yes
    FILE_MNG='file-mng$(EXEEXT)'; MNG_LIBS="-lmng $JPEG_LIBS $PNG_LIBS"
  else
    have_libmng="no (MNG header file not found)"
  fi
fi

AC_SUBST(FILE_MNG)
AC_SUBST(MNG_LIBS)


############################################################
# libexif: Library to allow exif tags to be read from, and
# saved to, jpeg files. Currently, this permits exif data to
# avoid destruction, but no data modification is performed.
############################################################

AC_ARG_WITH(libexif,  [  --without-libexif       build without EXIF support])

have_libexif=no
if test "x$with_libexif" != xno && test -z "$EXIF_LIBS" && test -n "$JPEG_LIBS"; then
  have_libexif=yes
  PKG_CHECK_MODULES(EXIF, libexif >= exif_required_version,
    AC_DEFINE(HAVE_EXIF, 1, [Define to 1 if libexif is available]),
    have_libexif="no (libexif not found or too old)")
fi

AC_SUBST(EXIF_CFLAGS)
AC_SUBST(EXIF_LIBS)

AM_CONDITIONAL(BUILD_METADATA, test "x$have_libexif" = xyes)


#################
# Check for libaa
#################

AC_ARG_WITH(aa,  [  --without-aa            do not build the AA plug-in])

have_libaa=no
if test "x$with_aa" != xno && test -z "$AA_LIBS"; then
  have_libaa=yes
  AC_CHECK_LIB(aa, aa_printf,
    [AC_CHECK_HEADER(aalib.h,
      FILE_AA='file-aa$(EXEEXT)'; AA_LIBS='-laa',
      [have_libaa="no (AA header file not found)"])],
    [have_libaa="no (AA library not found)"])
fi

AC_SUBST(FILE_AA)
AC_SUBST(AA_LIBS)


##################
# Check for libxpm
##################

gimp_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $X_CFLAGS"
gimp_save_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS $X_LIBS"
if test "$gdk_target" = x11; then
  lib_X11=-lX11
fi
have_libxpm=yes
if test -z "$XPM_LIBS"; then
  AC_CHECK_LIB(Xpm, XpmReadFileToXpmImage,
    [AC_CHECK_HEADER(X11/xpm.h,
      FILE_XPM='file-xpm$(EXEEXT)'; XPM_LIBS="$X_LIBS $X_PRE_LIBS $lib_X11 -lXpm",
      [have_libxpm="no (XPM header file not found)"])],
    [have_libxpm="no (XPM library not found)"], $X_PRE_LIBS $lib_X11)
fi
CFLAGS="$gimp_save_CFLAGS"
LDFLAGS="$gimp_save_LDFLAGS"

if test "x$have_libxpm" = xyes; then
  MIME_TYPES="$MIME_TYPES;image/x-xpixmap"
fi

AC_SUBST(FILE_XPM)
AC_SUBST(XPM_LIBS)


##################
# Check for webkit
##################

AC_ARG_WITH(webkit, [  --without-webkit        don't build the help-browser plug-in])

have_webkit=no
if test "x$with_webkit" != xno; then
  PKG_CHECK_MODULES(WEBKIT, webkit-1.0 >= webkit_required_version,
    have_webkit=yes,
    have_webkit="no (WebKit not found)")
else
  have_webkit="no (WebKit support disabled)"
fi

AM_CONDITIONAL(BUILD_HELP_BROWSER, test "x$have_webkit" = xyes)


###################
# Check for librsvg
###################

AC_ARG_WITH(librsvg,[  --without-librsvg       build without librsvg support])

have_librsvg=no
if test "x$with_librsvg" != xno; then
  have_librsvg=yes
  PKG_CHECK_MODULES(SVG, librsvg-2.0 >= rsvg_required_version,
    FILE_SVG='file-svg$(EXEEXT)',
    have_librsvg="no (librsvg not found)")
else
   have_librsvg="no (librsvg support disabled)"
fi

if test "x$have_librsvg" = xyes; then
  MIME_TYPES="$MIME_TYPES;image/svg+xml"
fi

AC_SUBST(FILE_SVG)


####################################
# Allow to disable the print plug-in
####################################

AC_ARG_WITH(print,[  --without-print         build without print support])

enable_print="no (disabled)"
if test "x$with_print" != xno; then
  enable_print="yes"
fi

AM_CONDITIONAL(BUILD_PRINT, test "x$enable_print" == xyes)


######################
# Check for libpoppler
######################

AC_ARG_WITH(poppler,[  --without-poppler       build without poppler support])

have_poppler=no
if test "x$with_poppler" != xno; then
  PKG_CHECK_MODULES(POPPLER,
    poppler-glib >= poppler_required_version gthread-2.0,
    FILE_PDF='file-pdf$(EXEEXT)'
    AC_DEFINE(HAVE_POPPLER, 1, [Define to 1 if libpoppler is available])
    have_poppler=yes,
    have_poppler="Using PostScript plug-in (libpoppler not found)")
else
  have_poppler="Using PostScript plug-in (libpoppler support disabled)"
fi

AC_SUBST(FILE_PDF)


################################
# Check for gio/gvfs and libcurl
################################

uri_plugin=no


AC_ARG_WITH(gvfs, [  --without-gvfs          build without GIO/GVfs support])

AM_CONDITIONAL(HAVE_GVFS, test "x$with_gvfs" != xno)


AC_ARG_WITH(libcurl,  [  --without-libcurl       build without curl support])

have_libcurl="no (disabled)"

if test "x$with_libcurl" != xno; then

PKG_CHECK_MODULES(CURL, libcurl >= libcurl_required_version,
  have_libcurl=yes,
  have_libcurl="no (libcurl not found)")

fi

AM_CONDITIONAL(HAVE_LIBCURL, test "x$have_libcurl" = xyes)


if test "x$with_gvfs" != xno; then
  uri_plugin="yes (using GIO/GVfs)"
elif test "x$have_libcurl" = xyes; then
  uri_plugin="yes (using libcurl)"
elif test "x$os_win32" = xno; then
  uri_plugin="yes (using wget)"
fi


###################
# Check for libwmf2
###################

AC_ARG_WITH(wmf, [  --without-wmf           build without WMF support])

have_libwmf=no
if test "x$with_wmf" != xno; then
  AC_PATH_PROG(WMF_CONFIG, libwmf-config, no)
  if test "x$WMF_CONFIG" != "xno" ; then
    have_libwmf=yes
    AC_MSG_CHECKING(for libwmf >= wmf_required_version)
    wmf_version=`$WMF_CONFIG --version`
    if expr $wmf_version \>= wmf_required_version >/dev/null; then
      AC_MSG_RESULT([yes (version $wmf_version)])
      FILE_WMF='file-wmf$(EXEEXT)'
      WMF_LIBS=`$WMF_CONFIG --libs`
      WMF_CFLAGS=`$WMF_CONFIG --cflags`
    else
      have_libwmf="no (libwmf is too old)"
   fi
  else
    have_libwmf="no (libwmf not found)"
  fi
fi

if test "x$have_libwmf" = xyes; then
  MIME_TYPES="$MIME_TYPES;image/x-wmf"
fi

AC_SUBST(FILE_WMF)
AC_SUBST(WMF_LIBS)
AC_SUBST(WMF_CFLAGS)


#####################
# Check for libjasper
#####################

AC_ARG_WITH(libjasper,  [  --without-libjasper     build without JPEG-2000 support])

have_jp2=no
if test "x$with_libjasper" != xno && test -z "$JASPER_LIBS"; then
  have_jp2=yes
  AC_CHECK_LIB(jasper, jas_init,
    FILE_JP2_LOAD='file-jp2-load$(EXEEXT)' ; JP2_LIBS=-ljasper,
    [have_jp2="no (JasPer library not found)"
    AC_MSG_WARN(*** JP2 plug-in will not be built (JasPer library not found) ***)])
else
   have_jp2="no (JasPer support disabled)"
fi

if test "x$have_jp2" = xyes; then
  MIME_TYPES="$MIME_TYPES;image/jp2;image/jpeg2000;image/jpx"
fi

AM_CONDITIONAL(BUILD_JP2, test "x$have_jp2" = xyes)
AC_SUBST(FILE_JP2_LOAD)
AC_SUBST(JP2_LIBS)


################
# Check for lcms
################

AC_ARG_WITH(lcms, [  --without-lcms          build without lcms support])

have_lcms="no (lcms support disabled)"
if test "x$with_lcms" != xno; then
  have_lcms=yes
  PKG_CHECK_MODULES(LCMS, lcms >= lcms_required_version,
    AC_DEFINE(HAVE_LCMS, 1, [Define to 1 if lcms is available])
    LCMS='lcms$(EXEEXT)',
    have_lcms="no (lcms not found or unusable)")
fi

AC_SUBST(LCMS)
AM_CONDITIONAL(HAVE_LCMS, test "x$have_lcms" = xyes)


######################
# Check for libXcursor
######################

AC_ARG_WITH(xmc,[  --without-xmc           build without Xcursor support])

have_xmc=no
if test "x$with_xmc" != xno; then
  have_xmc=yes
  PKG_CHECK_MODULES(XMC, xcursor,
    FILE_XMC='file-xmc$(EXEEXT)',
    have_xmc="no (libXcursor not found)")
else
   have_xmc="no (libXcursor support disabled)"
fi

if test "x$have_xmc" = xyes; then
  MIME_TYPES="$MIME_TYPES;image/x-xcursor"
fi

AC_SUBST(FILE_XMC)


################
# Check for alsa
################

AC_ARG_WITH(alsa, [  --without-alsa          disable alsa support in midi input controller])

have_alsa="no (alsa support disabled)"
if test "x$with_alsa" != xno; then
  have_alsa=yes
  AM_PATH_ALSA(alsa_required_version,
               AC_DEFINE(HAVE_ALSA, 1, [Define to 1 if alsa is available]),
               have_alsa="no (libasound not found or unusable)")
fi


#######################
# Check for Linux Input
#######################

AC_ARG_WITH(linux-input, [  --without-linux-input   don't build linux input event controller module])

have_linux_input="no (linux input support disabled)"
if test "x$with_linux_input" != "xno"; then
  AC_CHECK_HEADER(linux/input.h,
	AC_CHECK_DECL(KEY_OK,
		have_linux_input=yes,
		have_linux_input="no (needs Linux 2.6)",
		[#include <linux/input.h>]))
fi

AM_CONDITIONAL(HAVE_LINUX_INPUT, test "x$have_linux_input" = xyes)


###############################
# Check for DirectX DirectInput
###############################

AC_ARG_WITH(directx-sdk, [  --with-directx-sdk=PFX  prefix where the DirectX SDK is installed(optional)])

have_dx_dinput=no
if test "x$os_win32" = xyes; then
  case "$with_directx_sdk" in
    *" "*) AC_MSG_WARN([The DirectX SDK should be accessible through a path without spaces. Use MSYS mounts. Ignoring your --with-directx-sdk switch.])
           with_directx_sdk=;;
    *\\*) AC_MSG_WARN([Use plain (forward) slashes only in your --with-directx-sdk switch. Ignoring your --with-directx-sdk switch.])
           with_directx_sdk=;;
  esac

  dinput_save_CPPFLAGS="$CPPFLAGS"
  dinput_save_LDFLAGS="$LDFLAGS"
  if test "x$with_directx_sdk" != x; then
    CPPFLAGS="-I $with_directx_sdk/Include $CPPFLAGS"
    LDFLAGS="-L$with_directx_sdk/Lib/x86 $LDFLAGS"
    AC_CHECK_HEADER(dinput.h,
                    [AC_CHECK_LIB(dxguid, GUID_XAxis, have_dx_dinput=yes)])
  fi

  if test "x$have_dx_dinput" = xyes; then
    AC_DEFINE(HAVE_DX_DINPUT, 1, [Define to 1 if you have DirectX DirectInput headers and libraries])
  else
    CPPFLAGS="$dinput_save_CPPFLAGS"
    LDFLAGS="$dinput_save_LDFLAGS"
  fi
fi

AM_CONDITIONAL(HAVE_DX_DINPUT, test "x$have_dx_dinput" = xyes)


###############################
# Check for D-Bus GLib bindings
###############################

AC_ARG_WITH(dbus,  [  --without-dbus          build without D-Bus support])

have_dbus_glib="no (disabled)"

if test "x$with_dbus" != xno; then
  PKG_CHECK_MODULES(DBUS_GLIB, dbus-glib-1 >= dbus_glib_required_version,
    have_dbus_glib=yes,
    have_dbus_glib=no)
fi

if test "x$have_dbus_glib" = xyes; then
  AC_PATH_PROG(DBUS_BINDING_TOOL, dbus-binding-tool, no)
  if test "x$DBUS_BINDING_TOOL" = xno; then
    AC_MSG_WARN([Could not find dbus-binding-tool in your PATH])
    have_dbus_glib="no (dbus-binding-tool not found)"
  fi
fi

if test "x$have_dbus_glib" = xyes; then
  AC_DEFINE(HAVE_DBUS_GLIB, 1,
            [Define to 1 if D-Bus GLib bindings are available])
  GIMP_COMMAND='gimp-gimp_app_version'
fi


###############################################
# Check for libhal (Hardware Abstraction Layer)
###############################################

AC_ARG_WITH(hal,   [  --without-hal           build without HAL support])

have_libhal=no

if test "x$with_hal" = xno; then
  have_libhal="no (disabled)"
fi

if test "x$have_linux_input" = xyes && test "x$have_dbus_glib" = xyes &&
   test "x$with_hal" != xno; then
  PKG_CHECK_MODULES(HAL, hal >= libhal_required_version,
    have_libhal=yes,
    have_libhal="no (libhal not found)")
fi

if test "x$have_libhal" = xyes; then
  AC_DEFINE(HAVE_LIBHAL, 1, [Define to 1 if libhal is available])
fi


############################################################
# GIF compression: Allow builder to specify at compile time
# which compression style she wants to use for GIFs.
# LZW: Unisys patented Lempel-Ziff encoding.
# RLE: Runlength encoding (produces files about twice as big
#			   as the LZW compressed equivalent).
# None: Raw data.
############################################################

AC_ARG_WITH(gif-compression,
            [  --with-gif-compression=lzw|rle|none     GIF compression style (default=lzw)])

case x"$with_gif_compression" in
  xnone)
    AC_DEFINE(GIF_UN, 1, [Define to 1 for no GIF compression]) ;;
  xrle)
    AC_DEFINE(GIF_RLE, 1, [Define to 1 for RLE GIF compression]) ;;
esac


####################
# Check for sendmail
####################

AC_ARG_WITH(sendmail,[  --with-sendmail=DIR     set sendmail command location])

if test "x$with_sendmail" != "x"; then
  sendmail_path=$with_sendmail
else
  sendmail_path=$PATH:/usr/sbin:/usr/lib
fi

AC_PATH_PROG(SENDMAIL, sendmail, , $sendmail_path)
if test "x$SENDMAIL" != "x"; then
  AC_DEFINE_UNQUOTED(SENDMAIL, "$SENDMAIL",
	             [The MTA used by the mail plug-in.])
fi


################
# Check for perl
################

AC_PATH_PROGS(PERL,perl5 perl perl5.005 perl5.004,perl)


AC_ARG_WITH(pdbgen,
            [  --with-pdbgen           use 'pdbgen' code generation tool],,
  with_pdbgen=$enable_maintainer_mode)

AM_CONDITIONAL(WITH_PDBGEN, test "x$with_pdbgen" = xyes)


##################
# Check for python
##################

# Pygimp configure stuff ...
AC_ARG_ENABLE(python,
  AC_HELP_STRING([--disable-python],
                 [do not build the python extension]))

m4_define([pycheck_error], [
*** Could not find $1.
*** Please install $2, or skip building the python scripting extension by
*** passing --disable-python to configure (but then you will not be able
*** to use scripts for GIMP that are written in Python).])

if test "x$enable_python" != xno; then
  enable_python="yes"
  # check for Python
  AM_PATH_PYTHON(2.5.0,,
    [AC_MSG_ERROR([pycheck_error([Python 2.5.0 or newer], [it])])])
  AM_CHECK_PYTHON_HEADERS(,
    [AC_MSG_ERROR([pycheck_error([Python headers], [them])])])

  # Win32 needs all symbols resolved for linking, even for DLLs
  # Assume the link library is in $exec_prefix/libs
  # This might be able to be figured out from distutils, but it's
  # not documented so we won't rely on internal implementation
  PYLINK_LIBS=
  if test "x$platform_win32" = "xyes"; then
    PYBIN_PATH="$py_exec_prefix\pythonw.exe"
    PYTHON_INCLUDES=`echo "$PYTHON_INCLUDES" | sed -e 's/\\\\/\\//g'`
    py_exec_prefix=`echo "$py_exec_prefix" | sed -e 's/\\\\/\\//g'`
    pylibversion=`echo $PYTHON_VERSION | sed -e 's/\\.//'`
    PYLINK_LIBS="-L${py_exec_prefix}/libs -lpython${pylibversion}"
  else
    PYBIN_PATH="$PYTHON"
  fi
  AC_SUBST(PYLINK_LIBS)
  AC_SUBST(PYBIN_PATH)

  # check for PyGTK
  PKG_CHECK_MODULES(PYGTK, pygtk-2.0 >= pygtk_required_version,,
    [AC_MSG_ERROR([pycheck_error([PyGTK pygtk_required_version or newer], [it])])])

  AC_PATH_PROG(PYGTK_CODEGEN, pygtk-codegen-2.0, no)
  if test "x$PYGTK_CODEGEN" = xno; then
    AC_MSG_ERROR([pycheck_error([pygtk-codegen-2.0 script], [it])])
  fi

  AC_MSG_CHECKING(for pygtk defs)
  PYGTK_DEFSDIR=`$PKG_CONFIG --variable=defsdir pygtk-2.0`
  AC_SUBST(PYGTK_DEFSDIR)
  AC_MSG_RESULT($PYGTK_DEFSDIR)

  GIMP_DETECT_CFLAGS(PYGIMP_EXTRA_CFLAGS, '-fno-strict-aliasing')
  AC_SUBST(PYGIMP_EXTRA_CFLAGS)
fi
AM_CONDITIONAL(BUILD_PYTHON, test "x$enable_python" != xno)


#########################################
# Check whether Script-Fu should be built
#########################################

have_script_fu=yes
AC_ARG_WITH(script-fu,
  AC_HELP_STRING([--without-script-fu],
                 [do not build the script-fu extension]))

if test "x$with_script_fu" = "xno"; then
  have_script_fu="no (script-fu extension disabled)"
fi
AM_CONDITIONAL(BUILD_SCRIPT_FU, test "x$with_script_fu" != "xno")


###########################################################
# Some plug-ins don't build on Win32, others are Win32-only
###########################################################

if test "x$os_win32" = xno; then
  MAIL='mail$(EXEEXT)'
  SCREENSHOT='screenshot$(EXEEXT)'
fi

AC_SUBST(MAIL)
AC_SUBST(SCREENSHOT)


####################
# Check for Mac OS X
####################

AC_MSG_CHECKING([if compiling for Mac OS X])
case "$target_or_host" in
  *-*-darwin*)
     AC_MSG_RESULT(yes)
     AC_DEFINE(PLATFORM_OSX, 1, [define to 1 if compiling for Mac OS X])
     ;;
  *)
     AC_MSG_RESULT(no)
     ;;
esac


####################################
# Check for Mac OS X TWAIN framework
####################################

mac_twain_ok=no
AC_MSG_CHECKING([for Mac OS X TWAIN support])
AC_TRY_CPP([
#include <Carbon/Carbon.h>
#include <TWAIN/TWAIN.h>
], mac_twain_ok=yes)
AC_MSG_RESULT($mac_twain_ok)
AM_CONDITIONAL(HAVE_MAC_TWAIN, test "x$mac_twain_ok" = xyes)


##########################################################
# Determine where to install the desktop & mime info files
##########################################################

AC_ARG_WITH(desktop-dir, [  --with-desktop-dir=PATH where to install desktop files (default=PREFIX/share)])

if test "x$with_desktop_dir" != x; then
  DESKTOP_DATADIR=$with_desktop_dir
else
  DESKTOP_DATADIR="$datadir"
fi

AM_CONDITIONAL(DESKTOP_DATADIR, test "x$with_desktop_dir" != xno)
AC_SUBST(DESKTOP_DATADIR)


#####################
# Check for XML tools
#####################

AC_PATH_PROG(XMLLINT, xmllint, no)
AM_CONDITIONAL(HAVE_XMLLINT, test "x$XMLLINT" != "xno")

AC_PATH_PROG(XSLTPROC, xsltproc, no)
AM_CONDITIONAL(HAVE_XSLTPROC, test "x$XSLTPROC" != "xno")


######################################
# Checks for gtk-doc and docbook-tools
######################################

GTK_DOC_CHECK([1.0])

# NOTE: We need to use a separate automake conditional for this
#       to make this work with the tarballs.
AM_CONDITIONAL(ENABLE_GTK_DOC, test "x$enable_gtk_doc" = xyes)

AC_ARG_ENABLE(gtk-doc-app, [  --enable-gtk-doc-app    build developer documentation for app (default=no)], , enable_gtk_doc_app=no)
AM_CONDITIONAL(ENABLE_GTK_DOC_APP, test "x$enable_gtk_doc_app" = xyes)


#########################################
# Allow to configure the GIMP environment
#########################################

AC_ARG_ENABLE(default-binary, [  --enable-default-binary install this as the default gimp binary (default=gimp_stable)], , enable_default_binary=gimp_stable)

AM_CONDITIONAL(DEFAULT_BINARY, test "x$enable_default_binary" = xyes)


AC_ARG_ENABLE(gimp-console, [  --enable-gimp-console   build a console-only binary which does not link GTK+], , enable_gimp_console=yes)

AM_CONDITIONAL(ENABLE_GIMP_CONSOLE, test "x$enable_gimp_console" != xno)


# Possibly change default gimpdir from .gimp-major.minor
gimpdir=.gimp-gimp_user_version
AC_ARG_WITH(gimpdir, [  --with-gimpdir=DIR      change default gimpdir from .gimp-gimp_user_version to DIR],
if eval "test x$with_gimpdir != x"; then
  if eval "test x$with_gimpdir != xyes"; then
    gimpdir=$with_gimpdir
  fi
fi)

gimpdatadir="$datadir/$PACKAGE/gimp_data_version"
gimpplugindir="$libdir/$PACKAGE/gimp_plugin_version"
gimpsysconfdir="$sysconfdir/$PACKAGE/gimp_sysconf_version"

AC_SUBST(gimpdir)
AC_SUBST(gimpdatadir)
AC_SUBST(gimpplugindir)
AC_SUBST(gimpsysconfdir)


# GNU format message catalogs go into $datadir/locale,
# standard location is $libdir/locale. DATADIRNAME set accordingly.

gimplocaledir="$datadir/locale"
if test "$DATADIRNAME" = "lib"; then
   gimplocaledir="$libdir/locale"
fi

AC_SUBST(gimplocaledir)


############################
# Some defines for the build
############################

GIMPINSTALL=
if test "$INSTALL" = "$ac_install_sh"; then
  GIMPINSTALL="gimpinstall-gimp_tool_version"
fi
AC_SUBST(GIMPINSTALL)

AM_CONDITIONAL(STATICLIBS, test "x$enable_static" = xyes)

# a tuned version of glib-mkenums
GIMP_MKENUMS="\$(PERL) \$(top_srcdir)/tools/gimp-mkenums"
AC_SUBST(GIMP_MKENUMS)

#Add required trailing ; to list
MIME_TYPES="$MIME_TYPES;"

AC_SUBST(MIME_TYPES)


#########################
# Disable deprecated APIs
#########################

CPPFLAGS="${CPPFLAGS} -DGIMP_DISABLE_DEPRECATED -DBABL_DISABLE_DEPRECATED -DGSEAL_ENABLE"

if test "x$have_glib_2_23" != "xyes"; then
  CPPFLAGS="${CPPFLAGS} -DG_DISABLE_DEPRECATED"
fi

if test "x$have_gtk_2_19" != "xyes"; then
  CPPFLAGS="${CPPFLAGS} -DGDK_PIXBUF_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
fi

if test "x$have_pango_1_21" != "xyes"; then
  CPPFLAGS="${CPPFLAGS} -DPANGO_DISABLE_DEPRECATED"
fi


############################
# Require multihead safe API
############################

CPPFLAGS="${CPPFLAGS} -DGDK_MULTIHEAD_SAFE -DGTK_MULTIHEAD_SAFE"


##################
# configure output
##################

# easy way to skip the plug-in build
GIMP_PLUGINS=plug-ins
GIMP_MODULES=modules
AC_SUBST(GIMP_PLUGINS)
AC_SUBST(GIMP_MODULES)

AC_CONFIG_COMMANDS([libgimpbase/gimpversion.h], [
  outfile=gimpversion.h-tmp
  cat > $outfile <<\_______EOF
/* gimpversion.h
 *
 * This is a generated file.  Please modify 'configure.ac'
 */

#ifndef __GIMP_VERSION_H__
#define __GIMP_VERSION_H__

G_BEGIN_DECLS


_______EOF

  cat >>$outfile <<_______EOF
#define GIMP_MAJOR_VERSION                              ($gimp_major_ver)
#define GIMP_MINOR_VERSION                              ($gimp_minor_ver)
#define GIMP_MICRO_VERSION                              ($gimp_micro_ver)
#define GIMP_VERSION                                    "$gimp_ver"
#define GIMP_API_VERSION                                "$gimp_api_ver"
_______EOF

  cat >>$outfile <<\_______EOF
#define GIMP_CHECK_VERSION(major, minor, micro) \
    (GIMP_MAJOR_VERSION > (major) || \
     (GIMP_MAJOR_VERSION == (major) && GIMP_MINOR_VERSION > (minor)) || \
     (GIMP_MAJOR_VERSION == (major) && GIMP_MINOR_VERSION == (minor) && \
      GIMP_MICRO_VERSION >= (micro)))


G_END_DECLS

#endif /* __GIMP_VERSION_H__ */
_______EOF

  if cmp -s $outfile libgimpbase/gimpversion.h; then
    AC_MSG_NOTICE([libgimpbase/gimpversion.h is unchanged])
    rm -f $outfile
  else
    mv $outfile libgimpbase/gimpversion.h
  fi
],[
  gimp_major_ver=gimp_major_version
  gimp_minor_ver=gimp_minor_version
  gimp_micro_ver=gimp_micro_version
  gimp_ver=gimp_version
  gimp_api_ver=gimp_api_version
])

AC_CONFIG_FILES([
tools/Makefile
tools/pdbgen/Makefile
libgimp/Makefile
libgimpbase/Makefile
libgimpconfig/Makefile
libgimpcolor/Makefile
libgimpmath/Makefile
libgimpmodule/Makefile
libgimpthumb/Makefile
libgimpwidgets/Makefile
app/gimp.rc
app/Makefile
app/actions/Makefile
app/base/Makefile
app/config/Makefile
app/core/Makefile
app/gegl/Makefile
app/dialogs/Makefile
app/display/Makefile
app/file/Makefile
app/gui/Makefile
app/menus/Makefile
app/paint/Makefile
app/paint-funcs/Makefile
app/composite/Makefile
app/pdb/Makefile
app/plug-in/Makefile
app/text/Makefile
app/tools/Makefile
app/vectors/Makefile
app/widgets/Makefile
app/xcf/Makefile
app/tests/Makefile
plug-ins/Makefile
plug-ins/color-rotate/Makefile
plug-ins/color-rotate/images/Makefile
plug-ins/file-bmp/Makefile
plug-ins/file-faxg3/Makefile
plug-ins/file-fits/Makefile
plug-ins/file-fli/Makefile
plug-ins/file-ico/Makefile
plug-ins/file-jpeg/Makefile
plug-ins/file-psd/Makefile
plug-ins/file-sgi/Makefile
plug-ins/file-uri/Makefile
plug-ins/file-xjt/Makefile
plug-ins/flame/Makefile
plug-ins/fractal-explorer/Makefile
plug-ins/fractal-explorer/examples/Makefile
plug-ins/gfig/Makefile
plug-ins/gfig/gfig-examples/Makefile
plug-ins/gfig/images/Makefile
plug-ins/gimpressionist/Brushes/Makefile
plug-ins/gimpressionist/Makefile
plug-ins/gimpressionist/Paper/Makefile
plug-ins/gimpressionist/Presets/Makefile
plug-ins/gradient-flare/Makefile
plug-ins/gradient-flare/flares/Makefile
plug-ins/help-browser/Makefile
plug-ins/help/Makefile
plug-ins/ifs-compose/Makefile
plug-ins/imagemap/Makefile
plug-ins/imagemap/images/Makefile
plug-ins/lighting/Makefile
plug-ins/lighting/images/Makefile
plug-ins/map-object/Makefile
plug-ins/maze/Makefile
plug-ins/metadata/Makefile
plug-ins/pagecurl/Makefile
plug-ins/print/Makefile
plug-ins/pygimp/Makefile
plug-ins/pygimp/plug-ins/Makefile
plug-ins/script-fu/Makefile
plug-ins/script-fu/ftx/Makefile
plug-ins/script-fu/scripts/Makefile
plug-ins/script-fu/scripts/images/Makefile
plug-ins/script-fu/tinyscheme/Makefile
plug-ins/selection-to-path/Makefile
plug-ins/twain/Makefile
plug-ins/win-snap/Makefile
plug-ins/common/Makefile
modules/Makefile
devel-docs/Makefile
devel-docs/app/Makefile
devel-docs/app/version
devel-docs/libgimp/Makefile
devel-docs/libgimp/version
devel-docs/libgimpbase/Makefile
devel-docs/libgimpbase/version
devel-docs/libgimpconfig/Makefile
devel-docs/libgimpconfig/version
devel-docs/libgimpcolor/Makefile
devel-docs/libgimpcolor/version
devel-docs/libgimpmath/Makefile
devel-docs/libgimpmath/version
devel-docs/libgimpmodule/Makefile
devel-docs/libgimpmodule/version
devel-docs/libgimpthumb/Makefile
devel-docs/libgimpthumb/version
devel-docs/libgimpwidgets/Makefile
devel-docs/libgimpwidgets/version
devel-docs/tools/Makefile
docs/Makefile
menus/Makefile
cursors/Makefile
themes/Makefile
themes/Default/Makefile
themes/Default/images/Makefile
themes/Default/images/preferences/Makefile
themes/Small/Makefile
data/Makefile
data/brushes/Makefile
data/dynamics/Makefile
data/environ/Makefile
data/gradients/Makefile
data/images/Makefile
data/interpreters/Makefile
data/palettes/Makefile
data/patterns/Makefile
data/tags/Makefile
data/tips/Makefile
desktop/Makefile
desktop/gimp.desktop.in
etc/Makefile
m4macros/Makefile
po/Makefile.in
po-libgimp/Makefile.in
po-plug-ins/Makefile.in
po-python/Makefile.in
po-script-fu/Makefile.in
po-tips/Makefile.in
gimp-zip
Makefile
])

# Files with versions in their names
AC_CONFIG_FILES(
docs/gimp-gimp_app_version.1:docs/gimp.1.in
docs/gimprc-gimp_app_version.5:docs/gimprc.5.in
docs/gimptool-gimp_tool_version.1:docs/gimptool.1.in
docs/gimp-remote-gimp_app_version.1:docs/gimp-remote.1.in
gimp-gimp_pkgconfig_version.pc:gimp.pc.in
gimpthumb-gimp_pkgconfig_version.pc:gimpthumb.pc.in
gimpui-gimp_pkgconfig_version.pc:gimpui.pc.in
)

AC_CONFIG_COMMANDS([sed-po-makefiles],
[sed -e "/POTFILES =/r po-libgimp/POTFILES" po-libgimp/Makefile.in > po-libgimp/Makefile && touch po-libgimp/stamp-it
sed -e "/POTFILES =/r po-python/POTFILES" po-python/Makefile.in > po-python/Makefile && touch po-python/stamp-it
sed -e "/POTFILES =/r po-plug-ins/POTFILES" po-plug-ins/Makefile.in > po-plug-ins/Makefile && touch po-plug-ins/stamp-it
sed -e "/POTFILES =/r po-script-fu/POTFILES" po-script-fu/Makefile.in > po-script-fu/Makefile && touch po-script-fu/stamp-it
sed -e "/POTFILES =/r po-tips/POTFILES" po-tips/Makefile.in > po-tips/Makefile && touch po-tips/stamp-it])

AC_OUTPUT

# Print a summary of features enabled/disabled:
AC_MSG_RESULT([
Building GIMP with prefix=$prefix, datarootdir=$datarootdir
Desktop files install into $DESKTOP_DATADIR

Extra Binaries:
  gimp-console:        $enable_gimp_console
  gimp-remote:         $enable_gimp_remote

Optional Features:
  D-Bus service:       $have_dbus_glib
  Language selection:  $have_iso_codes

Optional Plug-Ins:
  Ascii Art:           $have_libaa
  Help Browser:        $have_webkit
  LCMS:                $have_lcms
  JPEG:                $jpeg_ok
  JPEG 2000:           $have_jp2
  MNG:                 $have_libmng
  PDF:                 $have_poppler
  PNG:                 $have_libpng
  Print:               $enable_print
  PSP:                 $have_zlib
  Python:              $enable_python
  Script-Fu:           $have_script_fu
  SVG:                 $have_librsvg
  TIFF:                $have_libtiff
  TWAIN (MacOS X):     $mac_twain_ok
  TWAIN (Win32):       $os_win32
  URI:                 $uri_plugin
  Windows ICO          $have_libpng
  WMF:                 $have_libwmf
  XJT:                 $have_xjt
  X11 Mouse Cursor:    $have_xmc
  XPM:                 $have_libxpm

Plug-In Features:
  EXIF support:        $have_libexif

Optional Modules:
  ALSA (MIDI Input):   $have_alsa
  Linux Input:         $have_linux_input (HAL support: $have_libhal)
  DirectInput (Win32): $have_dx_dinput
  Color Correction:    $have_lcms
  Soft Proof:          $have_lcms
]);
