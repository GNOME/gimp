#!/bin/bash
prefix=$1
encoding=$2
infile=$3

#replace [] with \[\]
prefix=$(echo "$prefix" | sed 's/[][]/\\\0/g')

#echo to stdout
sed '/^\w\+'"$prefix"'=/{s/\(.\)'"$prefix"'/\1/;n};/^\w.*=/d' "$infile" \
| iconv -f UTF-8 -t "$encoding"
