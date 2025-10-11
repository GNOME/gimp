#!/bin/sh

# Ensure the script work properly
case $(readlink /proc/$$/exe) in
  *bash)
    set -o posix
    ;;
esac
set -e
if [ "$0" != 'build/linux/snap/2_build-gimp-snapcraft.sh' ] && [ $(basename "$PWD") != 'snap' ]; then
  printf '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, call this script from the root of gimp git dir\n'
  exit 1
elif [ $(basename "$PWD") = 'snap' ]; then
  cd ../../..
fi
if [ -z "$GITLAB_CI" ]; then
  git submodule update --init
fi


# Install part of the deps
eval "$(sed -n '/Install part/,/End of check/p' build/linux/snap/1_build-deps-snapcraft.sh)"

if [ "$GITLAB_CI" ]; then
  # Extract deps from previous job
  tar xf _install-$RUNNER.tar -C /
  tar xf _build-$RUNNER.tar -C /
fi


# Prepare env (snapcraft does not allow to freely set the .yaml path)
cp build/linux/snap/snapcraft.yaml .


# Build GIMP
printf "\e[0Ksection_start:`date +%s`:gimp_build[collapsed=true]\r\e[0KBuilding GIMP\n"
sudo snapcraft stage gimp --destructive-mode --build-for=$(dpkg --print-architecture) --verbosity=verbose > gimp-snapcraft.log 2>&1 || { cat gimp-snapcraft.log; exit 1; }
if [ "$GITLAB_CI" ]; then
  tar cf gimp-meson-log.tar /root/parts/gimp/build/meson-logs/meson-log.txt >/dev/null 2>&1
fi
printf "\e[0Ksection_end:`date +%s`:gimp_build\r\e[0K\n"


# Bundle
printf "\e[0Ksection_start:`date +%s`:gimp_bundle[collapsed=true]\r\e[0KCreating prime dir\n"
sudo mkdir /tmp/craft-state
sudo snapcraft prime --destructive-mode --build-for=$(dpkg --print-architecture) --verbosity=trace >> gimp-snapcraft.log 2>&1 || { cat gimp-snapcraft.log; exit 1; }

if [ "$GITLAB_CI" ]; then
  ## On CI, make the .snap prematurely on each runner since snapcraft does not allow multiple prime/ directories
  eval "$(sed -n -e '/NAME=/,/TRACK=/ { s/  //; p }' build/linux/snap/3_dist-gimp-snapcraft.sh)"
  eval "$(grep 'snapcraft pack' build/linux/snap/3_dist-gimp-snapcraft.sh | sed 's/  //')"
fi
printf "\e[0Ksection_end:`date +%s`:gimp_bundle\r\e[0K\n"

rm snapcraft.yaml
