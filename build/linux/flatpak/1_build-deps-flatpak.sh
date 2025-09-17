#!/bin/sh

case $(readlink /proc/$$/exe) in
  *bash)
    set -o posix
    ;;
esac
set -e

if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
  if [ "$0" != 'build/linux/flatpak/1_build-deps-flatpak.sh' ] && [ ${PWD/*\//} != 'flatpak' ]; then
    echo -e '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/linux/'
    exit 1
  elif [ $(basename "$PWD") = 'flatpak' ]; then
    cd ../../..
  fi
fi


# Install part of the deps
if [ $(which flatpak-builder 2>/dev/null) ]; then
  export FLATPAK_BUILDER='flatpak-builder'
elif [ -f '/var/lib/flatpak/exports/bin/org.flatpak.Builder' ]; then
  export FLATPAK_BUILDER='flatpak run --system org.flatpak.Builder'
elif [ -f "${XDG_DATA_HOME:-$HOME/.local/share}/flatpak/exports/bin/org.flatpak.Builder" ]; then
  export FLATPAK_BUILDER='flatpak run --user org.flatpak.Builder'
else
  echo -e '\033[31m(ERROR)\033[0m: flatpak-builder not found. Please, install it using your package manager.'
  exit 1
fi
builder_version=$(eval $FLATPAK_BUILDER --version | sed 's/flatpak-builder//' | sed 's/-//' | sed 's/ //' | sed 's/\.//g')
if [ "$builder_version" -lt '143' ]; then
  ## Pre-1.4.3 flatpak-builder fails at Cmake deps, let's prevent this
  echo -e "\033[31m(ERROR)\033[0m: Installed flatpak-builder is too old. Our .json manifest requires at least 1.4.3."
  exit 1
fi #End of check

if [ -z "$GITLAB_CI" ]; then
  flatpak update -y
  flatpak remote-add --user --if-not-exists --from gnome-nightly https://nightly.gnome.org/gnome-nightly.flatpakrepo
  flatpak install --user gnome-nightly org.gnome.Sdk/$(uname -m)/master org.gnome.Platform/$(uname -m)/master -y
fi


# Prepare env (only GIMP_PREFIX is needed for flatpak)
if [ -z "$GIMP_PREFIX" ]; then
  export GIMP_PREFIX="$PWD/../_install"
fi


# Build some deps (including babl and GEGL)
if [ -z "$GITLAB_CI" ] && [ "$1" != '--ci' ]; then
  eval $FLATPAK_BUILDER --force-clean --ccache --state-dir=../.flatpak-builder --keep-build-dirs --stop-at=gimp \
                        "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json 2>&1 | tee flatpak-builder.log

elif [ "$GITLAB_CI" ] || [ "$1" = '--ci' ]; then
  echo -e "\e[0Ksection_start:`date +%s`:deps_build[collapsed=true]\r\e[0KBuilding dependencies not present in GNOME runtime"
  if [ "$CI_PIPELINE_SOURCE" = 'schedule' ]; then
    # Check dependencies versions with flatpak-external-data-checker
    export FLATPAK_SYSTEM_HELPER_ON_SESSION=foo
    flatpak install --user https://dl.flathub.org/repo/appstream/org.flathub.flatpak-external-data-checker.flatpakref -y
    flatpak run --user --filesystem=$CI_PROJECT_DIR org.flathub.flatpak-external-data-checker --check-outdated build/linux/flatpak/org.gimp.GIMP-nightly.json
    echo "(INFO): All dependencies sources are up to date. Building them..."
  fi

  ## (The deps building is too long and no complete output would be collected,
  ## even from GitLab runner messages. So, let's silent and save logs as a file.)
  eval $FLATPAK_BUILDER --force-clean --user --disable-rofiles-fuse --keep-build-dirs --build-only --stop-at=babl \
                        "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json &> flatpak-builder.log
  echo -e "\e[0Ksection_end:`date +%s`:deps_build\r\e[0K"

  echo -e "\e[0Ksection_start:`date +%s`:babl_build[collapsed=true]\r\e[0KBuilding babl"
  eval $FLATPAK_BUILDER --force-clean --user --disable-rofiles-fuse --keep-build-dirs --build-only --stop-at=gegl \
                        "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json
  if [ "$GITLAB_CI" ]; then
    tar cf babl-meson-log.tar .flatpak-builder/build/babl-1/_flatpak_build/meson-logs/meson-log.txt
  fi
  echo -e "\e[0Ksection_end:`date +%s`:babl_build\r\e[0K"

  echo -e "\e[0Ksection_start:`date +%s`:gegl_build[collapsed=true]\r\e[0KBuilding gegl"
  eval $FLATPAK_BUILDER --force-clean --user --disable-rofiles-fuse --keep-build-dirs --build-only --stop-at=gimp \
                        "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json
  if [ "$GITLAB_CI" ]; then
    tar cf gegl-meson-log.tar .flatpak-builder/build/gegl-1/_flatpak_build/meson-logs/meson-log.txt
    echo -e "\e[0Ksection_end:`date +%s`:gegl_build\r\e[0K"

    ## Save built deps for 'gimp-flatpak-x64' job
    tar cf .flatpak-builder.tar .flatpak-builder/
  fi
fi
