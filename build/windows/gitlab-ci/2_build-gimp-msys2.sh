#!/bin/bash
# $MSYSTEM_CARCH, $MSYSTEM_PREFIX and $MINGW_PACKAGE_PREFIX are defined by MSYS2.
# https://github.com/msys2/MSYS2-packages/blob/master/filesystem/msystem

set -e

if [[ "$MSYSTEM_CARCH" == "aarch64" ]]; then
    export ARTIFACTS_SUFFIX="-a64"
    export MSYS2_PREFIX="c:/msys64${MSYSTEM_PREFIX}"
elif [[ "$MSYSTEM_CARCH" == "x86_64" ]]; then
    export ARTIFACTS_SUFFIX="-x64"
    export MSYS2_PREFIX="c:/msys64${MSYSTEM_PREFIX}"
else # [[ "$MSYSTEM_CARCH" == "i686" ]];
    export ARTIFACTS_SUFFIX="-x86"
    export MSYS2_PREFIX="c:/msys64${MSYSTEM_PREFIX}"
fi

if [[ "$BUILD_TYPE" == "CI_NATIVE" ]]; then
  # XXX We've got a weird error when the prefix is in the current dir.
  # Until we figure it out, this trick seems to work, even though it's
  # completely ridiculous.
  rm -fr ~/_install${ARTIFACTS_SUFFIX}
  mv "_install${ARTIFACTS_SUFFIX}" ~
else
  # Make the script callable from every directory
  if [[ "$0" != "build/windows/gitlab-ci/2_build-gimp-msys2.sh" ]]; then
    GIMP_EXTDIR="$0"
    GIMP_EXTDIR=$(sed 's|build/windows/gitlab-ci/2_build-gimp-msys2.sh||g' <<< $GIMP_EXTDIR)
    cd $GIMP_EXTDIR
  else
    GIMP_GITDIR="$(pwd)"
    GIMP_GITDIR=$(sed 's|build/windows/gitlab-ci||g' <<< $GIMP_GITDIR)
    GIMP_GITDIR=$(sed 's|build/windows||g' <<< $GIMP_GITDIR)
    GIMP_GITDIR=$(sed 's|build||g' <<< $GIMP_GITDIR)
    cd $GIMP_GITDIR
  fi

  pacman --noconfirm -Suy
fi


# Install the required (pre-built) packages for GIMP
export DEPS_PATH="build/windows/gitlab-ci/all-deps-uni.txt"
sed -i "s/DEPS_ARCH_/${MINGW_PACKAGE_PREFIX}-/g" $DEPS_PATH
export GIMP_DEPS=`cat $DEPS_PATH`

retry=3
while [ $retry -gt 0 ]; do
  timeout --signal=KILL 3m pacman --noconfirm -S --needed base-devel                         \
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

# Install QOI header manually
# mingw32 package of qoi was removed from MSYS2, we have download it by ourselves
wget -O "${MSYS2_PREFIX}/include/qoi.h" https://raw.githubusercontent.com/phoboslab/qoi/master/qoi.h

# Build GIMP
export GIMP_PREFIX="`realpath ~/_install`${ARTIFACTS_SUFFIX}"
export PATH="$GIMP_PREFIX/bin:$PATH"
export PKG_CONFIG_PATH="${GIMP_PREFIX}/lib/pkgconfig:$PKG_CONFIG_PATH"
export PKG_CONFIG_PATH="${GIMP_PREFIX}/share/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="${GIMP_PREFIX}/lib:${LD_LIBRARY_PATH}"
export XDG_DATA_DIRS="${GIMP_PREFIX}/share:${MSYSTEM_PREFIX}/share/"

if [ ! -d "_ccache" ]; then
  mkdir -p _ccache
fi
export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"

# XXX Do not enable ccache this way because it breaks
# gobject-introspection rules. Let's see later for ccache.
# See: https://github.com/msys2/MINGW-packages/issues/9677
#export CC="ccache gcc"

#ccache --zero-stats
#ccache --show-stats

if [ ! -f "_build${ARTIFACTS_SUFFIX}/build.ninja" ]; then
  mkdir -p "_build${ARTIFACTS_SUFFIX}" && cd "_build${ARTIFACTS_SUFFIX}"
  # We disable javascript as we are not able for the time being to add a
  # javascript interpreter with GObject Introspection (GJS/spidermonkey
  # and Seed/Webkit are the 2 contenders so far, but they are not
  # available on MSYS2 and we are told it's very hard to build them).
  # TODO: re-enable javascript plug-ins when we can figure this out.
  meson setup .. -Dprefix="${GIMP_PREFIX}"           \
                 -Dgi-docgen=disabled                \
                 -Djavascript=disabled               \
                 -Ddirectx-sdk-dir="${MSYS2_PREFIX}" \
                 -Dbuild-id=org.gimp.GIMP_official
  ninja
  ninja install
else
  cd "_build${ARTIFACTS_SUFFIX}"
  ninja
  ninja install
fi


# XXX Functional fix to the problem of non-configured interpreters
make_cmd() {
  gimp_app_version=`grep -rI '\<version *:' ../meson.build | head -1 | sed "s/^.*version *: *'\([0-9]\+\.[0-9]\+\).[0-9]*' *,.*$/\1/"`
  echo "@echo off
        echo This is a $1 native build of GIMP.
        :: Don't run this under PowerShell since it produces UTF-16 files.
        echo .js   (JavaScript) plug-ins ^|^ NOT supported!
        (
        echo lua=$2\bin\luajit.exe
        echo luajit=$2\bin\luajit.exe
        echo /usr/bin/lua=$2\bin\luajit.exe
        echo /usr/bin/luajit=$2\bin\luajit.exe
        echo :Lua:E::lua::luajit:
        ) >%cd%\lib\gimp\GIMP_APP_VERSION\interpreters\lua.interp
        echo .lua  (Lua) plug-ins        ^|^ supported.
        (
        echo python=$2\bin\python.exe
        echo python3=$2\bin\python.exe
        echo /usr/bin/python=$2\bin\python.exe
        echo /usr/bin/python3=$2\bin\python.exe
        echo :Python:E::py::python:
        ) >%cd%\lib\gimp\GIMP_APP_VERSION\interpreters\pygimp.interp
        echo .py   (Python) plug-ins     ^|^ supported.
        (
        echo gimp-script-fu-interpreter=%cd%\bin\gimp-script-fu-interpreter-3.0.exe
        echo gimp-script-fu-interpreter-3.0=%cd%\bin\gimp-script-fu-interpreter-3.0.exe
        echo /usr/bin/gimp-script-fu-interpreter=%cd%\bin\gimp-script-fu-interpreter-3.0.exe
        echo :ScriptFu:E::scm::gimp-script-fu-interpreter-3.0.exe:
        ) >%cd%\lib\gimp\GIMP_APP_VERSION\interpreters\gimp-script-fu-interpreter.interp
        echo .scm  (ScriptFu) plug-ins   ^|^ supported.
        echo .vala (Vala) plug-ins       ^|^ supported.
        echo.
        @if not exist $2\lib\girepository-1.0\babl*.typelib (copy lib\girepository-1.0\babl*.typelib $2\lib\girepository-1.0) > nul
        @if not exist $2\lib\girepository-1.0\gegl*.typelib (copy lib\girepository-1.0\gegl*.typelib $2\lib\girepository-1.0) > nul
        @if not exist $2\lib\girepository-1.0\gimp*.typelib (copy lib\girepository-1.0\gimp*.typelib $2\lib\girepository-1.0) > nul
        set PATH=%PATH%;$2\bin
        bin\gimp-$gimp_app_version.exe" > ${GIMP_PREFIX}/gimp.cmd
  sed -i "s/GIMP_APP_VERSION/${gimp_app_version}/g" ${GIMP_PREFIX}/gimp.cmd
  sed -i 's|c:/|c:\\|g;s|msys64/|msys64\\|g' ${GIMP_PREFIX}/gimp.cmd
  echo "Please run the gimp.cmd file to get proper plug-in support."> ${GIMP_PREFIX}/README.txt
}

if [[ "$BUILD_TYPE" == "CI_NATIVE" ]]; then
  make_cmd CI %cd%

  cd ..

  #ccache --show-stats

  # XXX Moving back the prefix to be used as artifacts.
  mv "${GIMP_PREFIX}" .
else
  make_cmd local $MSYS2_PREFIX
fi
