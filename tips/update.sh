#!/bin/sh

PACKAGE="gimp14-tips"
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

intltool-extract --type=gettext/xml gimp-tips.xml.in
echo "Building the $PACKAGE.pot ..."
intltool-update --gettext-package $PACKAGE --pot

else

intltool-extract --type=gettext/xml gimp-tips.xml.in
echo "Building the $PACKAGE.pot, merging and updating ..."
intltool-update --gettext-package $PACKAGE $1

fi
