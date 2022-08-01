#!/bin/sh

# This is a wrapper to the tools/gimp-mkenums perl script which:
# * sets a few common values;
# * updates the ${filebase}enums.c directly in the source directory in
#   order for it to be versionned.
# * Create a no-op stamp-header file to be included by the resulting
#   enums.c. The goal is to make sure that meson will trigger a rebuild
#   of the enums.c generation before compiling, if the enums.h changed.
#   See the explanation here:
#   https://github.com/mesonbuild/meson/issues/10196#issuecomment-1080742592
#   This is also the trick used for pdbgen.

# Arguments to this script:
# The perl binary to use.
PERL="$1"
# Root of the source directory.
top_srcdir="$2"
# Current source folder.
srcdir="$3"
# Current build folder.
builddir="$4"
# Base of the generated enums.c file name.
filebase="$5"
# Includes before #include "${filebase}enums.h"
preincludes="$6"
# Includes after #include "${filebase}enums.h"
postincludes="$7"
# Value for --dtail option if the default doesn't fit.
dtail="$8"

if [ -z "$dtail" ]; then
  dtail="    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_type_set_translation_context (type, \"@enumnick@\");\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n"
fi

$PERL $top_srcdir/tools/gimp-mkenums \
	--fhead "#include \"stamp-${filebase}enums.h\"\n#include \"config.h\"\n$preincludes#include \"${filebase}enums.h\"\n$postincludes" \
	--fprod "\n/* enumerations from \"@basename@\" */" \
	--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
	--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
	--vtail "    { 0, NULL, NULL }\n  };\n" \
	--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
	--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
	--dtail "$dtail" \
	"$srcdir/${filebase}enums.h" > "$builddir/${filebase}enums-tmp.c"

if ! cmp -s "$builddir/${filebase}enums-tmp.c" "$srcdir/${filebase}enums.c"; then
  cp "$builddir/${filebase}enums-tmp.c" "$srcdir/${filebase}enums.c";
else
  touch "$srcdir/${filebase}enums.c"; 2> /dev/null || true;
fi

echo "/* Generated on `date`. */" > $builddir/stamp-${filebase}enums.h
