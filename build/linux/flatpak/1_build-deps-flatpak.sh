#!/bin/sh

# Flatpak design mandates to build natively
ARCH=$(uname -m)


# Install part of the deps
if [ -z "$GITLAB_CI" ]; then
  flatpak remote-add --if-not-exists --user --from gnome-nightly https://nightly.gnome.org/gnome-nightly.flatpakrepo
  flatpak install --user gnome-nightly org.gnome.Platform/$ARCH/master org.gnome.Sdk/$ARCH/master -y
fi
flatpak remote-add --if-not-exists --user --from flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.freedesktop.Sdk.Extension.llvm18 -y


# Clone and build the deps not present in GNOME runtime
if [ -z "$GITLAB_CI" ] && [ "$1" != '--ci' ]; then
  # Make the script work locally
  if [ "$0" != 'build/linux/flatpak/1_build-deps-flatpak.sh' ]; then
    echo 'To run this script locally, please do it from to the gimp git folder'
    exit 1
  fi
  flatpak update -y
  if [ -z "$GIMP_PREFIX" ]; then
    export GIMP_PREFIX="$PWD/../_install-$ARCH"
  fi
  if [ ! -d "$GIMP_PREFIX" ]; then
    mkdir -p "$GIMP_PREFIX"
  fi

  flatpak-builder --force-clean --ccache --state-dir=../.flatpak-builder --keep-build-dirs --stop-at=gimp \
                  "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json 2>&1 | tee flatpak-builder.log


elif [ "$GITLAB_CI" ] || [ "$1" = '--ci' ]; then
  export GIMP_PREFIX="$PWD/_install-$ARCH"

  # (The deps building is too long and no complete output would be collected,
  # even from GitLab runner messages. So, let's silent and save logs as a file.)
  echo 'Building dependencies not present in GNOME runtime'
  flatpak-builder --force-clean --user --disable-rofiles-fuse --keep-build-dirs --build-only --stop-at=babl \
                  "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json &> flatpak-builder.log

  # Let's output at least babl and GEGL building
  flatpak-builder --force-clean --user --disable-rofiles-fuse --keep-build-dirs --build-only --stop-at=gimp \
                  "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json
  if [ "$1" != '--ci' ]; then
    tar cf babl-meson-log.tar .flatpak-builder/build/babl-1/_flatpak_build/meson-logs/meson-log.txt
    tar cf gegl-meson-log.tar .flatpak-builder/build/gegl-1/_flatpak_build/meson-logs/meson-log.txt

    # Save built deps for 'gimp-flatpak-x64' job
    tar cf .flatpak-builder.tar .flatpak-builder/
  fi
fi
