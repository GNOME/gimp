#!/usr/bin/env python3
import os
import shutil
import re
import sys
import json

if not os.path.isfile("build.ninja"):
  print("\033[31m(ERROR)\033[0m: Script called standalone. This script should be only called from build systems.")
  sys.exit(1)


# This .py script should not even exist
# Ideally meson should take care of it automatically.
# See: https://github.com/mesonbuild/meson/issues/12977
if os.getenv("MESON_BUILD_ROOT", False):
  with open("meson-info/intro-installed.json", "r") as f:
    build_installed = json.load(f)
  for build_bin, installed_bin in build_installed.items():
    if build_bin.endswith((".dll", ".exe")):
      pdb_debug = os.path.splitext(build_bin)[0] + ".pdb"
      install_dir = os.path.dirname(installed_bin)
      if os.path.isfile(pdb_debug):
        if not os.getenv("MESON_INSTALL_DRY_RUN"):
          print(f"Installing {pdb_debug} to {install_dir}")
          shutil.copy2(pdb_debug, install_dir)

# This 'else:' part is injected by 1_build-deps-msys2.ps1 when
# we build some dependency that uses Cmake. Like meson,
# it also do not auto install .pdb files by default.
else:
  for build_root, dirs, files in os.walk(os.getcwd()):
    for file in files:
      if file.endswith('_install.cmake'):
        with open(os.path.join(build_root, file), "r") as f:
          content = f.read()
        installed_root = re.search(r'set\s*\(\s*CMAKE_INSTALL_PREFIX\s*"([^"]+)"\s*\)', content).group(1)
        for build_bin_line in re.findall(r'file\([^\)]*FILES\s+"[^"]+\.(?:dll|exe)"[^\)]*\)', content):
          build_bin = re.search(r'FILES\s+"([^"]+\.(?:dll|exe))"', build_bin_line).group(1)
          pdb_debug = os.path.splitext(build_bin)[0] + ".pdb"
          install_dir = os.path.join(installed_root, re.search(r'DESTINATION\s+"\$\{CMAKE_INSTALL_PREFIX\}(/[^"\s]+)"', build_bin_line).group(1).lstrip('/'))
          if os.path.isfile(pdb_debug):
            print(f"-- Installing: {install_dir}/{os.path.basename(pdb_debug)}")
            if not os.path.exists(install_dir):
              os.makedirs(install_dir)
            shutil.copy2(pdb_debug, install_dir)
