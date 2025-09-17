#!/bin/sh

# Ensure the script work properly
case $(readlink /proc/$$/exe) in
  *bash)
    set -o posix
    ;;
esac
set -e
if [ "$0" != 'build/linux/snap/3_dist-gimp-snapcraft.sh' ] && [ $(basename "$PWD") != 'snap' ]; then
  printf '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, call this script from the root of gimp git dir\n'
  exit 1
elif [ $(basename "$PWD") = 'snap' ]; then
  cd ../../..
fi


# Get snapcraft version
printf "\e[0Ksection_start:`date +%s`:snap_tlkt\r\e[0KChecking snap tools\n"
printf "(INFO): snapcraft version: $(snapcraft --version | sed 's|snapcraft ||')\n"
printf "\e[0Ksection_end:`date +%s`:snap_tlkt\r\e[0K\n"


# Global info
printf "\e[0Ksection_start:`date +%s`:snap_info\r\e[0KGetting snap global info\n"
#(we do not use config.h like other scripts because the info is on snapcraft.yaml too
#and `snapcraft remote-build` from previous job does not get config.h from launchpad)
cp build/linux/snap/snapcraft.yaml .

## Get info about GIMP version
GIMP_VERSION=$(awk '/^version:/ { print $2 }' snapcraft.yaml)

## Set proper Snap name and update track
NAME=$(awk '/^name:/ { print $2 }' snapcraft.yaml)
gimp_release=$([ "$(awk '/^grade:/ { print $2 }' snapcraft.yaml)" != 'devel' ] && echo true || echo false)
gimp_unstable=$(minor=$(echo "$GIMP_VERSION" | cut -d. -f2); [ $((minor % 2)) -ne 0 ] && echo true || echo false)
if [ "$gimp_release" = false ] || echo "$GIMP_VERSION" | grep -q 'git'; then
  export TRACK="experimental"
elif [ "$gimp_release" = true ] && [ "$gimp_unstable" = true ] || echo "$GIMP_VERSION" | grep -q 'RC'; then
  export TRACK="preview"
else
  export TRACK="latest"
fi
printf "(INFO): Name: $NAME (track: $TRACK) | Version: $GIMP_VERSION\n"

## Autodetects what archs will be packaged
supported_archs=$(find . -maxdepth 1 -iname "*.snap")
if [ "$supported_archs" = '' ]; then
  printf "(INFO): Arch: $(dpkg --print-architecture)\n"
  export supported_archs="./temp_${NAME}_${GIMP_VERSION}_$(dpkg --print-architecture).snap"
elif echo "$supported_archs" | grep -q 'arm64' && ! echo "$supported_archs" | grep -q 'amd64'; then
  printf '(INFO): Arch: arm64\n'
elif ! echo "$supported_archs" | grep -q 'arm64' && echo "$supported_archs" | grep -q 'amd64'; then
  printf '(INFO): Arch: amd64\n'
elif echo "$supported_archs" | grep -q 'arm64' && echo "$supported_archs" | grep -q 'amd64'; then
  printf '(INFO): Arch: arm64 and amd64\n'
fi
printf "\e[0Ksection_end:`date +%s`:snap_info\r\e[0K\n"


# Finish .snap to be exposed as artifact
for SNAP in $supported_archs; do
SNAP=$(echo "$SNAP" | sed 's|^\./temp_||')
printf "\e[0Ksection_start:`date +%s`:${SNAP}_making[collapsed=true]\r\e[0KFinishing ${SNAP}\n"
if [ -z "$GITLAB_CI" ]; then
  #as explained in 2_build-gimp-snapcraft.sh, we can only make snaps this way locally due to remote-build limitations
  sudo snapcraft pack --destructive-mode --output temp_${SNAP}
fi
mv temp_${SNAP} ${SNAP}
printf "(INFO): Suceeded. To test this build, install it from the artifact with: sudo snap install --dangerous ${SNAP}\n"
printf "\e[0Ksection_end:`date +%s`:${SNAP}_making\r\e[0K\n"


# Generate shasums for .snap
printf "\e[0Ksection_start:`date +%s`:${SNAP}_trust[collapsed=true]\r\e[0KChecksumming ${SNAP}\n"
printf "(INFO): ${SNAP} SHA-256: $(sha256sum ${SNAP} | cut -d ' ' -f 1)\n"
printf "(INFO): ${SNAP} SHA-512: $(sha512sum ${SNAP} | cut -d ' ' -f 1)\n"
printf "\e[0Ksection_end:`date +%s`:${SNAP}_trust\r\e[0K\n"


if [ "$GITLAB_CI" ]; then
  output_dir='build/linux/snap/_Output'
  mkdir -p $output_dir
  mv ${SNAP} $output_dir
fi


# Publish GIMP snap on Snap Store
#if [ "$GITLAB_CI" ] && [ "$CI_COMMIT_BRANCH" = "$CI_DEFAULT_BRANCH" ]; then
#  printf "\e[0Ksection_start:`date +%s`:${SNAP}_publish[collapsed=true]\r\e[0KPublishing snap to Snap Store\n"
#  snapcraft upload --release=$TRACK/stable $output_dir/${SNAP}
#  printf "\e[0Ksection_end:`date +%s`:${SNAP}_publish\r\e[0K\n"
#fi
done

rm snapcraft.yaml
