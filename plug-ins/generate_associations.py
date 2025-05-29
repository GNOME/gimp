#!/usr/bin/env python3
import sys
import os
import re

associations = set()

#Read file loading plug-ins sourcecode
source_files = sys.argv[2:]
for source_file in source_files:
  try:
    with open(source_file, 'r', encoding='utf-8') as f:
      content = f.read()
  except Exception as e:
    sys.stderr.write(f"(WARNING): Unable to open file {source_file}: {e}\n")
    continue

  #Parse extensions declared in the sourcecode
  source_file_ext = os.path.splitext(source_file)[1].lower()
  if source_file_ext == '.c':
    if "LOAD_PROC" not in content and "load_procedure" not in content:
      continue
    regex = (r'gimp_file_procedure_set_extensions\s*'
             r'\(\s*GIMP_FILE_PROCEDURE\s*\(\s*procedure\s*\)\s*,\s*"([^"]+)"')
  elif source_file_ext == '.py':
    if "LoadProcedure" not in content:
      continue
    regex = r'procedure\.set_extensions\s*\(\s*"([^"]+)"\s*\)'
  else:
    continue
  for match in re.findall(regex, content, re.DOTALL):
    #(Take care of extensions separated by commas)
    for extension in match.split(','):
      trimmed = extension.strip()
      if trimmed:
        associations.add(trimmed)

#Create list of associations with the parsed extensions
output_file = sys.argv[1]
try:
  with open(output_file, 'w', encoding='utf-8') as outf:
    outf.writelines(f"{assoc}\n" for assoc in sorted(associations))
except Exception as e:
  sys.stderr.write(f"(ERROR): When writing output file {output_file}: {e}\n")
  sys.exit(1)
