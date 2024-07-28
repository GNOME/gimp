#!/bin/sh

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
if [ -z "$GITLAB_CI" ]; then
  flatpak update -y
  flatpak remote-add --if-not-exists --user --from gnome-nightly https://nightly.gnome.org/gnome-nightly.flatpakrepo
  flatpak install --user gnome-nightly org.gnome.Platform/$(uname -m)/master org.gnome.Sdk/$(uname -m)/master -y
fi
flatpak remote-add --if-not-exists --user --from flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.freedesktop.Sdk.Extension.llvm18 -y


# Clone and build the deps not present in GNOME runtime
if [ -z "$GITLAB_CI" ] && [ "$1" != '--ci' ]; then
  ## Prepare env (only GIMP_PREFIX is needed for flatpak)
  if [ -z "$GIMP_PREFIX" ]; then
    export GIMP_PREFIX="$PWD/../_install"
  fi
  if [ ! -d "$GIMP_PREFIX" ]; then
    mkdir -p "$GIMP_PREFIX"
  fi

  ## Clone and build deps (including babl and GEGL)
  flatpak-builder --force-clean --ccache --state-dir=../.flatpak-builder --keep-build-dirs --stop-at=gimp \
                  "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json 2>&1 | tee flatpak-builder.log


elif [ "$GITLAB_CI" ] || [ "$1" = '--ci' ]; then
  ## Prepare env (only GIMP_PREFIX is needed for flatpak)
  export GIMP_PREFIX="$PWD/_install"

  ## Clone and build deps (including babl and GEGL)
  ## (The deps building is too long and no complete output would be collected,
  ## even from GitLab runner messages. So, let's silent and save logs as a file.)
  echo '(INFO): building dependencies not present in GNOME runtime (including babl and GEGL)'
  flatpak-builder --force-clean --user --disable-rofiles-fuse --keep-build-dirs --build-only --stop-at=gimp \
                  "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json &> flatpak-builder.log
  if [ "$1" != '--ci' ]; then
    tar cf babl-meson-log.tar .flatpak-builder/build/babl-1/_flatpak_build/meson-logs/meson-log.txt
    tar cf gegl-meson-log.tar .flatpak-builder/build/gegl-1/_flatpak_build/meson-logs/meson-log.txt

    ## Save built deps for 'gimp-flatpak-x64' job
    tar cf .flatpak-builder.tar .flatpak-builder/
  fi
fi
