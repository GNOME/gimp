#!/bin/bash

# crossroad don't have LLVM/Clang backend yet
if [[ "$BUILD_TYPE" != "CI_CROSS" ]] && [[ "$MSYSTEM_CARCH" != "i686" ]]; then
  export OBJCOPY="llvm-objcopy"
else
	export OBJCOPY="objcopy"
fi


# Generate .debug
find . \( -iname '*.dll' -or -iname '*.exe' -or -iname '*.pyd' \) -type f -exec $OBJCOPY --only-keep-debug '{}' '{}'.debug \;
find . \( -iname '*.dll' -or -iname '*.exe' -or -iname '*.pyd' \) -type f -exec $OBJCOPY --add-gnu-debuglink='{}'.debug '{}' --strip-unneeded \;

# Copy .debug to .debug folder
dbgList=$(find . -iname '*.debug') && dbgArray=($dbgList)
for dbg in "${dbgArray[@]}"; do
	FP="$dbg" 0> /dev/null
	NAME="${FP##*/}" 0> /dev/null
	DIR="${FP%/*}" 0> /dev/null
	echo "$FP -> $DIR/.debug" 0> /dev/null
	if [ ! -d "$DIR/.debug" ]; then
		mkdir "$DIR/.debug"
	fi
	mv "$FP" "$DIR/.debug"
done
