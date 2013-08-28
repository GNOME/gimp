#!/bin/bash

grep -e entry targets/*/_jhbuild/packagedb.xml | grep -o -e "package[^//]*" | while read line; do 
    COUPLE=`echo $line | grep -o -e "\"[^\"]*\"" | sed s/\"//g`
	PACKAGE=`echo "$COUPLE" | sed -n 1p`
	VERSION=`echo "$COUPLE" | sed -n 2p | grep -o -e "[^- :]*" | sed -n 1p`

	PACVERSION=`pacman -Qi $PACKAGE | grep Version | grep -o -e "[^- :]*" | sed -n 2p`
	if [ "$VERSION" != "$PACVERSION" ]; then
		echo $PACKAGE ":" $VERSION ":" $PACVERSION
	fi
done

#pacman -Qi $1 | grep Version | grep -o -e "[^- :]*" | sed -n 2p
