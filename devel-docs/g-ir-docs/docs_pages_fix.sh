#!/bin/sh

sed -i 's/<\(Prefix\|Image\)/\&lt;\1/g' "$@"
sed -i 's/&\([a-z0-9_]\+[^a-z0-9_;]\)/\&amp;\1/g' "$@"
