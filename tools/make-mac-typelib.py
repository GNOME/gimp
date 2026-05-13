#!/usr/bin/env python3
# macOS-only script to generate/patch .typelib files.
# Modes:
# - 'in-build': generates a temporary .typelib pointing to non-installed libs.
# - 'nonreloc': overwrites the .typelib prepending the install prefix.
# - 'reloc': overwrites the .typelib prepending '@rpath/'.
from datetime import datetime
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

# Arguments to this script:
gir_path = Path(sys.argv[1])
gir_name = gir_path.name
typelib_path = Path(sys.argv[2])
typelib_name = typelib_path.name
builddir = sys.argv[3]
prefix = sys.argv[4]
compiler = sys.argv[5]
mode = sys.argv[6] if len(sys.argv) > 6 else "in-build"

print("ARGS:", " ".join(sys.argv[1:]))

# This is only for macOS (see the rationale in the comment above).
tmp_gir_dir = Path(builddir) if mode not in ["in-build"] else Path(builddir) / "tmp"
tmp_gir_dir.mkdir(parents=True, exist_ok=True)
tmp_gir_path = Path(f"{tmp_gir_dir}/{gir_name}")
if mode in ["in-build"]:
  shutil.copy2(gir_path, tmp_gir_path)

def get_lib_path(p):
  name = os.path.basename(p)
  if mode == "nonreloc":
    return f"{prefix}/lib/{name}"
  elif mode == "reloc":
    return f"@rpath/{name}"
  else: # "in-build"
    folder = name.split('-')[0]
    #Exception: libgimpui is built inside the 'libgimp' directory
    if folder == "libgimpui":
      folder = "libgimp"
    return f"{folder}/{name}"

text = Path(tmp_gir_path).read_text()
text = re.sub(r'shared-library="([^"]+)"', lambda m: 'shared-library="' + ",".join(get_lib_path(p) for p in m.group(1).split(",")) + '"', text)
Path(tmp_gir_path).write_text(text)

output_typelib = typelib_path if mode not in ["in-build"] else tmp_gir_dir / typelib_name
subprocess.run([compiler, f"--includedir={tmp_gir_dir}", f"--includedir={prefix}/share/gir-1.0/", str(tmp_gir_path), "-o", str(output_typelib)], check=True)

stamp_path = Path(f"{builddir}/{gir_name}-macos-typelib-{mode}.stamp")
stamp_path.write_text(f"/* Generated on {datetime.now().strftime('%c')}. */\n")
