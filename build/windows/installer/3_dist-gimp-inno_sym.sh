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
    debug=$(echo "${bin}.debug")
    NAME="${bin##*/}"
    DIR="${debug%/*}/.debug/"

    if [ ! -f "$DIR/$NAME.debug" ]; then
      ## Split/extract DWARF symbols from binary to .debug
      echo "(INFO): extracting DWARF symbols from $NAME to $DIR"
      objcopy --only-keep-debug $bin $debug

      ## Link .debug to binary
      objcopy --add-gnu-debuglink=$debug $bin --strip-unneeded

      ## Move .debug files to .debug folder
      if [ ! -d "$DIR" ]; then
        mkdir "$DIR"
      fi
      mv "$debug" "$DIR"
    fi
  done
done
