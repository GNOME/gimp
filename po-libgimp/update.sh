#!/bin/sh

PACKAGE="gimp14-libgimp"
PATH="$PATH:.."

echo "Calling intltool-update for you ..."
intltool-update --gettext-package $PACKAGE $*
