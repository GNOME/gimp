#!/bin/sh

find . \( -iname '*.dll' -or -iname '*.exe' \) | \
while IFS= read -r build_bin;
do
  build_bin_name="${build_bin##*/}"
  installed_bin=$(find ${MESON_INSTALL_DESTDIR_PREFIX} -iname "$build_bin_name")
  if [ x"$installed_bin" != "x" ]; then
    install_dir=$(dirname ${installed_bin})
    pdb_debug=$(echo $build_bin|sed 's/\.\(dll\|exe\)$/.pdb/')
    if [ -f "$pdb_debug" ]; then
      # Note: meson hides script outputs anyway on success. But this can be
      # useful when debugging.
      echo Installing $pdb_debug to $install_dir
      if [ -z "$MESON_INSTALL_DRY_RUN" ]; then
        cp -f $pdb_debug $install_dir
      fi
    fi
  fi
done;
