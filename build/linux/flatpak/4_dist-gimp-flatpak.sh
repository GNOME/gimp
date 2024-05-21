#!/bin/sh

# This script should be used only by 'dist-flatpak-weekly' job but
# uploading repo/ in previous 'gimp-flatpak-x64' isn't possible
# Let's keep the script, anyway, for clarity about the dist procedure


# Generate a Flatpak bundle to be tested with GNOME runtime installed
# (it is NOT a "real"/full bundle, deps from GNOME runtime are not bundled)
flatpak build-bundle repo gimp-git.flatpak --runtime-repo=https://nightly.gnome.org/gnome-nightly.flatpakrepo org.gimp.GIMP ${BRANCH}

# Only distribute on GNOME nightly if from 'master' branch
if [ "$CI_COMMIT_BRANCH" != "$CI_DEFAULT_BRANCH" ]; then
  exit 0
fi

# Prepare OSTree repo for GNOME nightly distribution
tar cf repo.tar repo/

# The following commands we take directly from 'flatpak_ci_initiative.yml'
# See 'dist-flatpak-weekly' job