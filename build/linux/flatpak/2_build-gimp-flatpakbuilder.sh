#!/bin/sh

# Ensure the script work properly
case $(readlink /proc/$$/exe) in
  *bash)
    set -o posix
    ;;
esac
set -e
if [ "$0" != 'build/linux/flatpak/2_build-gimp-flatpakbuilder.sh' ] && [ $(basename "$PWD") != 'flatpak' ]; then
  printf '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/linux/\n'
  exit 1
elif [ $(basename "$PWD") = 'flatpak' ]; then
  cd ../../..
fi
if [ -z "$GITLAB_CI" ]; then
  git submodule update --init
fi


# Install part of the deps
eval "$(sed -n '/Install part/,/End of check/p' build/linux/flatpak/1_build-deps-flatpakbuilder.sh)"

if [ "$GITLAB_CI" ]; then
  # Extract deps from previous job
  tar xf _build-$RUNNER.tar.zst
fi


# Prepare env (only GIMP_PREFIX is needed for flatpak)
if [ -z "$GIMP_PREFIX" ]; then
  export GIMP_PREFIX="$PWD/../_install"
fi


# Build GIMP only
printf "\e[0Ksection_start:`date +%s`:gimp_build[collapsed=true]\r\e[0KBuilding GIMP\n"
eval $FLATPAK_BUILDER --force-clean --disable-rofiles-fuse --keep-build-dirs --build-only --disable-download \
                      "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json > gimp-flatpak-builder.log 2>&1 || { cat gimp-flatpak-builder.log; exit 1; }
if [ "$GITLAB_CI" ]; then
  tar cf gimp-meson-log.tar .flatpak-builder/build/gimp-1/_flatpak_build/meson-logs/meson-log.txt
fi
printf "\e[0Ksection_end:`date +%s`:gimp_build\r\e[0K\n"


# Cleanup GIMP_PREFIX (not working) and export it to OSTree repo
# https://github.com/flatpak/flatpak-builder/issues/14
printf "\e[0Ksection_start:`date +%s`:gimp_bundle[collapsed=true]\r\e[0KCreating OSTree repo\n"
eval $FLATPAK_BUILDER --disable-rofiles-fuse --finish-only --repo=repo \
                      "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json
if [ "$GITLAB_CI" ]; then
  tar cf repo-$(uname -m).tar repo/
  
  ## On CI, make the .flatpak prematurely on each runner since build-bundle is not arch neutral
  eval "$(sed -n -e '/APP_ID=/,/BRANCH=/ { s/  //; p }' build/linux/flatpak/3_dist-gimp-flatpakbuilder.sh)"
  eval "$(grep 'build-bundle repo' build/linux/flatpak/3_dist-gimp-flatpakbuilder.sh | sed 's/  //')"
fi
printf "\e[0Ksection_end:`date +%s`:gimp_bundle\r\e[0K\n"
