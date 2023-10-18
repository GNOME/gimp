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

export OPTIONAL_PACKAGES=""
if [[ "$MSYSTEM" == "CLANGARM64" ]]; then
  # No luajit package on clangarm64 for the time being.
  export OPTIONAL_PACKAGES="${MINGW_PACKAGE_PREFIX}-lua51"
else
  export OPTIONAL_PACKAGES="${MINGW_PACKAGE_PREFIX}-luajit"
fi

export PATH="${MSYS2_PREFIX}/bin:$PATH"

# Update everything
pacman --noconfirm -Suy

# Install the required packages
pacman --noconfirm -S --needed \
    base-devel \
    ${MINGW_PACKAGE_PREFIX}-toolchain \
    ${MINGW_PACKAGE_PREFIX}-autotools \
    ${MINGW_PACKAGE_PREFIX}-ccache \
    ${MINGW_PACKAGE_PREFIX}-meson \
    ${MINGW_PACKAGE_PREFIX}-pkgconf \
    \
    $OPTIONAL_PACKAGES \
    ${MINGW_PACKAGE_PREFIX}-lua51-lgi \
    \
    ${MINGW_PACKAGE_PREFIX}-aalib \
    ${MINGW_PACKAGE_PREFIX}-appstream-glib \
    ${MINGW_PACKAGE_PREFIX}-atk \
    ${MINGW_PACKAGE_PREFIX}-brotli \
    ${MINGW_PACKAGE_PREFIX}-cairo \
    ${MINGW_PACKAGE_PREFIX}-cfitsio \
    ${MINGW_PACKAGE_PREFIX}-drmingw \
    ${MINGW_PACKAGE_PREFIX}-gexiv2 \
    ${MINGW_PACKAGE_PREFIX}-ghostscript \
    ${MINGW_PACKAGE_PREFIX}-gi-docgen \
    ${MINGW_PACKAGE_PREFIX}-glib-networking \
    ${MINGW_PACKAGE_PREFIX}-gobject-introspection \
    ${MINGW_PACKAGE_PREFIX}-gobject-introspection-runtime \
    ${MINGW_PACKAGE_PREFIX}-graphviz \
    ${MINGW_PACKAGE_PREFIX}-gtk3 \
    ${MINGW_PACKAGE_PREFIX}-headers-git \
    ${MINGW_PACKAGE_PREFIX}-iso-codes \
    ${MINGW_PACKAGE_PREFIX}-json-c \
    ${MINGW_PACKAGE_PREFIX}-json-glib \
    ${MINGW_PACKAGE_PREFIX}-lcms2 \
    ${MINGW_PACKAGE_PREFIX}-lensfun \
    ${MINGW_PACKAGE_PREFIX}-libarchive \
    ${MINGW_PACKAGE_PREFIX}-libheif \
    ${MINGW_PACKAGE_PREFIX}-libiff \
    ${MINGW_PACKAGE_PREFIX}-libilbm \
    ${MINGW_PACKAGE_PREFIX}-libjxl \
    ${MINGW_PACKAGE_PREFIX}-libmypaint \
    ${MINGW_PACKAGE_PREFIX}-libspiro \
    ${MINGW_PACKAGE_PREFIX}-libwebp \
    ${MINGW_PACKAGE_PREFIX}-libwmf \
    ${MINGW_PACKAGE_PREFIX}-maxflow \
    ${MINGW_PACKAGE_PREFIX}-mypaint-brushes \
    ${MINGW_PACKAGE_PREFIX}-openexr \
    ${MINGW_PACKAGE_PREFIX}-pango \
    ${MINGW_PACKAGE_PREFIX}-poppler \
    ${MINGW_PACKAGE_PREFIX}-poppler-data \
    ${MINGW_PACKAGE_PREFIX}-python \
    ${MINGW_PACKAGE_PREFIX}-python-gobject \
    ${MINGW_PACKAGE_PREFIX}-qoi \
    ${MINGW_PACKAGE_PREFIX}-shared-mime-info \
    ${MINGW_PACKAGE_PREFIX}-suitesparse \
    ${MINGW_PACKAGE_PREFIX}-vala \
    ${MINGW_PACKAGE_PREFIX}-xpm-nox

# XXX We've got a weird error when the prefix is in the current dir.
# Until we figure it out, this trick seems to work, even though it's
# completely ridiculous.
rm -fr ~/_install${ARTIFACTS_SUFFIX}
mv "_install${ARTIFACTS_SUFFIX}" ~

export MSYS2_PREFIX="/c/msys64${MINGW_PREFIX}/"
export GIMP_PREFIX="`realpath ~/_install`${ARTIFACTS_SUFFIX}"
export PATH="$GIMP_PREFIX/bin:${MSYS2_PREFIX}/bin:$PATH"
export PKG_CONFIG_PATH="${GIMP_PREFIX}/lib/pkgconfig:$PKG_CONFIG_PATH"
export PKG_CONFIG_PATH="${GIMP_PREFIX}/share/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="${GIMP_PREFIX}/lib:${LD_LIBRARY_PATH}"
export ACLOCAL_FLAGS="-I${MSYS2_PREFIX}share/aclocal"
export XDG_DATA_DIRS="${GIMP_PREFIX}/share:${MINGW_PREFIX}/share/"

mkdir -p _ccache
export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"

# XXX Do not enable ccache this way because it breaks
# gobject-introspection rules. Let's see later for ccache.
# See: https://github.com/msys2/MINGW-packages/issues/9677
#export CC="ccache gcc"

#ccache --zero-stats
#ccache --show-stats

mkdir "_build${ARTIFACTS_SUFFIX}"
cd "_build${ARTIFACTS_SUFFIX}"
# We disable javascript as we are not able for the time being to add a
# javascript interpreter with GObject Introspection (GJS/spidermonkey
# and Seed/Webkit are the 2 contenders so far, but they are not
# available on MSYS2 and we are told it's very hard to build them).
# TODO: re-enable javascript plug-ins when we can figure this out.
meson .. -Dprefix="${GIMP_PREFIX}"           \
         -Ddirectx-sdk-dir="${MSYS2_PREFIX}" \
         -Djavascript=disabled               \
         -Dbuild-id=org.gimp.GIMP_official   \
         -Dgi-docgen=disabled
ninja
ninja install
cd ..

#ccache --show-stats

# XXX Moving back the prefix to be used as artifacts.
mv "${GIMP_PREFIX}" .
