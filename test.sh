#!/bin/sh

# Assuming that you have 'toolchain' installed
gcc -print-multi-os-directory 2>/dev/null | grep ./ && export LIB_DIR=$(gcc -print-multi-os-directory | sed 's/\.\.\///g') || export LIB_DIR="lib"
gcc -print-multiarch 2>/dev/null | grep . && export LIB_SUBDIR=$(echo $(gcc -print-multiarch)'/')

# Used to detect the build dependencies
export PKG_CONFIG_PATH="${GIMP_PREFIX}/${LIB_DIR}/${LIB_SUBDIR}pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
# Used to find the glib-introspection dependencies
export XDG_DATA_DIRS="${GIMP_PREFIX}/share:/usr/share${XDG_DATA_DIRS:+:$XDG_DATA_DIRS}"

# Used to find programs/tools during build and at runtime
export PATH="${GIMP_PREFIX}/bin:$PATH"
# Used to find the libraries at runtime
export LD_LIBRARY_PATH="${GIMP_PREFIX}/${LIB_DIR}/${LIB_SUBDIR}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
# Used to find introspection files
export GI_TYPELIB_PATH="${GIMP_PREFIX}/${LIB_DIR}/${LIB_SUBDIR}girepository-1.0${GI_TYPELIB_PATH:+:$GI_TYPELIB_PATH}"