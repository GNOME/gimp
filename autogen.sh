#!/bin/sh

# This script does all the magic calls to automake/autoconf and
# friends that are needed to configure a git clone. As described in
# the file HACKING you need a couple of extra tools to run this script
# successfully.
#
# If you are compiling from a released tarball you don't need these
# tools and you shouldn't use this script.  Just call ./configure
# directly.

# Run this to generate all the initial makefiles, etc.
test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.

PROJECT="GNU Image Manipulation Program"

srcdir=`dirname $0`
ORIGDIR=`pwd`
cd $srcdir

(test -f configure.ac) || {
        echo "*** ERROR: Directory "\`$srcdir\'" does not look like the top-level project directory ***"
        exit 1
}

autoreconf --verbose --force --install -Wno-portability || exit $?

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
