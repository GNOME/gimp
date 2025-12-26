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
# and copied into /path/install/Contents/Resources/lib folder. To copy the DYLIB,
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
      objdump = 'objdump.exe'
      if is_macos and shutil.which('otool'):
        objdump = 'otool'
      else:
        objdump = 'objdump'

    if shutil.which(objdump) is None:
      sys.stderr.write("Executable doesn't exist: {}\n".format(objdump))
      sys.exit(1)

    if not is_macos:
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

# Copy a DLL/DYLIB set into the /destdir/bin or /destdir/Contents/Resource/lib directory
def copy_dlls(dll_list, srcdirs, destdir):
  global bindir
  dest_bindir = bindir
  if is_macos:
    dest_bindir = 'Contents/Resources/lib'
  destbin = os.path.join(destdir, dest_bindir)
  os.makedirs(destbin, exist_ok=True)
  for dll in dll_list:
    if not os.path.exists(os.path.join(destbin, dll)):
      # Do not overwrite existing files.
      for srcdir in srcdirs:
        if is_macos:
          bindir = 'lib'
        full_file_name = os.path.join(srcdir, bindir, dll)
        if os.path.isfile(full_file_name):
          sys.stdout.write("Bundling {} to {}\n".format(full_file_name, destbin))
          shutil.copy(full_file_name, destbin)
          break
      else:
        # This should not happen. We determined that the dll is in one
        # of the srcdirs.
        sys.stderr.write("Missing DLL/DYLIB: {}\n".format(dll))
        sys.exit(1)

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
