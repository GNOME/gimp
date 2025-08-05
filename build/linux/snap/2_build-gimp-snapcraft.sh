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
## portable environment which works on CI (unlike lxd) and on VMs (unlike multipass)
export SNAPCRAFT_BUILD_ENVIRONMENT=host
build_environment_option='--destructive-mode'
## (snapcraft does not allow to freely set the .yaml path, so let's just temporarely copy it)
cp build/linux/snap/snapcraft.yaml .
printf "\e[0Ksection_end:`date +%s`:snap_environ\r\e[0K\n"


# Build (babl, gegl and) GIMP
printf "\e[0Ksection_start:`date +%s`:gimp_build[collapsed=true]\r\e[0KBuilding (babl, gegl and) GIMP\n"
if [ -z "$GITLAB_CI" ]; then
  sudo snapcraft stage gimp $build_environment_option --build-for=${DPKG_ARCH:-$(dpkg --print-architecture)} --verbosity=verbose
else
  ## (snapcraft remote-build does not allow building in parts, so we need to build and pack everything in one go)
  snapcraft remote-build --launchpad-accept-public-upload --build-for=$DPKG_ARCH --launchpad-timeout=7200 --verbosity=verbose
fi
printf "\e[0Ksection_end:`date +%s`:gimp_build\r\e[0K\n"

# Bundle
printf "\e[0Ksection_start:`date +%s`:gimp_bundle[collapsed=true]\r\e[0KBundling GIMP\n"
if [ -z "$GITLAB_CI" ]; then
  sudo snapcraft prime $build_environment_option --build-for=${DPKG_ARCH:-$(dpkg --print-architecture)} --verbosity=verbose
else
  # Rename .snap bundle file to avoid confusion (the distribution of the .snap is done only in 3_dist-gimp-snapcraft.sh)
  mv *.snap temp_$(echo *.snap)
fi
printf "\e[0Ksection_end:`date +%s`:gimp_bundle\r\e[0K\n"

rm snapcraft.yaml
