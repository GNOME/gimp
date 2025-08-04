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


# Prepare env
printf "\e[0Ksection_start:`date +%s`:snap_environ[collapsed=true]\r\e[0KPreparing build environment\n"
## Option 1: no container environment (recommended)
export SNAPCRAFT_BUILD_ENVIRONMENT=host
## Option 2: containerized environment by lxd
#sudo snap install lxd
#sudo lxd init --auto
#sudo usermod -a -G lxd $(whoami)
#sudo newgrp lxd
printf "\e[0Ksection_end:`date +%s`:snap_environ\r\e[0K\n"


# Build babl, gegl and GIMP
printf "\e[0Ksection_start:`date +%s`:gimp_build[collapsed=true]\r\e[0KBuilding babl, gegl and GIMP\n"
## (snapcraft does not allow to freely set the .yaml path, so let's just temporarely copy it)
cp build/linux/snap/snapcraft.yaml .
## (snapcraft does not allow building in parts like flatpak-builder, so we need to build everything in one go)
if [ -z "$GITLAB_CI" ]; then
  sudo snapcraft --build-for=$DPKG_ARCH --verbosity=verbose
else
  snapcraft remote-build --launchpad-accept-public-upload --build-for=$DPKG_ARCH --launchpad-timeout=7200 --verbosity=verbose
fi
rm snapcraft.yaml
printf "\e[0Ksection_end:`date +%s`:gimp_build\r\e[0K\n"


# Rename .snap bundle file to avoid confusion (the distribution of the .snap is done only in 3_dist-gimp-snapcraft.sh)
mv *.snap temp_$(echo *.snap)
