#!/bin/sh

set -e


# SHELL ENV
if [ -z "$CROSSROAD_PLATFORM" ]; then

if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
  if [ "$0" != "build/windows/gitlab-ci/2_build-gimp-crossroad.sh" ]; then
    echo "To run this script locally, please do it from to the gimp git folder"
    exit 1
  fi
  git submodule update --init
  PARENT_DIR='../'
fi

# So that we can use gimp-console from gimp-debian-x64 project.
GIMP_APP_VERSION=$(grep GIMP_APP_VERSION _build-x64/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
mkdir -p $PWD/${PARENT_DIR}.local/bin
GIMP_CONSOLE_PATH=$PWD/${PARENT_DIR}.local/bin/gimp-console-$GIMP_APP_VERSION
gcc -print-multi-os-directory 2>/dev/null | grep ./ && LIB_DIR=$(gcc -print-multi-os-directory | sed 's/\.\.\///g') || LIB_DIR="lib"
gcc -print-multiarch 2>/dev/null | grep . && LIB_SUBDIR=$(echo $(gcc -print-multiarch)'/')
echo "#!/bin/sh" > $GIMP_CONSOLE_PATH
echo export LD_LIBRARY_PATH="$PWD/${PARENT_DIR}_install-x64/${LIB_DIR}/${LIB_SUBDIR}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" >> $GIMP_CONSOLE_PATH
echo export GI_TYPELIB_PATH="$PWD/${PARENT_DIR}_install-x64/${LIB_DIR}/${LIB_SUBDIR}girepository-1.0${GI_TYPELIB_PATH:+:$GI_TYPELIB_PATH}" >> $GIMP_CONSOLE_PATH
echo "$PWD/${PARENT_DIR}_install-x64/bin/gimp-console-$GIMP_APP_VERSION \"\$@\"" >> $GIMP_CONSOLE_PATH
chmod u+x $GIMP_CONSOLE_PATH


# CROSSROAD ENV
export PATH="$PWD/${PARENT_DIR}.local/bin:$PWD/bin:$PATH"
export XDG_DATA_HOME="$PWD/${PARENT_DIR}.local/share"
crossroad w64 gimp --run="build/windows/gitlab-ci/2_build-gimp-crossroad.sh"
else
export ARTIFACTS_SUFFIX="-x64-cross"

## The required packages for GIMP are taken from the previous job

## Build GIMP
if [ ! -f "_build$ARTIFACTS_SUFFIX/build.ninja" ]; then
  mkdir -p _build$ARTIFACTS_SUFFIX && cd _build$ARTIFACTS_SUFFIX
  crossroad meson setup .. -Dgi-docgen=disabled                 \
                           -Djavascript=disabled -Dlua=disabled \
                           -Dpython=disabled -Dvala=disabled
else
  cd _build$ARTIFACTS_SUFFIX
fi
ninja
ninja install
ccache --show-stats
cd ..

## Wrapper just for easier GIMP running (to not look at the huge bin/ folder with many .DLLs)
GIMP_APP_VERSION=$(grep GIMP_APP_VERSION _build$ARTIFACTS_SUFFIX/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
echo "@echo off
      echo This is a CI crossbuild of GIMP.
      echo .js   (JavaScript) plug-ins ^|^ NOT supported!
      echo .lua  (Lua) plug-ins        ^|^ NOT supported!
      echo .py   (Python) plug-ins     ^|^ NOT supported!
      echo .scm  (ScriptFu) plug-ins   ^|^ NOT supported!
      echo .vala (Vala) plug-ins       ^|^ NOT supported!
      echo.
      bin\gimp-$GIMP_APP_VERSION.exe" > ${CROSSROAD_PREFIX}/gimp.cmd

## Copy built GIMP, babl and GEGL and pre-built packages to GIMP_PREFIX
if [ "$GITLAB_CI" ]; then
  cp -fr $CROSSROAD_PREFIX/ _install$ARTIFACTS_SUFFIX
fi

fi # END OF CROSSROAD ENV
