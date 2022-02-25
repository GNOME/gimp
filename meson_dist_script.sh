#!/usr/bin/env bash

cp -f 'INSTALL' "${MESON_DIST_ROOT}"
# rm -f "${MESON_DIST_ROOT}/INSTALL.in"

cp "$1" "${MESON_DIST_ROOT}"
