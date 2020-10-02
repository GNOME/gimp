#!/usr/bin/python3

################################################################################
# Small python script to retrieve DLL depencies with objdump
################################################################################

################################################################################
# Usage example
#
# python3 dll_link.py /path/to/run.exe /winenv/ /path/install
#
# In this case, the DLL depencies for executable run.exe will be extracted and
# copied into /path/install/bin folder. To copy the DLL, the root path to
# Windows environnement should be passed, here /winenv/.

import sys
import os
import subprocess
import re
import string
import shutil
from pathlib import Path

################################################################################
# Global variables

# Sets for executable and system DLL
dll_siril_set = set()
dll_sys_set = set()

# Install prefix
prefix = ''

# Windows environement root
basedir = ''

# Common paths
binary_dir = '/bin/'
lib_dir = '/lib/'
etc_dir = '/etc/'
share_dir = '/share/'

################################################################################
# Functions

# Main function
def main():
  global basedir
  global prefix

  if len(sys.argv) < 4:
    exit(1)

  filename = sys.argv[1]
  basedir = sys.argv[2]
  prefix = sys.argv[3]

  recursive(filename)
  copy_dll(dll_siril_set-dll_sys_set)

# List DLL of an executable file in a recursive way
def recursive(filename):
  # Check if DLL exist in /bin folder, if true extract depencies too.
  if os.path.exists(filename):
    objdump = None

    result = subprocess.run(['file', filename], stdout=subprocess.PIPE)
    file_type = result.stdout.decode('utf-8')
    if 'PE32+' in file_type:
      objdump = 'x86_64-w64-mingw32-objdump'
    elif 'PE32' in file_type:
      objdump = 'i686-w64-mingw32-objdump'

    if objdump is None:
      sys.stderr.write('File type of {} unknown: {}\n'.format(filename, file_type))
      sys.exit(os.EX_UNAVAILABLE)

    result = subprocess.run([objdump, '-p', filename], stdout=subprocess.PIPE)
    out = result.stdout.decode('utf-8')
    # Parse lines with DLL Name instead of lib*.dll directly
    items = re.findall(r"DLL Name: \S+.dll", out, re.MULTILINE)
    for x in items:
      x = x.split(' ')[2]
      l = len(dll_siril_set)
      dll_siril_set.add(x)
      if len(dll_siril_set) > l:
        new_dll = basedir + binary_dir + x
        recursive(new_dll)
  # Otherwise, it is a system DLL
  else:
    dll_sys_set.add(os.path.basename(filename))

# Copy a DLL set into the /prefix/bin directory
def copy_dll(dll_list):
  for file in dll_list:
    full_file_name = os.path.join(basedir + binary_dir, file)
    if os.path.isfile(full_file_name):
      shutil.copy(full_file_name, prefix+binary_dir)


if __name__ == "__main__":
  main()
