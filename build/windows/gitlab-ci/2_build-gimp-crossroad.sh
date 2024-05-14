#!/bin/sh

set -e


# BASH ENV
if [ -z "$CROSSROAD_PLATFORM" ]; then
if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
  if [ "$0" != "build/windows/gitlab-ci/1_build-deps-crossroad.sh" ]; then
    echo "To run this script locally, please do it from to the gimp git folder"
    exit 1
  fi
  export GIT_DEPTH=1
  git submodule update --init
  export WORK_DIR="$(dirname $PWD)'/'"
fi

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


# CROSSROAD ENV
crossroad w64 gimp --run="build/windows/gitlab-ci/2_build-gimp-crossroad.sh"
else
export ARTIFACTS_SUFFIX="-x64"

## The required packages for GIMP are taken from the previous job

## Build GIMP
if [ ! -f "_build${ARTIFACTS_SUFFIX}-cross/build.ninja" ]; then
  mkdir _build${ARTIFACTS_SUFFIX}-cross && cd _build${ARTIFACTS_SUFFIX}-cross
  crossroad meson setup .. -Dgi-docgen=disabled                 \
                           -Djavascript=disabled -Dlua=disabled \
                           -Dpython=disabled -Dvala=disabled
else
  cd _build${ARTIFACTS_SUFFIX}-cross
fi
ninja
ninja install
ccache --show-stats
cd ..

## Wrapper just for easier GIMP running (to not look at the huge bin/ folder with many .DLLs)
GIMP_APP_VERSION=$(grep GIMP_APP_VERSION _build${ARTIFACTS_SUFFIX}-cross/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
echo "@echo off
      echo This is a CI crossbuild of GIMP.
      :: Don't run this under PowerShell since it produces UTF-16 files.
      echo .js   (JavaScript) plug-ins ^|^ NOT supported!
      echo .lua  (Lua) plug-ins        ^|^ NOT supported!
      echo .py   (Python) plug-ins     ^|^ NOT supported!
      echo .scm  (ScriptFu) plug-ins   ^|^ NOT supported!
      echo .vala (Vala) plug-ins       ^|^ NOT supported!
      echo.
      bin\gimp-$GIMP_APP_VERSION.exe" > ${CROSSROAD_PREFIX}/gimp.cmd

## Copy built GIMP, babl and GEGL and pre-built packages to GIMP_PREFIX
cp -fr $CROSSROAD_PREFIX/ ${WORK_DIR}_install${ARTIFACTS_SUFFIX}-cross/

fi # END OF CROSSROAD ENV
