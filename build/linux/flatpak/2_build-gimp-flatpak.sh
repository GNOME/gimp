#!/bin/sh

# Flatpak design mandates to build natively
ARCH=$(uname -m)


if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
  if [ "$0" != 'build/linux/flatpak/2_build-gimp-flatpak.sh' ]; then
    echo 'To run this script locally, please do it from to the gimp git folder'
    exit 1
  fi
  git submodule update --init
  flatpak update -y
  export GIMP_PREFIX="$PWD/../_install-$ARCH"


  # Build GIMP only
  if [ ! -f "_build-$ARCH/build.ninja" ]; then
    mkdir -p _build-$ARCH && cd _build-$ARCH
    flatpak-builder --run --ccache "$GIMP_PREFIX" ../build/linux/flatpak/org.gimp.GIMP-nightly.json.in meson setup .. -Dprefix=/app/ -Dlibdir=/app/lib/
    if [ ! -f '.gitignore' ]; then
      echo '*' > .gitignore
    fi
  else
    cd _build-$ARCH
  fi
  flatpak-builder --run --ccache "$GIMP_PREFIX" ../build/linux/flatpak/org.gimp.GIMP-nightly.json.in ninja
  flatpak-builder --run "$GIMP_PREFIX" ../build/linux/flatpak/org.gimp.GIMP-nightly.json.in ninja install


else
  export GIMP_PREFIX="$PWD/_install-$ARCH"

  # Configure manifest (ugly but works on CI)
  flatpak build-init "$GIMP_PREFIX" org.gimp.GIMP org.gnome.Sdk org.gnome.Platform
  flatpak build "$GIMP_PREFIX" meson setup _build-$ARCH || echo "Generated log"
  GIMP_APP_VERSION=$(grep 'Project version' _build-$ARCH/meson-logs/meson-log.txt | head -1 | sed -e 's/^.*[^0-9]\([0-9]*\.[0-9]*\.[0-9]*\).*$/\1/' -e 's/\([0-9]\+\.[0-9]\+\)\..*/\1/')
  #flatpak build "$GIMP_PREFIX" meson setup _build-$ARCH -Dprefix=/app/ -Dlibdir=/app/lib/
  #GIMP_APP_VERSION=$(grep GIMP_APP_VERSION config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
  cp -r build/linux/flatpak/org.gimp.GIMP-nightly.json.in build/linux/flatpak/org.gimp.GIMP-nightly.json
  sed -i "s/@GIMP_APP_VERSION@/$GIMP_APP_VERSION/g" build/linux/flatpak/org.gimp.GIMP-nightly.json


  # GNOME script to customize gimp module in the manifest (not needed)
  #rewrite-flatpak-manifest build/linux/flatpak/org.gimp.GIMP-nightly.json gimp ${CONFIG_OPTS}


  # Clone and build the deps not present in GNOME runtime (and GIMP)
  # (The deps building is too long and no complete output would be collected,
  # even from GitLab runner messages. So, let's silent and save logs as a file.)
  flatpak-builder --force-clean --user --disable-rofiles-fuse --repo=repo ${BRANCH:+--default-branch=$BRANCH} \
                  "$GIMP_PREFIX" build/linux/flatpak/org.gimp.GIMP-nightly.json &> flatpak-builder.log
fi
