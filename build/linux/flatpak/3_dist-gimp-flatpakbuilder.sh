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


# GLOBAL INFO
for FLATPAK in $(find . -maxdepth 1 -iname "*.flatpak"); do
FLATPAK=$(echo "$FLATPAK" | sed 's|^\./temp_||')
ARCH=$(echo "$FLATPAK" | sed 's/.*-\([^-]*\)\.flatpak/\1/')


# GIMP FILES AS REPO (FOR FURTHER PUBLISHING)
if [ "$GITLAB_CI" ]; then
  # Extract previously exported OSTree repo/
  if [ -d 'repo' ]; then
    rm -fr repo
  fi
  mkdir repo
  tar xf repo-$ARCH.tar -C repo/ --warning=no-timestamp
fi


# FINISH .FLATPAK
printf "\e[0Ksection_start:`date +%s`:${FLATPAK}_making[collapsed=true]\r\e[0KFinishing ${FLATPAK}\n"
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
  printf "\e[0Ksection_start:`date +%s`:${FLATPAK}_publish[collapsed=true]\r\e[0KPublishing $ARCH repo to GNOME nightly\n"
  curl https://gitlab.gnome.org/GNOME/citemplates/raw/master/flatpak/flatpak_ci_initiative.yml --output flatpak_ci_initiative.yml
  eval "$(sed -n -e '/flatpak build-update-repo/,/exit $result/ { s/    - //; p }' flatpak_ci_initiative.yml)"
  printf "\e[0Ksection_end:`date +%s`:${FLATPAK}_publish\r\e[0K\n"
fi
done
