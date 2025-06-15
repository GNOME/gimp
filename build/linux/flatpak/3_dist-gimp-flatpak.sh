#!/bin/sh

case $(readlink /proc/$$/exe) in
  *bash)
    set -o posix
    ;;
esac
set -e


# GLOBAL INFO
APP_ID='org.gimp.GIMP.Nightly'


# GIMP FILES AS REPO
if [ "$GITLAB_CI" ]; then
  # Extract previously exported OSTree repo/
  tar xf repo.tar --warning=no-timestamp
fi


# CONSTRUCT .FLATPAK
# Generate a Flatpak "bundle" to be tested with GNOME runtime installed
# (it is NOT a real/full bundle, deps from GNOME runtime are not bundled)
printf "\e[0Ksection_start:`date +%s`:flat_making[collapsed=true]\r\e[0KPackaging repo as ${APP_ID}.flatpak\n"
flatpak build-bundle repo ${APP_ID}.flatpak --runtime-repo=https://nightly.gnome.org/gnome-nightly.flatpakrepo ${APP_ID} ${BRANCH}
printf "\e[0Ksection_end:`date +%s`:flat_making\r\e[0K\n"


# GENERATE SHASUMS FOR .FLATPAK
printf "\e[0Ksection_start:`date +%s`:flat_trust[collapsed=true]\r\e[0KChecksumming ${APP_ID}.flatpak\n"
printf "(INFO): ${APP_ID}.flatpak SHA-256: $(sha256sum ${APP_ID}.flatpak | cut -d ' ' -f 1)\n"
printf "(INFO): ${APP_ID}.flatpak SHA-512: $(sha512sum ${APP_ID}.flatpak | cut -d ' ' -f 1)\n"
printf "\e[0Ksection_end:`date +%s`:flat_trust\r\e[0K\n"


if [ "$GITLAB_CI" ]; then
  output_dir='build/linux/flatpak/_Output'
  mkdir -p $output_dir
  mv ${APP_ID}* $output_dir
fi


# PUBLISH GIMP REPO IN GNOME NIGHTLY
# We take the commands from 'flatpak_ci_initiative.yml'
if [ "$GITLAB_CI" ] && [ "$CI_COMMIT_BRANCH" = "$CI_DEFAULT_BRANCH" ]; then
  printf "\e[0Ksection_start:`date +%s`:flat_publish[collapsed=true]\r\e[0KPublishing repo to GNOME nightly\n"
  curl https://gitlab.gnome.org/GNOME/citemplates/raw/master/flatpak/flatpak_ci_initiative.yml --output flatpak_ci_initiative.yml
  eval "$(sed -n -e '/flatpak build-update-repo/,/exit $result/ { s/    - //; p }' flatpak_ci_initiative.yml)"
  printf "\e[0Ksection_end:`date +%s`:flat_publish\r\e[0K\n"
fi
