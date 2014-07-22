#!/bin/sh

# Purpose: 	set up the runtime environment and run GIMP

echo "Setting up environment..."

if test "x$GTK_DEBUG_LAUNCHER" != x; then
    set -x
fi

if test "x$GTK_DEBUG_GDB" != x; then
    EXEC="gdb --args"
else
    EXEC=exec
fi

name=`basename "$0"`
tmp="$0"
tmp=`dirname "$tmp"`
tmp=`dirname "$tmp"`
bundle=`dirname "$tmp"`
bundle_contents="$bundle"/Contents
bundle_res="$bundle_contents"/Resources
bundle_lib="$bundle_res"/lib
bundle_bin="$bundle_res"/bin
bundle_data="$bundle_res"/share
bundle_etc="$bundle_res"/etc

export DYLD_LIBRARY_PATH="$bundle_lib"
export XDG_CONFIG_DIRS="$bundle_etc"/xdg
export XDG_DATA_DIRS="$bundle_data"
export GTK_DATA_PREFIX="$bundle_res"
export GTK_EXE_PREFIX="$bundle_res"
export GTK_PATH="$bundle_res"

# Set up PATH variable
export PATH="$bundle_contents/MacOS:$bundle_bin:$PATH"

# Set up generic configuration
export GTK2_RC_FILES="$bundle_etc/gtk-2.0/gtkrc"
export GTK_IM_MODULE_FILE="$bundle_etc/gtk-2.0/gtk.immodules"
export GDK_PIXBUF_MODULE_FILE="$bundle_etc/gtk-2.0/gdk-pixbuf.loaders"
export GDK_PIXBUF_MODULEDIR="$bundle_lib/gdk-pixbuf-2.0/2.10.0/loaders"
#export PANGO_RC_FILE="$bundle_etc/pango/pangorc"

export PANGO_RC_FILE="$bundle_etc/pango/pangorc"
export PANGO_SYSCONFDIR="$bundle_etc"
export PANGO_LIBDIR="$bundle_lib"

# Specify Fontconfig configuration file
export FONTCONFIG_FILE="$bundle_etc/fonts/fonts.conf"

# Include GEGL path
export GEGL_PATH="$bundle_lib/gegl-0.3"

# Set up Python
echo "Enabling internal Python..."
export PYTHONHOME="$bundle_res"

# Add bundled Python modules
PYTHONPATH="$bundle_lib/python2.7:$PYTHONPATH"
PYTHONPATH="$bundle_lib/python2.7/site-packages:$PYTHONPATH"
PYTHONPATH="$bundle_lib/python2.7/site-packages/gtk-2.0:$PYTHONPATH"
PYTHONPATH="$bundle_lib/pygtk/2.0:$PYTHONPATH"

# Include gimp python modules
PYTHONPATH="$bundle_lib/gimp/2.0/python:$PYTHONPATH"

export PYTHONPATH

# Specify Ghostscript directories
# export GS_RESOURCE_DIR="$bundle_res/share/ghostscript/9.06/Resource"
# export GS_ICC_PROFILES="$bundle_res/share/ghostscript/9.06/iccprofiles/"
# export GS_LIB="$GS_RESOURCE_DIR/Init:$GS_RESOURCE_DIR:$GS_RESOURCE_DIR/Font:$bundle_res/share/ghostscript/fonts:$bundle_res/share/fonts/urw-fonts:$GS_ICC_PROFILES"
# export GS_FONTPATH="$bundle_res/share/ghostscript/fonts:$bundle_res/share/fonts/urw-fonts:~/Library/Fonts:/Library/Fonts:/System/Library/Fonts"

# set up character encoding aliases
if test -f "$bundle_lib/charset.alias"; then
 export CHARSETALIASDIR="$bundle_lib"
fi

# Extra arguments can be added in environment.sh.
EXTRA_ARGS=
if test -f "$bundle_res/environment.sh"; then
 source "$bundle_res/environment.sh"
fi

# Strip out the argument added by the OS.
if /bin/expr "x$1" : '^x-psn_' > /dev/null; then
 shift 1
fi


echo "Launching GIMP in a D-Bus session..."

# Test for an existing D-Bus daemon and if there, reuse it.
# Otherwise launch a new exclusive D-Bus daemon with an exclusive session 
# and run GIMP within it.
# Taken and adapted from  http://dbus.freedesktop.org/doc/dbus-launch.1.html
if test -z "$DBUS_SESSION_BUS_ADDRESS" ; then
  $bundle_bin/dbus-launch --sh-syntax --exit-with-session \
  --config-file "$bundle_etc/dbus-1/session.conf" \
  "$bundle_contents/MacOS/$name-bin" "$@" $EXTRA_ARGS
else
  $EXEC "$bundle_contents/MacOS/$name-bin" "$@" $EXTRA_ARGS
fi


echo "Cleaning up..."
$bundle_bin/dbus-cleanup-sockets /var/tmp