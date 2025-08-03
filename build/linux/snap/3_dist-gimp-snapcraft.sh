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
for SNAP in $(find . -maxdepth 1 -iname "*.snap"); do
SNAP=$(echo "$SNAP" | sed 's|^\./temp_||')


# Finish .snap to be exposed as artifact
printf "\e[0Ksection_start:`date +%s`:${SNAP}_making[collapsed=true]\r\e[0KFinishing ${SNAP}\n"
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
#  snapcraft upload --release=experimental/stable ${SNAP}
#  printf "\e[0Ksection_end:`date +%s`:${SNAP}_publish\r\e[0K\n"
#fi
done
