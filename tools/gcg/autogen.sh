#!/bin/sh

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

if test -z "$*"; then
	echo "I am going to run $srcdir/configure with no arguments - if you wish "
        echo "to pass any to it, please specify them on the $0 command line."
fi

echo processing...

THEDIR=`pwd`
cd $srcdir &&
aclocal $ACLOCAL_FLAGS &&
automake -a --foreign &&
autoconf &&
cd $THEDIR &&
$srcdir/configure "$@"

if [ $? -eq 0 ];then
	echo "Now run './configure', then 'make' to compile GCG."
else
	echo "Configuration error!"
fi
