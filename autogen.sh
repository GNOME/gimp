#!/bin/sh 

# This script does all the magic calls to automake/autoconf and
# friends that are needed to configure a cvs checkout.  As described in
# the file HACKING you need a couple of extra tools to run this script
# successfully.
#
# If you are compiling from a released tarball you don't need these
# tools and you shouldn't use this script.  Just call ./configure
# directly.


PROJECT="The GIMP"
TEST_TYPE=-d
FILE=plug-ins

LIBTOOL_REQUIRED_VERSION=1.4
LIBTOOL_WIN32=1.5
AUTOCONF_REQUIRED_VERSION=2.54
AUTOMAKE_REQUIRED_VERSION=1.6
GLIB_REQUIRED_VERSION=2.2.0
INTLTOOL_REQUIRED_VERSION=0.17


srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
ORIGDIR=`pwd`
cd $srcdir


check_version ()
{
    if expr $1 \>= $2 > /dev/null; then
	echo "yes (version $1)"
    else
	echo "Too old (found version $1)!"
	DIE=1
    fi
}

echo
echo "I am testing that you have the required versions of libtool, autoconf," 
echo "automake, glib-gettextize and intltoolize. This test is not foolproof,"
echo "so if anything goes wrong, see the file HACKING for more information..."
echo

DIE=0


OS=`uname -s`
case $OS in 
    *YGWIN* | *INGW*)
	echo "Looks like Win32, you will need libtool $LIBTOOL_WIN32 or newer."
	echo
	LIBTOOL_REQUIRED_VERSION=$LIBTOOL_WIN32
	;;
esac


echo -n "checking for libtool >= $LIBTOOL_REQUIRED_VERSION ... "
if (libtoolize --version) < /dev/null > /dev/null 2>&1; then
   LIBTOOLIZE=libtoolize
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


echo -n "checking for autoconf >= $AUTOCONF_REQUIRED_VERSION ... "
if (autoconf --version) < /dev/null > /dev/null 2>&1; then
    VER=`autoconf --version \
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
if (automake-1.7 --version) < /dev/null > /dev/null 2>&1; then
   AUTOMAKE=automake-1.7
   ACLOCAL=aclocal-1.7
elif (automake-1.8 --version) < /dev/null > /dev/null 2>&1; then
   AUTOMAKE=automake-1.8
   ACLOCAL=aclocal-1.8
   AUTOMAKE_REQUIRED_VERSION=1.8.3
elif (automake-1.9 --version) < /dev/null > /dev/null 2>&1; then
   AUTOMAKE=automake-1.9
   ACLOCAL=aclocal-1.9
elif (automake-1.6 --version) < /dev/null > /dev/null 2>&1; then
   AUTOMAKE=automake-1.6
   ACLOCAL=aclocal-1.6
else
    echo
    echo "  You must have automake 1.6 or newer installed to compile $PROJECT."
    echo "  Download the appropriate package for your distribution,"
    echo "  or get the source tarball at ftp://ftp.gnu.org/pub/gnu/automake/"
    echo
    DIE=1
fi
AUTOMAKE=automake-1.9
ACLOCAL=aclocal-1.9

if test x$AUTOMAKE != x; then
    VER=`$AUTOMAKE --version \
         | grep automake | sed "s/.* \([0-9.]*\)[-a-z0-9]*$/\1/"`
    check_version $VER $AUTOMAKE_REQUIRED_VERSION
fi


echo -n "checking for glib-gettextize >= $GLIB_REQUIRED_VERSION ... "
if (glib-gettextize --version) < /dev/null > /dev/null 2>&1; then
    VER=`glib-gettextize --version \
         | grep glib-gettextize | sed "s/.* \([0-9.]*\)/\1/"`
    check_version $VER $GLIB_REQUIRED_VERSION
else
    echo
    echo "  You must have glib-gettextize installed to compile $PROJECT."
    echo "  glib-gettextize is part of glib-2.0, so you should already"
    echo "  have it. Make sure it is in your PATH."
    echo
    DIE=1
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

# Special test for problematic versions of intltool.  Details at:
#   http://bugzilla.gnome.org/show_bug.cgi?id=137502
# Print a warning message, but do not exit.
INTLTOOL_BUG_MIN_VERSION=0.28
INTLTOOL_BUG_MAX_VERSION=0.31
echo -n "checking for intltool < $INTLTOOL_BUG_MIN_VERSION or > $INTLTOOL_BUG_MAX_VERSION ... "
if (intltoolize --version) < /dev/null > /dev/null 2>&1; then
    VER=`intltoolize --version \
         | grep intltoolize | sed "s/.* \([0-9.]*\)/\1/"`
    if expr $VER \>= $INTLTOOL_BUG_MIN_VERSION > /dev/null; then
        if expr $VER \<= $INTLTOOL_BUG_MAX_VERSION > /dev/null; then
            echo "no"
            echo
            echo "  Versions of intltool between 0.28 and 0.31 are known to"
            echo "  generate incorrect XML output.  Please consider using an"
            echo "  earlier version of intltool in order to avoid these"
            echo "  problems until a newer version of intltool is released."
	    echo
	    echo "  This problem is harmless, you may continue the build." 
	    echo
        else
            echo "yes"
        fi
    else
        echo "yes"
    fi
else
    echo "not found"
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


if test -z "$*"; then
    if test -z "$AUTOGEN_CONFIGURE_ARGS"; then
	echo
	echo "I am going to run ./configure with no arguments - if you wish "
	echo "to pass any to it, please specify them on the $0 command line "
	echo "or set the AUTOGEN_CONFIGURE_ARGS environment variable."
	echo
    else
	echo
	echo "I am going to run ./configure with the following arguments:"
	echo
	echo "  $AUTOGEN_CONFIGURE_ARGS"
	echo
	echo "If you wish to pass additional arguments, please specify them "
	echo "on the $0 command line."
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
            echo "         If these files are installed in /some/dir, set the ACLOCAL_FLAGS "
            echo "         environment variable to \"-I /some/dir\", or install"
            echo "         $acdir/$file."
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

# optionally feature autoheader
(autoheader --version)  < /dev/null > /dev/null 2>&1 && autoheader || exit 1

$AUTOMAKE --add-missing || exit $?
autoconf || exit $?

glib-gettextize --copy --force || exit $?
intltoolize --copy --force --automake || exit $?


cd $ORIGDIR

$srcdir/configure --enable-maintainer-mode $AUTOGEN_CONFIGURE_ARGS "$@"
RC=$?
if test $RC -ne 0; then
  echo
  echo "Configure failed or did not finish!"
  exit $RC
fi

echo
echo "Now type 'make' to compile $PROJECT."
