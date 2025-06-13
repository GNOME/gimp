#!/bin/sh

case $(readlink /proc/$$/exe) in
  *bash)
    set -o posix
    ;;
esac
set -e

if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
  if [ "$0" != 'build/linux/flatpak/2_build-gimp-flatpak.sh' ] && [ ${PWD/*\//} != 'flatpak' ]; then
    echo -e '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/linux/'
    exit 1
  elif [ $(basename "$PWD") = 'flatpak' ]; then
    cd ../../..
  fi

  git submodule update --init
fi


# Install part of the deps
source <(cat build/linux/flatpak/1_build-deps-flatpak.sh | sed -n "/Install part/,/End of check/p")

if [ "$GITLAB_CI" ]; then
  # Extract deps from previous job
  tar xf .flatpak-builder.tar
fi


# Prepare env (only GIMP_PREFIX is needed for flatpak)
if [ -z "$GIMP_PREFIX" ]; then
  export GIMP_PREFIX="$PWD/../_install"
fi


# Build GIMP only
if [ -z "$GITLAB_CI" ] && [ "$1" != '--ci' ]; then
  if [ ! -f "_build/build.ninja" ]; then
    eval $FLATPAK_BUILDER --run --ccache "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json meson setup _build -Dprefix=/app/ -Dlibdir=/app/lib/
    if [ ! -f '_build/.gitignore' ]; then
      echo '*' > _build/.gitignore
    fi
  fi
  cd _build
  eval $FLATPAK_BUILDER --run --ccache "$GIMP_PREFIX" ../build/linux/flatpak/org.gimp.GIMP-nightly.json ninja
  eval $FLATPAK_BUILDER --run "$GIMP_PREFIX" ../build/linux/flatpak/org.gimp.GIMP-nightly.json ninja install

elif [ "$GITLAB_CI" ] || [ "$1" = '--ci' ]; then
  echo -e "\e[0Ksection_start:`date +%s`:gimp_build[collapsed=true]\r\e[0KBuilding GIMP"
  eval $FLATPAK_BUILDER --force-clean --user --disable-rofiles-fuse --keep-build-dirs --disable-download \
                        "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json &> flatpak-builder.log
  if [ "$GITLAB_CI"  ]; then
    tar cf gimp-meson-log.tar .flatpak-builder/build/gimp-1/_flatpak_build/meson-logs/meson-log.txt
  fi
  echo -e "\e[0Ksection_end:`date +%s`:gimp_build\r\e[0K"

  if [ "$GITLAB_CI"  ]; then
    tar cf repo.tar repo/
  fi
fi
