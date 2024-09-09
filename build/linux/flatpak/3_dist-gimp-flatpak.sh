#!/bin/sh

set -e


if [ "$GITLAB_CI" ]; then
  # Extract previously exported OSTree repo/
  tar xf repo.tar --warning=no-timestamp
fi


# Generate a Flatpak "bundle" to be tested with GNOME runtime installed
# (it is NOT a real/full bundle, deps from GNOME runtime are not bundled)
echo -e "\e[0Ksection_start:`date +%s`:flat_making[collapsed=true]\r\e[0KPackaging repo as org.gimp.GIMP.Nightly.flatpak"
flatpak build-bundle repo org.gimp.GIMP.Nightly.flatpak --runtime-repo=https://nightly.gnome.org/gnome-nightly.flatpakrepo org.gimp.GIMP.Nightly ${BRANCH}
echo -e "\e[0Ksection_end:`date +%s`:flat_making\r\e[0K"


# Publish GIMP repo in GNOME nightly
# We take the commands from 'flatpak_ci_initiative.yml'
if [ "$GITLAB_CI" ] && [ "$CI_COMMIT_BRANCH" = "$CI_DEFAULT_BRANCH" ]; then
  echo -e "\e[0Ksection_start:`date +%s`:flat_publish[collapsed=true]\r\e[0KPublishing repo to GNOME nightly"
  curl https://gitlab.gnome.org/GNOME/citemplates/raw/master/flatpak/flatpak_ci_initiative.yml --output flatpak_ci_initiative.yml
  IFS=$'\n' cmd_array=($(cat flatpak_ci_initiative.yml | sed -n '/flatpak build-update-repo/,/exit $result\"/p' | sed 's/    - //'))
  IFS=$' \t\n'
  for cmd in "${cmd_array[@]}"; do
    eval "$cmd" || continue
  done
  echo -e "\e[0Ksection_end:`date +%s`:flat_publish\r\e[0K"
fi
