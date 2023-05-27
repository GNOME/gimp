#!/bin/bash

set -e

if [[ "$MSYSTEM" == "MINGW32" ]]; then
    export ARTIFACTS_SUFFIX="-w32"
    export MSYS2_ARCH="i686"
    export MSYS2_PREFIX="/c/msys64/mingw32"
    export GIMP_OPTIONS="-Dvala=disabled"
else
    export ARTIFACTS_SUFFIX="-w64"
    export MSYS2_ARCH="x86_64"
    export MSYS2_PREFIX="/c/msys64/mingw64/"
fi

export ACLOCAL_FLAGS="-I${MSYS2_PREFIX}/share/aclocal"
export PATH="${MSYS2_PREFIX}/bin:$PATH"

# Update everything
pacman --noconfirm -Suy

# Install the required packages
pacman --noconfirm -S --needed \
    base-devel \
    mingw-w64-$MSYS2_ARCH-toolchain \
    mingw-w64-$MSYS2_ARCH-autotools \
    mingw-w64-$MSYS2_ARCH-ccache \
    mingw-w64-$MSYS2_ARCH-meson \
    \
    mingw-w64-$MSYS2_ARCH-aalib \
    mingw-w64-$MSYS2_ARCH-appstream-glib \
    mingw-w64-$MSYS2_ARCH-atk \
    mingw-w64-$MSYS2_ARCH-brotli \
    mingw-w64-$MSYS2_ARCH-cairo \
    mingw-w64-$MSYS2_ARCH-cfitsio \
    mingw-w64-$MSYS2_ARCH-drmingw \
    mingw-w64-$MSYS2_ARCH-gexiv2 \
    mingw-w64-$MSYS2_ARCH-ghostscript \
    mingw-w64-$MSYS2_ARCH-gi-docgen \
    mingw-w64-$MSYS2_ARCH-glib-networking \
    mingw-w64-$MSYS2_ARCH-gobject-introspection \
    mingw-w64-$MSYS2_ARCH-gobject-introspection-runtime \
    mingw-w64-$MSYS2_ARCH-graphviz \
    mingw-w64-$MSYS2_ARCH-gtk3 \
    mingw-w64-$MSYS2_ARCH-headers-git \
    mingw-w64-$MSYS2_ARCH-iso-codes \
    mingw-w64-$MSYS2_ARCH-json-c \
    mingw-w64-$MSYS2_ARCH-json-glib \
    mingw-w64-$MSYS2_ARCH-lcms2 \
    mingw-w64-$MSYS2_ARCH-lensfun \
    mingw-w64-$MSYS2_ARCH-libarchive \
    mingw-w64-$MSYS2_ARCH-libheif \
    mingw-w64-$MSYS2_ARCH-libiff \
    mingw-w64-$MSYS2_ARCH-libilbm \
    mingw-w64-$MSYS2_ARCH-libjxl \
    mingw-w64-$MSYS2_ARCH-libmypaint \
    mingw-w64-$MSYS2_ARCH-libspiro \
    mingw-w64-$MSYS2_ARCH-libwebp \
    mingw-w64-$MSYS2_ARCH-libwmf \
    mingw-w64-$MSYS2_ARCH-luajit \
    mingw-w64-$MSYS2_ARCH-maxflow \
    mingw-w64-$MSYS2_ARCH-mypaint-brushes \
    mingw-w64-$MSYS2_ARCH-openexr \
    mingw-w64-$MSYS2_ARCH-pango \
    mingw-w64-$MSYS2_ARCH-poppler \
    mingw-w64-$MSYS2_ARCH-poppler-data \
    mingw-w64-$MSYS2_ARCH-python \
    mingw-w64-$MSYS2_ARCH-python-gobject \
    mingw-w64-$MSYS2_ARCH-qoi \
    mingw-w64-$MSYS2_ARCH-shared-mime-info \
    mingw-w64-$MSYS2_ARCH-suitesparse \
    mingw-w64-$MSYS2_ARCH-vala \
    mingw-w64-$MSYS2_ARCH-xpm-nox

# XXX We've got a weird error when the prefix is in the current dir.
# Until we figure it out, this trick seems to work, even though it's
# completely ridiculous.
mv "_install${ARTIFACTS_SUFFIX}" ~

export GIMP_PREFIX="`realpath ~/_install`${ARTIFACTS_SUFFIX}"
export PATH="$GIMP_PREFIX/bin:$PATH"
export PKG_CONFIG_PATH="${GIMP_PREFIX}/lib/pkgconfig:$PKG_CONFIG_PATH"
export PKG_CONFIG_PATH="${GIMP_PREFIX}/share/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="${GIMP_PREFIX}/lib:${LD_LIBRARY_PATH}"
export ACLOCAL_FLAGS="-I/c/msys64/mingw32/share/aclocal"
export XDG_DATA_DIRS="${GIMP_PREFIX}/share:/mingw64/share/"

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
         -Dgi-docgen=disabled                \
         ${GIMP_OPTIONS}
ninja
ninja install
cd ..

#ccache --show-stats

# XXX Moving back the prefix to be used as artifacts.
mv "${GIMP_PREFIX}" .
