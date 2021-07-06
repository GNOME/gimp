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

## GLib (patched from MSYS2) ##

git clone --branch 2.68.0 --depth=${GIT_DEPTH} https://gitlab.gnome.org/GNOME/glib.git _glib

cd _glib/
wget "https://raw.githubusercontent.com/msys2/MINGW-packages/master/mingw-w64-glib2/0001-Update-g_fopen-g_open-and-g_creat-to-open-with-FILE_.patch"
wget "https://raw.githubusercontent.com/msys2/MINGW-packages/master/mingw-w64-glib2/0001-disable-some-tests-when-static.patch"
wget "https://raw.githubusercontent.com/msys2/MINGW-packages/master/mingw-w64-glib2/0001-win32-Make-the-static-build-work-with-MinGW-when-pos.patch"
wget "https://raw.githubusercontent.com/msys2/MINGW-packages/master/mingw-w64-glib2/0002-disable_glib_compile_schemas_warning.patch"
git apply 0001-Update-g_fopen-g_open-and-g_creat-to-open-with-FILE_.patch
git apply 0001-win32-Make-the-static-build-work-with-MinGW-when-pos.patch
patch -p1 < 0001-disable-some-tests-when-static.patch
git apply 0002-disable_glib_compile_schemas_warning.patch
# Only this patch is different from MSYS2 build.
git apply ../build/windows/patches/glib-mr2020.patch

mkdir _build
cd _build
meson -Dprefix="${GIMP_PREFIX}" -Dlibelf=disabled --buildtype=release \
      --wrap-mode=nodownload --auto-features=enabled \
      -Ddefault_library=shared -Dforce_posix_threads=true -Dgtk_doc=false ..
ninja
ninja install
cd ../..

# glib-networking is needed. No need to rebuild it, since we build the
# same version of glib with the same options, and just some additional
# patches, so we assume MSYS2-built packages should be fine.
mkdir -p ${GIMP_PREFIX}/lib/gio/modules/
cp -fr ${MSYS_PREFIX}/lib/gio/modules/*.dll ${GIMP_PREFIX}/lib/gio/modules/
# TODO: what about /mingw64/share/locale/*/LC_MESSAGES/glib-networking.mo ?

## GTK (patched from MSYS2) ##

pacman --noconfirm -S --needed \
    mingw-w64-$MSYS2_ARCH-adwaita-icon-theme \
    mingw-w64-$MSYS2_ARCH-atk \
    mingw-w64-$MSYS2_ARCH-gdk-pixbuf2 \
    mingw-w64-$MSYS2_ARCH-libepoxy \
    mingw-w64-$MSYS2_ARCH-libxslt \
    mingw-w64-$MSYS2_ARCH-sassc \
    mingw-w64-$MSYS2_ARCH-shared-mime-info

git clone --branch 3.24.29 --depth=${GIT_DEPTH} https://gitlab.gnome.org/GNOME/gtk.git _gtk

cd _gtk/
wget "https://github.com/msys2/MINGW-packages/raw/master/mingw-w64-gtk3/0002-Revert-Quartz-Set-the-popup-menu-type-hint-before-re.patch"
wget "https://github.com/msys2/MINGW-packages/raw/master/mingw-w64-gtk3/0003-gtkwindow-Don-t-force-enable-CSD-under-Windows.patch"
wget "https://github.com/msys2/MINGW-packages/raw/master/mingw-w64-gtk3/0004-Disable-low-level-keyboard-hook.patch"
wget "https://gitlab.gnome.org/GNOME/gtk/-/merge_requests/1563.patch"
patch -p1 < 0002-Revert-Quartz-Set-the-popup-menu-type-hint-before-re.patch
patch -p1 < 0003-gtkwindow-Don-t-force-enable-CSD-under-Windows.patch
patch -p1 < 0004-Disable-low-level-keyboard-hook.patch
# Patches not in MSYS2 build.
patch -p1 < ../build/windows/patches/gtk3-24-mr3275-gimp-issue-5475.patch
# The --binary option is necessary to accomodate such errors: Hunk #1 FAILED at 15 (different line endings)
patch -p1 --ignore-whitespace --binary < 1563.patch

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
