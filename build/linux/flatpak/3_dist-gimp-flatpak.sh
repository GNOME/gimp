#!/bin/sh


if [ "$GITLAB_CI" ]; then
  # Extract previously exported repo/
  tar xf repo.tar
fi


# Generate a Flatpak "bundle" to be tested with GNOME runtime installed
# (it is NOT a real/full bundle, deps from GNOME runtime are not bundled)
echo '(INFO): packaging repo as .flatpak'
flatpak build-bundle repo gimp-git.flatpak --runtime-repo=https://nightly.gnome.org/gnome-nightly.flatpakrepo org.gimp.GIMP ${BRANCH}


# Publish GIMP repo in GNOME nightly
# We take the commands from 'flatpak_ci_initiative.yml'
if [ "$CI_COMMIT_BRANCH" = "$CI_DEFAULT_BRANCH" ]; then
  curl https://gitlab.gnome.org/GNOME/citemplates/raw/master/flatpak/flatpak_ci_initiative.yml --output flatpak_ci_initiative.yml
  IFS=$'\n' cmd_array=($(cat flatpak_ci_initiative.yml | sed -n '/flatpak build-update-repo/,/exit $result\"/p' | sed 's/    - //'))
  IFS=$' \t\n'
  for cmd in "${cmd_array[@]}"; do
    eval "$cmd" || continue
  done
fi
