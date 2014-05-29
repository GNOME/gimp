#!/bin/bash

./build list -r -a | sort | while read line; do
	# Parse jhbuild's module versions
	COUPLE=`echo $line | grep -o -e "[^ :()]*"`
	PACKAGE=`echo "$COUPLE" | sed -n 1p`
	VERSION=`echo "$COUPLE" | sed -n 2p | grep -o -e "[^- :]*" | sed -n 1p`

	# Filter out git versions (40 char hashes), and metamodules (empty strings)
	if [ ${#VERSION} == 40 ] || [ ${#VERSION} == 0 ]; then
		continue
	fi

	# Determine pacman's version of the package
	PACVERSION=`pacman -Qi $PACKAGE | grep Version | grep -o -e "[^- :]*" | sed -n 2p`

	# Filter out packages that pacman doesn't know about
	if [ ${#PACVERSION} == 0 ]; then
		continue
	fi

	# Warn if the versions are different
	if [ "$VERSION" != "$PACVERSION" ]; then
		echo $PACKAGE ":" $VERSION ":" $PACVERSION
	fi
done
