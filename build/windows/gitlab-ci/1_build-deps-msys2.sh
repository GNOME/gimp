#!/bin/bash
# $MSYSTEM_CARCH, $MINGW_PACKAGE_PREFIX and $MSYSTEM_PREFIX are defined by MSYS2.
# https://github.com/msys2/MSYS2-packages/blob/master/filesystem/msystem

set -e

if [[ "$MSYSTEM_CARCH" == "aarch64" ]]; then
  export ARTIFACTS_SUFFIX="-a64"
elif [[ "$MSYSTEM_CARCH" == "x86_64" ]]; then
  export ARTIFACTS_SUFFIX="-x64"
else # [[ "$MSYSTEM_CARCH" == "i686" ]];
  export ARTIFACTS_SUFFIX="-x86"
fi

if [[ "$BUILD_TYPE" != "CI_NATIVE" ]]; then
  # Make the script callable from every directory
  if [[ "$0" != "build/windows/gitlab-ci/1_build-deps-msys2.sh" ]]; then
    GIMP_EXTDIR="$0"
    GIMP_EXTDIR=$(sed 's|build/windows/gitlab-ci/1_build-deps-msys2.sh||g' <<< $GIMP_EXTDIR)
    GIMP_DIR="$GIMP_EXTDIR"
  else
    GIMP_GITDIR="$(pwd)"
    GIMP_GITDIR=$(sed 's|build/windows/gitlab-ci||g' <<< $GIMP_GITDIR)
    GIMP_GITDIR=$(sed 's|build/windows||g' <<< $GIMP_GITDIR)
    GIMP_GITDIR=$(sed 's|build||g' <<< $GIMP_GITDIR)
    GIMP_DIR="$GIMP_GITDIR"
  fi
  cd $GIMP_DIR

  pacman --noconfirm -Suy
fi


# Install the required (pre-built) packages for babl and GEGL
export DEPS_PATH="build/windows/gitlab-ci/all-deps-uni.txt"
sed -i "s/DEPS_ARCH_/${MINGW_PACKAGE_PREFIX}-/g" $DEPS_PATH
export GIMP_DEPS=`cat $DEPS_PATH`

retry=3
while [ $retry -gt 0 ]; do
  timeout --signal=KILL 3m pacman --noconfirm -S --needed git                                \
                                                          base-devel                         \
                                                          ${MINGW_PACKAGE_PREFIX}-toolchain  \
                                                          $GIMP_DEPS && break
  echo "MSYS2 pacman timed out. Trying again."
  taskkill //t //F //IM "pacman.exe"
  rm -f c:/msys64/var/lib/pacman/db.lck
  : $((--retry))
done

if [ $retry -eq 0 ]; then
  echo "MSYS2 pacman repeatedly failed. See: https://github.com/msys2/MSYS2-packages/issues/4340"
  exit 1
fi


# Clone babl and GEGL (follow master branch)
export GIT_DEPTH=1
export GIMP_PREFIX="`realpath ./_install`${ARTIFACTS_SUFFIX}"
export PATH="$GIMP_PREFIX/bin:$PATH"
export PKG_CONFIG_PATH="${GIMP_PREFIX}/lib/pkgconfig:$PKG_CONFIG_PATH"
export PKG_CONFIG_PATH="${GIMP_PREFIX}/share/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="${GIMP_PREFIX}/lib:${LD_LIBRARY_PATH}"
export XDG_DATA_DIRS="${GIMP_PREFIX}/share:${MSYSTEM_PREFIX}/share/"

clone_or_pull() {
  if [ ! -d "_${1}" ]; then
    git clone --depth=${GIT_DEPTH} https://gitlab.gnome.org/GNOME/${1}.git _${1} || exit 1
  else
    cd _${1} && git pull && cd ..
  fi
}

clone_or_pull babl
clone_or_pull gegl


# Build babl and GEGL
configure_or_build()
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

configure_or_build gegl "-Ddocs=false \
                         -Dcairo=enabled -Dumfpack=enabled \
                         -Dopenexr=enabled -Dworkshop=true"


if [[ "$BUILD_TYPE" != "CI_NATIVE" ]]; then
  mv _babl ~
  mv _gegl ~
  mv "${GIMP_PREFIX}" ~
fi
