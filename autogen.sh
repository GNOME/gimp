#!/bin/sh

# This script does all the magic calls to automake/autoconf and
# friends that are needed to configure a git clone. As described in
# the file HACKING you need a couple of extra tools to run this script
# successfully.
#
# If you are compiling from a released tarball you don't need these
# tools and you shouldn't use this script.  Just call ./configure
# directly.

ACLOCAL=${ACLOCAL-aclocal-1.11}
AUTOCONF=${AUTOCONF-autoconf}
AUTOHEADER=${AUTOHEADER-autoheader}
AUTOMAKE=${AUTOMAKE-automake-1.11}
LIBTOOLIZE=${LIBTOOLIZE-libtoolize}

AUTOCONF_REQUIRED_VERSION=2.54
AUTOMAKE_REQUIRED_VERSION=1.10.0
INTLTOOL_REQUIRED_VERSION=0.40.1
LIBTOOL_REQUIRED_VERSION=1.5
LIBTOOL_WIN32_REQUIRED_VERSION=2.2


PROJECT="GNU Image Manipulation Program"
TEST_TYPE=-d
FILE=plug-ins


srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
ORIGDIR=`pwd`
cd $srcdir


check_version ()
{
    VERSION_A=$1
    VERSION_B=$2

    save_ifs="$IFS"
    IFS=.
    set dummy $VERSION_A 0 0 0
    MAJOR_A=$2
    MINOR_A=$3
    MICRO_A=$4
    set dummy $VERSION_B 0 0 0
    MAJOR_B=$2
    MINOR_B=$3
    MICRO_B=$4
    IFS="$save_ifs"

    if expr "$MAJOR_A" = "$MAJOR_B" > /dev/null; then
        if expr "$MINOR_A" \> "$MINOR_B" > /dev/null; then
           echo "yes (version $VERSION_A)"
        elif expr "$MINOR_A" = "$MINOR_B" > /dev/null; then
            if expr "$MICRO_A" \>= "$MICRO_B" > /dev/null; then
               echo "yes (version $VERSION_A)"
            else
                echo "Too old (version $VERSION_A)"
                DIE=1
            fi
        else
            echo "Too old (version $VERSION_A)"
            DIE=1
        fi
    elif expr "$MAJOR_A" \> "$MAJOR_B" > /dev/null; then
	echo "Major version might be too new ($VERSION_A)"
    else
	echo "Too old (version $VERSION_A)"
	DIE=1
    fi
}

echo
echo "I am testing that you have the tools required to build the"
echo "$PROJECT from git. This test is not foolproof,"
echo "so if anything goes wrong, see the file HACKING for more information..."
echo

DIE=0

OS=`uname -s`
case $OS in
    *YGWIN* | *INGW*)
	echo "Looks like Win32, you will need libtool $LIBTOOL_WIN32_REQUIRED_VERSION or newer."
	echo
	LIBTOOL_REQUIRED_VERSION=$LIBTOOL_WIN32_REQUIRED_VERSION
	;;
esac

echo -n "checking for libtool >= $LIBTOOL_REQUIRED_VERSION ... "
if ($LIBTOOLIZE --version) < /dev/null > /dev/null 2>&1; then
   LIBTOOLIZE=$LIBTOOLIZE
elif (glibtoolize --version) < /dev/null > /dev/null 2>&1; then
   LIBTOOLIZE=glibtoolize
else
    echo
    echo "  You must have libtool installed to compile $PROJECT."
    echo "  Install the appropriate package for your distribution,"
    echo "  or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
    echo
    DIE=1
fi

if test x$LIBTOOLIZE != x; then
    VER=`$LIBTOOLIZE --version \
         | grep libtool | sed "s/.* \([0-9.]*\)[-a-z0-9]*$/\1/"`
    check_version $VER $LIBTOOL_REQUIRED_VERSION
fi

# check if gtk-doc is explicitely disabled
for ag_option in $AUTOGEN_CONFIGURE_ARGS $@
do
  case $ag_option in
    -disable-gtk-doc | --disable-gtk-doc)
    enable_gtk_doc=no
  ;;
  esac
done

if test x$enable_gtk_doc = xno; then
  echo "skipping test for gtkdocize"
else
  echo -n "checking for gtkdocize ... "
  if (gtkdocize --version) < /dev/null > /dev/null 2>&1; then
      echo "yes"
  else
      echo
      echo "  You must have gtk-doc installed to compile $PROJECT."
      echo "  Install the appropriate package for your distribution,"
      echo "  or get the source tarball at"
      echo "  http://ftp.gnome.org/pub/GNOME/sources/gtk-doc/"
      echo "  You can also use the option --disable-gtk-doc to skip"
      echo "  this test but then you will not be able to generate a"
      echo "  configure script that can build the API documentation."
      DIE=1
  fi
fi

echo -n "checking for autoconf >= $AUTOCONF_REQUIRED_VERSION ... "
if ($AUTOCONF --version) < /dev/null > /dev/null 2>&1; then
    VER=`$AUTOCONF --version | head -n 1 \
         | grep -iw autoconf | sed "s/.* \([0-9.]*\)[-a-z0-9]*$/\1/"`
    check_version $VER $AUTOCONF_REQUIRED_VERSION
else
    echo
    echo "  You must have autoconf installed to compile $PROJECT."
    echo "  Download the appropriate package for your distribution,"
    echo "  or get the source tarball at ftp://ftp.gnu.org/pub/gnu/autoconf/"
    echo
    DIE=1;
fi


echo -n "checking for automake >= $AUTOMAKE_REQUIRED_VERSION ... "
if ($AUTOMAKE --version) < /dev/null > /dev/null 2>&1; then
   AUTOMAKE=$AUTOMAKE
   ACLOCAL=$ACLOCAL
elif (automake-1.15 --version) < /dev/null > /dev/null 2>&1; then
   AUTOMAKE=automake-1.15
   ACLOCAL=aclocal-1.15
elif (automake-1.14 --version) < /dev/null > /dev/null 2>&1; then
   AUTOMAKE=automake-1.14
   ACLOCAL=aclocal-1.14
elif (automake-1.13 --version) < /dev/null > /dev/null 2>&1; then
   AUTOMAKE=automake-1.13
   ACLOCAL=aclocal-1.13
elif (automake-1.12 --version) < /dev/null > /dev/null 2>&1; then
   AUTOMAKE=automake-1.12
   ACLOCAL=aclocal-1.12
elif (automake-1.11 --version) < /dev/null > /dev/null 2>&1; then
   AUTOMAKE=automake-1.11
   ACLOCAL=aclocal-1.11
elif (automake-1.10 --version) < /dev/null > /dev/null 2>&1; then
   AUTOMAKE=automake-1.10
   ACLOCAL=aclocal-1.10
else
    echo
    echo "  You must have automake $AUTOMAKE_REQUIRED_VERSION or newer installed to compile $PROJECT."
    echo "  Download the appropriate package for your distribution,"
    echo "  or get the source tarball at ftp://ftp.gnu.org/pub/gnu/automake/"
    echo
    DIE=1
fi

if test x$AUTOMAKE != x; then
    VER=`$AUTOMAKE --version \
         | grep automake | sed "s/.* \([0-9.]*\)[-a-z0-9]*$/\1/"`
    check_version $VER $AUTOMAKE_REQUIRED_VERSION
fi


echo -n "checking for intltool >= $INTLTOOL_REQUIRED_VERSION ... "
if (intltoolize --version) < /dev/null > /dev/null 2>&1; then
    VER=`intltoolize --version \
         | grep intltoolize | sed "s/.* \([0-9.]*\)/\1/"`
    check_version $VER $INTLTOOL_REQUIRED_VERSION
else
    echo
    echo "  You must have intltool installed to compile $PROJECT."
    echo "  Get the latest version from"
    echo "  ftp://ftp.gnome.org/pub/GNOME/sources/intltool/"
    echo
    DIE=1
fi


echo -n "checking for xsltproc ... "
if (xsltproc --version) < /dev/null > /dev/null 2>&1; then
    echo "yes"
else
    echo
    echo "  You must have xsltproc installed to compile $PROJECT."
    echo "  Get the latest version from"
    echo "  ftp://ftp.gnome.org/pub/GNOME/sources/libxslt/"
    echo
    DIE=1
fi

if test "$DIE" -eq 1; then
    echo
    echo "Please install/upgrade the missing tools and call me again."
    echo	
    exit 1
fi


test $TEST_TYPE $FILE || {
    echo
    echo "You must run this script in the top-level $PROJECT directory."
    echo
    exit 1
}


if test -z "$NOCONFIGURE"; then
    echo
    echo "I am going to run ./configure with the following arguments:"
    echo
    echo "  $AUTOGEN_CONFIGURE_ARGS $@"
    echo

    if test -z "$*"; then
        echo "If you wish to pass additional arguments, please specify them "
        echo "on the $0 command line or set the AUTOGEN_CONFIGURE_ARGS "
        echo "environment variable."
        echo
    fi
fi


if test -z "$ACLOCAL_FLAGS"; then

    acdir=`$ACLOCAL --print-ac-dir`
    m4list="glib-2.0.m4 glib-gettext.m4 gtk-2.0.m4 intltool.m4 pkg.m4"

    for file in $m4list
    do
	if [ ! -f "$acdir/$file" ]; then
	    echo
	    echo "WARNING: aclocal's directory is $acdir, but..."
            echo "         no file $acdir/$file"
            echo "         You may see fatal macro warnings below."
            echo "         If these files are installed in /some/dir, set the "
            echo "         ACLOCAL_FLAGS environment variable to \"-I /some/dir\""
            echo "         or install $acdir/$file."
            echo
        fi
    done
fi

rm -rf autom4te.cache

$ACLOCAL $ACLOCAL_FLAGS
RC=$?
if test $RC -ne 0; then
   echo "$ACLOCAL gave errors. Please fix the error conditions and try again."
   exit $RC
fi

$LIBTOOLIZE --force || exit $?

if test x$enable_gtk_doc = xno; then
    if test -f gtk-doc.make; then :; else
       echo "EXTRA_DIST = missing-gtk-doc" > gtk-doc.make
    fi
    echo "WARNING: You have disabled gtk-doc."
    echo "         As a result, you will not be able to generate the API"
    echo "         documentation and 'make dist' will not work."
    echo
else
    gtkdocize || exit $?
fi

# optionally feature autoheader
($AUTOHEADER --version)  < /dev/null > /dev/null 2>&1 && $AUTOHEADER || exit 1

$AUTOMAKE --add-missing || exit $?
$AUTOCONF || exit $?

intltoolize --automake || exit $?


cd $ORIGDIR

if test -z "$NOCONFIGURE"; then
    $srcdir/configure $AUTOGEN_CONFIGURE_ARGS "$@"
    RC=$?
    if test $RC -ne 0; then
      echo
      echo "Configure failed or did not finish!"
      exit $RC
    fi

    echo
    echo "Now type 'make' to compile the $PROJECT."
fi
