#!/usr/bin/env python3
import os
import sys
import subprocess
from datetime import datetime

PERL = sys.argv[1]
top_srcdir = sys.argv[2]
top_builddir = sys.argv[3]

# Environment for the pdbgen.pl file.
os.environ['destdir'] = os.path.abspath(top_srcdir)
os.environ['builddir'] = os.path.abspath(top_builddir)

os.chdir(os.path.join(top_srcdir, 'pdb'))
result = subprocess.run(
  [PERL, 'pdbgen.pl', 'app', 'lib'],
  stdout=subprocess.PIPE,
  stderr=subprocess.PIPE
)
if result.returncode == 0:
  with open(os.path.join(top_builddir, 'pdb', 'stamp-pdbgen.h'), 'w') as f:
    f.write(f"/* Generated on {datetime.now().strftime('%a %b %d %H:%M:%S %Z %Y')}. */\n")
sys.exit(result.returncode)
