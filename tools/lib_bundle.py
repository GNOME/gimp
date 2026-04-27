#!/usr/bin/python3

################################################################################
# Small python script to retrieve DLL or DYLIB dependencies with object dumper
################################################################################

################################################################################
# Usage example
#
# python3 lib_bundle.py /path/to/run.exe /winenv/ /path/install
#
# In this case, the DLL dependencies for executable run.exe will be extracted
# and copied into /path/install/bin folder. To copy the DLL, the root path to
# Windows environnment should be passed, here /winenv/ (usually MSYSTEM_PREFIX).
#
# python3 lib_bundle.py /path/to/run /macOSenv/ /path/install
#
# In this case, the DYLIB dependencies for executable run will be extracted
# and copied into /path/install/Frameworks folder. To copy the DYLIB,
# the root path to macOS environnment should be passed, here /macOSenv/ (usually /opt/local).

import argparse
import os
import subprocess
import re
import shutil
import sys
import struct

################################################################################
# Global variables

# Sets for executable and system DLL/DYLIB
dlls = set()
sys_dlls = set()

# Previously done DLLs/DYLIBs in previous runs
done_dlls = set()

# Common paths
bindir = 'bin'

# Platform mode (detected at runtime)
is_macos = False

################################################################################
# Functions

# Main function
def main(binary, srcdirs, destdir, debug, dll_file):
  global done_dlls
  global is_macos
  try:
    if dll_file is not None:
      with open(dll_file, 'r') as f:
        done_dlls = { line.strip() for line in f if len(line.strip()) != 0 }
  except FileNotFoundError:
    pass

  #sys.stdout.write("{} (INFO): searching for dependencies of {} in {}.\n".format(os.path.basename(__file__),
  #                                                                               binary, ', '.join(srcdirs)))
  find_dependencies(os.path.abspath(binary), srcdirs)
  if debug in ['debug-only', 'debug-run']:
    if debug == 'debug-only':
      print("Running in debug-only mode (no DLL/DYLIB moved) for '{}'".format(binary))
    else:
      print("Running with debug output for '{}'".format(binary))

    if len(dlls) > 0:
      sys.stdout.write("Needed DLLs/DYLIBs:\n\t- ")
      sys.stdout.write("\n\t- ".join(list(dlls)))
      print()
    if len(sys_dlls) > 0:
      sys.stdout.write('System DLLs/DYLIBs:\n\t- ')
      sys.stdout.write("\n\t- ".join(sys_dlls))
      print()
    removed_dlls = sys_dlls & dlls
    if len(removed_dlls) > 0:
      sys.stdout.write('Non installed DLLs/DYLIBs:\n\t- ')
      sys.stdout.write("\n\t- ".join(removed_dlls))
      print()
    installed_dlls = dlls - sys_dlls
    if len(installed_dlls) > 0:
      sys.stdout.write('Installed DLLs/DYLIBs:\n\t- ')
      sys.stdout.write("\n\t- ".join(installed_dlls))
      print()

    if debug == 'debug-run':
      copy_dlls(dlls - sys_dlls, srcdirs, destdir)
  else:
    copy_dlls(dlls - sys_dlls, srcdirs, destdir)

  if dll_file is not None:
    with open(dll_file, 'w') as f:
      f.write("\n".join(set.union(done_dlls, dlls, sys_dlls)))

def find_dependencies(obj, srcdirs):
  '''
  List DLLs/DYLIBs of an object file in a recursive way.
  '''
  global is_macos
  if obj in set.union(done_dlls, sys_dlls):
    # Already processed, either in a previous run of the script
    # (done_dlls) or in this one.
    return True

  if not os.path.isabs(obj):
    for srcdir in srcdirs:
      bindir = 'bin'
      if is_macos:
        bindir = 'lib'
      abs_dll = os.path.join(srcdir, bindir, obj)
      abs_dll = os.path.abspath(abs_dll)
      if find_dependencies(abs_dll, srcdirs):
        return True
    else:
      # Found in none of the srcdirs: consider it a system DLL/DYLIB
      sys_dlls.add(os.path.basename(obj))
      return False
  elif os.path.exists(obj):
    # If DLL/DYLIB exists, extract dependencies.
    objdump = None

    try:
      with open(obj, 'rb') as f:
        f.seek(0x3C)
        pe_offset = struct.unpack('<I', f.read(4))[0]
        f.seek(pe_offset)
        pe_sig = f.read(4)
        if pe_sig == b'PE\0\0':
          f.seek(pe_offset + 24)
          file_type = struct.unpack('<H', f.read(2))[0]
          if file_type == 0x20b:
            objdump = 'x86_64-w64-mingw32-objdump'
          elif file_type == 0x10b:
            objdump = 'i686-w64-mingw32-objdump'
          else:
            return None
        else:
          # If not PE, then check for Mach-O magic (Feed Face/Face Feed variants)
          f.seek(0) #rewind to start
          magic = f.read(4)
          if magic in [b'\xfe\xed\xfa\xce', b'\xfe\xed\xfa\xcf',
                       b'\xce\xfa\xed\xfe', b'\xcf\xfa\xed\xfe',
                       b'\xca\xfe\xba\xbe', b'\xbe\xba\xfe\xca']:
            is_macos = True
            objdump = 'no_cross_objdump'
          else:
            return None
    except Exception as e:
      sys.stderr.write("Cannot read PE/Mach-O header for {}: {}\n".format(obj, e))
      return None

    if objdump is None:
      sys.stderr.write('File type of {} unknown: {}\n'.format(obj, file_type))
      sys.exit(1)
    elif shutil.which(objdump) is None:
      # For native objdump case.
      objdump = 'objdump'
      if not is_macos and shutil.which('dumpbin'):
        objdump = 'dumpbin'
      if is_macos and shutil.which('otool'):
        objdump = 'otool'

    if shutil.which(objdump) is None:
      sys.stderr.write("Executable doesn't exist: {}\n".format(objdump))
      sys.exit(1)

    if not is_macos:
      if objdump == 'dumpbin':
        result = subprocess.run([objdump, '/DEPENDENTS', obj], stdout=subprocess.PIPE)
        out = result.stdout.decode('utf-8')
        regex = re.finditer(r'\b([\w\-.]+\.dll)\b', out, re.MULTILINE)
      else:
        result = subprocess.run([objdump, '-p', obj], stdout=subprocess.PIPE)
        out = result.stdout.decode('utf-8')
        regex = re.finditer(r"DLL Name: *(\S+.dll)", out, re.MULTILINE)
    else:
      if objdump == 'otool':
        result = subprocess.run(['otool', '-l', obj], stdout=subprocess.PIPE)
      else:
        result = subprocess.run(['objdump', '--macho', '--private-headers', obj], stdout=subprocess.PIPE)
      out = result.stdout.decode('utf-8', errors='replace')
      regex = re.finditer(r"name\s+(?:\d+\s+)?(?:.*/)?([^/\s]+\.dylib)", out, re.MULTILINE)
    # Parse lines with DLL/DYLIB Name instead of lib*.dll/lib*.dylib directly
    for match in regex:
      dll = match.group(1)
      if dll not in set.union(done_dlls, dlls, sys_dlls):
        dlls.add(dll)
        find_dependencies(dll, srcdirs)

    return True
  else:
    return False

# Copy a DLL/DYLIB set into the /destdir/bin or /destdir/Frameworks directory
def copy_dlls(dll_list, srcdirs, destdir):
  global bindir
  dest_bindir = bindir
  if is_macos:
    dest_bindir = 'Frameworks'
  destbin = os.path.join(destdir, dest_bindir)
  os.makedirs(destbin, exist_ok=True)
  if is_macos:
    abs_binary = os.path.abspath(args.bin)
    abs_destdir = os.path.abspath(destdir)
    if abs_binary.startswith(abs_destdir + os.sep):
      #check_macos_version(abs_binary)
      set_rpath(abs_binary, destbin)
  for dll in dll_list:
    if not os.path.exists(os.path.join(destbin, dll)):
      # Do not overwrite existing files.
      for srcdir in srcdirs:
        if is_macos:
          bindir = 'lib'
        full_file_name = os.path.join(srcdir, bindir, dll)
        if os.path.isfile(full_file_name):
          if is_macos and not full_file_name.startswith(("/opt/local/", "/opt/homebrew/")):
            check_macos_version(full_file_name)
          sys.stdout.write("Bundling {} to {}\n".format(full_file_name, destbin))
          shutil.copy(full_file_name, destbin)
          if is_macos:
            set_rpath(os.path.join(destbin, dll), destbin)
          break
      else:
        # This should not happen. We determined that the dll is in one
        # of the srcdirs.
        sys.stderr.write("Missing DLL/DYLIB: {}\n".format(dll))
        sys.exit(1)

def check_macos_version(binary):
  """
  Check LC_BUILD_VERSION for compatibility with MACOSX_DEPLOYMENT_TARGET
  """
  supported_minos = os.environ.get("MACOSX_DEPLOYMENT_TARGET")
  if not supported_minos:
    return

  result = subprocess.run(['otool', '-l', binary], stdout=subprocess.PIPE, check=True)
  out = result.stdout.decode('utf-8', errors='replace')

  bin_minos = re.findall(r'minos\s+([\d\.]+)', out)[0]
  if tuple(map(int, bin_minos.split('.'))) > tuple(map(int, supported_minos.split('.'))):
    sys.stderr.write(f"\033[31m(ERROR)\033[0m: {binary} requires macOS {bin_minos}, which is higher than macOS {supported_minos} which GIMP was built against.\n")
    sys.exit(1)

def set_rpath(binary, destbin=None):
  """
  Remove all LC_RPATH entries except those identical to relative destbin,
  and set it if not. Also rewrite LC_LOAD_DYLIB and LC_ID_DYLIB to use it.
  """
  result = subprocess.run(['otool', '-l', binary], stdout=subprocess.PIPE)
  out = result.stdout.decode('utf-8', errors='replace')

  # Handle LC_RPATH (only on executables as a rule)
  if (".dylib" not in binary and ".so" not in binary) or "cmd LC_RPATH" in out:
    regex = re.findall(r'path (.+?) \(offset', out)
    new_rpath = os.path.join("@executable_path", os.path.relpath(destbin, os.path.dirname(os.path.abspath(binary))))
    for old_rpath in regex:
      if old_rpath == new_rpath:
        continue
      try:
        subprocess.run(['install_name_tool', '-delete_rpath', old_rpath, binary], check=True, stderr=subprocess.DEVNULL)
        #sys.stdout.write(f"Removed LC_RPATH {old_rpath} from {binary}\n")
      except subprocess.CalledProcessError as e:
        sys.stderr.write(f"Failed to remove rpath {old_rpath} from {binary}: {e}\n")
    if new_rpath not in regex:
      try:
        subprocess.run(['install_name_tool', '-add_rpath', new_rpath, binary], check=True, stderr=subprocess.DEVNULL)
        #sys.stdout.write(f"Added LC_RPATH {new_rpath} to {binary}\n")
      except subprocess.CalledProcessError as e:
        sys.stderr.write(f"Failed to add rpath {new_rpath} to {binary}: {e}\n")

  # Handle LC_LOAD_DYLIB (on executables, shared libraries)
  regex = re.findall(r'name (.+?) \(offset', out)
  for old_dylib_path in regex:
    if old_dylib_path.startswith("/usr") or old_dylib_path.startswith("/System"):
      continue
    if ".framework" in old_dylib_path:
      new_dylib_path = os.path.join("@rpath", old_dylib_path[old_dylib_path.rfind("/", 0, old_dylib_path.find(".framework")) + 1:])
    else:
      new_dylib_path = os.path.join("@rpath", os.path.basename(old_dylib_path))
    if old_dylib_path != new_dylib_path:
      try:
        subprocess.run(['install_name_tool', '-change', old_dylib_path, new_dylib_path, binary], check=True, stderr=subprocess.DEVNULL)
        #sys.stdout.write(f"Rewrote LC_LOAD_DYLIB {old_dylib_path} -> {new_dylib_path}\n")
      except subprocess.CalledProcessError as e:
        sys.stderr.write(f"Failed to rewrite LC_LOAD_DYLIB {old_dylib_path}: {e}\n")

  # Handle LC_ID_DYLIB (only on shared libraries)
  regex = re.search(r'cmd LC_ID_DYLIB.*?\n\s*name (.+?) \(offset', out, re.DOTALL)
  if (".dylib" in binary or ".so" in binary) and regex:
    old_dylib_path = regex.group(1).strip()
    new_dylib_path = os.path.join("@rpath", os.path.basename(old_dylib_path))
    if old_dylib_path != new_dylib_path:
      try:
        subprocess.run(['install_name_tool', '-id', new_dylib_path, binary], check=True, stderr=subprocess.DEVNULL)
        #sys.stdout.write(f"Rewrote LC_ID_DYLIB {old_dylib_path} -> {new_dylib_path}\n")
      except subprocess.CalledProcessError as e:
        sys.stderr.write(f"Failed to rewrite LC_ID_DYLIB {old_dylib_path}: {e}\n")

  # Normalize signature after all the changes
  result = subprocess.run(["codesign", "-v", binary], capture_output=True, text=True)
  if "invalid signature" in result.stdout + result.stderr:
    try:
      subprocess.run(["codesign", "--sign", "-", "--force", "--preserve-metadata=entitlements,requirements,flags,runtime", binary], check=True, stderr=subprocess.DEVNULL)
      #sys.stdout.write(f"Re-signed {binary}\n")
    except subprocess.CalledProcessError as e:
      sys.stderr.write(f"Failed to ad-hoc sign {binary}: {e}\n")


if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument('--debug', dest='debug',
                      choices=['debug-only', 'debug-run', 'run-only'], default = 'run-only')
  parser.add_argument('--output-dll-list', dest='dll_file', action = 'store', default = None)
  parser.add_argument('bin')
  parser.add_argument('src', nargs='+')
  parser.add_argument('dest')
  args = parser.parse_args(sys.argv[1:])

  main(args.bin, args.src, args.dest, args.debug, args.dll_file)
