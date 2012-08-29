#!/bin/bash

# Inherited from gtk-mac-bundler example launcher script
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

# Where we get the paths from
name=$(basename "$0")
echo $name

dirn=$(dirname "$0")
echo $dirn

bundle=$(cd "$dirn/../../" && pwd)
bundle_contents="$bundle"/Contents
bundle_res="$bundle_contents"/Resources
bundle_lib="$bundle_res"/lib
bundle_bin="$bundle_res"/bin
bundle_data="$bundle_res"/share
bundle_etc="$bundle_res"/etc

export PATH="$bundle_bin:$PATH"
echo $PATH

# Set $PYTHON to point inside the bundle
export PYTHON="$bundle_contents/MacOS/python"
# Add the bundle's python modules
PYTHONPATH="$bundle_lib/python2.7:$PYTHONPATH"
PYTHONPATH="$bundle_lib/python2.7/site-packages:$PYTHONPATH"
PYTHONPATH="$bundle_lib/gimp/2.0/python:$PYTHONPATH"
# Add our program's modules to $PYTHONPATH. 
PYTHONPATH="$bundle_lib/pygtk/2.0:$PYTHONPATH"
export PYTHONPATH

# Use fallback instead of normal dlyd path, may not be required
export DYLD_FALLBACK_LIBRARY_PATH="$bundle_lib:$DYLD_FALLBACK_LIBRARY_PATH"

# Help fontconfig find its configuration file
export FONTCONFIG_FILE="$bundle_etc/fonts/fonts.conf"

# Help gdk find its loader modules
export GDK_PIXBUF_MODULE_FILE="$bundle_etc/gtk-2.0/gdk-pixbuf.loaders"

# Fix for the theme engine paths
export GTK_PATH="$bundle_lib/gtk-2.0/2.10.0"

# GTK path no longer required
# export GTK_IM_MODULE_FILE="$bundle_etc/gtk-2.0/gtk.immodules"

# Pango path no longer required
# export PANGO_RC_FILE="$bundle_etc/pango/pangorc"

# Fix the gegl path issue
export GEGL_PATH="$bundle_lib/gegl-0.2"

# Define gtkrc file
export GTK2_RC_FILES="$bundle_etc/gtk-2.0/gtkrc"

# Workaround for <https://bugzilla.gnome.org/show_bug.cgi?id=671817>
if [ ! -f "$HOME/.local/share/recently-used.xbel" ]
then
    mkdir -p "$HOME/.local/share"
fi

# Strip out arguments added by the OS
if [ x`echo "x$1" | sed -e "s/^x-psn_.*//"` == x ]; then
	shift 1
fi

if [ "x$GTK_DEBUG_SHELL" != "x" ]; then
	exec bash

else
	$EXEC "$bundle_contents/MacOS/$name-bin"
fi
