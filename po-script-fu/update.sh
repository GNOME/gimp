#!/bin/sh

PACKAGE="gimp14-script-fu"
PATH="$PATH:.."

echo "Calling intltool-update for you ..."
intltool-update --gettext-package $PACKAGE $*
