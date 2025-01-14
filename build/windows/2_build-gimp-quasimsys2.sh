#!/bin/sh

set -e


# SHELL ENV
if [ -z "$QUASI_MSYS2_ROOT" ]; then

if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
  if [ "$0" != 'build/windows/2_build-gimp-quasimsys2.sh' ] && [ ${PWD/*\//} != 'windows' ]; then
    echo -e '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/windows/'
    exit 1
  elif [ ${PWD/*\//} = 'windows' ]; then
    cd ../..
  fi

  git submodule update --init

  PARENT_DIR='../'
fi


# FIXME: We need native/Linux gimp-console.
# https://gitlab.gnome.org/GNOME/gimp/-/issues/6393
if [ "$1" ]; then
  export BUILD_DIR="$1"
else
  export BUILD_DIR=$(echo _build*$RUNNER)
fi
if [ ! -d "$BUILD_DIR" ]; then
  echo -e "\033[31m(ERROR)\033[0m: Before running this script, first build GIMP natively in $BUILD_DIR"
fi
if [ "$GITLAB_CI" ]; then
  eval "$(grep 'export GIMP_PREFIX' .gitlab-ci.yml | head -1 | sed 's/    - //')" || $true
elif [ -z "$GITLAB_CI" ] && [ -z "$GIMP_PREFIX" ]; then
  export GIMP_PREFIX="$PWD/../_install"
fi
if [ ! -d "$GIMP_PREFIX" ]; then
  echo -e "\033[31m(ERROR)\033[0m: Before running this script, first install GIMP natively in $GIMP_PREFIX"
fi
if [ ! -d "$BUILD_DIR" ] || [ ! -d "$GIMP_PREFIX" ]; then
  echo 'Patches are very welcome: https://gitlab.gnome.org/GNOME/gimp/-/issues/6393'
  exit 1
fi
if [ -z $GITLAB_CI ]; then
  IFS=$'\n' VAR_ARRAY=($(cat .gitlab-ci.yml | sed -n '/export PATH=/,/GI_TYPELIB_PATH}\"/p' | sed 's/    - //'))
  IFS=$' \t\n'
  for VAR in "${VAR_ARRAY[@]}"; do
    eval "$VAR" || continue
  done
fi

if [ "$GITLAB_CI" ]; then
  # Install quasi-msys2 deps again
  # We take code from deps script to better maintenance
  echo "$(cat build/windows/1_build-deps-quasimsys2.sh            |
          sed -n '/# Beginning of install/,/# End of install/p')" | bash
fi

## The required packages for GIMP are taken from the result of previous script
sudo ln -nfs "$PWD/${PARENT_DIR}quasi-msys2/root/$MSYSTEM_PREFIX" /$MSYSTEM_PREFIX


# QUASI-MSYS2 ENV
echo -e "\e[0Ksection_start:`date +%s`:cross_environ[collapsed=true]\r\e[0KPreparing cross-build environment"
bash -c "source ${PARENT_DIR}quasi-msys2/env/all.src && bash build/windows/2_build-gimp-quasimsys2.sh"
else
export GIMP_PREFIX="$PWD/${PARENT_DIR}_install-$(echo $MSYSTEM_PREFIX | sed 's|/||')-cross"
IFS=$'\n' VAR_ARRAY=($(cat .gitlab-ci.yml | sed -n '/export PATH=/,/GI_TYPELIB_PATH}\"/p' | sed 's/    - //'))
IFS=$' \t\n'
for VAR in "${VAR_ARRAY[@]}"; do
  if [[ ! "$VAR" =~ 'multiarch' ]]; then
    unset LIB_SUBDIR
    eval "$VAR" || continue
  fi
done
echo -e "\e[0Ksection_end:`date +%s`:cross_environ\r\e[0K"


## Build GIMP
echo -e "\e[0Ksection_start:`date +%s`:gimp_build[collapsed=true]\r\e[0KBuilding GIMP"
if [ ! -f "_build-$(echo $MSYSTEM_PREFIX | sed 's|/||')-cross/build.ninja" ]; then
  meson setup _build-$(echo $MSYSTEM_PREFIX | sed 's|/||')-cross -Dprefix="$GIMP_PREFIX" -Dgi-docgen=disabled \
                                                                 -Djavascript=disabled -Dvala=disabled -Dms-store=true
fi
cd _build-$(echo $MSYSTEM_PREFIX | sed 's|/||')-cross
ninja
echo -e "\e[0Ksection_end:`date +%s`:gimp_build\r\e[0K"


# Bundle GIMP
echo -e "\e[0Ksection_start:`date +%s`:gimp_bundle[collapsed=true]\r\e[0KCreating bundle"
ninja install &> ninja_install.log | rm ninja_install.log || cat ninja_install.log
cd ..
echo -e "\e[0Ksection_end:`date +%s`:gimp_bundle\r\e[0K"
fi # END OF QUASI-MSYS2 ENV
