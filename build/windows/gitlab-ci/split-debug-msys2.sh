#!/bin/bash

if [ -z "$1" ]
then
	find . \( -iname '*.dll' -or -iname '*.exe' -or -iname '*.pyd' \) -type f -exec objcopy -v --only-keep-debug '{}' '{}'.debug \;
	find . \( -iname '*.dll' -or -iname '*.exe' -or -iname '*.pyd' \) -type f -exec objcopy -v --add-gnu-debuglink='{}'.debug '{}' --strip-unneeded \;
	find . -iname '*.debug' -exec "$0" {} +
else
	while [ -n "$1" ]
	do
		FP="$1"
		NAME="${FP##*/}"
		DIR="${FP%/*}"
		echo "$FP -> $DIR/.debug"
		if [ ! -d "$DIR/.debug" ]
		then
			mkdir "$DIR/.debug"
		fi
		mv "$FP" "$DIR/.debug"
		shift
	done
fi
