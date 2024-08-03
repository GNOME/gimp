#!/bin/sh

set -e


# SHELL ENV
if [ -z "$CROSSROAD_PLATFORM" ]; then

if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
   if [ "$0" != 'build/windows/1_build-deps-crossroad.sh' ] && [ ${PWD/*\//} != 'windows' ]; then
    echo -e '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/windows/'
    exit 1
  elif [ ${PWD/*\//} = 'windows' ]; then
    cd ../..
  fi
  export GIT_DEPTH=1
  export GIMP_DIR=$(echo "${PWD##*/}/")
  cd $(dirname $PWD)
fi

## Install crossroad and its deps
# Beginning of install code block
if [ "$GITLAB_CI" ]; then
  apt-get update -y
  # libjxl uses C++ features that require posix threads so we install
  # specifically g++-mingw-w64-x86-64-posix and gcc-mingw-w64-x86-64-posix.
  # Note that we can't install both the -posix and -win32 versions since
  # update gcc-mingw-w64 (25).
  apt-get install -y --no-install-recommends    \
                     git                        \
                     ccache                     \
                     cpio                       \
                     g++-mingw-w64-x86-64-posix \
                     gcc-mingw-w64-x86-64-posix \
                     meson                      \
                     pkg-config                 \
	                   python3-distutils          \
                     python3-docutils           \
                     python3-zstandard          \
                     rpm                        \
                     wine                       \
                     wine64
fi
# End of install code block
if [ -d 'crossroad' ]; then
  rm -r -f crossroad
fi
git clone --depth $GIT_DEPTH https://gitlab.freedesktop.org/crossroad/crossroad
cd crossroad
git apply ../${GIMP_DIR}build/windows/patches/0001-platforms-Enable-ccache.patch
# Needed because Debian adds by default a local/ folder to the install
# prefix of setup.py. This environment variable overrides this behavior.
export DEB_PYTHON_INSTALL_LAYOUT="deb"
./setup.py install --prefix=`pwd`/../.local
cd ..


# CROSSROAD ENV
export PATH="$PWD/.local/bin:$PATH"
export XDG_DATA_HOME="$PWD/.local/share"
crossroad w64 gimp --run="${GIMP_DIR}build/windows/1_build-deps-crossroad.sh"
else

## Install the required (pre-built) packages for babl, GEGL and GIMP
crossroad source msys2
crossroad install $(cat ${GIMP_DIR}build/windows/all-deps-uni.txt |
                    sed "s/\${MINGW_PACKAGE_PREFIX}-//g" | sed 's/\\//g')
if [ $? -ne 0 ]; then
  echo "Installation of pre-built dependencies failed.";
  exit 1;
fi

## Prepare env (no env var is needed, all are auto set to CROSSROAD_PREFIX)
export ARTIFACTS_SUFFIX="-cross"

## Build babl and GEGL
self_build ()
{
  # Clone source only if not already cloned or downloaded
  if [ ! -d "$1" ]; then
    git clone --depth $GIT_DEPTH https://gitlab.gnome.org/gnome/$1
  else
    cd $1 && git pull && cd ..
  fi

  if [ ! -f "$1/_build$ARTIFACTS_SUFFIX/build.ninja" ]; then
    mkdir -p $1/_build$ARTIFACTS_SUFFIX && cd $1/_build$ARTIFACTS_SUFFIX
    crossroad meson setup .. $2
  else
    cd $1/_build$ARTIFACTS_SUFFIX
  fi
  ninja
  ninja install
  ccache --show-stats
  cd ../..
}

self_build babl '-Denable-gir=false'
self_build gegl '-Dintrospection=false'

## FIXME: Build manually gio 'giomodule.cache' to fix error about
## absent libgiognutls.dll that prevents generating loaders.cache
echo 'libgiognomeproxy.dll: gio-proxy-resolver
libgiognutls.dll: gio-tls-backend
libgiolibproxy.dll: gio-proxy-resolver
libgioopenssl.dll: gio-tls-backend' > $CROSSROAD_PREFIX/lib/gio/modules/giomodule.cache

## FIXME: Build manually pixbuf 'loaders.cache' for GUI image support
echo '"lib\\gdk-pixbuf-2.0\\2.10.0\\loaders\\libpixbufloader-png.dll"
      "png" 5 "gdk-pixbuf" "PNG" "LGPL"
      "image/png" ""
      "png" ""
      "\211PNG\r\n\032\n" "" 100

      "lib\\gdk-pixbuf-2.0\\2.10.0\\loaders\\libpixbufloader-svg.dll"
      "svg" 6 "gdk-pixbuf" "Scalable Vector Graphics" "LGPL"
      "image/svg+xml" "image/svg" "image/svg-xml" "image/vnd.adobe.svg+xml" "text/xml-svg" "image/svg+xml-compressed" ""
      "svg" "svgz" "svg.gz" ""
      " <svg" "*    " 100
      " <!DOCTYPE svg" "*             " 100

      ' > $(echo $CROSSROAD_PREFIX/lib/gdk-pixbuf-*/*/)/loaders.cache

## FIXME: Build manually glib 'gschemas.compiled'
GLIB_PATH=$(echo $CROSSROAD_PREFIX/share/glib-*/schemas/)
wine $CROSSROAD_PREFIX/bin/glib-compile-schemas.exe --targetdir=$GLIB_PATH $GLIB_PATH >/dev/null 2>&1

fi # END OF CROSSROAD ENV
