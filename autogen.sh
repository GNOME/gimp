#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir
PROJECT=GIMP
TEST_TYPE=-d
FILE=plug-ins

DIE=0

(libtool --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have libtool installed to compile $PROJECT."
        echo "Install the appropriate package for your distribution,"
        echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
        DIE=1
}
(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have autoconf installed to compile $PROJECT."
	echo "Download the appropriate package for your distribution,"
	echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
	DIE=1
}
(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have automake installed to compile $PROJECT."
	echo "Get ftp://ftp.cygnus.com/pub/home/tromey/automake-1.4p1.tar.gz"
	echo "(or a newer version if it is available)"
	DIE=1
}
(glib-gettextize --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have glib-gettextize installed to compile $PROJECT."
	echo "glib-gettextize is part of glib-2.0, so you should already"
        echo "have it. Make sure it is in your PATH".
	DIE=1
}
(intltoolize --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have intltoolize installed to compile $PROJECT."
	echo "Get the latest version from"
        echo "ftp://ftp.gnome.org/pub/GNOME/stable/sources/intltool/"
	DIE=1
}

echo "I am testing that you have the required versions of libtool, autoconf," 
echo "automake, glib-gettextize and intltoolize. This test is not foolproof,"
echo "so if anything goes wrong, see the file HACKING for more information..."
echo

echo "Testing libtool... "
VER=`libtoolize --version | grep libtool | sed "s/.* \([0-9.]*\)[-a-z0-9]*$/\1/"`
if expr $VER \>= 1.3.4 >/dev/null; then
	echo "looks OK."
else
	echo "too old! (Need 1.3.4, have $VER)"
	DIE=1
fi

echo "Testing autoconf... "
VER=`autoconf --version | grep -iw autoconf | sed "s/.* \([0-9.]*\)[-a-z0-9]*$/\1/"`
if expr $VER \>= 2.13 >/dev/null; then
	echo "looks OK."
else
	echo "too old! (Need 2.13, have $VER)"
	DIE=1
fi

echo "Testing automake... "
VER=`automake --version | grep automake | sed "s/.* \([0-9.]*\)[-a-z0-9]*$/\1/"`
if expr $VER \>= 1.4 >/dev/null; then
	echo "looks OK."
else
	echo "too old! (Need 1.4, have $VER)"
	DIE=1
fi

echo "Testing glib-gettextize... "
VER=`glib-gettextize --version | grep glib-gettextize | sed "s/.* \([0-9.]*\)/\1/"`
if expr $VER \>= 1.3.14 >/dev/null; then
        echo "looks OK."
else
        echo "too old! (Need 1.3.14, have $VER)"
        DIE=1
fi

echo "Testing intltoolize... "
VER=`intltoolize --version | grep intltoolize | sed "s/.* \([0-9.]*\)/\1/"`
if expr $VER \>= 0.17 >/dev/null; then
        echo "looks OK."
else
        echo "too old! (Need 0.17, have $VER)"
        DIE=1
fi
echo

if test "$DIE" -eq 1; then
	exit 1
fi

test $TEST_TYPE $FILE || {
	echo "You must run this script in the top-level $PROJECT directory"
	exit 1
}

if test -z "$*"; then
	echo "I am going to run ./configure with no arguments - if you wish "
        echo "to pass any to it, please specify them on the $0 command line."
fi

case $CC in
*xlc | *xlc\ * | *lcc | *lcc\ *) am_opt=--include-deps;;
esac

if test -z "$ACLOCAL_FLAGS"; then

        acdir=`aclocal --print-ac-dir`
        m4list="glib-2.0.m4 glib-gettext.m4 gtk-2.0.m4 intltool.m4 pkg.m4"

        for file in $m4list
        do
                if [ ! -f "$acdir/$file" ]; then
                        echo "WARNING: aclocal's directory is $acdir, but..."
                        echo "         no file $acdir/$file"
                        echo "         You may see fatal macro warnings below."
                        echo "         If these files are installed in /some/dir, set the ACLOCAL_FLAGS "
                        echo "         environment variable to \"-I /some/dir\", or install"
                        echo "         $acdir/$file."
                        echo ""
                fi
        done
fi

aclocal $ACLOCAL_FLAGS

# optionally feature autoheader
(autoheader --version)  < /dev/null > /dev/null 2>&1 && autoheader

automake --add-missing $am_opt
autoconf

echo "Running glib-gettextize"
glib-gettextize --copy --force
echo "Running intltoolize"
intltoolize --copy --force --automake

cd $ORIGDIR

$srcdir/configure --enable-maintainer-mode "$@"

echo 
echo "Now type 'make' to compile $PROJECT."
