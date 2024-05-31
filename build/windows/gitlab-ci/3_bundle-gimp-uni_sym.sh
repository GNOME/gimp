#!/bin/bash

if [ "$MSYSTEM_CARCH" != 'i686' ]; then
  archsArray=('-a64'
              '-x64')
else
  archsArray=('-x86')
fi

# Splited .debug (DWARF) debug symbols
# (we extract and link them to make possible save space with Inno custom install)
for ARTIFACTS_SUFFIX in "${archsArray[@]}"; do

  ## Split/extract DWARF symbols from binary to .debug
  find gimp${ARTIFACTS_SUFFIX} \( -iname '*.dll' -or -iname '*.exe' -or -iname '*.pyd' \) -type f -exec objcopy --only-keep-debug '{}' '{}'.debug \;

  ## Link .debug to binary
  find gimp${ARTIFACTS_SUFFIX} \( -iname '*.dll' -or -iname '*.exe' -or -iname '*.pyd' \) -type f -exec objcopy --add-gnu-debuglink='{}'.debug '{}' --strip-unneeded \;

  ## Move .debug files to .debug folder
  debugList=$(find gimp${ARTIFACTS_SUFFIX} -iname '*.debug') && debugArray=($debugList)
  for debug in "${debugArray[@]}"; do
   #NAME="${debug##*/}"
    DIR="${debug%/*}"
    echo "$debug -> $DIR/.debug"
    if [ ! -d "$DIR/.debug" ]; then
      mkdir "$DIR/.debug"
    fi
    mv "$debug" "$DIR/.debug"
  done
done
