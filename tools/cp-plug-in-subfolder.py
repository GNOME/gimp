#!/usr/bin/python3

# Equivalent to:
# configure_file(input: src,
#                output: name / src,
#                copy: true,
#                install_dir: gimpplugindir / 'plug-ins' / name,
#                install_mode: 'rwxr-xr-x')
# Except that configure_file() does not accept output in a subdirectory. So we
# use this wrapper for now.
# See: https://github.com/mesonbuild/meson/issues/2320
import os
import shutil
import stat
import sys

src_file = sys.argv[1]
dir_name = sys.argv[2]
dummy_path = None
if len(sys.argv) > 3:
  dummy_path = sys.argv[3]

os.makedirs(dir_name, exist_ok=True)

file_name = os.path.basename(src_file)
dst_file  = os.path.join(dir_name, file_name)
shutil.copyfile(src_file, dst_file)
os.chmod(dst_file, stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)

if dummy_path is not None:
  # Just touch the dummy file.
  open(dummy_path, mode='w').close()
