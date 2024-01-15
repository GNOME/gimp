#!/bin/bash
# $MSYSTEM_CARCH, $MINGW_PACKAGE_PREFIX and $MSYSTEM_PREFIX are defined by MSYS2.
# https://github.com/msys2/MSYS2-packages/blob/master/filesystem/msystem

set -e

if [[ "$MSYSTEM_CARCH" == "aarch64" ]]; then
    export ARTIFACTS_SUFFIX="-a64"
elif [[ "$MSYSTEM_CARCH" == "x86_64" ]]; then
    export ARTIFACTS_SUFFIX="-x64"
else # [[ "$MSYSTEM_CARCH" == "i686" ]];
    export ARTIFACTS_SUFFIX="-x86"
fi

if [[ "$BUILD_TYPE" != "CI_NATIVE" ]]; then
    # Make the script callable from every directory
    cd ~
    
    pacman --noconfirm -Suy
fi


# Install the required (pre-built) packages for babl and GEGL
pacman --noconfirm -S --needed \
    base-devel \
    ${MINGW_PACKAGE_PREFIX}-toolchain \
    ${MINGW_PACKAGE_PREFIX}-meson \
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


# Clone babl and GEGL (follow master branch)
export GIT_DEPTH=1
export GIMP_PREFIX="`realpath ./_install`${ARTIFACTS_SUFFIX}"
export PATH="$GIMP_PREFIX/bin:$PATH"
export PKG_CONFIG_PATH="${GIMP_PREFIX}/lib/pkgconfig:$PKG_CONFIG_PATH"
export PKG_CONFIG_PATH="${GIMP_PREFIX}/share/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="${GIMP_PREFIX}/lib:${LD_LIBRARY_PATH}"
export XDG_DATA_DIRS="${GIMP_PREFIX}/share:${MSYSTEM_PREFIX}/share/"

clone_or_pull() {
    if [ ! -d "_${1}" ]; then
        git clone --depth=${GIT_DEPTH} https://gitlab.gnome.org/GNOME/${1}.git _${1}
    else
        cd _${1} && git pull && cd ..
    fi
}

clone_or_pull babl
clone_or_pull gegl


# Build babl and GEGL
configure_or_build() {
    if [ ! -f "_${1}/_build/build.ninja" ]; then
        mkdir -p _${1}/_build${ARTIFACTS_SUFFIX} && cd _${1}/_build${ARTIFACTS_SUFFIX}
        meson setup .. -Dprefix="${GIMP_PREFIX}" \
                       $2
        ninja && ninja install
        cd ../..
    else
        cd _${1}/_build${ARTIFACTS_SUFFIX}
        ninja && ninja install
        cd ../..
    fi
}

configure_or_build babl "-Dwith-docs=false"

configure_or_build gegl "-Ddocs=false \
                         -Dcairo=enabled -Dumfpack=enabled \
                         -Dopenexr=enabled -Dworkshop=true"