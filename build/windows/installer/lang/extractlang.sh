#!/bin/bash
prefix=$1
encoding=$2
infile=$3

#replace [] with \[\]
prefix=$(echo "$prefix" | sed 's/[][]/\\\0/g')

#echo to stdout
sed '/^\w\+'"$prefix"'=/{s/\(.\)'"$prefix"'/\1/;n};/^\w.*=/d' "$infile" \
| iconv -c -f UTF-8 -t "$encoding"

# TODO: currently we silently discard non-convertible characters with -c
# option on iconv. Eventually we would want to just use UTF-8 for all
# language files (ideally), instead of folloding the LanguageCodePage of
# the main .isl file as provided in the issrc repository.
