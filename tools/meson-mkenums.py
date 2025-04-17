#!/usr/bin/env python3
import sys
import os
import subprocess
import filecmp
import shutil
from datetime import datetime

# This is a wrapper to the tools/gimp-mkenums perl script which:
# * sets a few common values;
# * updates the ${filebase}enums.c directly in the source directory in
#   order for it to be versioned.
# * Create a no-op stamp-header file to be included by the resulting
#   enums.c. The goal is to make sure that meson will trigger a rebuild
#   of the enums.c generation before compiling, if the enums.h changed.
#   See the explanation here:
#   https://github.com/mesonbuild/meson/issues/10196#issuecomment-1080742592
#   This is also the trick used for pdbgen.

# Arguments to this script:
# The perl binary to use.
PERL = sys.argv[1]
# Root of the source directory.
top_srcdir = sys.argv[2]
# Current source folder.
srcdir = sys.argv[3]
# Current build folder.
builddir = sys.argv[4]
# Base of the generated enums.c file name.
filebase = sys.argv[5]
# Includes before #include "${filebase}enums.h"
preincludes = sys.argv[6]
# Includes after #include "${filebase}enums.h"
postincludes = sys.argv[7]
# Value for --dtail option if the default doesn't fit.
dtail = sys.argv[8] if len(sys.argv) >= 9 else None

if not dtail:
  dtail = "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_type_set_translation_context (type, \"@enumnick@\");\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n"

args = [
  PERL, os.path.join(top_srcdir, 'tools', 'gimp-mkenums'),
  '--fhead', f'#include "stamp-{filebase}enums.h"\n' f'#include "config.h"\n' f'{preincludes}' f'#include "{filebase}enums.h"\n' f'{postincludes}',
  '--fprod', '\n/* enumerations from "@basename@" */',
  '--vhead', 'GType\n' '@enum_name@_get_type (void)\n' '{\n' '  static const G@Type@Value values[] =\n' '  {',
  '--vprod', '    { @VALUENAME@, "@VALUENAME@", "@valuenick@" },',
  '--vtail', '    { 0, NULL, NULL }\n  };\n',
  '--dhead', '  static const Gimp@Type@Desc descs[] =\n  {',
  '--dprod', '    { @VALUENAME@, @valuedesc@, @valuehelp@ },' '@if (\'@valueabbrev@\' ne \'NULL\')@\n' '    /* Translators: this is an abbreviated version of @valueudesc@.\n' '       Keep it short. */\n' '    { @VALUENAME@, @valueabbrev@, NULL },@endif@',
  '--dtail', dtail,
  os.path.join(f"{filebase}enums.h")
]
tmp_enums_c = os.path.join(builddir, f"{filebase}enums-tmp.c")
with open(tmp_enums_c, 'w') as f:
  subprocess.run(args, check=True, stdout=f, text=True, cwd=srcdir)

target_enums_c = os.path.join(srcdir, f"{filebase}enums.c")
if not filecmp.cmp(tmp_enums_c, target_enums_c, shallow=False):
  shutil.copy(tmp_enums_c, target_enums_c)
else:
  try:
    os.utime(target_enums_c, None)
  except OSError:
    pass

with open(os.path.join(builddir, f"stamp-{filebase}enums.h"), 'w') as f:
  f.write(f"/* Generated on {datetime.now().strftime('%a %b %d %H:%M:%S %Z %Y')}. */\n")
