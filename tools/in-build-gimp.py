#!/usr/bin/env python3
try:
  # fcntl only exists on UNIX-like platforms. On particular, it won't
  # exist on Windows. But it's fine, because we don't need to
  # exclusively lock this script on Windows.
  import fcntl
except:
  pass
import os
import random
import re
import shutil
import stat
import string
import subprocess
import sys
import tempfile
import time
from pathlib import Path

# In some case, this script may not be run concurrently, in particular
# on macOS where we need to set rpath and install_name_tool doesn't like
# when a LC_RPATH already exists (see GIMP_TEMP_UPDATE_RPATH section below).
lock = None
def cleanup(lock):
  if lock is not None:
    fcntl.flock(lock, fcntl.LOCK_UN)
    os.close(lock)

try:
  # Get needed env vars
  if os.environ.get('GIMP_TESTING_PLUGINDIRS') is not None:
    sys.stderr.write(f"ERROR: do not set $GIMP_TESTING_PLUGINDIRS. This script will take care of it.\n")
    sys.stderr.write(f"       Set $GIMP_TESTING_PLUG_INS instead.\n")
    sys.exit(1)
  GIMP_GLOBAL_BUILD_ROOT  = os.environ.get("GIMP_GLOBAL_BUILD_ROOT", ".")
  GIMP_GLOBAL_SOURCE_ROOT = os.environ.get("GIMP_GLOBAL_SOURCE_ROOT")
  GIMP_TESTING_PLUG_INS   = os.environ.get("GIMP_TESTING_PLUG_INS")

  # Create the temporary config directory
  suffix = ''.join(random.choices(string.ascii_lowercase + string.digits, k=6))
  GIMP3_DIRECTORY = os.path.join(GIMP_GLOBAL_BUILD_ROOT, f".GIMP3-build-config-{suffix}")
  os.makedirs(GIMP3_DIRECTORY, mode=0o700, exist_ok=False)
  os.environ["GIMP3_DIRECTORY"] = GIMP3_DIRECTORY
  sys.stderr.write(f"INFO: temporary GIMP configuration directory: {os.path.relpath(GIMP3_DIRECTORY)}\n")

  # Copy just the strictly needed plug-ins by the script or test (seee issue #11385)
  def is_script_file(filepath):
    try:
      with open(filepath, 'rb') as f:
        return f.read(2) == b'#!'
    except IOError:
      return False
  if GIMP_TESTING_PLUG_INS is not None:
    processed_plugins = []
    plugins_dir = os.path.join(GIMP3_DIRECTORY, 'plug-ins')
    os.environ['GIMP_TESTING_PLUGINDIRS'] = plugins_dir
    plug_ins = GIMP_TESTING_PLUG_INS.split(':')
    for plug_in in plug_ins:
      if plug_in[-1] == '/':
        plug_in = plug_in[:-1]
      plug_in_name = os.path.basename(plug_in)
      plugin_dir = os.path.join(plugins_dir, plug_in_name)
      os.makedirs(plugin_dir, mode=0o700, exist_ok=False)
      dst = os.path.join(plugin_dir, plug_in_name)
      if plug_in.startswith('common/'):
        src = os.path.join(GIMP_GLOBAL_BUILD_ROOT, 'plug-ins', plug_in)
      elif plug_in.startswith('python'):
        src = os.path.join(GIMP_GLOBAL_SOURCE_ROOT, 'plug-ins', plug_in + '.py')
        dst += '.py'
      elif plug_in.startswith('libgimp'):
        if plug_in.endswith('.py'):
          src = os.path.join(GIMP_GLOBAL_SOURCE_ROOT, plug_in)
        else:
          src = os.path.join(GIMP_GLOBAL_BUILD_ROOT, plug_in)
      else:
        src = os.path.join(GIMP_GLOBAL_BUILD_ROOT, 'plug-ins', plug_in, plug_in)
      if sys.platform in ['win32', 'cygwin'] and not is_script_file(src):
        src += '.exe'
        dst += '.exe'
      shutil.copyfile(src, dst)
      os.chmod(dst, stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH | stat.S_IWRITE)
      if not is_script_file(dst):
        processed_plugins.append(dst)

  # Copy the non-installed data for running the script or test (e.g. Palette unit-test)
  data_types = [ 'palettes', 'brushes', 'patterns', 'dynamics', 'gradients' ]
  for dtype in data_types:
    env_var = os.environ.get("GIMP_TESTING_" + dtype.upper())
    if env_var is not None:
      ddir = os.path.join(GIMP3_DIRECTORY, dtype)
      os.makedirs(ddir, mode=0o700, exist_ok=False)
      datalist = env_var.split(':')
      for data in datalist:
        src = os.path.join(GIMP_GLOBAL_SOURCE_ROOT, data)
        dst = os.path.join(ddir, os.path.basename(data))
        shutil.copyfile(src, dst)

  if "GIMP_TEMP_UPDATE_RPATH" in os.environ:
    # Earlier code used to set DYLD_LIBRARY_PATH environment variable instead, but
    # it didn't work on contributor's builds because of System Integrity
    # Protection (SIP), though it did work in the CI which had older macOS.
    # So, we change LC_RPATH, but by only one in-build-gimp.py at a time (see #14236).
    rpath_array = [f"{GIMP_GLOBAL_BUILD_ROOT}/libgimp",
                  f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpbase",
                  f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpcolor",
                  f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpconfig",
                  f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpmath",
                  f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpmodule",
                  f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpthumb",
                  f"{GIMP_GLOBAL_BUILD_ROOT}/libgimpwidgets"]
    lock = os.open(__file__, os.O_RDONLY)
    fcntl.flock(lock, fcntl.LOCK_EX)
    binaries_to_patch = os.environ.get("GIMP_TEMP_UPDATE_RPATH", "").split(":")
    binaries_to_patch.extend(processed_plugins)
    for binary in binaries_to_patch:
      result = subprocess.run(['otool', '-l', binary], stdout=subprocess.PIPE)
      out = result.stdout.decode('utf-8', errors='replace')
      regex = re.findall(r'path (.+?) \(offset', out)
      for new_rpath in rpath_array:
        if not new_rpath in regex:
          subprocess.run(["install_name_tool", "-add_rpath", new_rpath, binary], check=True)

  # Ensure the same python from meson.build (GIMP_PYTHON_WITH_GI) is used by plugins
  # This is needed because GIMP_PYTHON_WITH_GI can not coincide with python3 from shebang
  # (on MacPorts, there is no python3 symlink, so we would misuse Xcode python3 without GI)
  different_python=False
  if sys.platform not in ['win32', 'cygwin'] and os.environ.get("GIMP_PYTHON_WITH_GI"):
    python_symlink = shutil.which("python3")
    pygobject_found=False
    if python_symlink and not os.path.samefile(python_symlink, os.environ.get("GIMP_PYTHON_WITH_GI")):
      result = subprocess.run([python_symlink,"-c","import sys, gi; version='3.0'; sys.exit(gi.check_version(version))"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
      pygobject_found = (result.returncode == 0)
    if not python_symlink or (python_symlink and not pygobject_found):
      different_python=True
      tmp_path = os.path.join(GIMP3_DIRECTORY, "tmp_python")
      os.makedirs(tmp_path, exist_ok=True)
      tmp_symlink = os.path.join(tmp_path, "python3")
      if not os.path.exists(tmp_symlink):
        os.symlink(os.environ.get("GIMP_PYTHON_WITH_GI"), tmp_symlink)
      os.environ["PATH"] = tmp_path + os.pathsep + os.environ.get("PATH", "")

  # Finally, run gimp!
  gimp_self = os.environ.get("GIMP_SELF_IN_BUILD")
  if "GIMP_DEBUG_SELF_WRAPPER" in os.environ and shutil.which("gdb"):
    gimp_debug_self = os.environ.get("GIMP_DEBUG_SELF_WRAPPER")
    sys.stderr.write(f"RUNNING: {os.path.relpath(gimp_debug_self)} --batch -x {os.environ['GIMP_GLOBAL_SOURCE_ROOT']}/tools/debug-in-build-gimp.py --args {os.path.relpath(gimp_self)} {' '.join(sys.argv[1:])}\n")
    subprocess.run([gimp_debug_self,"--return-child-result","--batch","-x",f"{os.environ['GIMP_GLOBAL_SOURCE_ROOT']}/tools/debug-in-build-gimp.py","--args", gimp_self] + sys.argv[1:], stdin=sys.stdin, check=True)
  else:
    sys.stderr.write(f"RUNNING: {os.path.relpath(gimp_self)} {' '.join(sys.argv[1:])}\n")
    subprocess.run([gimp_self] + sys.argv[1:],stdin=sys.stdin, check=True)

  # Undo GIMP_PYTHON_WITH_GI trick
  if sys.platform not in ['win32', 'cygwin'] and different_python:
    os.environ["PATH"] = os.pathsep.join([p for p in os.environ["PATH"].split(os.pathsep) if p != tmp_path])
    shutil.rmtree(tmp_path, ignore_errors=True)

  # Undo GIMP_TEMP_UPDATE_RPATH trick
  if "GIMP_TEMP_UPDATE_RPATH" in os.environ:
    for binary in binaries_to_patch:
      result = subprocess.run(['otool', '-l', binary], stdout=subprocess.PIPE)
      out = result.stdout.decode('utf-8', errors='replace')
      regex = re.findall(r'path (.+?) \(offset', out)
      for new_rpath in rpath_array:
        if new_rpath in regex:
          subprocess.run(["install_name_tool", "-delete_rpath", new_rpath, binary], check=True)
    cleanup(lock)

  # Clean-up the temporary config directory after each usage, yet making sure we
  # don't get tricked by weird redirections or anything of the sort. In particular
  # we check that this is a directory with user permission, not a symlink, and
  # that it's inside inside the project build's root.
  if "GIMP3_DIRECTORY" in os.environ and os.path.isdir(GIMP3_DIRECTORY):
    if os.path.islink(GIMP3_DIRECTORY):
      sys.stderr.write(f"ERROR: $GIMP3_DIRECTORY ({GIMP3_DIRECTORY}) should not be a symlink.\n")
      sys.exit(1)
    used_dir_prefix = str(Path(GIMP3_DIRECTORY).resolve().as_posix())[:-6]
    tmpl_dir_prefix = f"{Path(os.environ['GIMP_GLOBAL_BUILD_ROOT']).resolve().as_posix()}/.GIMP3-build-config-"
    if used_dir_prefix != tmpl_dir_prefix:
      sys.stderr.write(f"ERROR: $GIMP3_DIRECTORY ({GIMP3_DIRECTORY}) should be under the build directory with a specific prefix.\n")
      sys.stderr.write(f'       "{used_dir_prefix}" != "{tmpl_dir_prefix}"\n')
      sys.exit(1)
    sys.stderr.write(f"INFO: Running: shutil.rmtree({os.path.relpath(GIMP3_DIRECTORY)})\n")
    # This weird retry loop is because Windows may still keep a hold to
    # plug-in executable a bit longer than the process or even parent
    # process life. See MR !2837.
    attempts = max_attempts = 3
    while attempts > 0:
      try:
        shutil.rmtree(GIMP3_DIRECTORY)
        break
      except Exception as e:
        attempts -= 1
        if attempts == 0:
          sys.stderr.write(f"Tree removal error after {max_attempts} attempts: {str(e)}\n")
          sys.exit(1)
        else:
          time.sleep(1)
  elif not os.access(GIMP3_DIRECTORY, os.W_OK):
    sys.stderr.write(f"ERROR: $GIMP3_DIRECTORY ({GIMP3_DIRECTORY}) does not belong to the user\n")
    sys.exit(1)
  else:
    sys.stderr.write(f"ERROR: $GIMP3_DIRECTORY ({GIMP3_DIRECTORY}) is not a directory\n")
    sys.exit(1)

except subprocess.CalledProcessError as e:
  cleanup(lock)
  sys.stderr.write(f"Command failed with exit code {e.returncode}: {e.cmd}\n")
  sys.exit(e.returncode)
except Exception as e:
  cleanup(lock)
  sys.stderr.write(f"Error: {str(e)}\n")
  sys.exit(1)
