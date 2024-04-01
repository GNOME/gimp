#!/bin/bash

set -e

# $MSYSTEM_CARCH, $MSYSTEM_PREFIX and $MINGW_PACKAGE_PREFIX are defined by MSYS2.
# https://github.com/msys2/MSYS2-packages/blob/master/filesystem/msystem
if [[ "$MSYSTEM_CARCH" == "aarch64" ]]; then
  export ARTIFACTS_SUFFIX="-a64"
elif [[ "$MSYSTEM_CARCH" == "x86_64" ]]; then
  export ARTIFACTS_SUFFIX="-x64"
else # [[ "$MSYSTEM_CARCH" == "i686" ]];
  export ARTIFACTS_SUFFIX="-x86"
fi

if [[ "$BUILD_TYPE" == "CI_NATIVE" ]]; then
  GIMP_DIR=""
else
  # Make the script work locally
  if [[ "$0" != "build/windows/gitlab-ci/1_build-deps-msys2.sh" ]]; then
    echo "To run this script locally, please do it from to the gimp git folder"
    exit 1
  else
    GIMP_DIR=$(echo "${PWD##*/}/")
    DEPS_DIR=$(dirname $PWD)
    cd $DEPS_DIR
  fi

  pacman --noconfirm -Suy
fi


# Install the required (pre-built) packages for babl and GEGL
DEPS_LIST=$(cat ${GIMP_DIR}build/windows/gitlab-ci/all-deps-uni.txt)
DEPS_LIST=$(sed "s/\${MINGW_PACKAGE_PREFIX}-/${MINGW_PACKAGE_PREFIX}-/g" <<< $DEPS_LIST)
DEPS_LIST=$(sed 's/\\//g' <<< $DEPS_LIST)

if [[ "$MSYSTEM_CARCH" == "aarch64" ]]; then
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
# End of install


# Clone babl and GEGL (follow master branch)
export GIT_DEPTH=1

clone_or_pull ()
{
  repo="https://gitlab.gnome.org/GNOME/${1}.git"

  if [ "$CI_COMMIT_TAG" != "" ]; then
    # For tagged jobs (i.e. release or test jobs for upcoming releases), use the
    # last tag. Otherwise use the default branch's HEAD.
    tag=$(git ls-remote --tags --exit-code --refs "$repo" | grep -oi "$1_[0-9]*_[0-9]*_[0-9]*" | sort --version-sort | tail -1)
    git_options="--branch=$tag"
    echo "Using tagged release of $1: $tag"
  fi

  if [ ! -d "_${1}" ]; then
    git clone $git_options --depth=${GIT_DEPTH} $repo _${1} || exit 1
  else
    cd _${1} && git pull && cd ..
  fi
}

clone_or_pull babl
clone_or_pull gegl


# Build babl and GEGL
export GIMP_PREFIX="`realpath ./_install`${ARTIFACTS_SUFFIX}"
# Universal variables
export PATH="$GIMP_PREFIX/bin:$PATH"
gcc -print-multi-os-directory | grep . && LIB_DIR=$(gcc -print-multi-os-directory | sed 's/\.\.\///g') || LIB_DIR="lib"
gcc -print-multiarch | grep . && LIB_SUBDIR=$(echo $(gcc -print-multiarch)'/')
export PKG_CONFIG_PATH="${GIMP_PREFIX}/${LIB_DIR}/${LIB_SUBDIR}pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export LD_LIBRARY_PATH="${GIMP_PREFIX}/${LIB_DIR}/${LIB_SUBDIR}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export XDG_DATA_DIRS="${GIMP_PREFIX}/share:/usr/share${XDG_DATA_DIRS:+:$XDG_DATA_DIRS}"
export GI_TYPELIB_PATH="${GIMP_PREFIX}/${LIB_DIR}/${LIB_SUBDIR}girepository-1.0${GI_TYPELIB_PATH:+:$GI_TYPELIB_PATH}"
# End of universal variables

configure_or_build ()
{
  if [ ! -f "_${1}/_build/build.ninja" ]; then
    mkdir -p _${1}/_build${ARTIFACTS_SUFFIX} && cd _${1}/_build${ARTIFACTS_SUFFIX}
    (meson setup .. -Dprefix="${GIMP_PREFIX}" $2 && \
     ninja && ninja install) || exit 1
    cd ../..
  else
    cd _${1}/_build${ARTIFACTS_SUFFIX}
    (ninja && ninja install) || exit 1
    cd ../..
  fi
}

configure_or_build babl "-Dwith-docs=false"

configure_or_build gegl "-Ddocs=false -Dworkshop=true"
