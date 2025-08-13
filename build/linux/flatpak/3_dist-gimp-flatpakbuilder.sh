#!/bin/sh

# Ensure the script work properly
case $(readlink /proc/$$/exe) in
  *bash)
    set -o posix
    ;;
esac
set -e
if [ "$0" != 'build/linux/flatpak/3_dist-gimp-flatpakbuilder.sh' ] && [ $(basename "$PWD") != 'flatpak' ]; then
  printf '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/linux/\n'
  exit 1
elif [ $(basename "$PWD") = 'flatpak' ]; then
  cd ../../..
fi


# CHECK FLATPAK-BUILDER AND OTHER TOOLS
printf "\e[0Ksection_start:`date +%s`:flat_tlkt\r\e[0KChecking flatpak tools\n"
## flatpak-builder (we don't use it on this particular script but it is useful to know its version)
eval "$(sed -n '/Install part/,/End of check/p' build/linux/flatpak/1_build-deps-flatpakbuilder.sh)"

## flat-manager (only for master branch)
if [ "$GITLAB_CI" ] && [ "$CI_COMMIT_BRANCH" = "$CI_DEFAULT_BRANCH" ]; then
  #flat-manager is unreproducible: https://github.com/flatpak/flat-manager/issues/155
  flatmanager_text=" | flat-manager version: unknown"
fi
printf "(INFO): flatpak-builder version: $builder_version (over flatpak $(flatpak --version | sed 's|Flatpak ||'))${flatmanager_text}\n"
printf "\e[0Ksection_end:`date +%s`:flat_tlkt\r\e[0K\n"


# GLOBAL INFO
printf "\e[0Ksection_start:`date +%s`:flat_info\r\e[0KGetting flatpak global info\n"
## Get proper app ID and update branch
APP_ID=$(awk -F'"' '/"app-id"/ {print $4; exit}' build/linux/flatpak/org.gimp.GIMP-nightly.json)
BRANCH=$(awk -F'"' '/"branch"/ {print $4; exit}' build/linux/flatpak/org.gimp.GIMP-nightly.json)
printf "(INFO): App ID: $APP_ID (branch: $BRANCH)\n"

## Autodetects what archs will be packaged
supported_archs=$(find . -maxdepth 1 -iname "*.flatpak")
if [ "$supported_archs" = '' ]; then
  printf "(INFO): Arch: $(uname -m)\n"
  export supported_archs="./temp_${APP_ID}-$(uname -m).flatpak"
elif echo "$supported_archs" | grep -q 'aarch64' && ! echo "$supported_archs" | grep -q 'x86_64'; then
  printf '(INFO): Arch: aarch64\n'
elif ! echo "$supported_archs" | grep -q 'aarch64' && echo "$supported_archs" | grep -q 'x86_64'; then
  printf '(INFO): Arch: x86_64\n'
elif echo "$supported_archs" | grep -q 'aarch64' && echo "$supported_archs" | grep -q 'x86_64'; then
  printf '(INFO): Arch: aarch64 and x86_64\n'
fi
printf "\e[0Ksection_end:`date +%s`:flat_info\r\e[0K\n"


# GIMP FILES AS REPO (FOR .FLATPAK MAKING AND/OR FURTHER PUBLISHING)
for FLATPAK in $supported_archs; do
FLATPAK=$(echo "$FLATPAK" | sed 's|^\./temp_||')
ARCH=$(echo "$FLATPAK" | sed 's/.*-\([^-]*\)\.flatpak/\1/')
if [ "$GITLAB_CI" ]; then
  # Extract previously exported OSTree repo/
  if [ -d 'repo' ]; then
    rm -fr repo
  fi
  tar xf repo-$ARCH.tar --warning=no-timestamp
fi


# FINISH .FLATPAK
# Generate a Flatpak "bundle" to be tested with GNOME runtime installed
# (it is NOT a real/full bundle, deps from GNOME runtime are not bundled)
printf "\e[0Ksection_start:`date +%s`:${FLATPAK}_making[collapsed=true]\r\e[0KFinishing ${FLATPAK}\n"
if [ -z "$GITLAB_CI" ]; then
  #build-bundle is not arch neutral so on CI it is run on 2_build-gimp-flatpakbuilder.sh
  flatpak build-bundle repo temp_${APP_ID}-$(uname -m).flatpak --runtime-repo=https://nightly.gnome.org/gnome-nightly.flatpakrepo ${APP_ID} ${BRANCH}
fi
mv temp_${FLATPAK} ${FLATPAK}
printf "(INFO): Suceeded. To test this build, install it from the artifact with: flatpak install --user ${FLATPAK} -y\n"
printf "\e[0Ksection_end:`date +%s`:${FLATPAK}_making\r\e[0K\n"


# GENERATE SHASUMS FOR .FLATPAK
printf "\e[0Ksection_start:`date +%s`:${FLATPAK}_trust[collapsed=true]\r\e[0KChecksumming ${FLATPAK}\n"
printf "(INFO): ${FLATPAK} SHA-256: $(sha256sum ${FLATPAK} | cut -d ' ' -f 1)\n"
printf "(INFO): ${FLATPAK} SHA-512: $(sha512sum ${FLATPAK} | cut -d ' ' -f 1)\n"
printf "\e[0Ksection_end:`date +%s`:${FLATPAK}_trust\r\e[0K\n"


if [ "$GITLAB_CI" ]; then
  output_dir='build/linux/flatpak/_Output'
  mkdir -p $output_dir
  mv ${FLATPAK} $output_dir
fi


# PUBLISH GIMP REPO IN GNOME NIGHTLY
# We take the commands from 'flatpak_ci_initiative.yml'
if [ "$GITLAB_CI" ] && [ "$CI_COMMIT_BRANCH" = "$CI_DEFAULT_BRANCH" ]; then
  printf "\e[0Ksection_start:`date +%s`:${FLATPAK}_submission[collapsed=true]\r\e[0KPublishing $ARCH repo to GNOME nightly\n"
  curl https://gitlab.gnome.org/GNOME/citemplates/raw/master/flatpak/flatpak_ci_initiative.yml --output flatpak_ci_initiative.yml
  eval "$(sed -n -e '/flatpak build-update-repo/,/purge/ { s/    - //; p }' flatpak_ci_initiative.yml)"
  printf "\e[0Ksection_end:`date +%s`:${FLATPAK}_submission\r\e[0K\n"
fi
done
