#!/bin/sh

set -e


# BASH ENV
if [[ -z "$CROSSROAD_PLATFORM" ]]; then

# So that we can use gimp-console from gimp-debian-x64 project.
GIMP_APP_VERSION=$(grep GIMP_APP_VERSION _build${ARTIFACTS_SUFFIX}/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
mkdir bin
echo "#!/bin/sh" > bin/gimp-console-$GIMP_APP_VERSION
gcc -print-multi-os-directory | grep . && LIB_DIR=$(gcc -print-multi-os-directory | sed 's/\.\.\///g') || LIB_DIR="lib"
gcc -print-multiarch | grep . && LIB_SUBDIR=$(echo $(gcc -print-multiarch)'/')
echo export LD_LIBRARY_PATH="${GIMP_PREFIX}/${LIB_DIR}/${LIB_SUBDIR}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" >> bin/gimp-console-$GIMP_APP_VERSION
echo export GI_TYPELIB_PATH="${GIMP_PREFIX}/${LIB_DIR}/${LIB_SUBDIR}girepository-1.0${GI_TYPELIB_PATH:+:$GI_TYPELIB_PATH}" >> bin/gimp-console-$GIMP_APP_VERSION
echo "${GIMP_PREFIX}/bin/gimp-console-$GIMP_APP_VERSION \"\$@\"" >> bin/gimp-console-$GIMP_APP_VERSION
chmod u+x bin/gimp-console-$GIMP_APP_VERSION

git submodule update --init


# CROSSROAD ENV
else
export ARTIFACTS_SUFFIX="-x64"

## The required packages for GIMP are taken from the previous job

## Build GIMP
mkdir _build${ARTIFACTS_SUFFIX}-cross && cd _build${ARTIFACTS_SUFFIX}-cross
crossroad meson setup .. -Dgi-docgen=disabled                 \
                         -Djavascript=disabled -Dlua=disabled \
                         -Dpython=disabled -Dvala=disabled
ninja
ninja install
cd ..

## XXX Functional fix to the problem of non-configured interpreters
## XXX Also, functional generator of the pixbuf 'loaders.cache' for GUI image support
GIMP_APP_VERSION=$(grep GIMP_APP_VERSION _build${ARTIFACTS_SUFFIX}-cross/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
GDK_PATH=$(echo ${CROSSROAD_PREFIX}/lib/gdk-pixbuf-*/*/)
GDK_PATH=$(sed "s|${CROSSROAD_PREFIX}/||g" <<< $GDK_PATH)
GDK_PATH=$(sed 's|/|\\|g' <<< $GDK_PATH)
echo "@echo off
      echo This is a CI crossbuild of GIMP.
      :: Don't run this under PowerShell since it produces UTF-16 files.
      echo .js   (JavaScript) plug-ins ^|^ NOT supported!
      echo .lua  (Lua) plug-ins        ^|^ NOT supported!
      echo .py   (Python) plug-ins     ^|^ NOT supported!
      echo .scm  (ScriptFu) plug-ins   ^|^ NOT supported!
      echo .vala (Vala) plug-ins       ^|^ NOT supported!
      bin\gdk-pixbuf-query-loaders.exe ${GDK_PATH}loaders\*.dll > ${GDK_PATH}loaders.cache
      echo.
      bin\gimp-$GIMP_APP_VERSION.exe" > ${CROSSROAD_PREFIX}/gimp.cmd
echo "Please run the gimp.cmd file to know the actual plug-in support." > ${CROSSROAD_PREFIX}/README.txt

## Copy built GIMP, babl and GEGL and pre-built packages to GIMP_PREFIX
cp -fr $CROSSROAD_PREFIX/ _install${ARTIFACTS_SUFFIX}-cross/

fi # END OF CROSSROAD ENV
