#!/bin/bash

# Copy .debug to corresponding .debug folder
# (this is done in a separate script to reduce output)
while [ -n "$1" ]
do
	FP="$1"
	NAME="${FP##*/}"
	DIR="${FP%/*}"
	echo "$FP -> $DIR/.debug"
	if [ ! -d "$DIR/.debug" ]
	then
		mkdir "$DIR/.debug"
	fi
	mv "$FP" "$DIR/.debug"
	shift
done
