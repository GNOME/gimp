#!/usr/bin/env python3
import os
import subprocess
from pathlib import Path

os.chdir(Path(os.getenv("MESON_SOURCE_ROOT",".")))
result = subprocess.run(['git', 'status', '--untracked-files=no', '--short', '--ignore-submodules=dirty', 'gimp-data'], capture_output=True, text=True, check=True)
exit(len(result.stdout.splitlines()))
