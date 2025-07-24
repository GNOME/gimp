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
  tar xf .flatpak-builder-$RUNNER.tar
fi


# Prepare env (only GIMP_PREFIX is needed for flatpak)
if [ -z "$GIMP_PREFIX" ]; then
  export GIMP_PREFIX="$PWD/../_install"
fi


# Build GIMP only
if [ -z "$GITLAB_CI" ]; then
  BUILDER_ARGS='--ccache --state-dir=../.flatpak-builder'
else
  BUILDER_ARGS='--user --disable-rofiles-fuse'
fi

printf "\e[0Ksection_start:`date +%s`:gimp_build[collapsed=true]\r\e[0KBuilding GIMP\n"
eval $FLATPAK_BUILDER --force-clean $BUILDER_ARGS --keep-build-dirs --build-only --disable-download \
                      "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json > gimp-flatpak-builder.log 2>&1 || { cat gimp-flatpak-builder.log; exit 1; }
if [ "$GITLAB_CI" ]; then
  tar cf gimp-meson-log.tar .flatpak-builder/build/gimp-1/_flatpak_build/meson-logs/meson-log.txt
fi
printf "\e[0Ksection_end:`date +%s`:gimp_build\r\e[0K\n"


# Bundle files for distribution on 3_dist-gimp-flatpakbuilder.sh
printf "\e[0Ksection_start:`date +%s`:gimp_bundle[collapsed=true]\r\e[0KCreating OSTree repo\n"
## Cleanup GIMP_PREFIX (not working) and export it to OSTree repo
## https://github.com/flatpak/flatpak-builder/issues/14
eval $FLATPAK_BUILDER $BUILDER_ARGS --finish-only --repo=repo \
                      "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json
if [ "$GITLAB_CI" ]; then
  tar cf repo-$(uname -m).tar repo/
fi
## Make testable .flatpak "bundle" from OSTree repo
## (it is NOT a real/full bundle, deps from GNOME runtime are not bundled)
APP_ID=$(awk -F'"' '/"app-id"/ {print $4; exit}' build/linux/flatpak/org.gimp.GIMP-nightly.json)
BRANCH=$(awk -F'"' '/"branch"/ {print $4; exit}' build/linux/flatpak/org.gimp.GIMP-nightly.json)
flatpak build-bundle repo temp_${APP_ID}-$(uname -m).flatpak --runtime-repo=https://nightly.gnome.org/gnome-nightly.flatpakrepo ${APP_ID} ${BRANCH}
printf "\e[0Ksection_end:`date +%s`:gimp_bundle\r\e[0K\n"
