#!/bin/sh

set -e

if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
  if [ "$0" != 'build/linux/flatpak/1_build-deps-flatpak.sh' ] && [ ${PWD/*\//} != 'flatpak' ]; then
    echo -e '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/linux/'
    exit 1
  elif [ ${PWD/*\//} = 'flatpak' ]; then
    cd ../../..
  fi
fi


# Install part of the deps
if [ ! -f '/usr/bin/flatpak-builder' ]; then
  echo -e '\033[31m(ERROR)\033[0m: flatpak-builder not found. Please, install it using your package manager.'
  exit 1
fi
builder_version=$(flatpak-builder --version | sed 's/flatpak-builder//' | sed 's/-//' | sed 's/ //' | sed 's/\.//g')
if [ "$builder_version" -lt '143' ]; then
  ## Pre-1.4.3 flatpak-builder fails at Cmake deps, let's prevent this
  echo -e "\033[31m(ERROR)\033[0m: Installed flatpak-builder is too old. Our .json manifest requires at least 1.4.3."
  exit 1
fi

if [ -z "$GITLAB_CI" ]; then
  flatpak update -y
  flatpak remote-add --if-not-exists --user --from gnome-nightly https://nightly.gnome.org/gnome-nightly.flatpakrepo
  flatpak install --user gnome-nightly org.gnome.Platform/$(uname -m)/master org.gnome.Sdk/$(uname -m)/master -y
fi


# Prepare env (only GIMP_PREFIX is needed for flatpak)
if [ -z "$GIMP_PREFIX" ]; then
  export GIMP_PREFIX="$PWD/../_install"
fi


# Build some deps (including babl and GEGL)
if [ -z "$GITLAB_CI" ] && [ "$1" != '--ci' ]; then
  flatpak-builder --force-clean --ccache --state-dir=../.flatpak-builder --keep-build-dirs --stop-at=gimp \
                  "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json 2>&1 | tee flatpak-builder.log

elif [ "$GITLAB_CI" ] || [ "$1" = '--ci' ]; then
  ## (The deps building is too long and no complete output would be collected,
  ## even from GitLab runner messages. So, let's silent and save logs as a file.)
  echo -e "\e[0Ksection_start:`date +%s`:deps_build[collapsed=true]\r\e[0KBuilding dependencies not present in GNOME runtime"
  flatpak-builder --force-clean --user --disable-rofiles-fuse --keep-build-dirs --build-only --stop-at=babl \
                  "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json &> flatpak-builder.log
  echo -e "\e[0Ksection_end:`date +%s`:deps_build\r\e[0K"

  echo -e "\e[0Ksection_start:`date +%s`:babl_build[collapsed=true]\r\e[0KBuilding babl"
  flatpak-builder --force-clean --user --disable-rofiles-fuse --keep-build-dirs --build-only --stop-at=gegl \
                  "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json
  if [ "$GITLAB_CI" ]; then
    tar cf babl-meson-log.tar .flatpak-builder/build/babl-1/_flatpak_build/meson-logs/meson-log.txt
  fi
  echo -e "\e[0Ksection_end:`date +%s`:babl_build\r\e[0K"

  echo -e "\e[0Ksection_start:`date +%s`:gegl_build[collapsed=true]\r\e[0KBuilding gegl"
  flatpak-builder --force-clean --user --disable-rofiles-fuse --keep-build-dirs --build-only --stop-at=gimp \
                  "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json
  if [ "$GITLAB_CI" ]; then
    tar cf gegl-meson-log.tar .flatpak-builder/build/gegl-1/_flatpak_build/meson-logs/meson-log.txt
    echo -e "\e[0Ksection_end:`date +%s`:gegl_build\r\e[0K"

    ## Save built deps for 'gimp-flatpak-x64' job
    tar cf .flatpak-builder.tar .flatpak-builder/
  fi
fi
