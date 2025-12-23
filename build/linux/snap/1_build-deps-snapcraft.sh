#!/bin/sh

# Ensure the script work properly
case $(readlink /proc/$$/exe) in
  *bash)
    set -o posix
    ;;
esac
set -e
if [ "$0" != 'build/linux/snap/1_build-deps-snapcraft.sh' ] && [ $(basename "$PWD") != 'snap' ]; then
  printf '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, call this script from the root of gimp git dir\n'
  exit 1
elif [ $(basename "$PWD") = 'snap' ]; then
  cd ../../..
fi


# Install part of the deps
if ! which snapcraft >/dev/null 2>&1; then
  printf '\033[31m(ERROR)\033[0m: snapcraft not found. Please, install it on Ubuntu with: sudo snap install snapcraft --classic\n'
  exit 1
fi
base_target=$(sed -n 's/^base:[[:space:]]*//p' build/linux/snap/snapcraft.yaml)
base_host=$(grep 'SNAPCRAFT_BASE_VERSION:' .gitlab-ci.yml | sed 's/.*SNAPCRAFT_BASE_VERSION:[[:space:]]*"\([^_"]*\)_\([^"]*\)".*/\2/')
if [ "$base_host" != "$base_target" ] && [ "$GITLAB_CI" ] || [ ! -d /snap/$base_target/ ]; then
  ## Pre-base_target or Post-base_target fails since we build on host (see more on 'snapcraft pull' below), let's prevent this
  printf "\033[31m(ERROR)\033[0m: Installed base snap is too new or too old. Our snapcraft.yaml requires exactly $base_target. On CI, please, change the snapcraft-rocks image in the following .gitlab-ci.yml var: SNAPCRAFT_BASE_VERSION.\n"
  exit 1
fi #End of check

if [ -z "$GITLAB_CI" ] || [ "$1" = '--install-snaps' ]; then
  GNOME_SDK=$(grep '^_SDK_SNAP' $(sudo find $(dirname $(which snapcraft))/.. -name gnome.py | grep -i extensions) | sed -n "s/.*\"$base_target\": *\"\\([^\"]*\\)\".*/\\1/p")
  gnome_runtime=$(echo $GNOME_SDK | sed 's/-sdk//')
  mesa_runtime="mesa-$(echo $GNOME_SDK | sed -n 's/.*-\([0-9]\+\)-sdk/\1/p')"
  for snap in $GNOME_SDK $gnome_runtime $mesa_runtime; do
    if [ -z "$GITLAB_CI" ]; then
      sudo snap refresh
      sudo snap install $snap
    elif [ ! -d /snap/$snap/ ]; then
      #snapd can not be used to install snaps on CI since it is a daemon so we manually "install" them
      curl --progress-bar -L -o ${snap}.snap $(curl --progress-bar -H 'Snap-Device-Series: 16' https://api.snapcraft.io/v2/snaps/info/$snap | jq -r --arg arch "$(dpkg --print-architecture)" '.["channel-map"][] | select(.channel.architecture == $arch and .channel.track == "latest" and .channel.risk == "stable") | .download.url')
      mkdir -p /snap/$snap/current
      unsquashfs -d /snap/$snap/current ${snap}.snap
    fi
  done
  if [ "$1" = '--install-snaps' ]; then exit 0; fi
fi


# Prepare env (snapcraft does not allow to freely set the .yaml path)
cp build/linux/snap/snapcraft.yaml .


# Build babl and GEGL
printf "\e[0Ksection_start:`date +%s`:deps_install[collapsed=true]\r\e[0KInstalling dependencies not present in GNOME runtime snap\n"
## (we build on Ubuntu host env with '--destructive-mode' which works both locally and on CI container (unlike lxd) and VMs (unlike multipass))
sudo snapcraft pull --destructive-mode --build-for=$(dpkg --print-architecture) --verbosity=verbose
printf "\e[0Ksection_end:`date +%s`:deps_install\r\e[0K\n"

printf "\e[0Ksection_start:`date +%s`:babl_build[collapsed=true]\r\e[0KBuilding babl\n"
if [ "$CI_COMMIT_TAG" ] && grep 'grade: devel' build/linux/snap/snapcraft.yaml >/dev/null 2>&1; then
  #needed to avoid unreproducible testing builds. See: https://developer.gimp.org/core/maintainer/release/
  printf '\033[31m(ERROR)\033[0m: non-tagged babl and GEGL source should not be used on CI_COMMIT_TAG pipelines. Please, change grade value to stable on snapcraft.yaml\n'
  exit 1
fi
sudo snapcraft stage babl --destructive-mode --build-for=$(dpkg --print-architecture) --verbosity=verbose
if [ "$GITLAB_CI" ]; then
  tar cf babl-meson-log.tar /root/parts/babl/build/meson-logs/meson-log.txt >/dev/null 2>&1
fi
printf "\e[0Ksection_end:`date +%s`:babl_build\r\e[0K\n"

printf "\e[0Ksection_start:`date +%s`:gegl_build[collapsed=true]\r\e[0KBuilding gegl\n"
sudo snapcraft stage gegl --destructive-mode --build-for=$(dpkg --print-architecture) --verbosity=verbose
if [ "$GITLAB_CI" ]; then
  tar cf gegl-meson-log.tar /root/parts/gegl/build/meson-logs/meson-log.txt >/dev/null 2>&1
  printf "\e[0Ksection_end:`date +%s`:gegl_build\r\e[0K\n"

  ## Save built deps for 'gimp-snap' job
  tar cf _install-$RUNNER.tar /root/stage/ >/dev/null 2>&1
  tar cf _build-$RUNNER.tar /root/parts/ >/dev/null 2>&1
fi

rm snapcraft.yaml
