#!/bin/bash

# Copyright (C) 2015  Ville Pätsi <drc@gimp.org>

SCRIPT_FOLDER=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

FIRST_COMMIT="$1"
[ -z "$FIRST_COMMIT" ] && FIRST_COMMIT="950412fbdc720fe2600f58f04f25145d9073895d" # First after tag 2.8.0

declare -a FOLDERS=('app tools menus etc' \
    'libgimp libgimpbase libgimpcolor libgimpconfig libgimpmath libgimpmodule libgimpthumb libgimpwidgets' \
    'plug-ins' \
    'modules'
    'build' \
    'themes icons')

OUTPUTFILE=${SCRIPT_FOLDER}/../NEWS_since_"${FIRST_COMMIT}"

pushd ${SCRIPT_FOLDER}/..

for folderloop in "${FOLDERS[@]}"
do uppercase_folderloop="`echo ${folderloop:0:1} | tr  '[:lower:]' '[:upper:]'`${folderloop:1}"
    echo -e "${uppercase_folderloop}:\n" >> "${OUTPUTFILE}"
    git log --date-order --reverse --date=short --pretty=format:"- %h %s" "${FIRST_COMMIT}"..HEAD ${folderloop} >> "${OUTPUTFILE}"
    echo -e "\n\n" >> "${OUTPUTFILE}"
done

popd

echo "NEWS generated into ${OUTPUTFILE}"
