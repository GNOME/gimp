#!/bin/sh

if [ -z "$GIT_DEPTH" ]; then
  export GIT_DEPTH=1
fi


# SHELL ENV
if [ -z "$CROSSROAD_PLATFORM" ]; then
apt-get install -y --no-install-recommends \
                   wine \
                   wine64
git clone --depth $GIT_DEPTH https://gitlab.freedesktop.org/crossroad/crossroad.git
cd crossroad
git apply ../build/windows/patches/0001-platforms-Enable-ccache.patch
./setup.py install --prefix=`pwd`/../.local
cd ..
exit 0


# CROSSROAD ENV
else
export ARTIFACTS_SUFFIX="-x64"

## Install the required (pre-built) packages for babl, GEGL and GIMP
crossroad source msys2
DEPS_LIST=$(cat build/windows/gitlab-ci/all-deps-uni.txt |
            sed "s/\${MINGW_PACKAGE_PREFIX}-//g"         |
            sed 's/\\//g')
crossroad install $DEPS_LIST
if [ $? -ne 0 ]; then
  echo "Installation of pre-built dependencies failed.";
  exit 1;
fi

## Clone babl and GEGL (follow master branch)
mkdir _deps && cd _deps
git clone --depth $GIT_DEPTH https://gitlab.gnome.org/GNOME/babl.git _babl
git clone --depth $GIT_DEPTH https://gitlab.gnome.org/GNOME/gegl.git _gegl

## Build babl and GEGL
mkdir _babl/_build${ARTIFACTS_SUFFIX}-cross/ && cd _babl/_build${ARTIFACTS_SUFFIX}-cross/
crossroad meson setup .. -Denable-gir=false
ninja
ninja install
ccache --show-stats

mkdir ../../_gegl/_build${ARTIFACTS_SUFFIX}-cross/ && cd ../../_gegl/_build${ARTIFACTS_SUFFIX}-cross/
crossroad meson setup .. -Dintrospection=false
ninja
ninja install
ccache --show-stats
cd ../../

## "Build" part of deps
### Generator of the gio 'giomodule.cache' to fix error about
### libgiognutls.dll that prevents generating loaders.cache
echo 'libgiognomeproxy.dll: gio-proxy-resolver
libgiognutls.dll: gio-tls-backend
libgiolibproxy.dll: gio-proxy-resolver
libgioopenssl.dll: gio-tls-backend' > ${CROSSROAD_PREFIX}/lib/gio/modules/giomodule.cache

### Generator of the pixbuf 'loaders.cache' for GUI image support
GDK_PATH=$(echo ${CROSSROAD_PREFIX}/lib/gdk-pixbuf-*/*/)
echo '"lib\\gdk-pixbuf-2.0\\2.10.0\\loaders\\libpixbufloader-png.dll"
      "png" 5 "gdk-pixbuf" "PNG" "LGPL"
      "image/png" ""
      "png" ""
      "\211PNG\r\n\032\n" "" 100

      "lib\\gdk-pixbuf-2.0\\2.10.0\\loaders\\libpixbufloader-svg.dll"
      "svg" 6 "gdk-pixbuf" "Scalable Vector Graphics" "LGPL"
      "image/svg+xml" "image/svg" "image/svg-xml" "image/vnd.adobe.svg+xml" "text/xml-svg" "image/svg+xml-compressed" ""
      "svg" "svgz" "svg.gz" ""
      " <svg" "*    " 100
      " <!DOCTYPE svg" "*             " 100

      ' > $GDK_PATH/loaders.cache

### NOT WORKING: Fallback generator of the pixbuf 'loaders.cache' for GUI image support
#wine ${CROSSROAD_PREFIX}/bin/gdk-pixbuf-query-loaders.exe ${GDK_PATH}loaders/*.dll > ${GDK_PATH}loaders.cache
#sed -i "s&$CROSSROAD_PREFIX/&&" ${CROSSROAD_PREFIX}/${GDK_PATH}/loaders.cache
#sed -i '/.dll\"/s*/*\\\\*g' ${CROSSROAD_PREFIX}/${GDK_PATH}/loaders.cache

### Generator of the glib 'gschemas.compiled'
GLIB_PATH=$(echo ${CROSSROAD_PREFIX}/share/glib-*/schemas/)
wine ${CROSSROAD_PREFIX}/bin/glib-compile-schemas.exe --targetdir=${GLIB_PATH} ${GLIB_PATH}

fi # END OF CROSSROAD ENV
