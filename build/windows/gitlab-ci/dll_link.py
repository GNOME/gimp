#!/usr/bin/python3

################################################################################
# Small python script to retrieve DLL dependencies with objdump
################################################################################

################################################################################
# Usage example
#
# python3 dll_link.py /path/to/run.exe /winenv/ /path/install
#
# In this case, the DLL dependencies for executable run.exe will be extracted
# and copied into /path/install/bin folder. To copy the DLL, the root path to
# Windows environnment should be passed, here /winenv/.

import argparse
import os
import subprocess
import re
import shutil
import sys

################################################################################
# Global variables

# Sets for executable and system DLL
dlls = set()
sys_dlls = set()

# Common paths
bindir = 'bin'

################################################################################
# Functions

# Main function
def main(binary, srcdirs, destdir, debug):
  sys.stdout.write("{} (INFO): searching for dependencies of {} in {}.\n".format(os.path.basename(__file__),
                                                                                 binary, ', '.join(srcdirs)))
  find_dependencies(os.path.abspath(binary), srcdirs)
  if args.debug:
    print("Running in debug mode (no DLL moved)")
    if len(dlls) > 0:
      sys.stdout.write("Needed DLLs:\n\t- ")
      sys.stdout.write("\n\t- ".join(list(dlls)))
      print()
    if len(sys_dlls) > 0:
      sys.stdout.write('System DLLs:\n\t- ')
      sys.stdout.write("\n\t- ".join(sys_dlls))
      print()
    removed_dlls = sys_dlls & dlls
    if len(removed_dlls) > 0:
      sys.stdout.write('Non installed DLLs:\n\t- ')
      sys.stdout.write("\n\t- ".join(removed_dlls))
      print()
    installed_dlls = dlls - sys_dlls
    if len(installed_dlls) > 0:
      sys.stdout.write('Installed DLLs:\n\t- ')
      sys.stdout.write("\n\t- ".join(installed_dlls))
      print()
  else:
    copy_dlls(dlls - sys_dlls, srcdirs, destdir)

def find_dependencies(obj, srcdirs):
  '''
  List DLLs of an object file in a recursive way.
  '''
  if not os.path.isabs(obj):
    for srcdir in srcdirs:
      abs_dll = os.path.join(srcdir, bindir, obj)
      abs_dll = os.path.abspath(abs_dll)
      if find_dependencies(abs_dll, srcdirs):
        return True
    else:
      # Found in none of the srcdirs: consider it a system DLL
      sys_dlls.add(os.path.basename(obj))
      return False
  elif os.path.exists(obj):
    # If DLL exists, extract dependencies.
    objdump = None

    result = subprocess.run(['file', obj], stdout=subprocess.PIPE)
    file_type = result.stdout.decode('utf-8')
    if 'PE32+' in file_type:
      objdump = 'x86_64-w64-mingw32-objdump'
    elif 'PE32' in file_type:
      objdump = 'i686-w64-mingw32-objdump'

    if objdump is None:
      sys.stderr.write('File type of {} unknown: {}\n'.format(obj, file_type))
      sys.exit(1)
    elif shutil.which(objdump) is None:
      # For native objdump case.
      objdump = 'objdump.exe'

    if shutil.which(objdump) is None:
      sys.stderr.write("Executable doesn't exist: {}\n".format(objdump))
      sys.exit(1)

    result = subprocess.run([objdump, '-p', obj], stdout=subprocess.PIPE)
    out = result.stdout.decode('utf-8')
    # Parse lines with DLL Name instead of lib*.dll directly
    for match in re.finditer(r"DLL Name: *(\S+.dll)", out, re.MULTILINE):
      dll = match.group(1)
      if dll not in dlls and os.path.basename(dll) not in sys_dlls:
        dlls.add(dll)
        find_dependencies(dll, srcdirs)

    return True
  else:
    return False

# Copy a DLL set into the /destdir/bin directory
def copy_dlls(dll_list, srcdirs, destdir):
  destbin = os.path.join(destdir, bindir)
  os.makedirs(destbin, exist_ok=True)
  for dll in dll_list:
    if not os.path.exists(os.path.join(destbin, dll)):
      # Do not overwrite existing files.
      for srcdir in srcdirs:
        full_file_name = os.path.join(srcdir, bindir, dll)
        if os.path.isfile(full_file_name):
          sys.stdout.write("{} (INFO): copying {} to {}\n".format(os.path.basename(__file__), full_file_name, destbin))
          shutil.copy(full_file_name, destbin)
          break
      else:
        # This should not happen. We determined that the dll is in one
        # of the srcdirs.
        sys.stderr.write("Missing DLL: {}\n".format(dll))
        sys.exit(1)

if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument('--debug', dest='debug', action = 'store_true', default = False)
  parser.add_argument('bin')
  parser.add_argument('src', nargs='+')
  parser.add_argument('dest')
  args = parser.parse_args(sys.argv[1:])

  main(args.bin, args.src, args.dest, args.debug)
