#!/usr/bin/env python3
# macOS-only script to generate temporary .gir then .typelib files only to be
# used during build, pointing to the non-installed libgimp* .dylib libraries.
# This is needed because .typelib have path references to .dylib so we patch in
# a similar way we path GIMP_TEMP_UPDATE_RPATH binaries from tools/in-build-gimp.py
from datetime import datetime
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

# Arguments to this script:
gir_name = os.path.basename(sys.argv[1])
typelib_name = os.path.basename(sys.argv[2])
builddir = sys.argv[3]
prefix = sys.argv[4]

print("PWD:", os.getcwd())
print("ARGS:", " ".join(sys.argv[1:]))

# This is only for macOS (see the rationale in the comment above).
tmp_gir_dir = Path(builddir) / "tmp"
tmp_gir_dir.mkdir(parents=True, exist_ok=True)
tmp_gir_path = Path(f"{tmp_gir_dir}/{gir_name}")
shutil.copy2(Path(f"{builddir}/{gir_name}"), tmp_gir_path)

text = Path(tmp_gir_path).read_text()
text = re.sub(r'shared-library="([^"]+)"', lambda m: 'shared-library="' + ",".join("libgimp/" + os.path.basename(p) for p in m.group(1).split(",")) + '"', text)
Path(tmp_gir_path).write_text(text)
subprocess.run(["g-ir-compiler", f"--includedir={tmp_gir_dir}", f"--includedir={prefix}/share/gir-1.0/", str(tmp_gir_path), "-o", tmp_gir_dir / typelib_name], check=True)

stamp_path = Path(f"{builddir}/{gir_name}-macos-typelib.stamp")
stamp_path.write_text(f"/* Generated on {datetime.now().strftime('%c')}. */\n")
