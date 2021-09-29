#!/bin/bash

set -e

if [[ "$MSYSTEM" == "MINGW32" ]]; then
    export ARTIFACTS_SUFFIX="-w32"
    export MSYS2_ARCH="i686"
    # vapi build fails on 32-bit, with no error output. Let's just drop
    # it for this architecture.
    export BABL_OPTIONS="-Denable-vapi=false"
    export GEGL_OPTIONS="-Dvapigen=disabled"
    export MSYS_PREFIX="/c/msys64/mingw32/"
else
    export ARTIFACTS_SUFFIX="-w64"
    export MSYS2_ARCH="x86_64"
    export BABL_OPTIONS=""
    export GEGL_OPTIONS=""
    export MSYS_PREFIX="/c/msys64/mingw64/"
fi

# Update everything
pacman --noconfirm -Suy

# Install the required packages
pacman --noconfirm -S --needed \
    base-devel \
    mingw-w64-$MSYS2_ARCH-toolchain \
    mingw-w64-$MSYS2_ARCH-meson \
    \
    mingw-w64-$MSYS2_ARCH-cairo \
    mingw-w64-$MSYS2_ARCH-crt-git \
    mingw-w64-$MSYS2_ARCH-glib-networking \
    mingw-w64-$MSYS2_ARCH-gobject-introspection \
    mingw-w64-$MSYS2_ARCH-json-glib \
    mingw-w64-$MSYS2_ARCH-lcms2 \
    mingw-w64-$MSYS2_ARCH-lensfun \
    mingw-w64-$MSYS2_ARCH-libspiro \
    mingw-w64-$MSYS2_ARCH-maxflow \
    mingw-w64-$MSYS2_ARCH-openexr \
    mingw-w64-$MSYS2_ARCH-pango \
    mingw-w64-$MSYS2_ARCH-suitesparse \
    mingw-w64-$MSYS2_ARCH-vala

export GIT_DEPTH=1
export GIMP_PREFIX="`realpath ./_install`${ARTIFACTS_SUFFIX}"
export PATH="$GIMP_PREFIX/bin:$PATH"
export PKG_CONFIG_PATH="${GIMP_PREFIX}/lib/pkgconfig:$PKG_CONFIG_PATH"
export PKG_CONFIG_PATH="${GIMP_PREFIX}/share/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="${GIMP_PREFIX}/lib:${LD_LIBRARY_PATH}"
export ACLOCAL_FLAGS="-I/c/msys64/mingw64/share/aclocal"
export XDG_DATA_DIRS="${GIMP_PREFIX}/share:/mingw64/share/"

## AA-lib (not available in MSYS2) ##

wget https://downloads.sourceforge.net/aa-project/aalib-1.4rc5.tar.gz
echo "9801095c42bba12edebd1902bcf0a990 aalib-1.4rc5.tar.gz" | md5sum -c -
tar xzf aalib-1.4rc5.tar.gz
cd aalib-1.4.0
patch --binary -p1 < ../build/windows/patches/aalib-0001-Apply-patch-for-MSYS2.patch
patch --binary -p1 < ../build/windows/patches/aalib-0002-configure-src-tweak-Windows-link-flags.patch
wget "https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD" --output-document config.guess
wget "http://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub;hb=HEAD" --output-document config.sub
aclocal
libtoolize --force
automake --add-missing
autoconf
mkdir _build
cd _build
../configure --prefix="${GIMP_PREFIX}"
make
make install
cd ../..

## GTK (patched from MSYS2) ##

pacman --noconfirm -S --needed \
    mingw-w64-$MSYS2_ARCH-adwaita-icon-theme \
    mingw-w64-$MSYS2_ARCH-atk \
    mingw-w64-$MSYS2_ARCH-gdk-pixbuf2 \
    mingw-w64-$MSYS2_ARCH-libepoxy \
    mingw-w64-$MSYS2_ARCH-libxslt \
    mingw-w64-$MSYS2_ARCH-sassc \
    mingw-w64-$MSYS2_ARCH-shared-mime-info

git clone --branch 3.24.30 --depth=${GIT_DEPTH} https://gitlab.gnome.org/GNOME/gtk.git _gtk

cd _gtk/
wget "https://github.com/msys2/MINGW-packages/raw/master/mingw-w64-gtk3/0002-Revert-Quartz-Set-the-popup-menu-type-hint-before-re.patch"
wget "https://github.com/msys2/MINGW-packages/raw/master/mingw-w64-gtk3/0003-gtkwindow-Don-t-force-enable-CSD-under-Windows.patch"
wget "https://github.com/msys2/MINGW-packages/raw/master/mingw-w64-gtk3/0004-Disable-low-level-keyboard-hook.patch"
wget "https://github.com/msys2/MINGW-packages/raw/master/mingw-w64-gtk3/0005-gdkkeys-win32.c-fix-initialisation-of-key_state-in-u.patch"
patch -p1 < 0002-Revert-Quartz-Set-the-popup-menu-type-hint-before-re.patch
patch -p1 < 0003-gtkwindow-Don-t-force-enable-CSD-under-Windows.patch
patch -p1 < 0004-Disable-low-level-keyboard-hook.patch
patch -p1 < 0005-gdkkeys-win32.c-fix-initialisation-of-key_state-in-u.patch
# Patches not in MSYS2 build.
patch -p1 < ../build/windows/patches/gtk3-24-mr3661-gimp-issue-5475.patch

mkdir _build
cd _build
meson -Dprefix="${GIMP_PREFIX}" \
      --wrap-mode=nodownload --auto-features=enabled \
      --buildtype=release
ninja
ninja install
cd ../..

## babl and GEGL (follow master branch) ##

git clone --depth=${GIT_DEPTH} https://gitlab.gnome.org/GNOME/babl.git _babl
git clone --depth=${GIT_DEPTH} https://gitlab.gnome.org/GNOME/gegl.git _gegl

mkdir _babl/_build
cd _babl/_build
meson -Dprefix="${GIMP_PREFIX}" -Dwith-docs=false \
      ${BABL_OPTIONS} ..
ninja
ninja install

mkdir ../../_gegl/_build
cd ../../_gegl/_build
meson -Dprefix="${GIMP_PREFIX}" -Ddocs=false \
      -Dcairo=enabled -Dumfpack=enabled \
      -Dopenexr=enabled -Dworkshop=true \
      ${GEGL_OPTIONS} ..
ninja
ninja install
cd ../..

## libjxl ##
if [[ "$MSYS2_ARCH" == "x86_64" ]]; then
    pacman --noconfirm -S --needed \
        mingw64/mingw-w64-x86_64-brotli  \
        mingw64/mingw-w64-x86_64-highway \
        mingw64/mingw-w64-x86_64-cmake

    git clone --depth=${GIT_DEPTH} --branch v0.6.x --recursive https://github.com/libjxl/libjxl.git _libjxl

    mkdir _libjxl/_build
    cd _libjxl/_build
    cmake -G Ninja \
        -DCMAKE_INSTALL_PREFIX=${GIMP_PREFIX} \
        -DCMAKE_BUILD_TYPE=Release \
        -DJPEGXL_ENABLE_PLUGINS=OFF \
        -DBUILD_TESTING=OFF \
        -DJPEGXL_WARNINGS_AS_ERRORS=OFF \
        -DJPEGXL_ENABLE_SJPEG=OFF \
        -DJPEGXL_ENABLE_BENCHMARK=OFF \
        -DJPEGXL_ENABLE_EXAMPLES=OFF \
        -DJPEGXL_ENABLE_MANPAGES=OFF \
        -DJPEGXL_ENABLE_SKCMS=ON \
        -DJPEGXL_FORCE_SYSTEM_BROTLI=ON \
        -DJPEGXL_FORCE_SYSTEM_HWY=ON \
        -DJPEGXL_ENABLE_JNI=OFF \
        -DJPEGXL_ENABLE_TCMALLOC=OFF \
        -DJPEGXL_ENABLE_TOOLS=OFF \
        -DCMAKE_CXX_FLAGS="-DHWY_COMPILE_ONLY_SCALAR" ..
    ninja
    ninja install

    # move DLLs into correct location
    if [ -f ${GIMP_PREFIX}/lib/libjxl.dll ]; then
      mv --target-directory=${GIMP_PREFIX}/bin ${GIMP_PREFIX}/lib/libjxl.dll
    fi

    if [ -f ${GIMP_PREFIX}/lib/libjxl_threads.dll ]; then
      mv --target-directory=${GIMP_PREFIX}/bin ${GIMP_PREFIX}/lib/libjxl_threads.dll
    fi

    # install image/jxl mime type
    mkdir -p ${GIMP_PREFIX}/share/mime/packages
    cp --target-directory=${GIMP_PREFIX}/share/mime/packages ../plugins/mime/image-jxl.xml

    cd ../..
fi
