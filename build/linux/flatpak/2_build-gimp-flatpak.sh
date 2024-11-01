#!/bin/sh

set -e

if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
  if [ "$0" != 'build/linux/flatpak/2_build-gimp-flatpak.sh' ] && [ ${PWD/*\//} != 'flatpak' ]; then
    echo -e '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/linux/'
    exit 1
  elif [ ${PWD/*\//} = 'flatpak' ]; then
    cd ../../..
  fi

  git submodule update --init
fi


if [ "$GITLAB_CI" ]; then
  # Extract deps from previous job
  echo '(INFO): extracting previously built dependencies'
  tar xf .flatpak-builder.tar
fi


# GNOME script to customize gimp module in the manifest (not needed)
#rewrite-flatpak-manifest build/linux/flatpak/org.gimp.GIMP-nightly.json gimp ${CONFIG_OPTS}


# Prepare env (only GIMP_PREFIX is needed for flatpak)
if [ -z "$GIMP_PREFIX" ]; then
  export GIMP_PREFIX="$PWD/../_install"
fi


# Build GIMP only
if [ -z "$GITLAB_CI" ] && [ "$1" != '--ci' ]; then
  if [ ! -f "_build/build.ninja" ]; then
    flatpak-builder --run --ccache "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json meson setup _build -Dprefix=/app/ -Dlibdir=/app/lib/
    if [ ! -f '_build/.gitignore' ]; then
      echo '*' > _build/.gitignore
    fi
  fi
  cd _build
  flatpak-builder --run --ccache "$GIMP_PREFIX" ../build/linux/flatpak/org.gimp.GIMP-nightly.json ninja
  flatpak-builder --run "$GIMP_PREFIX" ../build/linux/flatpak/org.gimp.GIMP-nightly.json ninja install

elif [ "$GITLAB_CI" ] || [ "$1" = '--ci' ]; then
  flatpak-builder --force-clean --user --disable-rofiles-fuse --keep-build-dirs --build-only \
                  "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json
  if [ "$GITLAB_CI"  ]; then
    tar cf gimp-meson-log.tar .flatpak-builder/build/gimp-1/_flatpak_build/meson-logs/meson-log.txt
  fi

  ## Cleanup GIMP_PREFIX (not working) and export it to OSTree repo
  ## https://github.com/flatpak/flatpak-builder/issues/14
  flatpak-builder --user --disable-rofiles-fuse --finish-only --repo=repo \
                  "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json
  if [ "$GITLAB_CI"  ]; then
    tar cf repo.tar repo/
  fi
fi
