#!/bin/sh

PACKAGE="gimp14-std-plug-ins"
PATH="$PATH:.."

echo "Calling intltool-update for you ..."
intltool-update --gettext-package $PACKAGE $*
