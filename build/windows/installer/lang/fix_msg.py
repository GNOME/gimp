#!/usr/bin/python3
import os
import subprocess
import sys
import re
try:
  import charset_normalizer
except ImportError:
  if not os.getenv("MSYSTEM_PREFIX"):
    subprocess.check_call([sys.executable, '-m', 'pip', 'install', 'charset_normalizer'])
  else:
    MINGW_PACKAGE_PREFIX = os.getenv("MINGW_PACKAGE_PREFIX") or ("mingw-w64-clang-aarch64" if os.getenv("MSYSTEM_PREFIX") == "clangarm64" else "mingw-w64-clang-x86_64")
    subprocess.check_call(['powershell', 'pacman', '--noconfirm', '-S', '--needed', f"{MINGW_PACKAGE_PREFIX}-python-charset-normalizer"])
finally:
  from charset_normalizer import detect

langfilePath = sys.argv[1]
AppVer = sys.argv[2]

# Detect the encoding of the file
with open(langfilePath, 'rb') as file:
  raw_data = file.read()
detected_encoding = detect(raw_data)['encoding']
print(f"(INFO): temporarily patching {detected_encoding} {langfilePath} with {AppVer}")

# Read the file content with detected encoding
with open(langfilePath, 'r', encoding=detected_encoding) as file:
  lines = file.readlines()

# Patch 'SetupWindowTitle' and 'UninstallAppFullTitle'
for i, line in enumerate(lines):
  if 'SetupWindowTitle' in line:
    before = line.strip()
    after = re.sub(r'%1', f'%1 {AppVer}', before)
    lines[i] = line.replace(before, after)
  if 'UninstallAppFullTitle' in line:
    before = line.strip()
    after = re.sub(r'%1', f'%1 {AppVer}', before)
    lines[i] = line.replace(before, after)

# Write the patched content back to the file using the detected encoding
with open(langfilePath, 'w', encoding=detected_encoding) as file:
  file.writelines(lines)
