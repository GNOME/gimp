#!/bin/bash

if [ "$MSYSTEM_CARCH" != 'i686' ]; then
  archsArray=('-a64'
              '-x64')
else
  archsArray=('-x86')
fi


# (we extract and link DWARF .debug symbols to
# make possible save space with Inno custom install)
for ARTIFACTS_SUFFIX in "${archsArray[@]}"; do
  binArray=($(find gimp${ARTIFACTS_SUFFIX} \( -iname '*.dll' -or -iname '*.exe' -or -iname '*.pyd' \) -type f))
  for bin in "${binArray[@]}"; do
    echo "(INFO): extracting DWARF symbols from $bin"
    ## Split/extract DWARF symbols from binary to .debug
    objcopy --only-keep-debug $bin $bin.debug
    ## Link .debug to binary
    objcopy --add-gnu-debuglink=$bin.debug $bin --strip-unneeded
  done

  ## Move .debug files to .debug folder
  debugArray=($(find gimp${ARTIFACTS_SUFFIX} -iname '*.debug'))
  for debug in "${debugArray[@]}"; do
    DIR="${debug%/*}"
    if [ ! -d "$DIR/.debug" ]; then
      mkdir "$DIR/.debug"
    fi
    if [ "$DIR" != "$PREVIOUS_DIR" ]; then
      echo "(INFO): moving DWARF symbols to $DIR/.debug"
    fi
    mv "$debug" "$DIR/.debug"
    export PREVIOUS_DIR="$DIR"
  done
done
