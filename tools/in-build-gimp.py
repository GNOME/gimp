#!/usr/bin/env python3
import os
import random
import re
import shutil
import string
import subprocess
import sys
import tempfile
from pathlib import Path

try:
  GIMP_GLOBAL_BUILD_ROOT = os.environ.get("GIMP_GLOBAL_BUILD_ROOT", ".")

  suffix = ''.join(random.choices(string.ascii_lowercase + string.digits, k=6))
  GIMP3_DIRECTORY = os.path.join(GIMP_GLOBAL_BUILD_ROOT, f".GIMP3-build-config-{suffix}")
  os.makedirs(GIMP3_DIRECTORY, mode=0o700, exist_ok=False)
  os.environ["GIMP3_DIRECTORY"] = GIMP3_DIRECTORY
  print(f"INFO: temporary GIMP configuration directory: {GIMP3_DIRECTORY}")

  # Earlier code used to set DYLD_LIBRARY_PATH environment variable instead, but
  # it didn't work on contributor's builds because of System Integrity
  # Protection (SIP), though it did work in the CI which had older macOS.
  # So, we just set LC_RPATH on binaries, but this restrict us to only one
  # target at a time. See: #14236 and gimp-data/images/logo/meson.build
  rpath_array = [f"{GIMP_GLOBAL_BUILD_ROOT}/libgimp",
                 f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpbase",
                 f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpcolor",
                 f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpconfig",
                 f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpmath",
                 f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpmodule",
                 f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpthumb",
                 f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpwidgets"]

  if "GIMP_TEMP_UPDATE_RPATH" in os.environ:
    for binary in os.environ["GIMP_TEMP_UPDATE_RPATH"].split(":"):
      result = subprocess.run(['otool', '-l', binary], stdout=subprocess.PIPE)
      out = result.stdout.decode('utf-8', errors='replace')
      regex = re.findall(r'path (.+?) \(offset', out)
      for new_rpath in rpath_array:
        if not new_rpath in regex:
          subprocess.run(["install_name_tool", "-add_rpath", new_rpath, binary], check=True)

  #Ensure the same python from meson.build (GIMP_PYTHON_WITH_GI) is used by plugins
  #This is needed because GIMP_PYTHON_WITH_GI can not coincide with python3 from shebang
  #(on MacPorts, there is no python3 symlink, so we would misuse Xcode python3 without GI)
  python_symlink = shutil.which("python3")
  pygobject_found=False
  different_python=False
  if python_symlink and not os.path.samefile(python_symlink, os.environ.get("GIMP_PYTHON_WITH_GI")):
    result = subprocess.run([python_symlink,"-c","import sys, gi; version='3.0'; sys.exit(gi.check_version(version))"], check=False)
    pygobject_found = (result.returncode == 0)
  if not python_symlink or (python_symlink and not pygobject_found):
    different_python=True
    tmp_path = os.path.join(GIMP_GLOBAL_BUILD_ROOT, "tmp")
    os.makedirs(tmp_path, exist_ok=True)
    tmp_symlink = os.path.join(tmp_path, "python3")
    if not os.path.exists(tmp_symlink):
      os.symlink(os.environ.get("GIMP_PYTHON_WITH_GI"), tmp_symlink)
    os.environ["PATH"] = tmp_path + os.pathsep + os.environ.get("PATH", "")

  if "GIMP_DEBUG_SELF" in os.environ and shutil.which("gdb"):
    print(f"RUNNING: gdb --batch -x {os.environ['GIMP_GLOBAL_SOURCE_ROOT']}/tools/debug-in-build-gimp.py --args {os.environ['GIMP_SELF_IN_BUILD']} {' '.join(sys.argv[1:])}")
    subprocess.run(["gdb","--return-child-result","--batch","-x",f"{os.environ['GIMP_GLOBAL_SOURCE_ROOT']}/tools/debug-in-build-gimp.py","--args", os.environ["GIMP_SELF_IN_BUILD"]] + sys.argv[1:], stdin=sys.stdin, check=True)
  else:
    print(f"RUNNING: {os.environ['GIMP_SELF_IN_BUILD']} {' '.join(sys.argv[1:])}")
    subprocess.run([os.environ["GIMP_SELF_IN_BUILD"]] + sys.argv[1:],stdin=sys.stdin, check=True)

  if different_python:
    os.environ["PATH"] = os.pathsep.join([p for p in os.environ["PATH"].split(os.pathsep) if p != tmp_path])
    shutil.rmtree(tmp_path, ignore_errors=True)

  if "GIMP_TEMP_UPDATE_RPATH" in os.environ:
    for binary in os.environ["GIMP_TEMP_UPDATE_RPATH"].split(":"):
      result = subprocess.run(['otool', '-l', binary], stdout=subprocess.PIPE)
      out = result.stdout.decode('utf-8', errors='replace')
      regex = re.findall(r'path (.+?) \(offset', out)
      for new_rpath in rpath_array:
        if new_rpath in regex:
          subprocess.run(["install_name_tool", "-delete_rpath", new_rpath, binary], check=True)

  # Clean-up the temporary config directory after each usage, yet making sure we
  # don't get tricked by weird redirections or anything of the sort. In particular
  # we check that this is a directory with user permission, not a symlink, and
  # that it's inside inside the project build's root.
  if "GIMP3_DIRECTORY" in os.environ and os.path.isdir(GIMP3_DIRECTORY):
    if os.path.islink(GIMP3_DIRECTORY):
      print(f"ERROR: $GIMP3_DIRECTORY ({GIMP3_DIRECTORY}) should not be a symlink.")
      sys.exit(1)
    used_dir_prefix = str(Path(GIMP3_DIRECTORY).resolve().as_posix())[:-6]
    tmpl_dir_prefix = f"{Path(os.environ['GIMP_GLOBAL_BUILD_ROOT']).resolve().as_posix()}/.GIMP3-build-config-"
    if used_dir_prefix != tmpl_dir_prefix:
      print(f"ERROR: $GIMP3_DIRECTORY ({GIMP3_DIRECTORY}) should be under the build directory with a specific prefix.")
      print(f'       "{used_dir_prefix}" != "{tmpl_dir_prefix}"')
      sys.exit(1)
    print(f"INFO: Running: shutil.rmtree({GIMP3_DIRECTORY})")
    shutil.rmtree(GIMP3_DIRECTORY)
  elif not os.access(GIMP3_DIRECTORY, os.W_OK):
    print(f"ERROR: $GIMP3_DIRECTORY ({GIMP3_DIRECTORY}) does not belong to the user")
    sys.exit(1)
  else:
    print(f"ERROR: $GIMP3_DIRECTORY ({GIMP3_DIRECTORY}) is not a directory")
    sys.exit(1)

except subprocess.CalledProcessError as e:
  print(f"Command failed with exit code {e.returncode}: {e.cmd}")
  sys.exit(e.returncode)
except Exception as e:
  print(f"Error: {str(e)}")
  sys.exit(1)
