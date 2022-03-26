#!/bin/sh

sed -i 's/<\(Prefix\|Image\)/\&lt;\1/g' "$1"
