#!/bin/bash

set -e

# $MINGW_PREFIX, $MINGW_PACKAGE_PREFIX and $MSYSTEM_ARCH are environment
# variables defined by MSYS2 filesytem package.
# https://github.com/msys2/MSYS2-packages/blob/master/filesystem/msystem

if [[ "$MSYSTEM_CARCH" == "i686" ]]; then
    export ARTIFACTS_SUFFIX="-w32"
elif [[ "$MSYSTEM_CARCH" == "x86_64" ]]; then
    export ARTIFACTS_SUFFIX="-w64"
else # [[ "$MSYSTEM_CARCH" == "aarch64" ]]
    export ARTIFACTS_SUFFIX="-arm64"
fi

# Update everything
pacman --noconfirm -Suy

# Install the required packages
pacman --noconfirm -S --needed \
    base-devel \
    ${MINGW_PACKAGE_PREFIX}-toolchain \
    ${MINGW_PACKAGE_PREFIX}-autotools \
    ${MINGW_PACKAGE_PREFIX}-meson \
    ${MINGW_PACKAGE_PREFIX}-pkgconf \
    \
    ${MINGW_PACKAGE_PREFIX}-cairo \
    ${MINGW_PACKAGE_PREFIX}-crt-git \
    ${MINGW_PACKAGE_PREFIX}-glib-networking \
    ${MINGW_PACKAGE_PREFIX}-gobject-introspection \
    ${MINGW_PACKAGE_PREFIX}-json-glib \
    ${MINGW_PACKAGE_PREFIX}-lcms2 \
    ${MINGW_PACKAGE_PREFIX}-lensfun \
    ${MINGW_PACKAGE_PREFIX}-libspiro \
    ${MINGW_PACKAGE_PREFIX}-maxflow \
    ${MINGW_PACKAGE_PREFIX}-openexr \
    ${MINGW_PACKAGE_PREFIX}-pango \
    ${MINGW_PACKAGE_PREFIX}-suitesparse \
    ${MINGW_PACKAGE_PREFIX}-vala

export MSYS2_PREFIX="/c/msys64${MINGW_PREFIX}/"
export GIT_DEPTH=1
export GIMP_PREFIX="`realpath ./_install`${ARTIFACTS_SUFFIX}"
export PATH="$GIMP_PREFIX/bin:$PATH"
export PKG_CONFIG_PATH="${GIMP_PREFIX}/lib/pkgconfig:$PKG_CONFIG_PATH"
export PKG_CONFIG_PATH="${GIMP_PREFIX}/share/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="${GIMP_PREFIX}/lib:${LD_LIBRARY_PATH}"
export ACLOCAL_FLAGS="-I${MSYS2_PREFIX}share/aclocal"
export XDG_DATA_DIRS="${GIMP_PREFIX}/share:${MINGW_PREFIX}/share/"

## babl and GEGL (follow master branch) ##

git clone --depth=${GIT_DEPTH} https://gitlab.gnome.org/GNOME/babl.git _babl
git clone --depth=${GIT_DEPTH} https://gitlab.gnome.org/GNOME/gegl.git _gegl

mkdir _babl/_build
cd _babl/_build
meson setup -Dprefix="${GIMP_PREFIX}" -Dwith-docs=false \
      ${BABL_OPTIONS} ..
ninja
ninja install

mkdir ../../_gegl/_build
cd ../../_gegl/_build
meson setup -Dprefix="${GIMP_PREFIX}" -Ddocs=false \
      -Dcairo=enabled -Dumfpack=enabled \
      -Dopenexr=enabled -Dworkshop=true ..
ninja
ninja install
cd ../..
