#!/bin/sh

set -e


# SHELL ENV
if [ -z "$QUASI_MSYS2_ROOT" ]; then

if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
  if [ "$0" != 'build/windows/1_build-deps-quasi.sh' ] && [ ${PWD/*\//} != 'windows' ]; then
    echo -e '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/windows/'
    exit 1
  elif [ ${PWD/*\//} = 'windows' ]; then
    cd ../..
  fi

  export GIT_DEPTH=1

  cd $(dirname $PWD)
fi

export GIMP_DIR=$(echo "$PWD/")

## Install crossroad and its deps
# Beginning of install code block
if [ "$GITLAB_CI" ]; then
  apt-get update -y
  apt-get install -y --no-install-recommends \
                     git                     \
                     ccache                  \
                     meson                   \
                     clang                   \
                     make                    \
                     wget                    \
                     tar                     \
                     zstd                    \
                     gawk                    \
                     gpg                     \
                     wine                    \
                     wine64
fi
# End of install code block
if [ ! -d 'quasi-msys2' ]; then
  git clone --depth $GIT_DEPTH https://github.com/HolyBlackCat/quasi-msys2
fi
cd quasi-msys2
git pull

## Install the required (pre-built) packages for babl, GEGL and GIMP
echo CLANG64 > msystem.txt
#deps=$(cat ${GIMP_DIR}build/windows/all-deps-uni.txt |
#       sed "s/\${MINGW_PACKAGE_PREFIX}-/_/g"         | sed 's/\\//g')
make install _clang _lcms2 _glib2 _json-glib _libjpeg-turbo _libpng || $true


# QUASI-MSYS2 ENV
bash -c "source env/all.src && bash ${GIMP_DIR}build/windows/1_build-deps-quasi.sh"
else

## Prepare env (no env var is needed?)
export GIMP_PREFIX="$PWD/../_install-cross"

## Build babl and GEGL
self_build ()
{
  # Clone source only if not already cloned or downloaded
  if [ ! -d "$1" ]; then
    git clone --depth $GIT_DEPTH https://gitlab.gnome.org/gnome/$1
  fi
  cd $1
  git pull

  if [ ! -f "_build-cross/build.ninja" ]; then
    meson setup _build-cross -Dprefix="$GIMP_PREFIX" $2
  fi
  cd _build-cross
  ninja
  ninja install
  ccache --show-stats
  cd ../..
}

self_build babl '-Denable-gir=false'
self_build gegl '-Dintrospection=false'
fi # END OF QUASI-MSYS2 ENV
