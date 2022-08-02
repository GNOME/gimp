#!/usr/bin/env bash

GIT_VERSION_H="$1"

# MESON_SOURCE_ROOT and MESON_BUILD_ROOT environment variables are
# passed since meson 0.54.0 but we depend on 0.53.0 so I also pass them
# as script arguments. When we bump out meson requirement, this test may
# go away.
if [ -z "$MESON_SOURCE_ROOT" ]; then
  MESON_SOURCE_ROOT="$2"
fi
if [ -z "$MESON_BUILD_ROOT" ]; then
  MESON_BUILD_ROOT="$3"
fi

# `meson dist` doesn't trigger a build, which is a problem because we
# need to add some configured/built files. The case where we didn't
# build at all is the less annoying (we'd get a failure, which is
# actually acceptable as it warns us). But the case where we would copy
# outdated data is much more insidious (such as wrong INSTALL
# information or wrong git information) as it would be silent.
# See: https://github.com/mesonbuild/meson/issues/10650
# And: https://gitlab.gnome.org/GNOME/gimp/-/issues/7907
# This is why we manually trigger, not only a reconfigure, but also a
# rebuild of the main project before copying data around.

# INSTALL file is generated at configure time.
meson --reconfigure $MESON_SOURCE_ROOT
# git-version.h is generated at build time.
meson compile

cp -f 'INSTALL' "${MESON_DIST_ROOT}"
rm -f "${MESON_DIST_ROOT}/INSTALL.in"

cp "$GIT_VERSION_H" "${MESON_DIST_ROOT}"
