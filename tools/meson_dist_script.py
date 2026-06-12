#!/usr/bin/env python3
import os
import subprocess
import sys
import shutil

GIT_VERSION_H = sys.argv[1]

MESON_SOURCE_ROOT = os.environ.get('MESON_SOURCE_ROOT', sys.argv[2])
MESON_BUILD_ROOT = os.environ.get('MESON_BUILD_ROOT', sys.argv[3])

# `meson dist` doesn't trigger a build, which is a problem because we
# need to add some configured/built files. The case where we didn't
# build at all is the less annoying (we'd get a failure, which is
# actually acceptable as it warns us). But the case where we would copy
# outdated data is much more insidious (such as wrong INSTALL
# information or wrong git information) as it would be silent.
# See: https://github.com/mesonbuild/meson/issues/10650
# And: https://gitlab.gnome.org/GNOME/gimp/-/issues/7907
# This is why we manually trigger, not only a reconfigure, but also a
# rebuild of the main project before copying data around.

try:
  # INSTALL file is generated at configure time.
  subprocess.run(['meson', '--reconfigure', MESON_SOURCE_ROOT], check=True)
  # git-version.h is generated at build time.
  subprocess.run(['meson', 'compile'], check=True)

  shutil.copy2('INSTALL', os.environ['MESON_DIST_ROOT'])
  install_in = os.path.join(os.environ['MESON_DIST_ROOT'], 'INSTALL.in')
  if os.path.exists(install_in):
    os.remove(install_in)

  shutil.copy2(GIT_VERSION_H, os.environ['MESON_DIST_ROOT'])

except subprocess.CalledProcessError as e:
  sys.exit(e.returncode)
except Exception:
  sys.exit(1)
