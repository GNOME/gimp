#!/bin/sh

PACKAGE="gimp20-libgimp"
PATH="$PATH:.."

echo "Calling intltool-update for you ..."
intltool-update --gettext-package $PACKAGE $*
