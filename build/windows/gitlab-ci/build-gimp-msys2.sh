#!/bin/bash

set -e

if [[ "$MSYSTEM" == "MINGW32" ]]; then
    export MSYS2_ARCH="i686"
else
    export MSYS2_ARCH="x86_64"
fi

# Why do we even have to remove these manually? The whole thing is
# messed up, but it looks like the Gitlab runner fails to clean properly
# (it spews a bunch of "failed to remove" warnings at runner start, then
# ends with a "error: failed to commit transaction (conflicting files)"
# listing the various files it failed to remove).
# Might be tied to: https://gitlab.com/gitlab-org/gitlab-runner/-/issues/1839
rm -f /c/msys64/mingw64/bin/libpcre-1.dll
rm -f /c/msys64/mingw64/bin/libgio-2.0-0.dll
rm -f /c/msys64/mingw64/bin/libglib-2.0-0.dll
rm -f /c/msys64/mingw64/bin/libgmodule-2.0-0.dll
rm -f /c/msys64/mingw64/bin/libgobject-2.0-0.dll
rm -f /c/msys64/mingw64/bin/libpng16-16.dll
rm -f /c/msys64/mingw64/bin/gdk-pixbuf-pixdata.exe
rm -f /c/msys64/mingw64/bin/libgdk_pixbuf-2.0-0.dll
rm -f /c/msys64/mingw64/lib/gdk-pixbuf-2.0/2.10.0/loaders/libpixbufloader-png.dll

# Update everything
pacman --noconfirm -Suy

# Install the required packages
pacman --noconfirm -S --needed \
    base-devel \
    mingw-w64-$MSYS2_ARCH-toolchain \
    mingw-w64-$MSYS2_ARCH-ccache \
    \
    mingw-w64-$MSYS2_ARCH-appstream-glib \
    mingw-w64-$MSYS2_ARCH-atk \
    mingw-w64-$MSYS2_ARCH-cairo \
    mingw-w64-$MSYS2_ARCH-drmingw \
    mingw-w64-$MSYS2_ARCH-gexiv2 \
    mingw-w64-$MSYS2_ARCH-ghostscript \
    mingw-w64-$MSYS2_ARCH-glib-networking \
    mingw-w64-$MSYS2_ARCH-gobject-introspection \
    mingw-w64-$MSYS2_ARCH-gobject-introspection-runtime \
    mingw-w64-$MSYS2_ARCH-graphviz \
    mingw-w64-$MSYS2_ARCH-gtk3 \
    mingw-w64-$MSYS2_ARCH-gtk-doc \
    mingw-w64-$MSYS2_ARCH-iso-codes \
    mingw-w64-$MSYS2_ARCH-json-c \
    mingw-w64-$MSYS2_ARCH-json-glib \
    mingw-w64-$MSYS2_ARCH-lcms2 \
    mingw-w64-$MSYS2_ARCH-lensfun \
    mingw-w64-$MSYS2_ARCH-libarchive \
    mingw-w64-$MSYS2_ARCH-libheif \
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
    mingw-w64-$MSYS2_ARCH-python3-gobject \
    mingw-w64-$MSYS2_ARCH-shared-mime-info \
    mingw-w64-$MSYS2_ARCH-suitesparse \
    mingw-w64-$MSYS2_ARCH-vala \
    mingw-w64-$MSYS2_ARCH-xpm-nox

# XXX We've got a weird error when the prefix is in the current dir.
# Until we figure it out, this trick seems to work, even though it's
# completely ridiculous.
mv _install ~

export GIMP_PREFIX=`realpath ~/_install`
export PATH="$GIMP_PREFIX/bin:$PATH"
export PKG_CONFIG_PATH="${GIMP_PREFIX}/lib/pkgconfig:$PKG_CONFIG_PATH"
export PKG_CONFIG_PATH="${GIMP_PREFIX}/share/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="${GIMP_PREFIX}/lib:${LD_LIBRARY_PATH}"
export ACLOCAL_FLAGS="-I/c/msys64/mingw64/share/aclocal"
export XDG_DATA_DIRS="${GIMP_PREFIX}/share:/mingw64/share/"

mkdir -p _ccache
export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"
export CC="ccache gcc"

ccache --zero-stats
ccache --show-stats

./autogen.sh --prefix="${GIMP_PREFIX}"
make -j4
make install

ccache --show-stats

# XXX Moving back the prefix to be used as artifacts.
mv "${GIMP_PREFIX}" .
