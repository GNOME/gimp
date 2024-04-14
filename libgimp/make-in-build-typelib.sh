#!/bin/sh
# macOS-only script to generate temporary .gir and .typelib files only to be
# used during build, pointing to the non-installed libgimp* libraries.
# This allows to run non-installed GIMP binaries in a macOS development
# environment.

gimp_gir="$1"
gimp_typelib="$2"
gimpui_gir="$3"
gimpui_typelib="$4"
builddir="$5"
prefix="$6"
g_ir_compiler="$7"

echo PWD: $PWD
echo ARGS: "$*"

# This is only for macOS.
mkdir -p $builddir/tmp/
cp -f "$gimp_gir" "$gimpui_gir" "$builddir/tmp/"
cd "$builddir/tmp/"
gimp_gir=`basename "$gimp_gir"`
gimpui_gir=`basename "$gimpui_gir"`
gimp_typelib=`basename "$gimp_typelib"`
gimpui_typelib=`basename "$gimpui_typelib"`

sed -i '' "s|${prefix}/*||g" "$gimp_gir" "$gimpui_gir"
sed -i '' "s|@rpath/||g" "$gimp_gir" "$gimpui_gir"
sed -i '' 's|lib/\(libgimp\(ui\)\?-\([0-9.]*\).dylib\)|libgimp/\1|g; s|lib/\(libgimp\([a-z]*\)-\([0-9.]*\).dylib\)|libgimp\2/\1|g;' "$gimp_gir" "$gimpui_gir"
$g_ir_compiler --includedir=${prefix}/share/gir-1.0/ --includedir=. "$gimp_gir" -o "${gimp_typelib}"
$g_ir_compiler --includedir=${prefix}/share/gir-1.0/ --includedir=.  "$gimpui_gir" -o "${gimpui_typelib}"

echo "/* Generated on `date`. */" > $builddir/macos-typelib.stamp
