#!/bin/bash

set -e

if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
  if [ "$0" != 'build/windows/1_build-deps-msys2.sh' ] && [ ${PWD/*\//} != 'windows' ]; then
    echo -e '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/windows/'
    exit 1
  elif [ ${PWD/*\//} = 'windows' ]; then
    cd ../..
  fi
  export GIT_DEPTH=1
  GIMP_DIR=$(echo "${PWD##*/}/")
  cd $(dirname $PWD)
fi

if [ "$MSYSTEM_CARCH" = "i686" ]; then
  echo -e "\033[33m(WARNING)\033[0m: 32-bit builds will be dropped in a future release. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/10922"
fi


# Install the required (pre-built) packages for babl, GEGL and GIMP
if [ "$MSYSTEM_CARCH" != "aarch64" ]; then
  # https://gitlab.gnome.org/GNOME/gimp/-/issues/10782
  pacman --noconfirm -Suy
fi
# Beginning of install code block
DEPS_LIST=$(cat ${GIMP_DIR}build/windows/all-deps-uni.txt                |
            sed "s/\${MINGW_PACKAGE_PREFIX}-/${MINGW_PACKAGE_PREFIX}-/g" |
            sed 's/\\//g')

if [ "$MSYSTEM_CARCH" = "aarch64" ]; then
  retry=3
  while [ $retry -gt 0 ]; do
    timeout --signal=KILL 3m pacman --noconfirm -S --needed git                                \
                                                            base-devel                         \
                                                            ${MINGW_PACKAGE_PREFIX}-toolchain  \
                                                            $DEPS_LIST && break
    echo "MSYS2 pacman timed out. Trying again."
    taskkill //t //F //IM "pacman.exe"
    rm -f c:/msys64/var/lib/pacman/db.lck
    : $((--retry))
  done

  if [ $retry -eq 0 ]; then
    echo "MSYS2 pacman repeatedly failed. See: https://github.com/msys2/MSYS2-packages/issues/4340"
    exit 1
  fi
else
  pacman --noconfirm -S --needed git                                \
                                 base-devel                         \
                                 ${MINGW_PACKAGE_PREFIX}-toolchain  \
                                 $DEPS_LIST
fi
# End of install code block


# Prepare env
## We need to create the condition this ugly way to not break CI
if [ "$GITLAB_CI" ]; then
  export GIMP_PREFIX="$PWD/_install"
elif [ -z "$GITLAB_CI" ] && [ -z "$GIMP_PREFIX" ]; then
  export GIMP_PREFIX="$PWD/_install"
fi
## Universal variables from .gitlab-ci.yml
IFS=$'\n' VAR_ARRAY=($(cat ${GIMP_DIR}.gitlab-ci.yml | sed -n '/export PATH=/,/GI_TYPELIB_PATH}\"/p' | sed 's/    - //'))
IFS=$' \t\n'
for VAR in "${VAR_ARRAY[@]}"; do
  eval "$VAR" || continue
done


# Build babl and GEGL
self_build ()
{
  # Clone source only if not already cloned or downloaded
  repo="https://gitlab.gnome.org/GNOME/$1.git"

  if [ "$CI_COMMIT_TAG" != "" ]; then
    # For tagged jobs (i.e. release or test jobs for upcoming releases), use the
    # last tag. Otherwise use the default branch's HEAD.
    tag=$(git ls-remote --tags --exit-code --refs "$repo" | grep -oi "${1}_[0-9]*_[0-9]*_[0-9]*" | sort --version-sort | tail -1)
    git_options="--branch=$tag"
    echo "Using tagged release of $1: $tag"
  fi

  if [ ! -d "$1" ]; then
    git clone $git_options --depth $GIT_DEPTH $repo || exit 1
  else
    cd $1 && git pull && cd ..
  fi

  # Build
  if [ ! -f "$1/_build/build.ninja" ]; then
    mkdir -p $1/_build && cd $1/_build
    meson setup .. -Dprefix="$GIMP_PREFIX" $2
  else
    cd $1/_build
  fi
  ninja
  ninja install
  ccache --show-stats
  cd ../..
}

self_build babl '-Dwith-docs=false'
self_build gegl '-Dworkshop=true'
