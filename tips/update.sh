#!/bin/sh

PACKAGE="gimp20-tips"
PATH="$PATH:.."

echo -n "Testing intltool version ... "
VER=`intltool-extract --version | grep intltool | sed "s/.* \([0-9.]*\)/\1/"`
if expr $VER \>= 0.17 >/dev/null; then
        echo "looks OK."
else
        echo "too old! (Need 0.17, have $VER)"
        exit 1
fi

if [ "x$1" = "x--help" ]; then

echo Usage: ./update.sh langcode
echo --help                  display this help and exit
echo
echo Examples of use:
echo ./update.sh       just creates a new pot file from the source
echo ./update.sh da    created new pot file and updated the da.po file 

elif [ "x$1" = "x" ]; then 

echo "Building the $PACKAGE.pot ..."
intltool-update --gettext-package $PACKAGE --pot

else

echo "Building the $PACKAGE.pot, merging and updating ..."
intltool-update --gettext-package $PACKAGE $1

fi

intltool-merge . gimp-tips.xml.in gimp-tips.xml -x -u -c .intltool-merge-cache


xmllint=`which xmllint`
if test -n "$xmllint" && test -x "$xmllint"; then
    echo "Validating gimp-tips.xml."
    $xmllint --noout --valid gimp-tips.xml \
    || ( echo "*************************";
	 echo "* gimp-tips.xml INVALID *";
	 echo "*************************";
	 exit 1; )
else
	echo "Can't find xmllint to validate gimp-tips.xml; proceed with fingers crossed...";
fi
