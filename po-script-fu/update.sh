#!/bin/sh

PACKAGE="gimp20-script-fu"
PATH="$PATH:.."

echo "Calling intltool-update for you ..."
intltool-update --gettext-package $PACKAGE $*
