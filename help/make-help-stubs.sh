#!/bin/sh

path=`pwd`
dirs=`find C -type d | grep -v CVS`

for i in $dirs
do

cd $i
PATH=$path:$PATH makefile-am.sh
PATH=$path:$PATH dir-template.sh
cd $path

done
