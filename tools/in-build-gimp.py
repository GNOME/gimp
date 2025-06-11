#!/usr/bin/env python3
import os
import random
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

  if "GIMP_TEMP_UPDATE_RPATH" in os.environ:
    # Earlier code used to set DYLD_LIBRARY_PATH environment variable instead, but
    # it didn't work on contributor's builds because of System Integrity
    # Protection (SIP), though it did work in the CI.    
    # So, we just set LC_RPATH on binaries, but this restrict us to only one 'gimp_exe_depends' target for
    # macOS (the splash screen). Otherwise, multiple install_name_tool procs would clash over the same bin. See: #14236
    for binary in os.environ["GIMP_TEMP_UPDATE_RPATH"].split(":"):
      subprocess.run(["install_name_tool", "-add_rpath", f"{GIMP_GLOBAL_BUILD_ROOT}/libgimp", binary], check=True)
      subprocess.run(["install_name_tool", "-add_rpath", f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpbase", binary], check=True)
      subprocess.run(["install_name_tool", "-add_rpath", f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpcolor", binary], check=True)
      subprocess.run(["install_name_tool", "-add_rpath", f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpconfig", binary], check=True)
      subprocess.run(["install_name_tool", "-add_rpath", f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpmath", binary], check=True)
      subprocess.run(["install_name_tool", "-add_rpath", f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpmodule", binary], check=True)
      subprocess.run(["install_name_tool", "-add_rpath", f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpthumb", binary], check=True)
      subprocess.run(["install_name_tool", "-add_rpath", f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpwidgets", binary], check=True)

  if "GIMP_DEBUG_SELF" in os.environ and shutil.which("gdb"):
    print(f"RUNNING: gdb --batch -x {os.environ['GIMP_GLOBAL_SOURCE_ROOT']}/tools/debug-in-build-gimp.py --args {os.environ['GIMP_SELF_IN_BUILD']} {' '.join(sys.argv[1:])}")
    subprocess.run(["gdb","--return-child-result","--batch","-x",f"{os.environ['GIMP_GLOBAL_SOURCE_ROOT']}/tools/debug-in-build-gimp.py","--args", os.environ["GIMP_SELF_IN_BUILD"]] + sys.argv[1:], stdin=sys.stdin, check=True)
  else:
    print(f"RUNNING: {os.environ['GIMP_SELF_IN_BUILD']} {' '.join(sys.argv[1:])}")
    subprocess.run([os.environ["GIMP_SELF_IN_BUILD"]] + sys.argv[1:],stdin=sys.stdin, check=True)

  if "GIMP_TEMP_UPDATE_RPATH" in os.environ:
    for binary in os.environ["GIMP_TEMP_UPDATE_RPATH"].split(":"):
      subprocess.run(["install_name_tool", "-delete_rpath", f"{GIMP_GLOBAL_BUILD_ROOT}/libgimp", binary], check=True)
      subprocess.run(["install_name_tool", "-delete_rpath", f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpbase", binary], check=True)
      subprocess.run(["install_name_tool", "-delete_rpath", f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpcolor", binary], check=True)
      subprocess.run(["install_name_tool", "-delete_rpath", f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpconfig", binary], check=True)
      subprocess.run(["install_name_tool", "-delete_rpath", f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpmath", binary], check=True)
      subprocess.run(["install_name_tool", "-delete_rpath", f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpmodule", binary], check=True)
      subprocess.run(["install_name_tool", "-delete_rpath", f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpthumb", binary], check=True)
      subprocess.run(["install_name_tool", "-delete_rpath", f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpwidgets", binary], check=True)

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
