#!/bin/bash
prefix=$1
infile=$2

#replace [] with \[\]
prefix=$(echo "$prefix" | sed 's/[][]/\\\0/g')

# InnoSetup now supports UTF-8 for all languages, but it requires a BOM
# at the start of the file.
echo -ne "\\xEF\\xBB\\xBF";
sed '/^\w\+'"$prefix"'=/{s/\(.\)'"$prefix"'/\1/;n};/^\w.*=/d' "$infile"
