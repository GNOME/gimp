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
	echo "Get ftp://ftp.cygnus.com/pub/home/tromey/automake-1.2d.tar.gz"
	echo "(or a newer version if it is available)"
	DIE=1
}

echo "I am testing that you have the required versions of autoconf, automake"
echo "and gettext. This test is not foolproof, so if anything goes wrong,"
echo "see the file HACKING for more information..."
echo

echo "Testing autoconf... "
VER=`autoconf --version | sed "s/.* \([0-9.]*\)[a-z]\?$/\1/"`
if expr $VER \>= 2.13 >/dev/null; then
	echo "looks OK."
else
	echo "too old! (Need 2.13, have $VER)"
	DIE=1
fi

echo "Testing automake... "
VER=`automake --version | grep automake | sed "s/.* \([0-9.]*\)[a-z]\?$/\1/"`
if expr $VER \>= 1.4 >/dev/null; then
	echo "looks OK."
else
	echo "too old! (Need 1.4, have $VER)"
	DIE=1
fi

echo "Testing gettextize... "
VER=`gettextize --version | grep gettext | sed "s/.* \([0-9.]*\)[a-z]\?$/\1/;s/0.//"`
if expr $VER \>= 10.35 >/dev/null; then
	echo "looks OK."
else
	echo "too old! (Need (0.)10.35, have $VER)"
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
        m4list="gtk.m4 gettext.m4"

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

echo "Running gettextize...  Ignore non-fatal messages."
# Hmm, we specify --force here, since otherwise things dont'
# get added reliably, but we don't want to overwrite intl
# while making dist.
echo "no" | gettextize --copy --force

autogen_dirs="."
#if test -z "$NO_GCG"; then
#	autogen_dirs="$autogen_dirs tools/gcg"
#fi

for i in $autogen_dirs; do
	echo "Processing $i..."

	cd $i
	aclocal $ACLOCAL_FLAGS

	# optionally feature autoheader
	if grep AM_CONFIG_HEADER configure.in >/dev/null ; then
		(autoheader --version)  < /dev/null > /dev/null 2>&1 && autoheader
	fi

	automake --add-missing $am_opt
	autoconf
done

cd $ORIGDIR

$srcdir/configure --enable-maintainer-mode "$@"

echo 
echo "Now type 'make' to compile $PROJECT."
