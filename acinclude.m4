## Add your own autoconf macros here.

## Find the install dirs for the python installation.
##  By James Henstridge

dnl a macro to check for ability to create python extensions
dnl  AM_CHECK_PYTHON_HEADERS([ACTION-IF-POSSIBLE], [ACTION-IF-NOT-POSSIBLE])
dnl function also defines PYTHON_INCLUDES
AC_DEFUN([AM_CHECK_PYTHON_HEADERS],
[AC_REQUIRE([AM_PATH_PYTHON])
AC_MSG_CHECKING(for headers required to compile python extensions)
dnl deduce PYTHON_INCLUDES
py_prefix=`$PYTHON -c "import sys; print sys.prefix"`
py_exec_prefix=`$PYTHON -c "import sys; print sys.exec_prefix"`
PYTHON_INCLUDES="-I${py_prefix}/include/python${PYTHON_VERSION}"
if test "$py_prefix" != "$py_exec_prefix"; then
  PYTHON_INCLUDES="$PYTHON_INCLUDES -I${py_exec_prefix}/include/python${PYTHON_VERSION}"
fi
AC_SUBST(PYTHON_INCLUDES)
dnl check if the headers exist:
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $PYTHON_INCLUDES"
AC_TRY_CPP([#include <Python.h>],dnl
[AC_MSG_RESULT(found)
$1],dnl
[AC_MSG_RESULT(not found)
$2])
CPPFLAGS="$save_CPPFLAGS"
])



# Configure paths for gimp-print
# Roger Leigh -- Sat, 10 Feb 2001
# (based on gimpprint.m4 by Owen Taylor     97-11-3)

dnl AM_PATH_GIMPPRINT([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, MODULES]]]])
dnl Test for GIMP-PRINT, and define GIMPPRINT_CFLAGS and GIMPPRINT_LIBS
dnl
AC_DEFUN([AM_PATH_GIMPPRINT],
[dnl
dnl Get the cflags and libraries from the gimpprint-config script
dnl
AC_ARG_WITH(gimpprint-prefix,[  --with-gimpprint-prefix=PFX Prefix where GIMP-PRINT is installed (optional)],
            gimpprint_config_prefix="$withval", gimpprint_config_prefix="")
AC_ARG_WITH(gimpprint-exec-prefix,[  --with-gimpprint-exec-prefix=PFX Exec prefix where GIMP-PRINT is installed
                          (optional)],
            gimpprint_config_exec_prefix="$withval", gimpprint_config_exec_prefix="")
AC_ARG_ENABLE(gimpprinttest, [  --disable-gimpprinttest Do not try to compile and run a test GIMP-PRINT
                          program],
		    , enable_gimpprinttest=yes)

  if test x$gimpprint_config_exec_prefix != x ; then
     gimpprint_config_args="$gimpprint_config_args --exec-prefix=$gimpprint_config_exec_prefix"
     if test x${GIMPPRINT_CONFIG+set} != xset ; then
        GIMPPRINT_CONFIG=$gimpprint_config_exec_prefix/bin/gimpprint-config
     fi
  fi
  if test x$gimpprint_config_prefix != x ; then
     gimpprint_config_args="$gimpprint_config_args --prefix=$gimpprint_config_prefix"
     if test x${GIMPPRINT_CONFIG+set} != xset ; then
        GIMPPRINT_CONFIG=$gimpprint_config_prefix/bin/gimpprint-config
     fi
  fi

  AC_PATH_PROG(GIMPPRINT_CONFIG, gimpprint-config, no)
  min_gimpprint_version=ifelse([$1], ,4.1.4,$1)
  AC_MSG_CHECKING(for GIMP-PRINT - version >= $min_gimpprint_version)
  no_gimpprint=""
  if test "$GIMPPRINT_CONFIG" = "no" ; then
    no_gimpprint=yes
  else
    GIMPPRINT_CFLAGS=`$GIMPPRINT_CONFIG $gimpprint_config_args --cflags`
    GIMPPRINT_LIBS=`$GIMPPRINT_CONFIG $gimpprint_config_args --libs`
    gimpprint_config_major_version=`$GIMPPRINT_CONFIG $gimpprint_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    gimpprint_config_minor_version=`$GIMPPRINT_CONFIG $gimpprint_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    gimpprint_config_micro_version=`$GIMPPRINT_CONFIG $gimpprint_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_gimpprinttest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GIMPPRINT_CFLAGS"
      LIBS="$GIMPPRINT_LIBS $LIBS"
dnl
dnl Now check if the installed GIMP-PRINT is sufficiently new. (Also sanity
dnl checks the results of gimpprint-config to some extent
dnl
      rm -f conf.gimpprinttest
      AC_TRY_RUN([
#include <gimp-print/gimp-print.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.gimpprinttest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = strdup("$min_gimpprint_version");
  if (!tmp_version) {
     exit(1);
   }
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_gimpprint_version");
     exit(1);
   }

  if ((gimpprint_major_version != $gimpprint_config_major_version) ||
      (gimpprint_minor_version != $gimpprint_config_minor_version) ||
      (gimpprint_micro_version != $gimpprint_config_micro_version))
    {
      printf("\n*** 'gimpprint-config --version' returned %d.%d.%d, but GIMP-PRINT (%d.%d.%d)\n",
             $gimpprint_config_major_version, $gimpprint_config_minor_version, $gimpprint_config_micro_version,
             gimpprint_major_version, gimpprint_minor_version, gimpprint_micro_version);
      printf ("*** was found! If gimpprint-config was correct, then it is best\n");
      printf ("*** to remove the old version of GIMP-PRINT. You may also be able to fix the\n");
      printf("*** error by modifying your LD_LIBRARY_PATH enviroment variable, or by\n");
      printf("*** editing /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If gimpprint-config was wrong, set the environment variable GIMPPRINT_CONFIG\n");
      printf("*** to point to the correct copy of gimpprint-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
    }
#if defined (GIMPPRINT_MAJOR_VERSION) && defined (GIMPPRINT_MINOR_VERSION) && defined (GIMPPRINT_MICRO_VERSION)
  else if ((gimpprint_major_version != GIMPPRINT_MAJOR_VERSION) ||
	   (gimpprint_minor_version != GIMPPRINT_MINOR_VERSION) ||
           (gimpprint_micro_version != GIMPPRINT_MICRO_VERSION))
    {
      printf("\n*** GIMP-PRINT header files (version %d.%d.%d) do not match\n",
	     GIMPPRINT_MAJOR_VERSION, GIMPPRINT_MINOR_VERSION, GIMPPRINT_MICRO_VERSION);
      printf("*** library (version %d.%d.%d)\n",
	     gimpprint_major_version, gimpprint_minor_version, gimpprint_micro_version);
    }
#endif /* defined (GIMPPRINT_MAJOR_VERSION) ... */
  else
    {
      if ((gimpprint_major_version > major) ||
        ((gimpprint_major_version == major) && (gimpprint_minor_version > minor)) ||
        ((gimpprint_major_version == major) && (gimpprint_minor_version == minor) && (gimpprint_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of GIMP-PRINT (%d.%d.%d) was found.\n",
               gimpprint_major_version, gimpprint_minor_version, gimpprint_micro_version);
        printf("*** You need a version of GIMP-PRINT newer than %d.%d.%d. The latest version of\n",
	       major, minor, micro);
        printf("*** GIMP-PRINT is always available from\n");
	printf("*** http://sourceforge.net/project/showfiles.php?group_id=1537.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the gimpprint-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of GIMP-PRINT, but you can also set the GIMPPRINT_CONFIG environment to\n");
        printf("*** point to the correct copy of gimpprint-config. (In this case, you will have\n");
        printf("*** to modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_gimpprint=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_gimpprint" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])
  else
     AC_MSG_RESULT(no)
     if test "$GIMPPRINT_CONFIG" = "no" ; then
       echo "*** The gimpprint-config script installed by GIMP-PRINT could not be found"
       echo "*** If GIMP-PRINT was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the GIMPPRINT_CONFIG environment variable to the"
       echo "*** full path to gimpprint-config."
     else
       if test -f conf.gimpprinttest ; then
        :
       else
          echo "*** Could not run GIMP-PRINT test program, checking why..."
          CFLAGS="$CFLAGS $GIMPPRINT_CFLAGS"
          LIBS="$LIBS $GIMPPRINT_LIBS"
          AC_TRY_LINK([
#include <gimp-print/gimp-print.h>
#include <stdio.h>
#include <string.h>
],      [ return ((gimpprint_major_version) || (gimpprint_minor_version) || (gimpprint_micro_version)); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding GIMP-PRINT or finding the wrong"
          echo "*** version of GIMP-PRINT. If it is not finding GIMP-PRINT, you'll need to set"
          echo "*** your LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means GIMP-PRINT was incorrectly"
          echo "*** installed or that you have moved GIMP-PRINT since it was installed. In the"
          echo "*** latter case, you may want to edit the gimpprint-config script:"
	  echo "*** $GIMPPRINT_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     GIMPPRINT_CFLAGS=""
     GIMPPRINT_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GIMPPRINT_CFLAGS)
  AC_SUBST(GIMPPRINT_LIBS)
  rm -f conf.gimpprinttest
])



dnl Configure Paths for Alsa
dnl Some modifications by Richard Boulton <richard-alsa@tartarus.org>
dnl Christopher Lansdown <lansdoct@cs.alfred.edu>
dnl Jaroslav Kysela <perex@suse.cz>
dnl Last modification: alsa.m4,v 1.23 2004/01/16 18:14:22 tiwai Exp
dnl AM_PATH_ALSA([MINIMUM-VERSION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for libasound, and define ALSA_CFLAGS and ALSA_LIBS as appropriate.
dnl enables arguments --with-alsa-prefix=
dnl                   --with-alsa-enc-prefix=
dnl                   --disable-alsatest
dnl
dnl For backwards compatibility, if ACTION_IF_NOT_FOUND is not specified,
dnl and the alsa libraries are not found, a fatal AC_MSG_ERROR() will result.
dnl
AC_DEFUN([AM_PATH_ALSA],
[dnl Save the original CFLAGS, LDFLAGS, and LIBS
alsa_save_CFLAGS="$CFLAGS"
alsa_save_LDFLAGS="$LDFLAGS"
alsa_save_LIBS="$LIBS"
alsa_found=yes

dnl
dnl Get the cflags and libraries for alsa
dnl
AC_ARG_WITH(alsa-prefix,
[  --with-alsa-prefix=PFX  Prefix where Alsa library is installed(optional)],
[alsa_prefix="$withval"], [alsa_prefix=""])

AC_ARG_WITH(alsa-inc-prefix,
[  --with-alsa-inc-prefix=PFX  Prefix where include libraries are (optional)],
[alsa_inc_prefix="$withval"], [alsa_inc_prefix=""])

dnl FIXME: this is not yet implemented
AC_ARG_ENABLE(alsatest,
[  --disable-alsatest      Do not try to compile and run a test Alsa program],
[enable_alsatest="$enableval"],
[enable_alsatest=yes])

dnl Add any special include directories
AC_MSG_CHECKING(for ALSA CFLAGS)
if test "$alsa_inc_prefix" != "" ; then
	ALSA_CFLAGS="$ALSA_CFLAGS -I$alsa_inc_prefix"
	CFLAGS="$CFLAGS -I$alsa_inc_prefix"
fi
AC_MSG_RESULT($ALSA_CFLAGS)
CFLAGS="$alsa_save_CFLAGS"

dnl add any special lib dirs
AC_MSG_CHECKING(for ALSA LDFLAGS)
if test "$alsa_prefix" != "" ; then
	ALSA_LIBS="$ALSA_LIBS -L$alsa_prefix"
	LDFLAGS="$LDFLAGS $ALSA_LIBS"
fi

dnl add the alsa library
ALSA_LIBS="$ALSA_LIBS -lasound -lm -ldl -lpthread"
LIBS=`echo $LIBS | sed 's/-lm//'`
LIBS=`echo $LIBS | sed 's/-ldl//'`
LIBS=`echo $LIBS | sed 's/-lpthread//'`
LIBS=`echo $LIBS | sed 's/  //'`
LIBS="$ALSA_LIBS $LIBS"
AC_MSG_RESULT($ALSA_LIBS)

dnl Check for a working version of libasound that is of the right version.
min_alsa_version=ifelse([$1], ,0.1.1,$1)
AC_MSG_CHECKING(for libasound headers version >= $min_alsa_version)
no_alsa=""
    alsa_min_major_version=`echo $min_alsa_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    alsa_min_minor_version=`echo $min_alsa_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    alsa_min_micro_version=`echo $min_alsa_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

AC_LANG_SAVE
AC_LANG_C
AC_TRY_COMPILE([
#include <alsa/asoundlib.h>
], [
/* ensure backward compatibility */
#if !defined(SND_LIB_MAJOR) && defined(SOUNDLIB_VERSION_MAJOR)
#define SND_LIB_MAJOR SOUNDLIB_VERSION_MAJOR
#endif
#if !defined(SND_LIB_MINOR) && defined(SOUNDLIB_VERSION_MINOR)
#define SND_LIB_MINOR SOUNDLIB_VERSION_MINOR
#endif
#if !defined(SND_LIB_SUBMINOR) && defined(SOUNDLIB_VERSION_SUBMINOR)
#define SND_LIB_SUBMINOR SOUNDLIB_VERSION_SUBMINOR
#endif

#  if(SND_LIB_MAJOR > $alsa_min_major_version)
  exit(0);
#  else
#    if(SND_LIB_MAJOR < $alsa_min_major_version)
#       error not present
#    endif

#   if(SND_LIB_MINOR > $alsa_min_minor_version)
  exit(0);
#   else
#     if(SND_LIB_MINOR < $alsa_min_minor_version)
#          error not present
#      endif

#      if(SND_LIB_SUBMINOR < $alsa_min_micro_version)
#        error not present
#      endif
#    endif
#  endif
exit(0);
],
  [AC_MSG_RESULT(found.)],
  [AC_MSG_RESULT(not present.)
   ifelse([$3], , [AC_MSG_ERROR(Sufficiently new version of libasound not found.)])
   alsa_found=no]
)
AC_LANG_RESTORE

dnl Now that we know that we have the right version, let's see if we have the library and not just the headers.
if test "x$enable_alsatest" = "xyes"; then
AC_CHECK_LIB([asound], [snd_ctl_open],,
	[ifelse([$3], , [AC_MSG_ERROR(No linkable libasound was found.)])
	 alsa_found=no]
)
fi

LDFLAGS="$alsa_save_LDFLAGS"
LIBS="$alsa_save_LIBS"

if test "x$alsa_found" = "xyes" ; then
   ifelse([$2], , :, [$2])
else
   ALSA_CFLAGS=""
   ALSA_LIBS=""
   ifelse([$3], , :, [$3])
fi

dnl That should be it.  Now just export out symbols:
AC_SUBST(ALSA_CFLAGS)
AC_SUBST(ALSA_LIBS)
])

dnl The following lines were copied from gtk-doc.m4

dnl Usage:
dnl   GTK_DOC_CHECK([minimum-gtk-doc-version])
AC_DEFUN([GTK_DOC_CHECK],
[
  AC_BEFORE([AC_PROG_LIBTOOL],[$0])dnl setup libtool first
  AC_BEFORE([AM_PROG_LIBTOOL],[$0])dnl setup libtool first
  dnl for overriding the documentation installation directory
  AC_ARG_WITH(html-dir,
    AC_HELP_STRING([--with-html-dir=PATH], [path to installed docs]),,
    [with_html_dir='${datadir}/gtk-doc/html'])
  HTML_DIR="$with_html_dir"
  AC_SUBST(HTML_DIR)

  dnl enable/disable documentation building
  AC_ARG_ENABLE(gtk-doc,
    AC_HELP_STRING([--enable-gtk-doc],
                   [use gtk-doc to build documentation (default=no)]),,
    enable_gtk_doc=no)

  have_gtk_doc=no
  if test x$enable_gtk_doc = xyes; then
    if test -z "$PKG_CONFIG"; then
      AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
    fi
    if test "$PKG_CONFIG" != "no" && $PKG_CONFIG --exists gtk-doc; then
      have_gtk_doc=yes
    fi

  dnl do we want to do a version check?
ifelse([$1],[],,
    [gtk_doc_min_version=$1
    if test "$have_gtk_doc" = yes; then
      AC_MSG_CHECKING([gtk-doc version >= $gtk_doc_min_version])
      if $PKG_CONFIG --atleast-version $gtk_doc_min_version gtk-doc; then
        AC_MSG_RESULT(yes)
      else
        AC_MSG_RESULT(no)
        have_gtk_doc=no
      fi
    fi
])
    if test "$have_gtk_doc" != yes; then
      enable_gtk_doc=no
    fi
  fi

  AM_CONDITIONAL(ENABLE_GTK_DOC, test x$enable_gtk_doc = xyes)
  AM_CONDITIONAL(GTK_DOC_USE_LIBTOOL, test -n "$LIBTOOL")
])

dnl GIMP_DETECT_CFLAGS(RESULT, FLAGSET)
dnl Detect if the compiler supports a set of flags

AC_DEFUN([GIMP_DETECT_CFLAGS],
[
  $1=
  for flag in $2; do
    if test -z "[$]$1"; then
      $1_save_CFLAGS="$CFLAGS"
      CFLAGS="$CFLAGS $flag"
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
