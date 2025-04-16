#!/usr/bin/env python3
import os
import glob
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

MESON_BUILD_ROOT = Path(os.environ.get("MESON_BUILD_ROOT",".")).as_posix() 
MESON_SOURCE_ROOT = Path(os.environ.get("MESON_SOURCE_ROOT",sys.argv[2])).as_posix() 

## Get list of Inno and GIMP supported languages
po_inno_files = sorted(glob.glob(os.path.join(MESON_SOURCE_ROOT,'po-windows-installer/*.po')))
po_inno_array = [re.sub(r'^po-windows-installer/|\.po$', '', f) for f in po_inno_files]
po_files = sorted(glob.glob(os.path.join(MESON_SOURCE_ROOT,'po/*.po')))
po_array = [re.sub(r'^po/|\.po$', '', f) for f in po_files]

## Get strings for GIMP and Inno supported languages
xml_file = os.path.join(MESON_SOURCE_ROOT, 'build/windows/installer/lang/iso_639_custom.xml')
tree = ET.parse(xml_file)
root = tree.getroot()


## Create list of lang [Languages]
if sys.argv[1] == 'msg':
  msg_list = []
  for po in po_inno_array:
    # Change po
    po = po.replace('\\', '').replace('//', '').replace('..', '').replace('po-windows-installer', '')
    # Change isl
    inno_code = None
    for entry in root.findall('.//iso_639_entry'):
      if entry.get('dl_code') == po:
        inno_code = entry.get('inno_code').replace('\\\\', '\\')
        break
    # Create line
    if inno_code is not None:
      msg_line = f'Name: "{po}"; MessagesFile: "compiler:{inno_code},{{#ASSETS_DIR}}\\lang\\{po}.setup.isl"'
      msg_list.append(msg_line)
  output_file = os.path.join(MESON_BUILD_ROOT, 'build/windows/installer/base_po-msg.list')
  with open(output_file, 'w', encoding='utf-8') as f:
    f.write('\n'.join(msg_list))

## Create list of lang [Components]
elif sys.argv[1] == 'cmp':
  cmp_list = []
  for po in po_array:
    # Change po
    po = po.replace('\\', '').replace('//', '').replace('..', '').replace('po', '')
    po_clean = po.replace('@', '_')
    # Change desc
    desc = None
    for entry in root.findall('.//iso_639_entry'):
      if entry.get('dl_code') == po:
        desc = entry.get('name')
        break
    # Create line
    if desc is not None:
      cmp_line = f'Name: loc\\{po_clean}; Description: "{desc}"; Types: full custom'
      cmp_list.append(cmp_line)
  output_file = os.path.join(MESON_BUILD_ROOT, 'build/windows/installer/base_po-cmp.list')
  with open(output_file, 'w', encoding='utf-8') as f:
    f.write('\n'.join(cmp_list))

## Create list of lang [Files]
elif sys.argv[1] == 'files':
  files_list = []
  for po in po_array:
    # Change po
    po = po.replace('\\', '').replace('//', '').replace('..', '').replace('po', '')
    po_clean = po.replace('@', '_')
    # Create line
    files_line = f'Source: "{{#GIMP_DIR32}}\\share\\locale\\{po}\\*"; DestDir: "{{app}}\\share\\locale\\{po}"; Components: loc\\{po_clean}; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion'
    files_list.append(files_line)
  output_file = os.path.join(MESON_BUILD_ROOT, 'build/windows/installer/base_po-files.list')
  with open(output_file, 'w') as f:
    f.write('\n'.join(files_list))
