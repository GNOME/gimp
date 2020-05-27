# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.54)

# Making releases on the stable branch:
#   gimp_micro_version += 1;
#   gimp_interface_age += 1;
#
# For a description of libtool version variables, see:
# devel-docs/libtool-instructions.txt

m4_define([gimp_major_version], [2])
m4_define([gimp_minor_version], [99])
m4_define([gimp_micro_version], [1])
m4_define([gimp_real_version],
          [gimp_major_version.gimp_minor_version.gimp_micro_version])
m4_define([gimp_version], [gimp_real_version])
m4_define([gimp_interface_age], [0])
m4_define([gimp_binary_age],
          [m4_eval(100 * gimp_minor_version + gimp_micro_version)])

# For overriding the version string. Comment out if not needed.
# m4_define([gimp_version], [2.99.1])

# This is the X.Y used in -lgimp-X.Y
m4_define([gimp_api_major_version], [3])
m4_define([gimp_api_minor_version], [0])
m4_define([gimp_api_version],
          [gimp_api_major_version.gimp_api_minor_version])

# Versions used for apps, plugins, tools, pkg-config files, and data,
# as well as global and user prefs
m4_define([gimp_app_version], [2.99])
m4_define([gimp_plugin_version], [2.99])
m4_define([gimp_tool_version], [2.99])
m4_define([gimp_pkgconfig_version], [3.0])
m4_define([gimp_data_version], [2.99])
m4_define([gimp_sysconf_version], [2.99])
m4_define([gimp_user_version], [2.99])

m4_define([gimp_unstable],
          m4_if(m4_eval(gimp_minor_version % 2), [1], [yes], [no]))
m4_define([gimp_stable],
          m4_if(m4_eval(gimp_minor_version % 2), [0], [yes], [no]))

m4_define([gimp_full_name], [GNU Image Manipulation Program])

# required versions of other packages
m4_define([alsa_required_version], [1.0.0])
m4_define([appstream_glib_required_version], [0.7.7])
m4_define([atk_required_version], [2.4.0])
m4_define([babl_major_minor_version], [0.1])
m4_define([babl_micro_version], [76])
m4_define([babl_required_version],
          [babl_major_minor_version.babl_micro_version])
m4_define([babl_required_version], [0.1.74])
m4_define([cairo_required_version], [1.14.0])
m4_define([cairo_pdf_required_version], [1.12.2])
m4_define([fontconfig_required_version], [2.12.4])
m4_define([freetype2_required_version], [2.1.7])
m4_define([gdk_pixbuf_required_version], [2.30.8])
m4_define([gegl_major_minor_version], [0.4])
m4_define([gegl_micro_version], [23])
m4_define([gegl_required_version],
          [gegl_major_minor_version.gegl_micro_version])
m4_define([gexiv2_required_version], [0.10.6])
m4_define([glib_required_version], [2.56.2])
m4_define([gtk_mac_integration_required_version], [2.0.0])
m4_define([gtk_required_version], [3.22.29])
m4_define([gtkdoc_required_version], [1.0])
m4_define([harfbuzz_required_version], [0.9.19])
m4_define([intltool_required_version], [0.40.1])
m4_define([introspection_required_version], [1.32.0])
m4_define([json_glib_required_version], [1.2.6])
m4_define([lcms_required_version], [2.8])
m4_define([libgudev_required_version], [167])
m4_define([libheif_required_version], [1.3.2])
m4_define([liblzma_required_version], [5.0.0])
m4_define([libmypaint_required_version], [1.3.0])
m4_define([libpng_required_version], [1.6.25])
m4_define([libunwind_required_version], [1.1.0])
m4_define([openexr_required_version], [1.6.1])
m4_define([openjpeg_required_version], [2.1.0])
m4_define([pangocairo_required_version], [1.42.0])
m4_define([perl_required_version], [5.10.0])
m4_define([poppler_required_version], [0.69.0])
m4_define([poppler_data_required_version], [0.4.9])
m4_define([python3_required_version], [3.6.0])
m4_define([rsvg_required_version], [2.40.6])
m4_define([vala_required_version], [0.40.0])
m4_define([webkitgtk_required_version], [2.20.3])
m4_define([webp_required_version], [0.6.0])
m4_define([wmf_required_version], [0.2.8])

# Current test considers only 2 version numbers. If we update the recommended
# version of gettext with more version numbers, please update the tests.
m4_define([xgettext_required_version], [0.19])

AC_INIT([GIMP], [gimp_version],
        [https://gitlab.gnome.org/GNOME/gimp/issues/new],
        [gimp])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([app/core/gimp.c])
AC_CONFIG_MACRO_DIR([m4macros])

AM_INIT_AUTOMAKE(no-define dist-bzip2 dist-xz no-dist-gzip -Wno-portability)

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
GIMP_API_MAJOR_VERSION=gimp_api_major_version
GIMP_API_MINOR_VERSION=gimp_api_minor_version
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
AC_SUBST(GIMP_API_MAJOR_VERSION)
AC_SUBST(GIMP_API_MINOR_VERSION)
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

# Version strings used in some source, though it seems unnecessary to
# declare them in the public API (libgimpbase/gimpversion.h).
AC_DEFINE(GIMP_PKGCONFIG_VERSION, "gimp_pkgconfig_version",
          [libgimp pkg-config version])
AC_DEFINE(GIMP_TOOL_VERSION, "gimp_tool_version",
          [GIMP tools version])

# These are used for substitution (.pc, INSTALL, etc.).
APPSTREAM_GLIB_REQUIRED_VERSION=appstream_glib_required_version
ATK_REQUIRED_VERSION=atk_required_version
BABL_MAJOR_MINOR_VERSION=babl_major_minor_version
BABL_REQUIRED_VERSION=babl_required_version
CAIRO_REQUIRED_VERSION=cairo_required_version
CAIRO_PDF_REQUIRED_VERSION=cairo_pdf_required_version
FONTCONFIG_REQUIRED_VERSION=fontconfig_required_version
FREETYPE2_REQUIRED_VERSION=freetype2_required_version
GDK_PIXBUF_REQUIRED_VERSION=gdk_pixbuf_required_version
GEGL_MAJOR_MINOR_VERSION=gegl_major_minor_version
GEGL_REQUIRED_VERSION=gegl_required_version
GEXIV2_REQUIRED_VERSION=gexiv2_required_version
GLIB_REQUIRED_VERSION=glib_required_version
GTK_REQUIRED_VERSION=gtk_required_version
HARFBUZZ_REQUIRED_VERSION=harfbuzz_required_version
INTLTOOL_REQUIRED_VERSION=intltool_required_version
LCMS_REQUIRED_VERSION=lcms_required_version
LIBHEIF_REQUIRED_VERSION=libheif_required_version
LIBLZMA_REQUIRED_VERSION=liblzma_required_version
LIBMYPAINT_REQUIRED_VERSION=libmypaint_required_version
LIBPNG_REQUIRED_VERSION=libpng_required_version
OPENEXR_REQUIRED_VERSION=openexr_required_version
OPENJPEG_REQUIRED_VERSION=openjpeg_required_version
PANGOCAIRO_REQUIRED_VERSION=pangocairo_required_version
PERL_REQUIRED_VERSION=perl_required_version
POPPLER_REQUIRED_VERSION=poppler_required_version
POPPLER_DATA_REQUIRED_VERSION=poppler_data_required_version
PYTHON3_REQUIRED_VERSION=python3_required_version
RSVG_REQUIRED_VERSION=rsvg_required_version
VALA_REQUIRED_VERSION=vala_required_version
WEBKITGTK_REQUIRED_VERSION=webkitgtk_required_version
WEBP_REQUIRED_VERSION=webp_required_version
WMF_REQUIRED_VERSION=wmf_required_version
LIBUNWIND_REQUIRED_VERSION=libunwind_required_version
XGETTEXT_REQUIRED_VERSION=xgettext_required_version
AC_SUBST(APPSTREAM_GLIB_REQUIRED_VERSION)
AC_SUBST(ATK_REQUIRED_VERSION)
AC_SUBST(BABL_MAJOR_MINOR_VERSION)
AC_SUBST(BABL_REQUIRED_VERSION)
AC_SUBST(CAIRO_REQUIRED_VERSION)
AC_SUBST(CAIRO_PDF_REQUIRED_VERSION)
AC_SUBST(FONTCONFIG_REQUIRED_VERSION)
AC_SUBST(FREETYPE2_REQUIRED_VERSION)
AC_SUBST(GDK_PIXBUF_REQUIRED_VERSION)
AC_SUBST(GEGL_MAJOR_MINOR_VERSION)
AC_SUBST(GEGL_REQUIRED_VERSION)
AC_SUBST(GEXIV2_REQUIRED_VERSION)
AC_SUBST(GLIB_REQUIRED_VERSION)
AC_SUBST(GTK_REQUIRED_VERSION)
AC_SUBST(HARFBUZZ_REQUIRED_VERSION)
AC_SUBST(INTLTOOL_REQUIRED_VERSION)
AC_SUBST(LCMS_REQUIRED_VERSION)
AC_SUBST(LIBHEIF_REQUIRED_VERSION)
AC_SUBST(LIBLZMA_REQUIRED_VERSION)
AC_SUBST(LIBMYPAINT_REQUIRED_VERSION)
AC_SUBST(LIBPNG_REQUIRED_VERSION)
AC_SUBST(OPENEXR_REQUIRED_VERSION)
AC_SUBST(OPENJPEG_REQUIRED_VERSION)
AC_SUBST(PANGOCAIRO_REQUIRED_VERSION)
AC_SUBST(PERL_REQUIRED_VERSION)
AC_SUBST(POPPLER_REQUIRED_VERSION)
AC_SUBST(POPPLER_DATA_REQUIRED_VERSION)
AC_SUBST(PYTHON3_REQUIRED_VERSION)
AC_SUBST(RSVG_REQUIRED_VERSION)
AC_SUBST(VALA_REQUIRED_VERSION)
AC_SUBST(WEBKITGTK_REQUIRED_VERSION)
AC_SUBST(WEBP_REQUIRED_VERSION)
AC_SUBST(WMF_REQUIRED_VERSION)
AC_SUBST(LIBUNWIND_REQUIRED_VERSION)
AC_SUBST(XGETTEXT_REQUIRED_VERSION)

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
GETTEXT_PACKAGE=gimp30
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
                   [The prefix for our gettext translation domains.])

# work around intltool-update issues during 'make distcheck'
AS_IF([test "x$0" != "x./configure"], [
        AC_SUBST([INTLTOOL_UPDATE], [/bin/true])
])

# Determine a C compiler to use
AC_PROG_CC
AX_PROG_CC_FOR_BUILD
# Works around a bug in AX_PROG_CC_FOR_BUILD for build with MinGW on
# Windows. See bug 780270, comment 18.
case "$build_os" in
  cygwin*|mingw32*|mingw64*)	BUILD_EXEEXT=.exe ;;
esac
AM_PROG_CC_C_O

# Determine a C++ compiler to use
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX([14], , [mandatory])

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static win32-dll])

AM_PROG_AS

# Initialize maintainer mode
AM_MAINTAINER_MODE([enable])

# Check for pkg-config
PKG_PROG_PKG_CONFIG(0.16)


#########################
# Check host architecture
#########################

AC_CANONICAL_HOST

AC_MSG_CHECKING([for host type])
AC_MSG_RESULT([$host])

AC_MSG_CHECKING([for host processor])
case "$host_cpu" in
  i*86)
    have_x86=yes
    AC_DEFINE(ARCH_X86, 1, [Define to 1 if you are compiling for ix86.])
    AC_MSG_RESULT([x86])
    ;;
  x86_64)
    have_x86=yes
    AC_DEFINE(ARCH_X86, 1, [Define to 1 if you are compiling for ix86.])
    AC_DEFINE(ARCH_X86_64, 1, [Define to 1 if you are compiling for amd64.])
    AC_MSG_RESULT([x86-64])
    ;;
  ppc | powerpc)
    have_ppc=yes
    AC_DEFINE(ARCH_PPC, 1, [Define to 1 if you are compiling for PowerPC.])
    AC_MSG_RESULT([PowerPC])
    ;;
  ppc64 | powerpc64)
    have_ppc=yes
    AC_DEFINE(ARCH_PPC, 1, [Define to 1 if you are compiling for PowerPC.])
    AC_DEFINE(ARCH_PPC64, 1, [Define to 1 if you are compiling for PowerPC64.])
    AC_MSG_RESULT([64-bit PowerPC])
    ;;
  *)
    AC_MSG_RESULT([unknown])
    ;;
esac


#################
# Check for Linux
#################

AC_MSG_CHECKING([if compiling for Linux])
case "$host_os" in
  linux*)
    platform_linux=yes
    ;;
  *)
    platform_linux=no
    ;;
esac
AC_MSG_RESULT([$platform_linux])
AM_CONDITIONAL(PLATFORM_LINUX, test "$platform_linux" = "yes")


#################
# Check for Win32
#################

AC_MSG_CHECKING([if compiling for Win32])
case "$host_os" in
  mingw* | cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = "yes")

PATHSEP=':'
os_win32=no
if test "x$platform_win32" != xno; then
  AC_MSG_CHECKING([if this is native Win32])
  case "$host_os" in
    mingw*)
      os_win32=yes
      case "$host_cpu" in
        x86_64)
        ;;
        *)
        WIN32_LARGE_ADDRESS_AWARE='-Wl,--large-address-aware'
        ;;
      esac
      PATHSEP=';'
      ;;
    *)
      ;;
  esac
  AC_MSG_RESULT([$os_win32])
fi
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


####################
# Check for Mac OS X
####################

platform_osx=no
AC_MSG_CHECKING([if compiling for Mac OS X])
case "$host_os" in
  darwin*)
     AC_MSG_RESULT(yes)
     AC_DEFINE(PLATFORM_OSX, 1, [define to 1 if compiling for Mac OS X])
     platform_osx=yes
     ;;
  *)
     ;;
esac
AC_MSG_RESULT($platform_osx)
AM_CONDITIONAL(PLATFORM_OSX, test "x$platform_osx" = xyes)


###############
# Generic stuff
###############

# Checks for programs.
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LN_S

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

# We really don't want anyone using code with missing prototypes, which
# can cause random behavior. If compile breaks due to the following,
# come to us and we will point you to a newer version which works.
#
GIMP_DETECT_CFLAGS(extra_warnings, '-Werror=missing-prototypes')
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

GIMP_DETECT_CFLAGS(extra_warnings, '-Wlogical-op')
CFLAGS="$CFLAGS $extra_warnings"

GIMP_DETECT_CFLAGS(extra_warnings, '-Wtype-limits')
CFLAGS="$CFLAGS $extra_warnings"

GIMP_DETECT_CFLAGS(extra_warnings, '-fno-common')
CFLAGS="$CFLAGS $extra_warnings"

GIMP_DETECT_CFLAGS(extra_warnings, '-fdiagnostics-show-option')
CFLAGS="$CFLAGS $extra_warnings"

GIMP_DETECT_CFLAGS(extra_warnings, '-Wreturn-type')
CFLAGS="$CFLAGS $extra_warnings"

# -Wreturn-type warns on forgotten return on non-void functions,
# but not of g_return_if_fail() on non-void functions.
# -Wsystem-headers is needed for that.
# Unfortunately it pulls tons of warnings on some systems.
# So we comment out for now.
# GIMP_DETECT_CFLAGS(extra_warnings, '-Wsystem-headers')
# CFLAGS="$CFLAGS $extra_warnings"

# Extra optimization flags for super performance
GIMP_DETECT_CFLAGS(extra_warnings, '--omg-optimized')
CFLAGS="$CFLAGS $extra_warnings"

GIMP_DETECT_CFLAGS(extra_warnings, '--disable-instructions')
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

if test "x$GCC" = xyes; then
  gcc_version=`$CC -v 2>&1|grep 'gcc version'|sed -e 's/.*gcc version \([[0-9.]]\+\)[[^0-9.]].*/\1/'`
  if test "x$gcc_version" = "x7.2.0" ; then
    warning_gcc="
WARNING: GCC 7.2.0 has a serious bug affecting GEGL/GIMP. We advise
         against using this version of the compiler (previous and
         further versions are fine).
         See https://bugzilla.gnome.org/show_bug.cgi?id=787222"
  fi
  # For GCC, use -v which has more information than --version.
  CC_VERSION_OPTS="-v"
else
  # This is the list of common options used by autotools to check
  # versions for various compilers.
  CC_VERSION_OPTS="--version -v -V -qversion"
fi

for CC_VERSION_OPT in $CC_VERSION_OPTS; do
  # We run $CC, and escape and format its output, in a single step,
  # since some shells expand escape sequences in "echo" arguments.
  CC_VERSION="`$CC $CC_VERSION_OPT 2>&1 | sed -e 's/\\\\/\\\\\\\\\\\\\\\\/g;s/^/\\\\t/;s/$/\\\\n/' | tr -d '\n'`"
  if test $? -eq 0; then
    break
  fi
done

AC_SUBST(CC_VERSION)

AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_TIME

AC_CHECK_HEADERS(execinfo.h fcntl.h sys/param.h sys/prctl.h sys/thr.h sys/time.h sys/times.h sys/wait.h unistd.h)
AC_CHECK_FUNCS(backtrace, , AC_CHECK_LIB(execinfo, backtrace))

AC_TYPE_PID_T
AC_FUNC_VPRINTF

AC_FUNC_ALLOCA

# check some more funcs
AC_CHECK_FUNCS(fsync)
AC_CHECK_FUNCS(difftime mmap)
AC_CHECK_FUNCS(thr_self)


# _NL_MEASUREMENT_MEASUREMENT is an enum and not a define
AC_MSG_CHECKING([for _NL_MEASUREMENT_MEASUREMENT])
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM(
    [[#include <langinfo.h>]],
    [[char c = *((unsigned char *)  nl_langinfo(_NL_MEASUREMENT_MEASUREMENT));]])],
  [nl_ok=yes],
  [nl_ok=no])
AC_MSG_RESULT($nl_ok)
if test "$nl_ok" = "yes"; then
  AC_DEFINE(HAVE__NL_MEASUREMENT_MEASUREMENT, 1,
	    [Define to 1 if _NL_MEASUREMENT_MEASUREMENT is available])
fi

# Macro to keep track of failed dependencies.

required_deps=''

# To add a required dependency error, call: add_deps_error([dependency]).
# If details about the dependency are needed:
# add_deps_error([dependency], [first line, second line])
m4_define([add_deps_error], [
  AC_MSG_NOTICE([Eeeeeeeeeeeeeeeeeeeeek! Missing dep: $1])
  required_deps="$required_deps
  - Error: missing dependency $1"
  m4_foreach([more], [$2], [
  required_deps="$required_deps
      *** more"])])

######################
# Internationalisation
######################

IT_PROG_INTLTOOL(intltool_required_version)
AM_GLIB_GNU_GETTEXT

# Testing xgettext version since we had some problem with localizing script-fu. See bug 720052.
AC_MSG_CHECKING([for xgettext version (>= xgettext_required_version)])
XGETTEXT_REQUIRED_VERSION_MAJOR=`echo $XGETTEXT_REQUIRED_VERSION | awk -F. '{ print $ 1; }'`
XGETTEXT_REQUIRED_VERSION_MINOR_1=`echo $XGETTEXT_REQUIRED_VERSION | awk -F. '{ print $ 2; }'`

XGETTEXT_APPLIED_VERSION=`$XGETTEXT --version | head -1 | cut -d" " -f4`
XGETTEXT_APPLIED_VERSION_MAJOR=`echo $XGETTEXT_APPLIED_VERSION | awk -F. '{ print $ 1; }'`
XGETTEXT_APPLIED_VERSION_MINOR_1=`echo $XGETTEXT_APPLIED_VERSION | awk -F. '{ print $ 2; }'`

have_required_xgettext="no"
if test -n "$XGETTEXT_APPLIED_VERSION_MAJOR"; then
    if test "$XGETTEXT_REQUIRED_VERSION_MAJOR" -lt "$XGETTEXT_APPLIED_VERSION_MAJOR"; then
        have_required_xgettext="yes"
    elif test "$XGETTEXT_REQUIRED_VERSION_MAJOR" -eq "$XGETTEXT_APPLIED_VERSION_MAJOR"; then
        if test -n "$XGETTEXT_APPLIED_VERSION_MINOR_1"; then
            if test "$XGETTEXT_REQUIRED_VERSION_MINOR_1" -le "$XGETTEXT_APPLIED_VERSION_MINOR_1"; then
                have_required_xgettext="yes"
            fi
        fi
    fi
fi
AC_MSG_RESULT($have_required_xgettext)

if test "x$have_required_xgettext" = "xno"; then
    add_deps_error([xgettext], [xgettext >= xgettext_required_version])
fi

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

PKG_CHECK_MODULES(BABL, [babl >= babl_required_version],,
                  [add_deps_error([babl >= babl_required_version])])
PKG_CHECK_MODULES(GEGL, gegl-gegl_major_minor_version >= gegl_required_version,,
                  [add_deps_error([gegl-gegl_major_minor_version >= gegl_required_version])])
PKG_CHECK_MODULES(ATK, atk >= atk_required_version,,
                  [add_deps_error([atk >= atk_required_version])])

AM_PATH_GLIB_2_0(glib_required_version,,
                 [add_deps_error([glib >= glib_required_version],
                                 [Test for GLIB failed])],
                 gobject)

PKG_CHECK_MODULES(GMODULE_NO_EXPORT, gmodule-no-export-2.0,,
                  [add_deps_error([gmodule-no-export-2.0])])

PKG_CHECK_MODULES(GIO, gio-2.0,,
                  [add_deps_error([gio-2.0])])

if test "x$os_win32" = "xyes"; then
  PKG_CHECK_MODULES(GIO_WINDOWS, gio-windows-2.0,,
                    [add_deps_error([gio-windows-2.0])])
else
  PKG_CHECK_MODULES(GIO_UNIX, gio-unix-2.0,,
                    [add_deps_error([gio-unix-2.0])])
fi

AC_MSG_CHECKING([for glib-networking (GIO TLS implementation)])
gimp_save_CFLAGS="$CFLAGS"
gimp_save_LIBS="$LIBS"
CFLAGS="$CFLAGS $GIO_CFLAGS"
LIBS="$LIBS $GIO_LIBS"
AC_RUN_IFELSE([AC_LANG_PROGRAM([#include <gio/gio.h>],
                               [return !g_tls_backend_supports_tls (g_tls_backend_get_default ());])],
              [have_glib_networking="yes"],
              [have_glib_networking="no"],
              [have_glib_networking="unknown (cross-compiling)"])
CFLAGS="$gimp_save_CFLAGS"
LIBS="$gimp_save_LIBS"
AC_MSG_RESULT($have_glib_networking)

if test "x$have_glib_networking" = "xno"; then
  add_deps_error([glib-networking],
      [Test for glib-networking failed. This is required.])
elif test "x$have_glib_networking" != "xyes"; then
  warning_glib_networking="
WARNING: Test for glib-networking cannot be performed while cross-compiling.
         Make sure glib-networking is installed, otherwise GIMP will not be able
         to display the remote help pages through the help browser, nor will it
         be able to open remote HTTPS (or other protocol using SSL/TLS) files.
         HTTPS is becoming the expected standard and should not be considered
         optional anymore."
fi

# AM_PATH_GLIB_2_0 already fills in the variable GLIB_COMPILE_RESOURCES.
# Unfortunately it looks it up using $PKG_CONFIG, which search in
# target-installed libraries, hence it would return a tool for the
# target architecture in case of cross-compilation (which can't be run
# during build, unless there is some compatibility layer in the OS).
# Therefore we make our own check.
AC_MSG_CHECKING([for native glib-compile-resources])
if test -z "$HOST_GLIB_COMPILE_RESOURCES"; then
  # Do not use $PKG_CONFIG as it will search target files.
  HOST_GLIB_COMPILE_RESOURCES=`pkg-config --variable=glib_compile_resources gio-2.0`
  if test -z "$HOST_GLIB_COMPILE_RESOURCES"; then
    # As fallback, search in path.
    AC_PATH_PROG(HOST_GLIB_COMPILE_RESOURCES, glib-compile-resources, no)
    if test "x$HOST_GLIB_COMPILE_RESOURCES" = xno; then
      add_deps_error([glib-compile-resources],
                     [Could not find glib-compile-resources in your PATH.])
    fi
  fi
fi
AC_MSG_RESULT($HOST_GLIB_COMPILE_RESOURCES)
AC_SUBST(HOST_GLIB_COMPILE_RESOURCES)

AC_PATH_PROG(GDBUS_CODEGEN, gdbus-codegen, no)
if test "x$GDBUS_CODEGEN" = xno; then
  add_deps_error([gdbus-codegen], [Could not find gdbus-codegen in your PATH.])
fi

AC_MSG_CHECKING([if GLib is version 2.57.0 or newer])
if $PKG_CONFIG --atleast-version=2.57.0 glib-2.0; then
  have_glib_2_57=yes
else
  have_glib_2_57=no
fi
AC_MSG_RESULT($have_glib_2_57)

AC_PATH_PROG(GEGL, gegl, no)
if test "x$GEGL" = xno; then
  add_deps_error([gegl], [Could not find native gegl executable in your PATH.])
fi


# Check for bind_textdomain_codeset, including -lintl if GLib brings it in.
gimp_save_LIBS=$LIBS
LIBS="$LIBS $GLIB_LIBS"
AC_CHECK_FUNCS(bind_textdomain_codeset,,
  add_deps_error([bind_textdomain_codeset()],
                 [Check for bind_textdomain_codeset() failed! This is required.]))
LIBS=$gimp_save_LIBS


AC_PATH_XTRA

# Do not delay GTK errors and exit immediately as it will provoke
# harder to understand syntax errors of missing autoconf macros.
PKG_CHECK_MODULES(GTK, gtk+-3.0 >= gtk_required_version);

AC_PATH_PROGS(GTK_UPDATE_ICON_CACHE, gtk-update-icon-cache-3.0 gtk-update-icon-cache, no)
if test "x$GTK_UPDATE_ICON_CACHE" = xno; then
  add_deps_error([gtk-update-icon-cache],
                 [Could not find gtk-update-icon-cache in your PATH.])
fi

PKG_CHECK_MODULES(GDK_PIXBUF, gdk-pixbuf-2.0 >= gdk_pixbuf_required_version,,
                  [add_deps_error([gdk-pixbuf-2.0 >= gdk_pixbuf_required_version])])

AC_PATH_PROG(GDK_PIXBUF_CSOURCE, gdk-pixbuf-csource, no)
if test "x$GDK_PIXBUF_CSOURCE" = xno; then
  add_deps_error([gdk-pixbuf-csource],
                 [Could not find gdk-pixbuf-csource in your PATH.])
fi

PKG_CHECK_MODULES(CAIRO, cairo >= cairo_required_version,,
                  [add_deps_error([cairo >= cairo_required_version])])

PKG_CHECK_MODULES(PANGOCAIRO,
                  [pangocairo >= pangocairo_required_version pangoft2], :,
                  [add_deps_error([pangocairo >= pangocairo_required_version and pangoft2],
                                  [We require Pango with the optional support for Cairo compiled in.])])

gimp_save_CFLAGS=$CFLAGS
CFLAGS="$PANGOCAIRO_CFLAGS $CFLAGS"

# Windows platform had a few bugs fixed in earlier fontconfig.
# The bugs are serious enough (for instance very slow startups at each
# GIMP start, but also slowness when closing, broken font features,
# etc.) that we should bump the requirement for Win32.
# See bugs 708110 (fontconfig 2.11.0), 703331 (2.11.1) and 782676
# (2.12.4).
PKG_CHECK_MODULES(FONTCONFIG, fontconfig >= fontconfig_required_version,,
                  [add_deps_error([fontconfig >= fontconfig_required_version])])

AC_MSG_CHECKING([if Pango is version 1.43.0 or newer])
if $PKG_CONFIG --atleast-version=1.43.0 pango; then
  have_pango_1_43=yes
else
  have_pango_1_43=no
fi
AC_MSG_RESULT($have_pango_1_43)

CFLAGS="$PANGOCAIRO_CFLAGS $CFLAGS"

AC_MSG_CHECKING([if Pango is built with a recent fontconfig])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM(
    [[#include <fontconfig/fontconfig.h>]],
    [[FcObjectSet *os; os = FcObjectSetBuild (FC_FAMILY, FC_WIDTH);]])],
  [fontconfig_ok=yes],
  [fontconfig_ok=no])
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


PKG_CHECK_MODULES(FREETYPE, freetype2 >= freetype2_required_version,,
                  [add_deps_error([freetype2_required_version >= freetype2_required_version])])

PKG_CHECK_MODULES(HARFBUZZ, harfbuzz >= harfbuzz_required_version,,
                 [add_deps_error([harfbuzz >= harfbuzz_required_version])])

PKG_CHECK_MODULES(GEXIV2, gexiv2 >= gexiv2_required_version,,
                 [add_deps_error([gexiv2 >= gexiv2_required_version])])

#################################
# Check for debug console (Win32)
#################################

AC_ARG_ENABLE(win32-debug-console,
              [  --enable-win32-debug-console
                          open a console when starting the program
                          (default=auto)],,
              enable_win32_debug_console=auto)

AC_MSG_CHECKING([if GIMP should open a debug console at start])
if test "x$platform_win32" != "xyes"; then
   enable_win32_debug_console=no
elif test "x$enable_win32_debug_console" != xno &&
     test "x$enable_win32_debug_console" != xyes; then
  # By default, we enable the debug console only for unstable builds.
   enable_win32_debug_console=gimp_unstable
fi

if test "x$enable_win32_debug_console" = "xyes"; then
  AC_DEFINE(ENABLE_WIN32_DEBUG_CONSOLE, 1,
            [Define to 1 if GIMP should open a debug console at start])
fi
AC_MSG_RESULT([$enable_win32_debug_console])

######################################
# Check for 32-bit DLLs (Win32 64-bit)
######################################

AC_ARG_WITH(win32-32bit-dll-folder,
            [  --with-win32-32bit-dll-folder[=DIR]
                          alternative folder with 32-bit versions of DLL libraries on Windows (default: 32/bin/)],,
            with_win32_32bit_dll_folder="32/bin")

AC_MSG_CHECKING([the alternative folder containing 32-bit DLLs on Windows])
if test "x$platform_win32" = "xyes"; then
  case "$host_cpu" in
    x86_64)
    ;;
    *)
    with_win32_32bit_dll_folder="no"
    ;;
  esac
else
    with_win32_32bit_dll_folder="no"
fi

if test "x$with_win32_32bit_dll_folder" != "xno"; then
  AC_DEFINE_UNQUOTED(WIN32_32BIT_DLL_FOLDER, "$with_win32_32bit_dll_folder",
                     [Define if 32-bit versions of libraries are packaged in an alternative folder])
fi
AC_MSG_RESULT([$with_win32_32bit_dll_folder])

#####################
# Check for Dr. Mingw
#####################

enable_drmingw="no"
if test "x$platform_win32" = "xyes"; then
   # Check for Dr. Mingw
   AC_CHECK_LIB(exchndl, ExcHndlSetLogFileNameA,
                [
                 AC_DEFINE(HAVE_EXCHNDL, 1, [Define to 1 when Dr. MingW is available])
                 LIBS="-lexchndl $LIBS"
                 enable_drmingw="yes"
                ],
                [enable_drmingw="no (ExcHndl library missing)"]
               )
fi
AM_CONDITIONAL(HAVE_EXCHNDL, test "x$ac_cv_lib_exchndl_ExcHndlSetLogFileNameA" = "xyes")

###########################
# Check for backtrace() API
###########################

AC_CHECK_HEADERS([execinfo.h])

#########################
# Check for Matting Levin
#########################

AC_MSG_CHECKING([for matting engine Levin])
if $GEGL --exists "gegl:matting-levin"; then
  have_matting_levin=yes
else
  have_matting_levin='no (missing GEGL operation "gegl:matting-levin")'
fi
AC_MSG_RESULT($have_matting_levin)

##########################################
# Check for some special functions we need
##########################################

AC_CHECK_HEADERS(math.h ieeefp.h)

# DU4 native cc currently needs -std1 for ANSI mode (instead of K&R)
AC_MSG_CHECKING([for extra flags to get ANSI library prototypes])
gimp_save_LIBS=$LIBS
LIBS="$LIBS -lm"
AC_RUN_IFELSE(
  [AC_LANG_SOURCE([[
    #include <math.h>
    int main (void) { return (log(1) != log(1.)); }]])],
  [AC_MSG_RESULT(none needed)],
  [gimp_save_CFLAGS=$CFLAGS
   CFLAGS="$CFLAGS -std1"
   AC_RUN_IFELSE(
     [AC_LANG_SOURCE([[
       #include <math.h>
       int main (void) { return (log(1) != log(1.)); }]])],
     [AC_MSG_RESULT(-std1)],
     [AC_MSG_RESULT()
      CFLAGS=$gimp_save_CFLAGS
      AC_MSG_WARN([No ANSI prototypes found in library. (-std1 didn't work.)])],
     [AC_MSG_RESULT])],
  [AC_MSG_RESULT()])

LIBS=$gimp_save_LIBS


# check for rint
AC_CHECK_FUNC(rint, AC_DEFINE(HAVE_RINT, 1,
                              [Define to 1 if you have the rint function.]), [
  AC_CHECK_LIB(m, rint, [
    AC_DEFINE(HAVE_RINT)])])


# check for vfork
AC_CHECK_FUNCS(vfork, ,)
AC_SUBST(HAVE_VFORK)


##############################
# Check for attributes we need
##############################

AX_GCC_FUNC_ATTRIBUTE(destructor)

#####################################################################
# Check for extra libs needed for getaddrinfo, getnameinfo and socket
#####################################################################

gimp_save_LIBS=$LIBS
LIBS=""

AC_CHECK_FUNCS(getaddrinfo, , AC_CHECK_LIB(nsl, getaddrinfo))
AC_CHECK_FUNCS(getnameinfo, , AC_CHECK_LIB(nsl, getnameinfo))
AC_CHECK_LIB(socket, socket)

SOCKET_LIBS="$LIBS"
LIBS=$gimp_save_LIBS

AC_SUBST(SOCKET_LIBS)

###################################
# Check for Intel vector intrinsics
###################################
intrinsics_save_CFLAGS="$CFLAGS"
#FIXME: Check the CFLAGS separately
GIMP_DETECT_CFLAGS(SSE_MATH_CFLAG, '-mfpmath=sse')
GIMP_DETECT_CFLAGS(SSE2_CFLAG, '-msse2')
SSE2_EXTRA_CFLAGS="$SSE_MATH_CFLAG $SSE2_CFLAG"
CFLAGS="$SSE2_EXTRA_CFLAGS $intrinsics_save_CFLAGS"

AC_MSG_CHECKING(whether we can compile SSE2 intrinsics)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <emmintrin.h>]],[[__m128i one = _mm_set1_epi32 (1);]])],
  AC_DEFINE(COMPILE_SSE2_INTRINISICS, 1, [Define to 1 if SSE2 intrinsics are available.])
  AC_SUBST(SSE2_EXTRA_CFLAGS)
  AC_MSG_RESULT(yes)
,
  AC_MSG_RESULT(no)
  AC_MSG_WARN([SSE2 intrinsics not available.])
)


GIMP_DETECT_CFLAGS(SSE4_1_CFLAG, '-msse4.1')
SSE4_1_EXTRA_CFLAGS="$SSE_MATH_CFLAG $SSE4_1_CFLAG"
CFLAGS="$SSE4_1_EXTRA_CFLAGS $intrinsics_save_CFLAGS"

AC_MSG_CHECKING(whether we can compile SSE4.1 intrinsics)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <smmintrin.h>]],[[__v4sf a, b, c; c = _mm_blend_ps(a, b, 0x08);]])],
  AC_DEFINE(COMPILE_SSE4_1_INTRINISICS, 1, [Define to 1 if SSE4.1 intrinsics are available.])
  AC_SUBST(SSE4_1_EXTRA_CFLAGS)
  AC_MSG_RESULT(yes)
,
  AC_MSG_RESULT(no)
  AC_MSG_WARN([SSE4.1 intrinsics not available.])
)
CFLAGS="$intrinsics_save_CFLAGS"


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

  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[[asm ("movq 0, %mm0");]])],

    AC_DEFINE(USE_MMX, 1, [Define to 1 if MMX assembly is available.])
    AC_MSG_RESULT(yes)

    if test "x$enable_sse" = xyes; then
      GIMP_DETECT_CFLAGS(sse_flag, '-msse')
      SSE_EXTRA_CFLAGS="$MMX_EXTRA_CFLAGS $sse_flag"

      AC_MSG_CHECKING(whether we can compile SSE code)

      CFLAGS="$CFLAGS $sse_flag"

      AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[[asm ("movntps %xmm0, 0");]])],
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
    case "$host_os" in
      darwin*)
        can_use_altivec=yes
        AC_DEFINE(HAVE_ALTIVEC_SYSCTL, 1,
         [Define to 1 if the altivec runtime test should use a sysctl.])
        ;;
      *)
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[[asm ("vand %v0, %v0, %v0");]])],
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
case "$host_os" in
  darwin* | rhapsody* | machten*)
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
    AC_RUN_IFELSE(
      [AC_LANG_SOURCE([[
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
      ]])],
      [AC_DEFINE([IPC_RMID_DEFERRED_RELEASE],[1],
                 [Define to 1 if shared memory segments are released deferred.])
       AC_MSG_RESULT(yes)],
      [AC_MSG_RESULT(no)],
      [AC_MSG_RESULT(assuming no)])

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
    case "$host_cpu" in
	  x86_64) SYMPREFIX='' ;;
	  *) SYMPREFIX='_' ;;
	esac
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
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [[#include <sys/types.h>]],
      [[fd_set readMask, writeMask;]])],
    [gimp_ok=yes],
    [gimp_ok=no])
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


################################
# Some plug-ins are X11 specific
################################

# Check if building for GTK+-X11
GTK_CHECK_BACKEND([x11], [gtk_required_version],
                  [have_gtk_x11=yes], [have_gtk_x11=no])

have_libxmu="no (building for `pkg-config --variable=targets gtk+-3.0`)"

if test "x$have_gtk_x11" = "xyes"; then

   # doc-shooter is X11 specific
   DOC_SHOOTER=doc-shooter

   # screenshot plug-in needs to link to X11 explicitly
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
          XMU_LIBS="$X_LIBS $X_PRE_LIBS -lX11 -lXmu -lXt"
          SCREENSHOT_LIBS="$XMU_LIBS",
	  [have_libxmu="no (XMU header file not found)"],[#include <gdk/gdkx.h>])],
	[have_libxmu="no (XMU library not found)"], -lXt -lX11)
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


GIMP_COMMAND='gimp-gimp_app_version'

AC_SUBST(GIMP_COMMAND)


############
# MIME types
############

# The list of MIME types that are supported by plug-ins that are compiled
# unconditionally:
MIME_TYPES="image/bmp;image/g3fax;image/gif;image/x-fits;image/x-pcx;image/x-portable-anymap;image/x-portable-bitmap;image/x-portable-graymap;image/x-portable-pixmap;image/x-psd;image/x-sgi;image/x-tga;image/x-xbitmap;image/x-xwindowdump;image/x-xcf;image/x-compressed-xcf;image/x-gimp-gbr;image/x-gimp-pat;image/x-gimp-gih"

#################
# Bundle packages
#################

AC_MSG_CHECKING([whether we build a relocatable package])
AC_ARG_ENABLE(relocatable-bundle,
              [  --enable-relocatable-bundle
                          build with resources considered bundled under the same prefix
                          (default=auto)],,
              enable_relocatable_bundle=auto)

if test "x$enable_relocatable_bundle" != xno &&
   test "x$enable_relocatable_bundle" != xyes; then
  # By default, assume building for Windows or macOS everything to be on
  # the same prefix and can be relocated.
  # On other platforms, build-time paths are meaningful.
  if test "x$platform_osx" = "xyes" ||
     test "x$platform_win32" = "xyes"; then
     enable_relocatable_bundle=yes
  else
     enable_relocatable_bundle=no
  fi
fi
if test "x$enable_relocatable_bundle" = xyes; then
  AC_DEFINE(ENABLE_RELOCATABLE_RESOURCES, 1,
            [Define to 1 if resources are considered bundled under the same prefix])
fi
AM_CONDITIONAL(ENABLE_RELOCATABLE_RESOURCES, test x$enable_relocatable_bundle = xyes)

AC_MSG_RESULT([$enable_relocatable_bundle])

###################
# Check for libtiff
###################

libtiff_error=
if test -z "$TIFF_LIBS"; then
  AC_CHECK_LIB(tiff, TIFFReadScanline,
    [AC_CHECK_HEADER(tiffio.h,
      FILE_TIFF_LOAD='file-tiff-load$(EXEEXT)'; FILE_TIFF_SAVE='file-tiff-save$(EXEEXT)'; TIFF_LIBS='-ltiff',
      [libtiff_error="TIFF header files not found"])],
    [AC_CHECK_LIB(tiff, TIFFWriteScanline,
      [AC_CHECK_HEADER(tiffio.h,
        FILE_TIFF_LOAD='file-tiff-load$(EXEEXT)'; FILE_TIFF_SAVE='file-tiff-save$(EXEEXT)'; TIFF_LIBS='-ltiff -ljpeg -lz',
	[libtiff_error="TIFF header files not found"])],
      [AC_CHECK_LIB(tiff34, TIFFFlushData,
	[AC_CHECK_HEADER(tiffio.h,
          FILE_TIFF_LOAD='file-tiff-load$(EXEEXT)'; FILE_TIFF_SAVE='file-tiff-save$(EXEEXT)'; TIFF_LIBS='-ltiff34 -ljpeg -lz',
	  [libtiff_error="TIFF header files not found"])],
      [libtiff_error="TIFF library not found"], -ljpeg -lz -lm)], -ljpeg -lz -lm)], -lm)
fi

if test -z "$TIFF_LIBS"; then
  add_deps_error([libtiff], [Checks for TIFF library failed: $libtiff_error])
fi

MIME_TYPES="$MIME_TYPES;image/tiff"

AC_SUBST(TIFF_LIBS)


###################
# Check for libjpeg
###################

PKG_CHECK_MODULES(JPEG, [libjpeg],, [add_deps_error([libjpeg])])
MIME_TYPES="$MIME_TYPES;image/jpeg"


################
# Check for libz
################

if test -z "$Z_LIBS"; then
  AC_CHECK_LIB(z, gzsetparams,
    [AC_CHECK_HEADER(zlib.h,
      Z_LIBS='-lz',
      add_deps_error([zlib], [ZLIB header files not found]))],
    add_deps_error([zlib], [ZLIB library not found]))
fi

MIME_TYPES="$MIME_TYPES;image/x-psp"

AC_SUBST(Z_LIBS)


####################
# Check for libbzip2
####################

gimp_save_LIBS=$LIBS
LIBS="$LIBS -lbz2"

if test -z "$BZIP2_LIBS"; then
  # We used to use AC_CHECK_LIB, but bz2 uses the WINAPI calling convention on
  # windows, so the test needs to #include <bzlib.h> to be able to link properly
  AC_CHECK_HEADER(bzlib.h,
    [AC_MSG_CHECKING([for BZ2_bzCompress in -lbz2]);
    AC_LINK_IFELSE(
      [AC_LANG_PROGRAM(
        [[#include <bzlib.h>]],
        [[BZ2_bzCompress (NULL, 0);]])],
      [AC_MSG_RESULT([yes]); BZIP2_LIBS='-lbz2'],
      add_deps_error([libbzip2], [libbzip2 library not found]))],
    add_deps_error([libbzip2], [libbzip2 header files not found]))
fi

LIBS=$gimp_save_LIBS

AC_SUBST(BZIP2_LIBS)


###################
# Check for liblzma
###################

PKG_CHECK_MODULES(LZMA, liblzma >= liblzma_required_version,,
                 [add_deps_error([liblzma >= liblzma_required_version])])


#############################
# Check for extension support
#############################

PKG_CHECK_MODULES(APPSTREAM_GLIB, appstream-glib >= appstream_glib_required_version,,
                  [add_deps_error([appstream-glib >= appstream_glib_required_version])])

PKG_CHECK_MODULES(LIBARCHIVE, libarchive,,
                  [add_deps_error([libarchive])])

###############################
# Check for Ghostscript library
###############################

AC_ARG_WITH(gs,  [  --without-gs            build without Ghostscript support])

have_gs=no
if test "x$with_gs" != xno; then
  have_gs=yes
  AC_CHECK_LIB(gs, gsapi_new_instance,
    [AC_CHECK_HEADER(ghostscript/iapi.h,
      FILE_PS='file-ps$(EXEEXT)'; GS_LIBS='-lgs',
      [have_gs="no (Ghostscript header files not found)"])],
    [have_gs="no (Ghostscript library not found)"])
  if test "x$have_gs" != xyes -a "x$with_gs" = xyes; then
    AC_MSG_ERROR([
*** Checks for Ghostscript library failed. You can build without it by passing
*** --without-gs to configure but you won't be able to use PostScript files then.])
  fi
fi

if test "x$have_gs" = xyes; then
  MIME_TYPES="$MIME_TYPES;application/postscript"
fi

AC_SUBST(FILE_PS)
AM_CONDITIONAL(HAVE_GS, test "x$have_gs" = xyes)
AC_SUBST(GS_LIBS)


##################
# Check for libpng
##################

PKG_CHECK_MODULES(PNG, libpng >= libpng_required_version,,
                  [add_deps_error([libpng >= libpng_required_version])])

MIME_TYPES="$MIME_TYPES;image/png;image/x-icon"
PNG_CFLAGS="$PNG_CFLAGS -DPNG_PEDANTIC_WARNINGS"


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
    FILE_MNG='file-mng$(EXEEXT)'; MNG_LIBS="-lmng $JPEG_LIBS $PNG_LIBS"; MNG_CFLAGS="$PNG_CFLAGS"
    if test "x$platform_win32" = "xyes"; then
      MNG_CFLAGS="$MNG_CFLAGS -DMNG_USE_DLL"
    fi
  else
    have_libmng="no (MNG header file not found)"
  fi
fi

AC_SUBST(FILE_MNG)
AC_SUBST(MNG_LIBS)
AC_SUBST(MNG_CFLAGS)


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

AC_ARG_WITH(libxpm,  [  --without-libxpm        build without XPM support])

gimp_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $X_CFLAGS"
gimp_save_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS $X_LIBS"
if test "x$have_gtk_x11" = "xyes"; then
  lib_X11=-lX11
fi
have_libxpm=no
if test "x$with_libxpm" != xno && test -z "$XPM_LIBS"; then
  have_libxpm=yes
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


###################
# Check for OpenEXR
###################

AC_ARG_WITH(openexr, [  --without-openexr       build without OpenEXR support])

have_openexr=no
if test "x$with_openexr" != xno; then
  have_openexr=yes
  PKG_CHECK_MODULES(OPENEXR, OpenEXR >= openexr_required_version,
    FILE_EXR='file-exr$(EXEEXT)',
    [have_openexr="no (OpenEXR not found)"])
fi

if test "x$have_openexr" = xyes; then
  MIME_TYPES="$MIME_TYPES;image/x-exr"
fi

AC_SUBST(OPENEXR_CFLAGS)
AC_SUBST(OPENEXR_LIBS)
AC_SUBST(FILE_EXR)

AM_CONDITIONAL(HAVE_OPENEXR, test "x$have_openexr" = xyes)


################
# Check for WebP
################

AC_ARG_WITH(webp, [  --without-webp       build without WebP support])

have_webp=no
if test "x$with_webp" != xno; then
  have_webp=yes
  PKG_CHECK_MODULES(WEBP, libwebp >= webp_required_version,
    [
      PKG_CHECK_MODULES(WEBPMUX, libwebpmux >= webp_required_version,
        [
          PKG_CHECK_MODULES(WEBPDEMUX, libwebpdemux >= webp_required_version,,
            [have_webp="no (WebP not built with libwebpdemux)"])
        ],
        [have_webp="no (WebP not built with libwebpmux)"])
    ],
    [have_webp="no (WebP not found)"])
fi

if test "x$have_webp" = xyes; then
  MIME_TYPES="$MIME_TYPES;image/x-webp"
fi

AM_CONDITIONAL(HAVE_WEBP, test "x$have_webp" = xyes)


###################
# Check for libheif
###################

AC_ARG_WITH(libheif, [  --without-libheif       build without libheif support])

# Overcomplex tests, but basically:
# - Profiles supported only for libheif >= 1.4.0.
# - libheif 1.5.0 and 1.5.1 are very broken and easily crash.
# As a consequence we prioritize any libheif other than these 2 versions
# (even when under 1.4.0 thus we lose profile support), and only use
# these as last resort, outputting a warning.
have_libheif=no
have_libheif_1_4_0=no
if test "x$with_libheif" != xno; then
  have_libheif_1_4_0=yes
  have_libheif=yes
  PKG_CHECK_MODULES(LIBHEIF, libheif > 1.5.1,,
    [
      PKG_CHECK_MODULES(LIBHEIF, libheif = 1.4.0,,
        [
          have_libheif_1_4_0="no (libheif >= 1.4.0 not found)"
          PKG_CHECK_MODULES(LIBHEIF, libheif = libheif_required_version,,
                            [
                              PKG_CHECK_MODULES(LIBHEIF, libheif >= 1.5.0,
                                                [
                                                  have_libheif_1_4_0=yes
                                                  warning_libheif="
WARNING: libheif version 1.5.0 and 1.5.1 are known to crash when
         exporting (bug #4185). Please update."
                                                ],
                                                [ have_libheif="no (libheif not found)" ])
                            ])
        ])
    ])
fi

if test "x$have_libheif" = xyes; then
  FILE_HEIF='file-heif$(EXEEXT)'
fi

AC_SUBST(FILE_HEIF)
AM_CONDITIONAL(HAVE_LIBHEIF, test "x$have_libheif" = xyes)

if test "x$have_libheif_1_4_0" = xyes; then
  AC_DEFINE(HAVE_LIBHEIF_1_4_0, 1,
            [Define to 1 if libheif >= 1.4.0 is available])

  if test "x$warning_libheif" != "x"; then
    have_libheif_1_4_0="yes (see warning below)"
  fi
fi

if test "x$have_libheif" = xyes; then
  MIME_TYPES="$MIME_TYPES;image/heif;image/heic"
  have_libheif="yes [[profile support: $have_libheif_1_4_0]]"
fi


########################
# Check for libbacktrace
########################

AC_ARG_WITH(libbacktrace, [  --without-libbacktrace       build without libbacktrace support])

have_libbacktrace=no
if test "x$with_libbacktrace" != xno; then
  have_libbacktrace=yes

  gimp_save_LIBS=$LIBS
  LIBS="$LIBS -lbacktrace"

  AC_MSG_CHECKING([for LIBBACKTRACE])
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
      [[#include <stddef.h>
        #include <backtrace.h>
        #include <backtrace-supported.h>

        #if ! BACKTRACE_SUPPORTED
        #error ! BACKTRACE_SUPPORTED
        #endif
      ]],
      [[(void) backtrace_create_state (NULL, 0, NULL, NULL);]])],
    [AC_MSG_RESULT([yes]); LIBBACKTRACE_LIBS='-lbacktrace'],
    [AC_MSG_RESULT([no]); have_libbacktrace='no (libbacktrace is not found or not supported)'])

  LIBS=$gimp_save_LIBS

  AC_SUBST(LIBBACKTRACE_LIBS)
fi

if test "x$have_libbacktrace" = xyes; then
  AC_DEFINE(HAVE_LIBBACKTRACE, 1,
            [Define to 1 if libbacktrace is available])
fi


#####################
# Check for libunwind
#####################

AC_ARG_WITH(libunwind, [  --without-libunwind       build without libunwind support])

have_libunwind=no
if test "x$with_libunwind" != xno; then
  have_libunwind=yes
  PKG_CHECK_MODULES(LIBUNWIND, libunwind >= libunwind_required_version,
    [],
    [have_libunwind="no (libunwind not found)"])
fi

if test "x$have_libunwind" = xyes; then
  AC_DEFINE(HAVE_LIBUNWIND, 1,
            [Define to 1 if libunwind is available])
fi


#######################################
# Check for detailed backtraces support
#######################################

detailed_backtraces=no
if test "x$platform_linux" = xyes; then
  if test "x$have_libbacktrace" = xyes -o "x$have_libunwind" = xyes; then
    detailed_backtraces=yes
  else
    detailed_backtraces='no (libbacktrace and libunwind are not found or not supported)'
  fi
elif test "x$platform_win32" = xyes; then
  detailed_backtraces=$enable_drmingw
fi


######################
# Check for libmypaint
######################

PKG_CHECK_MODULES(LIBMYPAINT, libmypaint >= 1.4.0,,
  [
    PKG_CHECK_MODULES(LIBMYPAINT, libmypaint >= libmypaint_required_version,,
                      [add_deps_error([libmypaint >= libmypaint_required_version])])
    warning_libmypaint="
WARNING: libmypaint lower than version 1.4.0 is known to crash when
         parsing MyPaint 2 brushes. Please update."
  ])

PKG_CHECK_MODULES(MYPAINT_BRUSHES, mypaint-brushes-1.0,,
                  [add_deps_error([mypaint-brushes-1.0])])

if test "x$enable_relocatable_bundle" != xno; then
  mypaint_brushes_dir='${gimp_installation_dir}/share/mypaint-data/1.0/brushes'
else
  mypaint_brushes_dir=`$PKG_CONFIG --variable=brushesdir mypaint-brushes-1.0`
fi
AC_SUBST(mypaint_brushes_dir)


##################
# Check for webkit
##################

AC_ARG_WITH(webkit, [  --without-webkit        don't build the help-browser and web-page plug-ins])

have_webkit=no
if test "x$with_webkit" != xno; then
  PKG_CHECK_MODULES(WEBKIT, webkit2gtk-4.0 >= webkitgtk_required_version,
    WEB_PAGE='web-page$(EXEEXT)'
    have_webkit=yes,
    have_webkit="no (WebKit not found)")
else
  have_webkit="no (WebKit support disabled)"
fi

AC_SUBST(WEB_PAGE)
if test "x$have_webkit" = "xyes"; then
  AC_DEFINE(HAVE_WEBKIT, 1, [Define to 1 if webkit is available])
fi
AM_CONDITIONAL(HAVE_WEBKIT, test "x$have_webkit" = xyes)


###################
# Check for librsvg
###################

PKG_CHECK_MODULES(SVG, librsvg-2.0 >= rsvg_required_version,,
                  [add_deps_error([rsvg >= rsvg_required_version])])

MIME_TYPES="$MIME_TYPES;image/svg+xml"

AC_CHECK_PROGS(GTK_ENCODE_SYMBOLIC_SVG, gtk-encode-symbolic-svg, no)

if test "x$GTK_ENCODE_SYMBOLIC_SVG" = xno; then
  add_deps_error([gtk-encode-symbolic-svg],
                 [Could not find gtk-encode-symbolic-svg in your PATH.])
fi

####################################
# Allow to disable the print plug-in
####################################

AC_ARG_WITH(print,[  --without-print         build without print support])

enable_print="no (disabled)"
if test "x$with_print" != xno; then
  enable_print="yes"
fi

AM_CONDITIONAL(BUILD_PRINT, test "x$enable_print" = xyes)


######################
# Check for libpoppler
######################

# PDF import support is a granted feature.
PKG_CHECK_MODULES(POPPLER, [poppler-glib >= poppler_required_version],,
                  [add_deps_error([poppler-glib >= poppler_required_version])])
# All modern distributions have at least this version and CJK/Cyrillic
# support should not be considered as "second-zone citizen".
PKG_CHECK_MODULES(POPPLER_DATA, [poppler-data >= poppler_data_required_version],,
                  [add_deps_error([poppler-data >= poppler_data_required_version])])

# MIME_TYPES is for the formats we can open as used for instance in the
# desktop file. Even if cairo-pdf (used for PDF export) is not
# installed, the mime-type list should contain application/pdf.
MIME_TYPES="$MIME_TYPES;application/pdf"

#####################
# Check for cairo-pdf
#####################

AC_ARG_WITH(cairo_pdf,[  --without-cairo-pdf     build without cairo-pdf support])
have_cairo_pdf=no
if test "x$with_cairo_pdf" != xno; then
  have_cairo_pdf=yes
  PKG_CHECK_MODULES(CAIRO_PDF, cairo-pdf >= cairo_pdf_required_version,
    FILE_PDF_SAVE='file-pdf-save$(EXEEXT)',
    have_cairo_pdf="no (cairo-pdf not found)")
else
   have_cairo_pdf="no (cairo-pdf support disabled)"
fi

AC_SUBST(FILE_PDF_SAVE)


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
      have_libwmf=yes
      PKG_CHECK_MODULES(WMF, libwmf >= wmf_required_version,
                        FILE_WMF='file-wmf$(EXEEXT)',
                        have_libwmf="no (libwmf not found)")
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


####################
# Check for OpenJPEG
####################

AC_ARG_WITH(jpeg2000, [  --without-jpeg2000      build without JPEG 2000 support])

have_openjpeg=no
if test "x$with_jpeg2000" != xno; then
  have_openjpeg=yes
  PKG_CHECK_MODULES(OPENJPEG, libopenjp2 >= openjpeg_required_version,
    FILE_JP2_LOAD='file-jp2-load$(EXEEXT)'
    have_openjpeg=yes,
    have_openjpeg="no (OpenJPEG not found)")
fi

if test "x$have_openjpeg" = xyes; then
  MIME_TYPES="$MIME_TYPES;image/jp2"
fi

AM_CONDITIONAL(HAVE_OPENJPEG, test "x$have_openjpeg" = xyes)
AC_SUBST(FILE_JP2_LOAD)


################
# Check for lcms
################

PKG_CHECK_MODULES(LCMS, lcms2 >= lcms_required_version,,
                  [add_deps_error([lcms2 >= lcms_required_version])])


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


####################
# Check for libgudev
####################

AC_ARG_WITH(gudev, [  --without-gudev         build without GUdev support])

have_libgudev=no

if test "x$with_gudev" = xno; then
  have_libgudev="no (disabled)"
fi

if test "x$have_linux_input" = xyes && test "x$with_gudev" != xno; then
  PKG_CHECK_MODULES(GUDEV, gudev-1.0 >= libgudev_required_version,
    have_libgudev=yes,
    have_libgudev="no (libgudev not found)")
fi

if test "x$have_libgudev" = xyes; then
  AC_DEFINE(HAVE_LIBGUDEV, 1, [Define to 1 if libgudev is available])
fi


##########################
# Check for Email Settings
##########################

# The default behavior is that xdg-email is searched in the user PATH,
# and email plugin is enabled if found.
# Sendmail can be used instead of xdg-email, but must be explicitly
# enabled with --with-sendmail.

have_email="no (xdg-email not found)"

AC_ARG_WITH(sendmail,[  --with-sendmail[=DIR]     use sendmail instead of xdg-email])

have_sendmail=no
# Sendmail must be explicitly enabled to replace xdg-email implementation.
if test "x$with_sendmail" != "x" && test "x$with_sendmail" != "xno"; then
  if test "x$with_sendmail" = "xyes"; then
      sendmail_path=$PATH:/usr/sbin:/usr/lib
      AC_DEFINE_UNQUOTED(SENDMAIL, "",
                         [The MTA used by the mail plug-in.])
  else
      sendmail_path=$with_sendmail
      AC_DEFINE_UNQUOTED(SENDMAIL, "$with_sendmail",
                         [The MTA used by the mail plug-in.])
  fi

  AC_PATH_PROG(SENDMAIL, sendmail, , $sendmail_path)
  if test "x$SENDMAIL" != "x"; then
    have_email="yes (sendmail)"
  else
    # Not having sendmail at runtime is not a blocker.
    have_email="needs runtime dependency: sendmail"
  fi
  have_sendmail=yes
fi

if test "x$have_sendmail" = xno; then
  AC_PATH_PROG(XDG_EMAIL, xdg-email)
  if test "x$XDG_EMAIL" != "x"; then
    have_email="yes (xdg-email)"
  else
    # Not having xdg-email at runtime is not a blocker.
    # But let's inform the builder that for the feature to be available,
    # xdg-email is a runtime dependency.
    have_email="needs runtime dependency: xdg-email"
  fi
fi

################
# Check for perl
################

AC_PATH_PROGS(PERL,perl5 perl,perl)
AX_PROG_PERL_VERSION([perl_required_version],,
                     [add_deps_error([perl >= perl_required_version])])


AC_ARG_WITH(pdbgen,
            [  --with-pdbgen           use 'pdbgen' code generation tool],,
  with_pdbgen=$USE_MAINTAINER_MODE)

AM_CONDITIONAL(WITH_PDBGEN, test "x$with_pdbgen" = xyes)

#################################
# Check for GObject Introspection
#################################

GOBJECT_INTROSPECTION_REQUIRE(introspection_required_version)

#####################
# Check for json-glib
#####################

PKG_CHECK_MODULES(JSON_GLIB, json-glib-1.0 >= json_glib_required_version,,
                  [add_deps_error([json-glib-1.0 >= json_glib_required_version])])

#######################################
# Check for python runtime dependencies
#######################################

# By default, we want packagers to install Python plug-ins to get the
# optimum experience. --with-python=yes will check for a Python 3
# interpreter and PyGObject, and fails without.
# --with-python=force will install the plug-ins even though the
# dependencies are not found at build time. It is up to the packager to
# ensure they are available at run time. This can be useful in
# particular when cross-compiling since anyway the interpreter is not
# useful at build time.
# --with-python=no won't install Python plug-ins. It is discouraged.
AC_ARG_WITH(python,
            [  --with-python=no|yes|force install Python 3 plug-ins (default=yes)], ,
            [with_python=yes])

if test "x$with_python" = xno; then
  warning_python="
WARNING: you disabled the installation of core Python plug-ins. This is
         discouraged as it won't provide the full GIMP experience.
         Note that you may install the Python plug-ins even if you have
         no Python interpreter at build-time by passing the configure
         option --with-python=force.
         Just make sure that a Python python3_required_version or newer
         interpreter is available at run-time."
elif test "x$with_python" = xforce; then
  with_python="yes"
  warning_python=
  AM_PATH_PYTHON3(python3_required_version, ,
                  warning_python="Python python3_required_version or newer not found")
  if test "x$warning_python" = "x"; then
    AM_PYTHON_CHECK_PYGOBJECT("$PYTHON", , ,
                              warning_python="PyGObject 3.0 not found")
  fi
  if test "x$warning_python" != "x"; then
    warning_python="
WARNING: ${warning_python}
         Python plug-ins will be installed anyway but you should make
         sure that a compatible Python interpreter and PyGObject are
         available at installation, otherwise installed plug-ins won't
         be usable."
    with_python="yes (see warning below)"
  fi
else
  with_python="yes"
  error_python=
  AM_PATH_PYTHON3(python3_required_version,,
                  error_python="- Python python3_required_version or newer not found")
  if test "x$error_python" = "x"; then
    AM_PYTHON_CHECK_PYGOBJECT("$PYTHON", , ,
                              error_python="PyGObject 3.0 not found")
  fi
  if test "x$error_python" != "x"; then
    required_deps="$required_deps
    $error_python
        *** Please install Python 3 and PyGObject 3.
        *** Note that you may install the Python plug-ins even if you have
        *** no Python interpreter at build-time by passing the configure
        *** option --with-python=force.
        *** Just make sure that the Python dependencies are available at run-time."
    with_python="no"
  fi
fi

if test "x$with_python" = "xyes"; then
  if test "x$platform_win32" = "xyes"; then
    PYBIN_PATH="$py_exec_prefix\pythonw.exe"
  else
    PYBIN_PATH="$PYTHON"
  fi
  AC_SUBST(PYBIN_PATH)
fi
AM_CONDITIONAL(HAS_PYTHON_INTERP, test "x$PYBIN_PATH" != "x")
AM_CONDITIONAL(BUILD_PYTHON, test "x$with_python" != xno)

###########################################
# Check for JavaScript runtime dependencies
###########################################

AC_ARG_WITH(javascript,
            [  --with-javascript=no|yes|force install JavaScript plug-ins (default=yes)], ,
            [with_javascript=yes])

if test "x$with_javascript" = xno; then
  warning_javascript="
WARNING: you disabled the installation of core JavaScript plug-ins. This
         is discouraged as it won't provide the full GIMP experience.
         Note that you may install the JavaScript plug-ins even if the
         GJS interpreter is not installed at build-time by passing the
         configure option --with-javascript=force.
         Just make sure that GJS is available at run-time."
elif test "x$with_javascript" = xforce; then
  with_javascript="yes"
  warning_javascript=
  AC_CHECK_PROGS(GJS, gjs, no)
  if test "x$GJS" = xno; then
    warning_javascript="
WARNING: GJS not found.
         JavaScript plug-ins will be installed anyway but you should make
         sure that the JavaScript interpreter GJS is available at
         installation, otherwise installed plug-ins won't be usable."
    with_javascript="yes (see warning below)"
  fi
else
  with_javascript="yes"
  AC_CHECK_PROGS(GJS, gjs, no)
  if test "x$GJS" = xno; then
    required_deps="$required_deps
    - GJS not found.
        *** Please install the JavaScript interpreter GJS.
        *** Note that you may install the JavaScript plug-ins even if
        *** GJS is not installed at build-time by passing the configure
        *** option --with-javascript=force.
        *** Just make sure that GJS is available at run-time."
    with_javascript="no"
  fi
fi

AM_CONDITIONAL(HAS_JAVASCRIPT_INTERP, test "x$GJS" != "xno")
AM_CONDITIONAL(BUILD_JAVASCRIPT, test "x$with_javascript" != xno)

####################################
# Check for Lua runtime dependencies
####################################

AC_ARG_WITH(lua,
            [  --with-lua=no|yes|force install Lua plug-ins (default=yes)], ,
            [with_lua=yes])

if test "x$with_lua" = xno; then
  warning_lua="
WARNING: you disabled the installation of core Lua plug-ins. This
         is discouraged as it won't provide the full GIMP experience.
         Note that you may install the Lua plug-ins even if Lua LGI
         is not installed at build-time by passing the configure option
         --with-lua=force.
         Just make sure that it is available at run-time."
elif test "x$with_lua" = xforce; then
  with_lua="yes"
  warning_lua=
  AC_CHECK_PROGS(LUA, luajit, no)
  if test "x$LUA" = xno; then
    warning_lua="
WARNING: luajit not found.
         Lua plug-ins will be installed anyway but you should make
         sure that luajit and LGI are available at installation,
         otherwise installed plug-ins won't be usable."
    with_lua="yes (see warning below)"
  fi
else
  with_lua="yes"
  AC_CHECK_PROGS(LUA, luajit, no)
  if test "x$LUA" = xno; then
    required_deps="$required_deps
    - luajit not found.
        *** Please install the Lua interpreter luajit.
        *** Note that you may install the Lua plug-ins even if luajit
        *** is not installed at build-time by passing the configure
        *** option --with-lua=force.
        *** Just make sure that luajit and LGI are available at run-time."
    with_lua="no"
  fi
fi

AM_CONDITIONAL(HAS_LUA_INTERP, test "x$LUA" != "xno")
AM_CONDITIONAL(BUILD_LUA, test "x$with_lua" != xno)

#######################################
# Check for Vala buildtime dependencies
#######################################

AC_ARG_WITH(vala,
            [  --with-vala=no|yes install Vala plug-ins (default=yes)], ,
            [with_vala=yes])

if test "x$with_vala" = xno; then
  warning_vala="
WARNING: you disabled the installation of core Vala plug-ins. This
         is discouraged as it won't provide the full GIMP experience."
else
  VAPIGEN_CHECK($VALA_REQUIRED_VERSION)
  with_vala="yes"
  warning_vala=
  AC_PATH_PROG(VALAC, valac, no)
  if test "x$VALAC" = xno; then
    AC_MSG_ERROR([Vala-plugins are enabled, but could not find vala compiler])
  fi
fi

AM_CONDITIONAL(BUILD_VALA, test "x$with_vala" != xno)

###########################################################
# Some plug-ins don't build on Win32, others are Win32-only
###########################################################

if test "x$os_win32" = xno; then
  MAIL='mail$(EXEEXT)'
fi

AC_SUBST(MAIL)


###############################
# Check for GTK Mac Integration
###############################

gtk_mac_integration_ok=no
if test "x$platform_osx" != xno &&
   (echo `pkg-config --variable=targets gtk+-3.0` | grep quartz); then

  PKG_CHECK_MODULES(GTK_MAC_INTEGRATION, gtk-mac-integration-gtk3 >= gtk_mac_integration_required_version,
    gtk_mac_integration_ok=yes,
    gtk_mac_integration_ok=no)

  if test "x$gtk_mac_integration_ok" != "xyes"; then
    PKG_CHECK_MODULES(GTK_MAC_INTEGRATION, gtk-mac-integration >= gtk_mac_integration_required_version,
      gtk_mac_integration_ok=yes,
      gtk_mac_integration_ok=no)
  fi
fi

AM_CONDITIONAL(PLATFORM_OSX_QUARTZ, test "x$gtk_mac_integration_ok" = xyes)


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


########################
# Check for vector icons
########################

# If not cross-compiling, default build's link and compile flags to the
# target's flags.
if test "$cross_compiling" != "yes"; then
  if test "x$LDFLAGS_FOR_BUILD" = "x"; then
    LDFLAGS_FOR_BUILD="$LDFLAGS"
  fi
  if test "x$CFLAGS_FOR_BUILD" = "x"; then
    CFLAGS_FOR_BUILD="$CFLAGS"
  fi
  if test "x$CPPFLAGS_FOR_BUILD" = "x"; then
    CPPFLAGS_FOR_BUILD="$CPPFLAGS"
  fi
fi

AC_ARG_ENABLE(vector-icons, [  --disable-vector-icons  use raster icons rather than vector ones (default=auto)], ,
              enable_vector_icons=auto)

if test "x$enable_vector_icons" != "xno"; then
  have_shared_mime_info=""
  # shared-mime-info is needed to correctly detect SVG files (except on
  # Windows, apparently).
  PKG_CHECK_MODULES(MIME_INFO, [shared-mime-info],
                    have_shared_mime_info="yes", have_shared_mime_info="no")
  if test "x$have_shared_mime_info" = "xyes" ||
     test "x$platform_win32" = "xyes"; then
    # Check if librsvg was built with --disable-pixbuf-loader.
    gdk_pixbuf_moduledir=`$PKG_CONFIG --variable=gdk_pixbuf_moduledir gdk-pixbuf-2.0`
    # AC_CHECK_FILE macro does not work when cross-compiling and exits with:
    # error: cannot check for file existence when cross compiling
    # So let's test files the shell way.
    if (test "x$platform_win32" = "xyes" &&
        test -f "$gdk_pixbuf_moduledir/libpixbufloader-svg.dll") ||
       test -f "$gdk_pixbuf_moduledir/libpixbufloader-svg.so"; then
      # We must not use $PKG_CONFIG nor PKG_CHECK_* macros because we need
      # to make sure we use the native pkg-config (in case we cross-compile).
      if pkg-config --atleast-version=glib_required_version glib-2.0 &&
         pkg-config gio-2.0; then
        NATIVE_GLIB_LIBS=`pkg-config --libs gio-2.0 glib-2.0`
        NATIVE_GLIB_CFLAGS=`pkg-config --cflags gio-2.0 glib-2.0`
        enable_vector_icons="yes"

        if test "x$platform_win32" = "xyes"; then
          warning_vector_icons_windows="
WARNING: You enabled vector icons on Win32. Make sure to run:
         $ gdk-pixbuf-query-loaders.exe --update-cache
       on the target machine (this command generates loaders.cache)
       so that GdkPixbuf knows where to find the SVG loader."
        fi
      else
        enable_vector_icons="no (missing native glib-2.0 > glib_required_version or gio-2.0)"
      fi
    else
      enable_vector_icons="no (librsvg GdkPixbuf loader missing)"
    fi
  else
    enable_vector_icons="no (missing shared-mime-info)"
  fi
fi

AC_SUBST(NATIVE_GLIB_LIBS)
AC_SUBST(NATIVE_GLIB_CFLAGS)
AM_CONDITIONAL(ENABLE_VECTOR_ICONS, test "x$enable_vector_icons" = "xyes")

####################
# Check for xvfb-run
####################

AC_ARG_WITH(xvfb-run,  [  --without-xvfb-run      do not use xvfb-run for UI-dependent automatic tests])

have_xvfb_run="no (disabled)"
if test "x$with_xvfb_run" != "xno"; then
  AC_PATH_PROG(XVFB_RUN, xvfb-run, no)
  if test "x$XVFB_RUN" != "xno"; then
    have_xvfb_run="yes"
    AC_DEFINE(HAVE_XVFB_RUN, 1, [Define to 1 if xvfb-run is available])
  else
    have_xvfb_run="no (not found)"
  fi
fi
AM_CONDITIONAL(HAVE_XVFB_RUN, test "x$have_xvfb_run" = "xyes")

##########################
# Check for appstream-util
##########################

AC_ARG_WITH(appdata-test,  [  --without-appdata-test  do not validate the appdata file])

have_appstream_util="no (disabled)"
if test "x$with_appdata_test" != "xno"; then
  AC_PATH_PROG(APPSTREAM_UTIL, appstream-util, no)
  if test "x$APPSTREAM_UTIL" != "xno"; then
    have_appstream_util="yes"
  else
    have_appstream_util="no (appstream-util not found)"
  fi
fi
AM_CONDITIONAL(HAVE_APPSTREAM_UTIL, test "x$have_appstream_util" = "xyes")
if test "x$have_appstream_util" = "xyes"; then
  have_appstream_util="yes (this test requires network access; --without-appdata-test to disable)"
fi


#################################
# Check for desktop-file-validate
#################################

AC_PATH_PROG(DESKTOP_FILE_VALIDATE, desktop-file-validate, no)
if test "x$DESKTOP_FILE_VALIDATE" != "xno"; then
  have_desktop_file_validate="yes"
else
  have_desktop_file_validate="no (desktop-file-validate not found)"
fi
AM_CONDITIONAL(HAVE_DESKTOP_FILE_VALIDATE, test "x$DESKTOP_FILE_VALIDATE" != "xno")

####################
# Set bug report URL
####################

# Allowing third-party packagers to set their own bugtracker URL, in
# order to filter first packaging bugs from core bugs.

AC_ARG_WITH(bug-report-url,
            [  --with-bug-report-url   URL used by the debug dialog to report bugs],,
            [with_bug_report_url="$PACKAGE_BUGREPORT"])

if test "x$with_bug_report_url" = "x"; then
  with_bug_report_url="$PACKAGE_BUGREPORT"
fi

BUG_REPORT_URL="$with_bug_report_url"
AC_DEFINE_UNQUOTED(BUG_REPORT_URL, "$with_bug_report_url",
                   [The URL to open in order to report bugs])
AC_SUBST(BUG_REPORT_URL)

override_bug_report_url=
if test "x$with_bug_report_url" = "x$PACKAGE_BUGREPORT"; then
override_bug_report_url="
NOTE: if you plan on packaging GIMP for distribution, it is recommended
      to override the bug report URL with option:
        --with-bug-report-url=https://example.com/
      so that you can filter packaging bugs from core bugs before
      reporting upstream.
"
fi

#####################
# Build identifiers #
#####################

AC_ARG_WITH(build-id,
            [  --with-build-id   A unique string used to define your build],,
            [with_build_id="unknown"])
AC_DEFINE_UNQUOTED(GIMP_BUILD_ID, "$with_build_id",
                   [The string defining the build])

AC_ARG_WITH(revision,
            [  --with-revision   Revision increment for a package],,
            [with_revision="0"])
GIMP_PACKAGE_REVISION=$with_revision
AC_SUBST(GIMP_PACKAGE_REVISION)

AC_ARG_ENABLE(check-update,
              [  --disable-check-update
                          GIMP will not look up new version availability on startup],
              check_update=no,
              check_update=yes)
if test "x$check_update" = xyes; then
  AC_DEFINE(CHECK_UPDATE, 1,
              [Define to 1 if automatic update check is enabled])
fi

AC_DEFINE_UNQUOTED(GIMP_BUILD_PLATFORM, "$host_os",
                   [The OS this was built for])
if test "x$platform_linux" = xyes; then
  platform_family="linux"
elif test "x$platform_win32" = xyes; then
  platform_family="windows"
elif test "x$platform_osx" = xyes; then
  platform_family="macos"
else
  platform_family="other"
fi
AC_DEFINE_UNQUOTED(GIMP_BUILD_PLATFORM_FAMILY, "$platform_family",
                   [The OS family this was built for])

#########################
# Default ICC directory #
#########################

# This is necessary because some Unix systems may have a different
# standard path for color profiles. And in particular, sandbox builds
# might mount the host system at a different root. This is for
# instance the case of flatpak which mount the host root at /run/host/.
if test "x$platform_osx" = "xno" &&
   test "x$platform_win32" = "xno"; then
  AC_MSG_CHECKING([default directory for color profiles])
  AC_ARG_WITH(icc-directory,
              [  --with-icc-directory    Path to default color profiles for this system],,
              [with_icc_directory="/usr/share/color/icc"])

  if test "x$with_icc_directory" = "x"; then
    with_icc_directory="/usr/share/color/icc"
  fi

  AC_DEFINE_UNQUOTED(COLOR_PROFILE_DIRECTORY,
                     "$with_icc_directory",
                     [The path to default color profiles for this system])
  AC_MSG_RESULT([$with_icc_directory])
fi

######################################
# Checks for gtk-doc and docbook-tools
######################################

# Check for GTK_DOC_CHECK availability. The GTK_DOC_CHECK invocation
# must be on its own line, gtkdocize relies on it
m4_ifdef([GTK_DOC_CHECK], [
GTK_DOC_CHECK([1.0])
])

# NOTE: We need to use a separate automake conditional for this
#       to make this work with the tarballs.
AM_CONDITIONAL(ENABLE_GTK_DOC, test "x$enable_gtk_doc" = xyes)

AC_ARG_ENABLE(gtk-doc-app, [  --enable-gtk-doc-app    build developer documentation for app (default=no)], , enable_gtk_doc_app=no)
AM_CONDITIONAL(ENABLE_GTK_DOC_APP, test "x$enable_gtk_doc_app" = xyes)


#############################
# Check for Windows installer
#############################

AC_MSG_CHECKING([if building the Windows installer])
AC_ARG_ENABLE(windows-installer,
              [  --enable-windows-installer
                          generate files needed for the Windows installer
                          (default=no)],,
              enable_windows_installer=no)

if test "x$enable_windows_installer" != xno; then
  if test ! -e "$srcdir/build/windows/installer/lang/setup.isl.in"; then
    AC_MSG_RESULT([no (not building from git)])
    AC_MSG_ERROR([Building the Windows installer is only supported when building from git.])
  fi
fi

AC_MSG_RESULT([$enable_windows_installer])
AM_CONDITIONAL(BUILD_WINDOWS_INSTALLER, test "x$enable_windows_installer" != xno)


#########################################
# Allow to configure the GIMP environment
#########################################

AC_ARG_ENABLE(default-binary, [  --enable-default-binary install this as the default gimp binary (default=gimp_stable)], , enable_default_binary=gimp_stable)

AM_CONDITIONAL(DEFAULT_BINARY, test "x$enable_default_binary" = xyes)


AC_ARG_ENABLE(gimp-console, [  --enable-gimp-console   build a console-only binary which does not link GTK+], , enable_gimp_console=yes)

AM_CONDITIONAL(ENABLE_GIMP_CONSOLE, test "x$enable_gimp_console" != xno)


# Possibly change default gimpdir from $XDG_CONFIG_HOME/GIMP/gimp_user_version
gimpdir=GIMP
AC_ARG_WITH(gimpdir, [  --with-gimpdir=DIR      change default gimpdir from $XDG_CONFIG_HOME/GIMP/gimp_user_version to $XDG_CONFIG_HOME/DIR/gimp_user_version, or to DIR if DIR is an absolute path],
if eval "test x$with_gimpdir != x"; then
  if eval "test x$with_gimpdir != xyes"; then
    gimpdir=$with_gimpdir
  fi
fi)

# Build the gimpdir to be used in manpages here, because the manpage
# source is unreadable enough already
case "$gimpdir" in
  /*)
    manpage_gimpdir="$gimpdir"
    ;;
  *)
    manpage_gimpdir="\\fB\$XDG_CONFIG_HOME\\fP/$gimpdir/gimp_user_version"
    ;;
esac

gimpdatadir="$datadir/$PACKAGE/gimp_data_version"
gimpplugindir="$libdir/$PACKAGE/gimp_plugin_version"
gimpsysconfdir="$sysconfdir/$PACKAGE/gimp_sysconf_version"

AC_SUBST(gimpdir)
AC_SUBST(manpage_gimpdir)
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

# Make sure not to disable deprecated APIs for unreleased versions.
# We must build without problems with future releases of libraries
# and disabling deprecated API risks breaking the build

if test "x$have_glib_2_57" != "xyes"; then
  CPPFLAGS="${CPPFLAGS} -DG_DISABLE_DEPRECATED"
fi

if test "x$have_pango_1_43" != "xyes"; then
  CPPFLAGS="${CPPFLAGS} -DPANGO_DISABLE_DEPRECATED"
fi


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

#if !defined (__GIMP_BASE_H_INSIDE__) && !defined (GIMP_BASE_COMPILATION)
#error "Only <libgimpbase/gimpbase.h> can be included directly."
#endif

#ifndef __GIMP_VERSION_H__
#define __GIMP_VERSION_H__

G_BEGIN_DECLS


/**
 * SECTION: gimpversion
 * @title: gimpversion
 * @short_description: Macros and constants useful for determining
 *                     GIMP's version number and capabilities.
 *
 * Macros and constants useful for determining GIMP's version number and
 * capabilities.
 **/

_______EOF

  cat >>$outfile <<_______EOF
/**
 * GIMP_MAJOR_VERSION:
 *
 * The major GIMP version number.
 **/
#define GIMP_MAJOR_VERSION                              ($gimp_major_ver)

/**
 * GIMP_MINOR_VERSION:
 *
 * The minor GIMP version number.
 **/
#define GIMP_MINOR_VERSION                              ($gimp_minor_ver)

/**
 * GIMP_MICRO_VERSION:
 *
 * The micro GIMP version number.
 **/
#define GIMP_MICRO_VERSION                              ($gimp_micro_ver)

/**
 * GIMP_VERSION:
 *
 * The GIMP version as a string.
 **/
#define GIMP_VERSION                                    "$gimp_ver"

/**
 * GIMP_API_VERSION:
 *
 * Since: 2.2
 **/
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
INSTALL
tools/Makefile
pdb/Makefile
libgimp/Makefile
libgimpbase/Makefile
libgimpconfig/Makefile
libgimpcolor/Makefile
libgimpmath/Makefile
libgimpmodule/Makefile
libgimpthumb/Makefile
libgimpwidgets/Makefile
app/Makefile
app/actions/Makefile
app/config/Makefile
app/core/Makefile
app/operations/Makefile
app/operations/layer-modes/Makefile
app/operations/layer-modes-legacy/Makefile
app/operations/tests/Makefile
app/gegl/Makefile
app/dialogs/Makefile
app/display/Makefile
app/file/Makefile
app/file-data/Makefile
app/gui/Makefile
app/menus/Makefile
app/paint/Makefile
app/pdb/Makefile
app/plug-in/Makefile
app/propgui/Makefile
app/text/Makefile
app/tools/Makefile
app/vectors/Makefile
app/widgets/Makefile
app/xcf/Makefile
app/tests/Makefile
app/tests/files/Makefile
app/tests/gimpdir-empty/Makefile
app/tests/gimpdir-empty/brushes/Makefile
app/tests/gimpdir-empty/gradients/Makefile
app/tests/gimpdir-empty/patterns/Makefile
app/tests/gimpdir/Makefile
app/tests/gimpdir/brushes/Makefile
app/tests/gimpdir/gradients/Makefile
app/tests/gimpdir/patterns/Makefile
app-tools/Makefile
build/Makefile
build/windows/Makefile
build/windows/gimp.rc
build/windows/gimp-plug-ins.rc
build/windows/installer/Makefile
build/windows/installer/lang/Makefile
plug-ins/Makefile
plug-ins/file-bmp/Makefile
plug-ins/file-dds/Makefile
plug-ins/file-exr/Makefile
plug-ins/file-faxg3/Makefile
plug-ins/file-fits/Makefile
plug-ins/file-fli/Makefile
plug-ins/file-ico/Makefile
plug-ins/file-jpeg/Makefile
plug-ins/file-psd/Makefile
plug-ins/file-raw/Makefile
plug-ins/file-sgi/Makefile
plug-ins/file-tiff/Makefile
plug-ins/file-webp/Makefile
plug-ins/flame/Makefile
plug-ins/fractal-explorer/Makefile
plug-ins/fractal-explorer/examples/Makefile
plug-ins/gfig/Makefile
plug-ins/gfig/gfig-examples/Makefile
plug-ins/gfig/images/Makefile
plug-ins/ui/Makefile
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
plug-ins/metadata/Makefile
plug-ins/pagecurl/Makefile
plug-ins/print/Makefile
]
dnl plug-ins/pygimp/Makefile
dnl plug-ins/pygimp/plug-ins/Makefile
[
plug-ins/python/Makefile
plug-ins/python/python-console/Makefile
plug-ins/screenshot/Makefile
plug-ins/script-fu/Makefile
plug-ins/script-fu/ftx/Makefile
plug-ins/script-fu/scripts/Makefile
plug-ins/script-fu/scripts/images/Makefile
plug-ins/script-fu/tinyscheme/Makefile
plug-ins/selection-to-path/Makefile
plug-ins/twain/Makefile
plug-ins/common/Makefile
plug-ins/goat-exercises/Makefile
modules/Makefile
devel-docs/Makefile
devel-docs/version
devel-docs/app/Makefile
devel-docs/libgimp/Makefile
devel-docs/libgimpbase/Makefile
devel-docs/libgimpconfig/Makefile
devel-docs/libgimpcolor/Makefile
devel-docs/libgimpmath/Makefile
devel-docs/libgimpmodule/Makefile
devel-docs/libgimpthumb/Makefile
devel-docs/libgimpwidgets/Makefile
devel-docs/performance-logs/Makefile
devel-docs/tools/Makefile
docs/Makefile
menus/Makefile
cursors/Makefile
icons/Makefile
icons/Color/Makefile
icons/Legacy/Makefile
icons/Symbolic/Makefile
themes/Makefile
themes/System/Makefile
data/Makefile
data/brushes/Makefile
data/brushes/Basic/Makefile
data/brushes/Fun/Makefile
data/brushes/Media/Makefile
data/brushes/Legacy/Makefile
data/brushes/Sketch/Makefile
data/brushes/Splatters/Makefile
data/brushes/Texture/Makefile
data/dynamics/Makefile
data/dynamics/Basic/Makefile
data/dynamics/FX/Makefile
data/environ/Makefile
data/gradients/Makefile
data/images/Makefile
data/interpreters/Makefile
data/palettes/Makefile
data/patterns/Makefile
data/patterns/Animal/Makefile
data/patterns/Fabric/Makefile
data/patterns/Food/Makefile
data/patterns/Legacy/Makefile
data/patterns/Paper/Makefile
data/patterns/Plant/Makefile
data/patterns/Sky/Makefile
data/patterns/Stone/Makefile
data/patterns/Water/Makefile
data/patterns/Wood/Makefile
data/tags/Makefile
data/tips/Makefile
data/tool-presets/Makefile
data/tool-presets/Crop/Makefile
data/tool-presets/FX/Makefile
data/tool-presets/Paint/Makefile
data/tool-presets/Selection/Makefile
data/tool-presets/Sketch/Makefile
desktop/Makefile
desktop/gimp-data-extras.metainfo.xml.in
desktop/gimp.desktop.in
desktop/org.gimp.GIMP.appdata.xml.in
desktop/test-appdata.sh
etc/Makefile
etc/gimprc
m4macros/Makefile
po/Makefile.in
po-libgimp/Makefile.in
po-plug-ins/Makefile.in
po-python/Makefile.in
po-script-fu/Makefile.in
po-tips/Makefile.in
po-windows-installer/Makefile.in
gimp-zip
Makefile
])

# Files with versions in their names
AC_CONFIG_FILES(
docs/gimp-gimp_app_version.1:docs/gimp.1.in
docs/gimprc-gimp_app_version.5:docs/gimprc.5.in
docs/gimptool-gimp_tool_version.1:docs/gimptool.1.in
gimp-gimp_pkgconfig_version.pc:gimp.pc.in
gimpthumb-gimp_pkgconfig_version.pc:gimpthumb.pc.in
gimpui-gimp_pkgconfig_version.pc:gimpui.pc.in
)

# By default, autoconf will create only po/Makefile.
# This code is used to create the POTFILES and Makefile for all
# additional gettext catalogs.
m4_define([generate_po_makefile], [
  AC_MSG_NOTICE([Generating POTFILES and Makefile for $1])

  AC_CONFIG_COMMANDS([$1/stamp-it],
  [[
    if  ! grep "^# INTLTOOL_MAKEFILE$" "$1/Makefile.in" > /dev/null ; then
       as_fn_error $? "$1/Makefile.in.in was not created by intltoolize." "$LINENO" 5
    fi
    rm -f "$1/stamp-it" "$1/POTFILES"
    sed -e '/^#/d' -e 's/^[[].*] *//' \
        -e '/^[ 	]*$/d'        \
        -e "s|^|	$ac_top_srcdir/|" \
        "$srcdir/$1/POTFILES.in" | sed '$!s/$/ \\/' >"$1/POTFILES" && \
    sed -e "/^# This comment gets stripped out/ {"  \
        -e 'r $1/POTFILES'                          \
        -e "d" -e "}"                               \
        "$1/Makefile.in" >"$1/Makefile" && \
    touch "$1/stamp-it"
  ]])
])

generate_po_makefile([po-libgimp])
generate_po_makefile([po-python])
generate_po_makefile([po-plug-ins])
generate_po_makefile([po-script-fu])
generate_po_makefile([po-tips])
generate_po_makefile([po-windows-installer])

# Print a summary of features enabled/disabled:
optional_deps="
Extra Binaries:
  gimp-console:              $enable_gimp_console

Optional Features:
  Check updates at startup:  $check_update
  Levin matting engine:      $have_matting_levin
  Language selection:        $have_iso_codes
  Vector icons:              $enable_vector_icons
  Dr. Mingw (Win32):         $enable_drmingw
  Relocatable bundle:        $enable_relocatable_bundle
  Default ICC directory:     $with_icc_directory
  Debug console (Win32):     $enable_win32_debug_console
  32-bit DLL folder (Win32): $with_win32_32bit_dll_folder
  Detailed backtraces:       $detailed_backtraces

Optional Plug-Ins:
  Ascii Art:                 $have_libaa
  Ghostscript:               $have_gs
  Help Browser:              $have_webkit
  JPEG 2000:                 $have_openjpeg
  MNG:                       $have_libmng
  OpenEXR:                   $have_openexr
  WebP:                      $have_webp
  Heif:                      $have_libheif
  PDF (export):              $have_cairo_pdf
  Print:                     $enable_print
  Python 3 plug-ins:         $with_python
  JavaScript plug-ins:       $with_javascript
  Lua plug-ins:              $with_lua
  Vala plug-ins:             $with_vala
  TWAIN (Win32):             $os_win32
  Webpage:                   $have_webkit
  WMF:                       $have_libwmf
  X11 Mouse Cursor:          $have_xmc
  XPM:                       $have_libxpm
  Email:                     $have_email

Optional Modules:
  ALSA (MIDI Input):         $have_alsa
  Linux Input:               $have_linux_input (GUdev support: $have_libgudev)
  DirectInput (Win32):       $have_dx_dinput

Tests:
  Use xvfb-run               $have_xvfb_run
  Test appdata               $have_appstream_util
  Test desktop file          $have_desktop_file_validate

Bug report URL: $with_bug_report_url
$override_bug_report_url$warning_vector_icons_windows$warning_glib_networking$warning_gcc$warning_python$warning_javascript$warning_lua$warning_libheif$warning_libmypaint"

if test "x$required_deps" = "x"; then
  AC_OUTPUT
  AC_MSG_RESULT([
Building GIMP with prefix=$prefix, datarootdir=$datarootdir
Desktop files install into $DESKTOP_DATADIR
$optional_deps]);
else
  AC_MSG_ERROR([
$optional_deps

Error: GIMP configuration failed.
$required_deps

See the file 'INSTALL' for more help.]);
fi
