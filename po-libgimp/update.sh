#!/bin/sh

PACKAGE="gimp14-libgimp"
PATH="$PATH:.."

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

echo "Building the $PACKAGE.pot ..."
intltool-update --gettext-package $PACKAGE --pot

echo "Now merging $1.po with $PACKAGE.pot, and creating an updated $1.po ..." 
mv $1.po $1.po.old && msgmerge $1.po.old $PACKAGE.pot -o $1.po && rm $1.po.old;
msgfmt --statistics $1.po

fi
