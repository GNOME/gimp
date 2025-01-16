#!/bin/sh

set -e


# SHELL ENV
if [ -z "$QUASI_MSYS2_ROOT" ]; then

if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
  if [ "$0" != 'build/windows/1_build-deps-quasimsys2.sh' ] && [ ${PWD/*\//} != 'windows' ]; then
    echo -e '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/windows/'
    exit 1
  elif [ ${PWD/*\//} = 'windows' ]; then
    cd ../..
  fi

  export GIT_DEPTH=1

  export GIMP_DIR=$(echo "$PWD/")
  cd $(dirname $PWD)
fi


## Install quasi-msys2 and its deps
# Beginning of install code block
if [ "$GITLAB_CI" ]; then
  apt-get update -y >/dev/null
  apt-get install -y --no-install-recommends \
                     clang                   \
                     lld                     \
                     llvm >/dev/null
  apt-get install -y --no-install-recommends \
                     gawk                    \
                     gpg                     \
                     gpgv                    \
                     make                    \
                     sudo                    \
                     tar                     \
                     wget                    \
                     zstd >/dev/null
fi
# End of install code block
if [ ! -d 'quasi-msys2' ]; then
  git clone --depth $GIT_DEPTH https://github.com/HolyBlackCat/quasi-msys2
fi
cd quasi-msys2
git pull
cd ..

## Install the required (pre-built) packages for babl, GEGL and GIMP
echo -e "\e[0Ksection_start:`date +%s`:deps_install[collapsed=true]\r\e[0KInstalling dependencies provided by MSYS2"
echo ${MSYSTEM_PREFIX^^} > quasi-msys2/msystem.txt
deps=$(cat ${GIMP_DIR}build/windows/all-deps-uni.txt | sed 's/toolchain/clang/g' |
       sed "s/\${MINGW_PACKAGE_PREFIX}-/_/g"         | sed 's/\\//g')
cd quasi-msys2
make install _clang $deps || $true
cd ..
sudo ln -nfs "$PWD/quasi-msys2/root/$MSYSTEM_PREFIX" /$MSYSTEM_PREFIX

## Manually build gio 'giomodule.cache' to fix fatal error about
## absent libgiognutls.dll that prevents generating loaders.cache
echo 'libgiognomeproxy.dll: gio-proxy-resolver
libgiognutls.dll: gio-tls-backend
libgiolibproxy.dll: gio-proxy-resolver
libgioopenssl.dll: gio-tls-backend' > /$MSYSTEM_PREFIX/lib/gio/modules/giomodule.cache

## Manually build pixbuf 'loaders.cache'
## (the reason for pixbuf is on '2_bundle-gimp-uni_base.sh' script)
echo '"lib\\gdk-pixbuf-2.0\\2.10.0\\loaders\\libpixbufloader-png.dll"
      "png" 5 "gdk-pixbuf" "PNG" "LGPL"
      "image/png" ""
      "png" ""
      "\211PNG\r\n\032\n" "" 100

      "lib\\gdk-pixbuf-2.0\\2.10.0\\loaders\\pixbufloader_svg.dll"
      "svg" 6 "gdk-pixbuf" "Scalable Vector Graphics" "LGPL"
      "image/svg+xml" "image/svg" "image/svg-xml" "image/vnd.adobe.svg+xml" "text/xml-svg" "image/svg+xml-compressed" ""
      "svg" "svgz" "svg.gz" ""
      " <svg" "*    " 100
      " <!DOCTYPE svg" "*             " 100

      ' > $(echo /$MSYSTEM_PREFIX/lib/gdk-pixbuf-*/*/)/loaders.cache

## Manually build glib 'gschemas.compiled'
## (the reason for glib schemas is on '2_bundle-gimp-uni_base.sh' script)
GLIB_PATH=$(echo /$MSYSTEM_PREFIX/share/glib-*/schemas/)
glib-compile-schemas --targetdir=$GLIB_PATH $GLIB_PATH >/dev/null 2>&1
echo -e "\e[0Ksection_end:`date +%s`:deps_install\r\e[0K"


# QUASI-MSYS2 ENV
echo -e "\e[0Ksection_start:`date +%s`:cross_environ[collapsed=true]\r\e[0KPreparing cross-build environment"
bash -c "source quasi-msys2/env/all.src && bash ${GIMP_DIR}build/windows/1_build-deps-quasimsys2.sh"
else
export GIMP_PREFIX="$PWD/_install-$(echo $MSYSTEM_PREFIX | sed 's|/||')-cross"
IFS=$'\n' VAR_ARRAY=($(cat ${GIMP_DIR}.gitlab-ci.yml | sed -n '/export PATH=/,/GI_TYPELIB_PATH}\"/p' | sed 's/    - //'))
IFS=$' \t\n'
for VAR in "${VAR_ARRAY[@]}"; do
  if [[ ! "$VAR" =~ 'multiarch' ]]; then
    unset LIB_SUBDIR
    eval "$VAR" || continue
  fi
done
echo -e "\e[0Ksection_end:`date +%s`:cross_environ\r\e[0K"


## Build babl and GEGL
self_build ()
{
  echo -e "\e[0Ksection_start:`date +%s`:${1}_build[collapsed=true]\r\e[0KBuilding $1"

  # Clone source only if not already cloned or downloaded
  if [ ! -d "$1" ]; then
    git clone --depth $GIT_DEPTH https://gitlab.gnome.org/gnome/$1
  fi
  cd $1
  git pull

  if [ ! -f "_build-$(echo $MSYSTEM_PREFIX | sed 's|/||')-cross/build.ninja" ]; then
    meson setup _build-$(echo $MSYSTEM_PREFIX | sed 's|/||')-cross -Dprefix="$GIMP_PREFIX" $2
  fi
  cd _build-$(echo $MSYSTEM_PREFIX | sed 's|/||')-cross
  ninja
  ninja install
  cd ../..
  echo -e "\e[0Ksection_end:`date +%s`:${1}_build\r\e[0K"
}

self_build babl '-Denable-gir=false'
self_build gegl '-Dintrospection=false' '-Dworkshop=false'
fi # END OF QUASI-MSYS2 ENV
