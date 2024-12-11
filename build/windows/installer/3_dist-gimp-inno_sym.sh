#!/bin/bash

if [ ! "$1" ]; then
  echo -e "\033[31m(ERROR)\033[0m: Script called without specifying a bundle. Please, call it this way: '$0 bundle_dir'."
  exit 1
fi


# (we extract and link DWARF .debug symbols to
# make possible save space with Inno custom install)
echo "(INFO): extracting DWARF symbols from binaries in $1 bundle"
binArray=($(find $1 \( -iname '*.dll' -or -iname '*.exe' -or -iname '*.pyd' \) -type f))
for bin in "${binArray[@]}"; do
  debug=$(echo "${bin}.debug")
  NAME="${bin##*/}"
  DIR="${debug%/*}/.debug/"

  if [ ! -f "$DIR/$NAME.debug" ]; then
    ## Split/extract DWARF symbols from binary to .debug
    #echo "(INFO): extracting DWARF symbols from $NAME to $DIR"
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
