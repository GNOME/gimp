#!/bin/sh

if test -z "$*"; then
	echo "I am going to run ./configure with no arguments - if you wish "
        echo "to pass any to it, please specify them on the $0 command line."
fi

echo processing...

aclocal $ACLOCAL_FLAGS &&
automake -a --foreign &&
autoconf &&
./configure "$@"

if [ $? -eq 0 ];then
	echo "Now type 'make' to compile GCG."
else
	echo "Configuration error!"
fi
