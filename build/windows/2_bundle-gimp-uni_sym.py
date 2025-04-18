#!/usr/bin/env python3
import os
import shutil
import fnmatch

for build_root, _, build_bins in os.walk(os.getenv("MESON_BUILD_ROOT")):
  for file in build_bins:
    if fnmatch.fnmatch(file, '*.dll') or fnmatch.fnmatch(file, '*.exe'):
      build_bin = os.path.join(build_root, file)
      installed_bin = None
      for installed_root, _, installed_bins in os.walk(os.getenv("MESON_INSTALL_DESTDIR_PREFIX")):
        if os.path.basename(build_bin) in installed_bins:
          installed_bin = os.path.join(installed_root, os.path.basename(build_bin))
          break
      if installed_bin and not "test-plug-ins" in build_bin:
        install_dir = os.path.dirname(installed_bin)
        pdb_debug = os.path.splitext(build_bin)[0] + '.pdb'
        print(f"Installing {pdb_debug} to {install_dir}")

        # Clang correctly puts the .pdb along the $installed_bin
        if os.path.isfile(pdb_debug):
          if not os.getenv("MESON_INSTALL_DRY_RUN"):
            shutil.copy2(pdb_debug, install_dir)
        
        # GCC dumbly puts the .pdb in $MESON_BUILD_ROOT
        else:
          if not os.getenv("MESON_INSTALL_DRY_RUN"):
            for gcc_root, _, gcc_files in os.walk(os.getenv("MESON_BUILD_ROOT")):
              for gcc_file in gcc_files:
                if fnmatch.fnmatch(gcc_file, os.path.basename(pdb_debug)):
                  shutil.copy2(os.path.join(gcc_root, gcc_file), install_dir)
                  break
