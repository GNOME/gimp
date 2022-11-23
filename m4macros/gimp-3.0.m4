# Configure paths for LIGMA-3.0
# Manish Singh, Sven Neumann
# Large parts shamelessly stolen from Owen Taylor

dnl AM_PATH_LIGMA_3_0([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for LIGMA, and define LIGMA_CFLAGS and LIGMA_LIBS
dnl
AC_DEFUN([AM_PATH_LIGMA_3_0],
[dnl 
dnl Get the cflags and libraries from pkg-config
dnl

AC_ARG_ENABLE(ligmatest, [  --disable-ligmatest      do not try to compile and run a test LIGMA program],, enable_ligmatest=yes)

  pkg_name=ligma-3.0
  pkg_config_args="$pkg_name ligmaui-3.0"

  no_ligma=""

  AC_PATH_PROG(PKG_CONFIG, pkg-config, no)

  if test x$PKG_CONFIG != xno ; then
    if pkg-config --atleast-pkgconfig-version 0.7 ; then
      :
    else
      echo *** pkg-config too old; version 0.7 or better required.
      no_ligma=yes
      PKG_CONFIG=no
    fi
  else
    no_ligma=yes
  fi

  min_ligma_version=ifelse([$1], ,3.0.0,$1)
  AC_MSG_CHECKING(for LIGMA - version >= $min_ligma_version)

  if test x$PKG_CONFIG != xno ; then
    ## don't try to run the test against uninstalled libtool libs
    if $PKG_CONFIG --uninstalled $pkg_config_args; then
	  echo "Will use uninstalled version of LIGMA found in PKG_CONFIG_PATH"
	  enable_ligmatest=no
    fi

    if $PKG_CONFIG --atleast-version $min_ligma_version $pkg_config_args; then
	  :
    else
	  no_ligma=yes
    fi
  fi

  if test x"$no_ligma" = x ; then
    LIGMA_CFLAGS=`$PKG_CONFIG $pkg_config_args --cflags`
    LIGMA_LIBS=`$PKG_CONFIG $pkg_config_args --libs`
    LIGMA_CFLAGS_NOUI=`$PKG_CONFIG $pkg_name --cflags`
    LIGMA_LIBS_NOUI=`$PKG_CONFIG $pkg_name --libs`
    LIGMA_DATA_DIR=`$PKG_CONFIG $pkg_name --variable=ligmadatadir`
    LIGMA_PLUGIN_DIR=`$PKG_CONFIG $pkg_name --variable=ligmalibdir`

    ligma_pkg_major_version=`$PKG_CONFIG --modversion $pkg_name | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    ligma_pkg_minor_version=`$PKG_CONFIG --modversion $pkg_name | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    ligma_pkg_micro_version=`$PKG_CONFIG --modversion $pkg_name | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_ligmatest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $LIGMA_CFLAGS"
      LIBS="$LIGMA_LIBS $LIBS"

dnl
dnl Now check if the installed LIGMA is sufficiently new. (Also sanity
dnl checks the results of pkg-config to some extent
dnl
      rm -f conf.ligmatest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>

#include <libligma/ligma.h>

LigmaPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  NULL,  /* query_proc */
  NULL   /* run_proc */
};

int main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.ligmatest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_ligma_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_ligma_version");
     exit(1);
   }

    if (($ligma_pkg_major_version > major) ||
        (($ligma_pkg_major_version == major) && ($ligma_pkg_minor_version > minor)) ||
        (($ligma_pkg_major_version == major) && ($ligma_pkg_minor_version == minor) && ($ligma_pkg_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'pkg-config --modversion %s' returned %d.%d.%d, but the minimum version\n", "$pkg_name", $ligma_pkg_major_version, $ligma_pkg_minor_version, $ligma_pkg_micro_version);
      printf("*** of LIGMA required is %d.%d.%d. If pkg-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If pkg-config was wrong, set the environment variable PKG_CONFIG_PATH\n");
      printf("*** to point to the correct the correct configuration files\n");
      return 1;
    }
}

],, no_ligma=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_ligma" = x ; then
     AC_MSG_RESULT(yes (version $ligma_pkg_major_version.$ligma_pkg_minor_version.$ligma_pkg_micro_version))
     ifelse([$2], , :, [$2])     
  else
     if test "$PKG_CONFIG" = "no" ; then
       echo "*** A new enough version of pkg-config was not found."
       echo "*** See http://www.freedesktop.org/software/pkgconfig/"
     else
       if test -f conf.ligmatest ; then
        :
       else
          echo "*** Could not run LIGMA test program, checking why..."
          CFLAGS="$CFLAGS $LIGMA_CFLAGS"
          LIBS="$LIBS $LIGMA_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <libligma/ligma.h>

LigmaPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  NULL,  /* query_proc */
  NULL   /* run_proc */
};
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding LIGMA or finding the wrong"
          echo "*** version of LIGMA. If it is not finding LIGMA, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occurred. This usually means LIGMA is incorrectly installed."])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     LIGMA_CFLAGS=""
     LIGMA_LIBS=""
     LIGMA_CFLAGS_NOUI=""
     LIGMA_LIBS_NOUI=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(LIGMA_CFLAGS)
  AC_SUBST(LIGMA_LIBS)
  AC_SUBST(LIGMA_CFLAGS_NOUI)
  AC_SUBST(LIGMA_LIBS_NOUI)
  AC_SUBST(LIGMA_DATA_DIR)
  AC_SUBST(LIGMA_PLUGIN_DIR)
  rm -f conf.ligmatest
])
