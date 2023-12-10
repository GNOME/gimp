#!/bin/bash
# $MINGW_PACKAGE_PREFIX is defined by MSYS2.
# https://github.com/msys2/MSYS2-packages/blob/master/filesystem/msystem

set -e

if [[ "$MSYSTEM" == "MINGW32" ]]; then
    export ARTIFACTS_SUFFIX="-w32"
    export MSYS2_ARCH_FOLDER="mingw32"
    export MSYS2_PREFIX="/c/msys64/mingw32"
elif [[ "$MSYSTEM" == "MINGW64" ]]; then
    export ARTIFACTS_SUFFIX="-w64"
    export MSYS2_ARCH_FOLDER="mingw64"
    export MSYS2_PREFIX="/c/msys64/mingw64/"
else # [[ "$MSYSTEM" == "CLANGARM64" ]];
    export ARTIFACTS_SUFFIX="-arm64"
    export MSYS2_ARCH_FOLDER="clangarm64"
    export MSYS2_PREFIX="/c/msys64/clangarm64/"
fi

export PATH="${MSYS2_PREFIX}/bin:$PATH"

# Update everything
pacman --noconfirm -Suy

# Install the required (pre-built) packages for GIMP
export DEPS_PATH="build/windows/gitlab-ci/all-deps-uni.txt"
sed -i "s/DEPS_ARCH_/${MINGW_PACKAGE_PREFIX}-/g" $DEPS_PATH
export GIMP_DEPS=`cat $DEPS_PATH`
pacman --noconfirm -S --needed base-devel $GIMP_DEPS

# XXX We've got a weird error when the prefix is in the current dir.
# Until we figure it out, this trick seems to work, even though it's
# completely ridiculous.
rm -fr ~/_install${ARTIFACTS_SUFFIX}
mv "_install${ARTIFACTS_SUFFIX}" ~

export GIMP_PREFIX="`realpath ~/_install`${ARTIFACTS_SUFFIX}"
export PATH="$GIMP_PREFIX/bin:$PATH"
export PKG_CONFIG_PATH="${GIMP_PREFIX}/lib/pkgconfig:$PKG_CONFIG_PATH"
export PKG_CONFIG_PATH="${GIMP_PREFIX}/share/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="${GIMP_PREFIX}/lib:${LD_LIBRARY_PATH}"
export ACLOCAL_FLAGS="-I/c/msys64/${MSYS2_ARCH_FOLDER}/share/aclocal"
export XDG_DATA_DIRS="${GIMP_PREFIX}/share:/${MSYS2_ARCH_FOLDER}/share/"

mkdir -p _ccache
export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"

# XXX Do not enable ccache this way because it breaks
# gobject-introspection rules. Let's see later for ccache.
# See: https://github.com/msys2/MINGW-packages/issues/9677
#export CC="ccache gcc"

#ccache --zero-stats
#ccache --show-stats

mkdir "_build${ARTIFACTS_SUFFIX}"
cd "_build${ARTIFACTS_SUFFIX}"
# We disable javascript as we are not able for the time being to add a
# javascript interpreter with GObject Introspection (GJS/spidermonkey
# and Seed/Webkit are the 2 contenders so far, but they are not
# available on MSYS2 and we are told it's very hard to build them).
# TODO: re-enable javascript plug-ins when we can figure this out.
meson .. -Dprefix="${GIMP_PREFIX}"           \
         -Ddirectx-sdk-dir="${MSYS2_PREFIX}" \
         -Djavascript=disabled               \
         -Dbuild-id=org.gimp.GIMP_official   \
         -Dgi-docgen=disabled
ninja
ninja install
cd ..

#ccache --show-stats

# XXX Moving back the prefix to be used as artifacts.
mv "${GIMP_PREFIX}" .
