#!/usr/bin/env bash

# Copy tests resources
for dir in files gimpdir gimpdir-empty; do
    rm -rf "${MESON_BUILD_ROOT}/${MESON_SUBDIR}/${dir}"
    cp -r "${MESON_SOURCE_ROOT}/${MESON_SUBDIR}/${dir}" \
        "${MESON_BUILD_ROOT}/${MESON_SUBDIR}"
done

# Link to Color icon theme for tests
IconsRoot="${MESON_SOURCE_ROOT}/icons/Color"
IconsDirs=$(find "${IconsRoot}" -name [0-9]* -type d -printf '%f\n' | sort -n)
for dir in ${IconsDirs} ; do
    mkdir  "${MESON_BUILD_ROOT}/${MESON_SUBDIR}/gimp-test-icon-theme/hicolor/${dir}x${dir}" -p
    LnName="${MESON_BUILD_ROOT}/${MESON_SUBDIR}/gimp-test-icon-theme/hicolor/${dir}x${dir}/apps"
    rm -rf "${LnName}"
    ln -s "${IconsRoot}/${dir}" "${LnName}"
done

LnName="${MESON_BUILD_ROOT}/${MESON_SUBDIR}/gimp-test-icon-theme/hicolor/index.theme"
rm -rf "${LnName}"
ln -s "${IconsRoot}/index.theme" "${LnName}"


# Create output dirs
rm -rf "${MESON_BUILD_ROOT}/${MESON_SUBDIR}/gimpdir-output"
for dir in brushes gradients patterns; do
    mkdir -p "${MESON_BUILD_ROOT}/${MESON_SUBDIR}/gimpdir-output/${dir}"
done
