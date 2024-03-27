#!/bin/bash

# Based on https://gitlab.com/inkscape/inkscape/-/commit/b280917568051872793a0c7223b8d3f3928b7d26

set -e

export ARCH=x86_64


GIMP_DISTRIB="$CI_PROJECT_DIR/build/linux/appimage/AppDir"
GIMP_PREFIX="$GIMP_DISTRIB/usr"

LIB_DIR=$(gcc -print-multi-os-directory | sed 's/\.\.\///g')
gcc -print-multiarch | grep . && LIB_SUBDIR=$(echo $(gcc -print-multiarch)'/')
export PKG_CONFIG_PATH="${GIMP_PREFIX}/${LIB_DIR}/${LIB_SUBDIR}pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export LD_LIBRARY_PATH="${GIMP_PREFIX}/${LIB_DIR}/${LIB_SUBDIR}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export XDG_DATA_DIRS="${GIMP_PREFIX}/share:/usr/share${XDG_DATA_DIRS:+:$XDG_DATA_DIRS}"
export GI_TYPELIB_PATH="${GIMP_PREFIX}/${LIB_DIR}/${LIB_SUBDIR}girepository-1.0${GI_TYPELIB_PATH:+:$GI_TYPELIB_PATH}"


# Prepare environment
export CC=clang
export CXX=clang++
apt-get install -y --no-install-recommends clang libomp-dev wget


# Build babl
mkdir -p "$GIMP_PREFIX"
git clone --depth=1 https://gitlab.gnome.org/GNOME/babl.git _babl
mkdir _babl/_build-${ARCH} && cd _babl/_build-${ARCH}
meson setup .. -Dprefix="${GIMP_PREFIX}" -Dwith-docs=false
ninja
ninja install

# Build GEGL
cd ../..
git clone --depth=1 https://gitlab.gnome.org/GNOME/gegl.git _gegl
mkdir _gegl/_build-${ARCH} && cd _gegl/_build-${ARCH}
meson setup .. -Dprefix="${GIMP_PREFIX}" -Ddocs=false -Dworkshop=true
ninja
ninja install

# Build GIMP
cd ../..
mkdir -p "_build-${ARCH}" && cd "_build-${ARCH}"
meson setup .. -Dprefix="${GIMP_PREFIX}"         \
               -Dgi-docgen=disabled              \
               -Drelocatable-bundle=yes          \
               -Dbuild-id=org.gimp.GIMP_official
ninja
ninja install


# Generate AppImage (part 1)
cd ..
goappimage="appimagetool-823-x86_64.AppImage"
wget "https://github.com/probonopd/go-appimage/releases/download/continuous/$goappimage"
chmod +x "$goappimage"

"./$goappimage" --appimage-extract-and-run -s deploy $GIMP_DISTRIB/usr/share/applications/gimp.desktop
sed -i -e "s|$GIMP_DISTRIB/${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-.*/.*/loaders/||g" $GIMP_DISTRIB/${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-*/*/loaders.cache
echo 'BABL_PATH_TEMP=$(find "$HERE" -name gimp-8bit.so) && export BABL_PATH="${BABL_PATH_TEMP%/*}"' >> $GIMP_DISTRIB/AppRun
echo "BABL_PATH is $BABL_PATH"
echo 'GEGL_PATH_TEMP=$(find "$HERE" -name gegl-common.so) && export GEGL_PATH="${GEGL_PATH_TEMP%/*}"' >> $GIMP_DISTRIB/AppRun
echo "BABL_PATH is $GEGL_PATH"

# Generate AppImage (part 2)
appimagekit="appimagetool-x86_64.AppImage"
wget "https://github.com/AppImage/AppImageKit/releases/download/continuous/$appimagekit"
chmod +x "$appimagekit"

cp $GIMP_DISTRIB/usr/share/icons/hicolor/256x256/apps/gimp.png $GIMP_DISTRIB
"./$appimagekit" --appimage-extract-and-run $GIMP_DISTRIB


mkdir build/linux/appimage/_Output
SHA="$(git rev-parse --short HEAD)"
mv GNU*.AppImage* "build/linux/appimage/_Output/GIMP-$SHA.AppImage"
