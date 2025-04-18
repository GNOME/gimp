#!/usr/bin/env python3
import os
import sys
import subprocess
import tempfile
from datetime import date

os.chdir(os.environ['GIMP_TESTING_BUILDDIR'])
if int(os.environ.get('GIMP_RELEASE', '0')) == 1:
  result = subprocess.run(['appstreamcli', 'validate', 'org.gimp.GIMP.appdata.xml'])
  sys.exit(result.returncode)
else:
  temp_fd, APPDATA = tempfile.mkstemp(prefix='org.gimp.GIMP.appdata.',suffix='.xml',dir='.')
  with open('org.gimp.GIMP.appdata.xml', 'r', encoding='utf-8') as f:
    content = f.read()
  new_content = content.replace(
    'date="TODO"',
    f'date="{date.today().isoformat()}"'
  )
  with os.fdopen(temp_fd, 'w', encoding='utf-8') as f:
    f.write(new_content)
  result = subprocess.run(['appstreamcli', 'validate', APPDATA])
  os.remove(APPDATA)
  sys.exit(result.returncode)
