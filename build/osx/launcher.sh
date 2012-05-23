#!/bin/bash

if test "x$GTK_DEBUG_LAUNCHER" != x; then
	set -x
fi

if test "x$GTK_DEBUG_GDB" != x; then
	EXEC="gdb --args"
elif test "x$GTK_DEBUG_DTRUSS" != x; then
	EXEC="dtruss"
else
	EXEC=exec
fi

# Where is GIMP.app located
name=$(basename "$0")

dirn=$(dirname "$0")

bundle=$(cd "$dirn/../../" && pwd)
bundle_contents="$bundle"/Contents
bundle_res="$bundle_contents"/Resources
bundle_lib="$bundle_res"/lib
bundle_bin="$bundle_res"/bin
bundle_data="$bundle_res"/share
bundle_etc="$bundle_res"/etc

export PATH="$bundle_bin:$PATH"

# This is a compromise between clashing with system libs and no libs; it may no longer be needed
export DYLD_FALLBACK_LIBRARY_PATH="$bundle_lib:$DYLD_FALLBACK_LIBRARY_PATH"

# The fontconfig file needs to be explicitly defined
export FONTCONFIG_FILE="$bundle_etc/fonts/fonts.conf"

# Gdk needs it's own specifications
export GDK_PIXBUF_MODULE_FILE="$bundle_etc/gtk-2.0/gdk-pixbuf.loaders"

# Fix the theme engine paths
export GTK_PATH="$bundle_lib/gtk-2.0/2.10.0"

# Backup paths
# export GTK_IM_MODULE_FILE="$bundle_etc/gtk-2.0/gtk.immodules"
# export PANGO_RC_FILE="$bundle_etc/pango/pangorc"

# Strip out arguments added by the OS.
if [ x`echo "x$1" | sed -e "s/^x-psn_.*//"` == x ]; then
	shift 1
fi

if [ "x$GTK_DEBUG_SHELL" != "x" ]; then
	exec bash

else
	$EXEC "$bundle_contents/MacOS/$name-bin"
fi
