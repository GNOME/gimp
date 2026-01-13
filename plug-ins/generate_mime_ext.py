#!/usr/bin/env python3
import sys
import os
import re

types_associations_dict = {}

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
  source_file_ext = os.path.splitext(source_file)[1].lower()
  if source_file_ext == '.c':
    if "LOAD_PROC" not in content and "load_procedure" not in content:
      continue
    proc_regex  = r'gimp_(load|vector_load)_procedure_new'
    mime_regex  = r'gimp_file_procedure_set_mime_types\s*\([^,]+,\s*"([^"]+)"'
    ext_regex   = r'gimp_file_procedure_set_extensions\s*\([^,]+,\s*"([^"]+)"'
    label_regex = r'gimp_procedure_set_menu_label\s*\([^,]+,\s*_\("([^"]+)"\)\s*\)'
  elif source_file_ext == '.py':
    if "LoadProcedure" not in content:
      continue
    proc_regex  = r'(LoadProcedure|VectorLoadProcedure)\.new'
    mime_regex  = r'procedure\.set_mime_types\s*\(\s*"([^"]+)"\s*\)'
    ext_regex   = r'procedure\.set_extensions\s*\(\s*"([^"]+)"\s*\)'
    label_regex = r'procedure\.set_menu_label\s*\(\s*_\("([^"]+)"\)\s*\)'
  else:
    continue
  for proc_match in re.finditer(proc_regex, content):
    #proc_name = proc_match.group(1).strip()
    #key = (source_file, f"{proc_name}_{proc_match.start()}")
    key = (source_file, proc_match.start())
    if key not in types_associations_dict:
      types_associations_dict[key] = {"mime": set(), "extensions": set(), "label": set()}
    mime_match = re.search(mime_regex, content[proc_match.end():])
    if mime_match:
      #(Take care of mimes separated by commas)
      for mime in mime_match.group(1).split(','):
        trimmed_mime = mime.strip()
        if trimmed_mime:
          types_associations_dict[key]["mime"].add(trimmed_mime)
    ext_match = re.search(ext_regex, content[proc_match.end():])
    if ext_match:
      #(Take care of extensions separated by commas)
      for ext in ext_match.group(1).split(','):
        trimmed_ext = ext.strip()
        if trimmed_ext:
          types_associations_dict[key]["extensions"].add(trimmed_ext)
    label_match = re.search(label_regex, content[proc_match.end():])
    if label_match:
      types_associations_dict[key]["label"] = label_match.group(1).strip()

mode = sys.argv[1]
if mode == "--mime":
  #Output string with the parsed MIME types
  all_mime = set()
  for values in types_associations_dict.values():
    all_mime.update(values["mime"])
  print(";".join(sorted(all_mime)))
elif mode == "--association":
  #Create list with the parsed extensions
  output_file = sys.argv[2]
  all_ext = set()
  for values in types_associations_dict.values():
    all_ext.update(values["extensions"])
  try:
    with open(output_file, 'w', encoding='utf-8') as outf:
      outf.writelines(f"{assoc}\n" for assoc in sorted(all_ext))
  except Exception as e:
    sys.stderr.write(f"(ERROR): When writing output file {output_file}: {e}\n")
    sys.exit(1)
elif mode == "--pseudo-uti":
  output_file = sys.argv[2]
  try:
    with open(output_file, 'w', encoding='utf-8') as outf:
      for (source_file, proc_key), values in types_associations_dict.items():
        extensions = sorted(values["extensions"])
        mimes = sorted(values["mime"])
        if not extensions and not mimes:
          continue

        outf.write("<dict>\n")
        if extensions:
          outf.write("  <key>CFBundleTypeExtensions</key>\n")
          outf.write("  <array>\n")
          for ext in extensions:
            outf.write(f"    <string>{ext}</string>\n")
            outf.write(f"    <string>{ext.upper()}</string>\n")
          outf.write("  </array>\n")

        if mimes:
          outf.write("  <key>CFBundleTypeMIMETypes</key>\n")
          outf.write("  <array>\n")
          for mime in mimes:
            outf.write(f"    <string>{mime}</string>\n")
          outf.write("  </array>\n")

        outf.write("  <key>CFBundleTypeIconFile</key>\n")
        outf.write("  <string>fileicon</string>\n")

        if "label" in values:
          outf.write("  <key>CFBundleTypeName</key>\n")
          outf.write(f"  <string>{values['label']}</string>\n")
        elif extensions:
          outf.write("  <key>CFBundleTypeName</key>\n")
          outf.write(f"  <string>{extensions[0].upper()} file</string>\n")

        outf.write("  <key>CFBundleTypeRole</key>\n")
        outf.write("  <string>Viewer</string>\n")

        outf.write("  <key>LSHandlerRank</key>\n")
        outf.write("  <string>Default</string>\n")
        outf.write("</dict>\n")
  except Exception as e:
    sys.stderr.write(f"(ERROR): When writing pseudo-UTI file {output_file}: {e}\n")
    sys.exit(1)
