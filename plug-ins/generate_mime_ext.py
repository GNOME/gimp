#!/usr/bin/env python3
import sys
import os
import re

types_associations_list = set()

#Read file loading plug-ins sourcecode
source_files = sys.argv[3:]
for source_file in source_files:
  try:
    with open(source_file, 'r', encoding='utf-8') as f:
      content = f.read()
  except Exception as e:
    sys.stderr.write(f"(WARNING): Unable to open file {source_file}: {e}\n")
    continue

  #Parse MIME types or extensions declared in the sourcecode
  mode = sys.argv[1]
  if mode == "--mime":
    function_suffix = 'set_mime_types'
  elif mode == "--association":
    function_suffix = 'set_extensions'
  source_file_ext = os.path.splitext(source_file)[1].lower()
  if source_file_ext == '.c':
    if "LOAD_PROC" not in content and "load_procedure" not in content:
      continue
    regex = (fr'gimp_file_procedure_{function_suffix}\s*'
             fr'\(\s*GIMP_FILE_PROCEDURE\s*\(\s*procedure\s*\)\s*,\s*"([^"]+)"')
  elif source_file_ext == '.py':
    if "LoadProcedure" not in content:
      continue
    regex = fr'procedure\.{function_suffix}\s*\(\s*"([^"]+)"\s*\)'
  else:
    continue
  for match in re.findall(regex, content, re.DOTALL):
    #(Take care of extensions separated by commas)
    for mime_extension in match.split(','):
      trimmed = mime_extension.strip()
      if trimmed:
        types_associations_list.add(trimmed)

if mode == "--mime":
  #Output string with the parsed MIME types
  print(";".join(sorted(types_associations_list)))
elif mode == "--association":
  #Create list with the parsed extensions
  output_file = sys.argv[2]
  try:
    with open(output_file, 'w', encoding='utf-8') as outf:
      outf.writelines(f"{assoc}\n" for assoc in sorted(types_associations_list))
  except Exception as e:
    sys.stderr.write(f"(ERROR): When writing output file {output_file}: {e}\n")
    sys.exit(1)
