#!/usr/bin/env python3
import os
import shutil
from pathlib import Path

binaries = list(Path(os.environ.get('MESON_BUILD_ROOT')).rglob('*.*'))
build_bins = [
  p for p in binaries
  if p.suffix.lower() in ('.dll', '.exe') and p.is_file()
]
for build_bin in build_bins:
  installed_bins = []
  for p in Path(os.environ.get('MESON_INSTALL_DESTDIR_PREFIX')).rglob('*'):
    if p.is_file() and p.name.lower() == build_bin.name.lower():
      installed_bins.append(p)
for installed_bin in installed_bins:
  install_dir = installed_bin.parent
  pdb_debug = f"{build_bin.stem}.pdb"
  print(f"Installing {pdb_debug} to {install_dir}")

  # Clang correctly puts the .pdb along the $installed_bin
  if pdb_debug.exists():
    if not os.environ.get('MESON_INSTALL_DRY_RUN'):
      shutil.copy2(build_bin.with_stem(pdb_debug).with_suffix('.pdb'), install_dir)
    
  # GCC dumbly puts the .pdb in $MESON_BUILD_ROOT        
  else:
    if not os.environ.get('MESON_INSTALL_DRY_RUN'):
      for pdb in build_root.rglob(pdb_debug):
        shutil.copy2(pdb, install_dir)
