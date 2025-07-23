#!/bin/sh

case $(readlink /proc/$$/exe) in
  *bash)
    set -o posix
    ;;
esac
set -e

if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
  if [ "$0" != 'build/linux/snap/1_build-deps-snapcraft.sh' ] && [ $(basename "$PWD") != 'snap' ]; then
    printf '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, call this script from the root of gimp git dir\n'
    exit 1
  elif [ $(basename "$PWD") = 'snap' ]; then
    cd ../../..
  fi
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
