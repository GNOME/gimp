#!/bin/bash

if [[ "$MSYSTEM" == "CLANGARM64" ]]; then
  # Apparently -v is unknown argument for clangarm64 version of objcopy.exe.
  export OBJCOPY_OPTIONS=""
else
  export OBJCOPY_OPTIONS="-v"
fi

if [ -z "$1" ]
then
	find . \( -iname '*.dll' -or -iname '*.exe' -or -iname '*.pyd' \) -type f -exec objcopy ${OBJCOPY_OPTIONS} --only-keep-debug '{}' '{}'.debug \;
	find . \( -iname '*.dll' -or -iname '*.exe' -or -iname '*.pyd' \) -type f -exec objcopy ${OBJCOPY_OPTIONS} --add-gnu-debuglink='{}'.debug '{}' --strip-unneeded \;
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
