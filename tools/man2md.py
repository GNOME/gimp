#!/usr/bin/env python3

import os
import re
import subprocess
import sys
import datetime

man_file = sys.argv[1]
md_file  = sys.argv[2]
prefix   = sys.argv[3]

if not man_file.endswith(('.1', '.5')):
  sys.stderr.write(f'File "{man_file}" does not end in .1 or .5.\n')
  sys.exit(1)

# Convert from troff to Markdown
try:
  subprocess.run(['pandoc','-f', 'man','-t', 'markdown','--shift-heading-level-by=1',
                 man_file,'-o', md_file], check=True)
except subprocess.CalledProcessError as e:
  sys.stderr.write(f"(ERROR): {man_file} conversion failed: {e}")
  sys.exit(1)

try:
  with open(md_file, 'r', encoding='utf-8') as f:
    content = f.read()

  # Generate pelican metadata block
  metadata = (f"Title: {os.path.splitext(os.path.basename(md_file))[0]} Man Page\n"
               "Date: 2015-08-17T11:31:37-05:00\n"
              f"Modified: {datetime.datetime.now().isoformat()}\n"
              "Authors: GIMP Team\n"
              "Status: hidden\n")
  content = f"{metadata}\n\n" + content

  # Remove build-time prefix
  new_prefix = "/usr"
  if prefix not in content:
    raise ValueError(f"Prefix '{prefix}' not found in {md_file}")
  content = content.replace(prefix, new_prefix)

  with open(md_file, 'w', encoding='utf-8') as f:
    f.write(content)

  sys.stderr.write(f"(INFO): generated: {md_file}")
except Exception as e:
  sys.stderr.write(f"(ERROR): {md_file} processing failed: {e}")
  sys.exit(1)
