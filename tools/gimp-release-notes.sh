#!/bin/bash

tag1=$1
tag2=$2

echo
echo "  Contributors:"

git shortlog -s ${tag1}..${tag2} \
    app \
    cursors \
    data \
    desktop \
    devel-docs \
    docs \
    etc \
    libgimp* \
    menus \
    modules \
    plug-ins \
    themes \
    tools \
    configure.ac \
    autogen.sh \
    *.pc \
    Makefile.am \
    build \
    INSTALL \
    NEWS* | cut -b 8- | sed -e 's/$/,/g'

echo
echo "  Translators:"

git shortlog -s ${tag1}..${tag2} \
    po* | cut -b 8- | sed -e 's/$/,/g'
