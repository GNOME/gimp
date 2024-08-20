#!/bin/bash

set -e

if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
  if [ "$0" != 'build/windows/2_build-gimp-msys2.sh' ] && [ ${PWD/*\//} != 'windows' ]; then
    echo -e '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/windows/'
    exit 1
  elif [ ${PWD/*\//} = 'windows' ]; then
    cd ../..
  fi
  git submodule update --init --force
fi

if [ "$MSYSTEM_CARCH" = "i686" ]; then
  echo -e "\033[33m(WARNING)\033[0m: 32-bit builds will be dropped in a future release. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/10922"
fi


if [ "$GITLAB_CI" ]; then
  # Install the required (pre-built) packages for GIMP again
  # We take code from deps script to better maintenance
  echo "$(cat build/windows/1_build-deps-msys2.sh                 |
          sed -n '/# Beginning of install/,/# End of install/p')" | bash
fi


# Prepare env
## We need to create the condition this ugly way to not break CI
if [ "$GITLAB_CI" ]; then
  export GIMP_PREFIX="$PWD/_install"
elif [ -z "$GITLAB_CI" ] && [ -z "$GIMP_PREFIX" ]; then
  export GIMP_PREFIX="$PWD/../_install"
fi
## Universal variables from .gitlab-ci.yml
IFS=$'\n' VAR_ARRAY=($(cat .gitlab-ci.yml | sed -n '/export PATH=/,/GI_TYPELIB_PATH}\"/p' | sed 's/    - //'))
IFS=$' \t\n'
for VAR in "${VAR_ARRAY[@]}"; do
  eval "$VAR" || continue
done


# Build GIMP
if [ -z "$GITLAB_CI" ] && [ "$1" != "--relocatable" ]; then
  echo "(INFO): GIMP will be built in MSYS2 friendly mode"
  export MESON_OPTIONS='-Drelocatable-bundle=no -Dwindows-installer=false -Dms-store=false'
elif [ "$GITLAB_CI" ] || [ "$1" = '--relocatable' ]; then
  echo "(INFO): GIMP will be built as a relocatable bundle"
  export MESON_OPTIONS='-Drelocatable-bundle=yes -Dwindows-installer=true -Dms-store=true'
fi

if [ ! -f "_build/build.ninja" ]; then
  mkdir -p "_build" && cd "_build"
  echo "$1" > last_mode
  # We disable javascript as we are not able for the time being to add a
  # javascript interpreter with GObject Introspection (GJS/spidermonkey
  # and Seed/Webkit are the 2 contenders so far, but they are not
  # available on MSYS2 and we are told it's very hard to build them).
  # TODO: re-enable javascript plug-ins when we can figure this out.
  meson setup .. -Dprefix="${GIMP_PREFIX}"             \
                 -Dgi-docgen=disabled                  \
                 -Djavascript=disabled                 \
                 -Ddirectx-sdk-dir="${MSYSTEM_PREFIX}" \
                 -Denable-default-bin=enabled          \
                 -Dbuild-id=org.gimp.GIMP_official $MESON_OPTIONS
else
  cd "_build"
  if [[ $(head -1 last_mode) != "$1" ]]; then
    echo "$1" > last_mode
    meson setup .. --reconfigure $MESON_OPTIONS
  fi
fi
ninja
ninja install
ccache --show-stats
cd ..


# Wrapper just for easier GIMP running
make_cmd ()
{
  if [ "$4" == "do_wizardry" ]; then
    interp_lua="(
                echo lua=$2\bin\luajit.exe
                echo luajit=$2\bin\luajit.exe
                echo /usr/bin/lua=$2\bin\luajit.exe
                echo /usr/bin/luajit=$2\bin\luajit.exe
                echo :Lua:E::lua::luajit:
                ) >%cd%\lib\gimp\GIMP_API_VERSION\interpreters\lua.interp"
    interp_pyt="(
                echo python=$2\bin\python.exe
                echo python3=$2\bin\python.exe
                echo /usr/bin/python=$2\bin\python.exe
                echo /usr/bin/python3=$2\bin\python.exe
                echo :Python:E::py::python:
                ) >%cd%\lib\gimp\GIMP_API_VERSION\interpreters\pygimp.interp"
    interp_scm="(
                echo gimp-script-fu-interpreter=%cd%\bin\gimp-script-fu-interpreter-GIMP_API_VERSION.exe
                echo gimp-script-fu-interpreter-GIMP_API_VERSION=%cd%\bin\gimp-script-fu-interpreter-GIMP_API_VERSION.exe
                echo /usr/bin/gimp-script-fu-interpreter=%cd%\bin\gimp-script-fu-interpreter-GIMP_API_VERSION.exe
                echo :ScriptFu:E::scm::gimp-script-fu-interpreter-GIMP_API_VERSION.exe:
                ) >%cd%\lib\gimp\GIMP_API_VERSION\interpreters\gimp-script-fu-interpreter.interp"
    cp_typelib="@if not exist MSYS2_PREFIX\lib\girepository-1.0\babl*.typelib (copy lib\girepository-1.0\babl*.typelib $2\lib\girepository-1.0) > nul
                @if not exist MSYS2_PREFIX\lib\girepository-1.0\gegl*.typelib (copy lib\girepository-1.0\gegl*.typelib $2\lib\girepository-1.0) > nul
                @if not exist MSYS2_PREFIX\lib\girepository-1.0\gimp*.typelib (copy lib\girepository-1.0\gimp*.typelib $2\lib\girepository-1.0) > nul"
      set_path="set PATH=%PATH%;$2\bin"
    dl_typelib="@if exist MSYS2_PREFIX\lib\girepository-1.0\babl*.typelib (if exist lib\girepository-1.0\babl*.typelib (del $2\lib\girepository-1.0\babl*.typelib)) > nul
                @if exist MSYS2_PREFIX\lib\girepository-1.0\gegl*.typelib (if exist lib\girepository-1.0\gegl*.typelib (del $2\lib\girepository-1.0\gegl*.typelib)) > nul
                @if exist MSYS2_PREFIX\lib\girepository-1.0\gimp*.typelib (if exist lib\girepository-1.0\gimp*.typelib (del $2\lib\girepository-1.0\gimp*.typelib)) > nul"
  fi
  echo "@echo off
        echo This is a $1 native build of GIMP$3.
        echo .js   (JavaScript) plug-ins ^|^ NOT supported!
        $interp_lua
        echo .lua  (Lua) plug-ins        ^|^ supported.
        $interp_pyt
        echo .py   (Python) plug-ins     ^|^ supported.
        $interp_scm
        echo .scm  (ScriptFu) plug-ins   ^|^ supported.
        echo .vala (Vala) plug-ins       ^|^ supported.
        echo.
        $cp_typelib
        $set_path
        bin\gimp-GIMP_APP_VERSION.exe
        $dl_typelib" > ${GIMP_PREFIX}/gimp.cmd
  sed -i "s/GIMP_API_VERSION/$(grep GIMP_PKGCONFIG_VERSION _build/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')/g" ${GIMP_PREFIX}/gimp.cmd
  sed -i "s/GIMP_APP_VERSION/$(grep GIMP_APP_VERSION _build/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')/g" ${GIMP_PREFIX}/gimp.cmd
  sed -i -e "s|MSYS2_PREFIX|c:\/msys64${MSYSTEM_PREFIX}|g" -e 's|c:/|c:\\|g;s|msys64/|msys64\\|g' ${GIMP_PREFIX}/gimp.cmd
}

if [ -z "$GITLAB_CI" ] && [ "$1" != "--relocatable" ]; then
  make_cmd local MSYS2_PREFIX " (please run bin/gimp-GIMP_APP_VERSION.exe under $MSYSTEM shell)" do_wizardry
elif [ "$GITLAB_CI" ] || [ "$1" = "--relocatable" ]; then
  make_cmd CI %cd% ""
fi


if [ "$GITLAB_CI" ] || [ "$1" = "--relocatable" ]; then
  # Bundle GIMP
  bash build/windows/2_bundle-gimp-uni_base.sh --authorized
fi
