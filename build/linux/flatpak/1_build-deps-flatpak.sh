#!/bin/sh

# Flatpak design mandates to build natively
ARCH=$(uname -m)


if [ -z "$GITLAB_CI" ]; then
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
fi


# Install part of the deps
if [ -z "$GITLAB_CI" ]; then
  flatpak remote-add --if-not-exists --user --from gnome-nightly https://nightly.gnome.org/gnome-nightly.flatpakrepo
  flatpak install --user gnome-nightly org.gnome.Platform/$ARCH/master org.gnome.Sdk/$ARCH/master -y
fi
flatpak remote-add --if-not-exists --user --from flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.freedesktop.Sdk.Extension.llvm17 -y


# Clone and build the deps not present in GNOME runtime
# (Flatpak design mandates to reinstall everything ('--force-clean') on the prefix)
if [ -z "$GITLAB_CI" ]; then
  flatpak-builder --force-clean --ccache --state-dir=../.flatpak-builder --keep-build-dirs --stop-at=gimp \
                  "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json.in 2>&1 | tee flatpak-builder.log
fi
