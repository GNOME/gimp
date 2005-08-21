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
          echo "*** exact error that occurred. This usually means GIMP-PRINT was incorrectly"
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
