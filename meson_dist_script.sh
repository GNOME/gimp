#!/usr/bin/env bash

cp -f 'INSTALL' "${MESON_DIST_ROOT}"
# rm -f "${MESON_DIST_ROOT}/INSTALL.in"

cp 'git-version.h' "${MESON_DIST_ROOT}"
