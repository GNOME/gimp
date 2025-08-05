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


# Install part of the deps (snapcraft)
if [ -z "$GITLAB_CI" ]; then
  #Only snapcraft snap is available locally
  if ! which snapcraft >/dev/null 2>&1; then
    sudo apt update
    sudo apt install snapd
    sudo snap install snapd
    sudo snap install snapcraft --classic
  fi
else
  #Only snapcraft docker image is available in CI
  echo "FROM ghcr.io/canonical/snapcraft:${SNAPCRAFT_CORE_VERSION}" > Dockerfile
  echo "ENTRYPOINT [\"\"]" >> Dockerfile
  echo "RUN apt-get update -y" >> Dockerfile
  echo "RUN apt-get install -y git" >> Dockerfile
  export SNAPCRAFT_CREDENTIALS_PATH="$HOME/.local/share/snapcraft/launchpad-credentials"
  echo "RUN mkdir -p $(dirname $SNAPCRAFT_CREDENTIALS_PATH)" >> Dockerfile
  echo "RUN printf '[1]\n' > $SNAPCRAFT_CREDENTIALS_PATH" >> Dockerfile
  echo "RUN printf 'consumer_key = System-wide:\ Ubuntu (Ubuntu-vai)\n' >> $SNAPCRAFT_CREDENTIALS_PATH" >> Dockerfile && sed 's|\\ Ubuntu| Ubuntu|g' -i Dockerfile
  echo "RUN printf 'consumer_secret = \n' >> $SNAPCRAFT_CREDENTIALS_PATH" >> Dockerfile
  echo "RUN printf 'access_token = %s\n' $LAUNCHPAD_CREDENTIALS_ACCESS_TOKEN >> $SNAPCRAFT_CREDENTIALS_PATH" >> Dockerfile
  echo "RUN printf 'access_secret = %s\n' $LAUNCHPAD_CREDENTIALS_ACCESS_SECRET >> $SNAPCRAFT_CREDENTIALS_PATH" >> Dockerfile
  /kaniko/executor --context $CI_PROJECT_DIR --dockerfile $CI_PROJECT_DIR/Dockerfile --destination $CI_REGISTRY_IMAGE:build-snap-${SNAPCRAFT_CORE_VERSION} --cache=true --cache-ttl=120h --image-fs-extract-retry 1 --verbosity=warn
fi 


# Prepare env
if [ -z "$GITLAB_CI" ]; then
printf "\e[0Ksection_start:`date +%s`:snap_environ[collapsed=true]\r\e[0KPreparing build environment\n"
## portable environment which works on CI (unlike lxd) and on VMs (unlike multipass)
export SNAPCRAFT_BUILD_ENVIRONMENT=host
build_environment_option='--destructive-mode'
## (snapcraft does not allow to freely set the .yaml path, so let's just temporarely copy it)
cp build/linux/snap/snapcraft.yaml .
printf "\e[0Ksection_end:`date +%s`:snap_environ\r\e[0K\n"


# Build babl and gegl
printf "\e[0Ksection_start:`date +%s`:deps_install[collapsed=true]\r\e[0KInstalling dependencies provided by Ubuntu\n"
sudo snapcraft pull $build_environment_option --build-for=${DPKG_ARCH:-$(dpkg --print-architecture)} --verbosity=verbose
printf "\e[0Ksection_end:`date +%s`:deps_install\r\e[0K\n"
  
printf "\e[0Ksection_start:`date +%s`:babl_build[collapsed=true]\r\e[0KBuilding babl\n"
sudo snapcraft stage babl $build_environment_option --build-for=${DPKG_ARCH:-$(dpkg --print-architecture)} --verbosity=verbose
printf "\e[0Ksection_end:`date +%s`:babl_build\r\e[0K\n"
  
printf "\e[0Ksection_start:`date +%s`:gegl_build[collapsed=true]\r\e[0KBuilding gegl\n"
sudo snapcraft stage gegl $build_environment_option --build-for=${DPKG_ARCH:-$(dpkg --print-architecture)} --verbosity=verbose
printf "\e[0Ksection_end:`date +%s`:gegl_build\r\e[0K\n"

rm snapcraft.yaml
fi
