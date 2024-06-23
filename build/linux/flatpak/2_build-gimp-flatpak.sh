#!/bin/sh

# Flatpak design mandates to build natively
ARCH=$(uname -m)


if [ -z "$GITLAB_CI" ] && [ "$1" != '--ci' ]; then
  # Make the script work locally
  if [ "$0" != 'build/linux/flatpak/2_build-gimp-flatpak.sh' ]; then
    echo 'To run this script locally, please do it from to the gimp git folder'
    exit 1
  fi
  git submodule update --init
  flatpak update -y
  if [ -z "$GIMP_PREFIX" ]; then
    export GIMP_PREFIX="$PWD/../_install-$ARCH"
  fi


  # Build GIMP only
  if [ ! -f "_build-$ARCH/build.ninja" ]; then
    mkdir -p _build-$ARCH && cd _build-$ARCH
    flatpak-builder --run --ccache "$GIMP_PREFIX" ../build/linux/flatpak/org.gimp.GIMP-nightly.json meson setup .. -Dprefix=/app/ -Dlibdir=/app/lib/
    if [ ! -f '.gitignore' ]; then
      echo '*' > .gitignore
    fi
  else
    cd _build-$ARCH
  fi
  flatpak-builder --run --ccache "$GIMP_PREFIX" ../build/linux/flatpak/org.gimp.GIMP-nightly.json ninja
  flatpak-builder --run "$GIMP_PREFIX" ../build/linux/flatpak/org.gimp.GIMP-nightly.json ninja install


elif [ "$GITLAB_CI" ] || [ "$1" = '--ci' ]; then
  export GIMP_PREFIX="$PWD/_install-$ARCH"


  # GNOME script to customize gimp module in the manifest (not needed)
  #rewrite-flatpak-manifest build/linux/flatpak/org.gimp.GIMP-nightly.json gimp ${CONFIG_OPTS}


  # Clone and build the deps not present in GNOME runtime (and GIMP)
  # (The deps building is too long and no complete output would be collected,
  # even from GitLab runner messages. So, let's silent and save logs as a file.)
  flatpak-builder --force-clean --user --disable-rofiles-fuse --repo=repo ${BRANCH:+--default-branch=$BRANCH} \
                  "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json &> flatpak-builder.log
fi
