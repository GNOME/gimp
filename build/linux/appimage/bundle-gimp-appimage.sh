#!/bin/sh

# Loosely based on:
# https://github.com/AppImage/AppImageSpec/blob/master/draft.md
# https://gitlab.com/inkscape/inkscape/-/commit/b280917568051872793a0c7223b8d3f3928b7d26

set -e


# AGNOSTIC VARIABLES (only touch them to make even more portable, without casuistry)

## This script is arch-agnostic. The packager can specify it when calling the script
if [[ -z "$1" ]]; then
  export ARCH=$(uname -m)
else
  export ARCH=$1
fi
INSTALL_ARTIF=$(echo _install*)
BUILD_ARTIF=$(echo _build*)

## This script is "filesystem-agnostic". The packager can quickly choose either
## putting everything in /usr or in AppDir(root) just specifying the 2nd parameter.
GIMP_DISTRIB="$CI_PROJECT_DIR/build/linux/appimage/AppDir"
GIMP_PREFIX="$GIMP_DISTRIB/usr"
if [[ -z "$2" ]] || [[ "$2" == "usr" ]]; then
  OPT_PREFIX="${GIMP_PREFIX}"
elif [[ "$2" == "AppDir" ]]; then
  OPT_PREFIX="${GIMP_DISTRIB}"
fi

## This script is distro-agnostic too.
## TODO: take this code from .gitlab-ci.yml
gcc -print-multi-os-directory | grep . && LIB_DIR=$(gcc -print-multi-os-directory | sed 's/\.\.\///g') || LIB_DIR="lib"
gcc -print-multiarch | grep . && LIB_SUBDIR=$(echo $(gcc -print-multiarch)'/')


#(MOSTLY) AGNOSTIC FUNCTIONS
prep_pkg ()
{
  apt-get install -y --no-install-recommends $1
}

find_bin ()
{
  find /usr/bin -name ${1} -execdir cp -r '{}' $OPT_PREFIX/bin \;
  find /bin -name ${1} -execdir cp -r '{}' $OPT_PREFIX/bin \;
}

find_lib ()
{
  find /usr/${LIB_DIR}/${LIB_SUBDIR} -maxdepth 1 -name ${1} -execdir cp -r '{}' $OPT_PREFIX/${LIB_DIR}/${LIB_SUBDIR} \;
  find /usr/${LIB_DIR} -maxdepth 1 -name ${1} -execdir cp -r '{}' $OPT_PREFIX/${LIB_DIR}/${LIB_SUBDIR} \;
  find /${LIB_DIR}/${LIB_SUBDIR} -maxdepth 1 -name ${1} -execdir cp -r '{}' $OPT_PREFIX/${LIB_DIR}/${LIB_SUBDIR} \;
  find /${LIB_DIR} -maxdepth 1 -name ${1} -execdir cp -r '{}' $OPT_PREFIX/${LIB_DIR}/${LIB_SUBDIR} \;
}

find_dat ()
{
  DAT_PATH=$(echo $1)
  DAT_PATH=$(sed "s|/usr/||g" <<< $DAT_PATH)
  mkdir -p $OPT_PREFIX/$DAT_PATH
  cp -r $1/$2 $OPT_PREFIX/$DAT_PATH/$3
}

conf_app ()
{
  VAR_PATH=$(echo $2/$3)
  VAR_PATH=$(sed "s|${2}/||g" <<< $VAR_PATH)
  sed -i "s|${1}_WILD|OPT_PREFIX_WILD${VAR_PATH}|" build/linux/appimage/AppRun
}


# PREPARE ENVIRONMENT
apt-get install -y --no-install-recommends wget

## For now, we always use the latest version of go-appimagetool
wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-x86_64.AppImage" | head -n 1 | cut -d '"' -f 2)
mv *.AppImage appimagetool.appimage
go_appimagetool=appimagetool.appimage
chmod +x "$go_appimagetool"

## go-appimagetool have buggy appstreamcli so we need to use the legacy one
legacy_appimagetool="appimagetool-x86_64.AppImage"
wget "https://github.com/AppImage/AppImageKit/releases/download/continuous/$legacy_appimagetool"
chmod +x "$legacy_appimagetool"


# BUNDLE FILES

## System base (needed to use GIMP or to avoid polluting the terminal output)
### Glib needed files
find_dat "/usr/share/glib-*/schemas" "*"
### Glib commonly required modules
prep_pkg "gvfs"
find_lib "gvfs*"
find_lib "gio*"
conf_app GIO_MODULE_DIR "/usr" "${LIB_DIR}/${LIB_SUBDIR}gio"
### GTK needed files
prep_pkg "gnome-icon-theme"
find_dat "/usr/share/icons/gnome" "*"
find_dat "/usr/share/mime" "*"
conf_app GDK_PIXBUF_MODULEDIR "/usr" "${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-*/*.*.*"
conf_app GDK_PIXBUF_MODULE_FILE "/usr" "${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-*/*.*.*"
### GTK commonly required modules
prep_pkg "libibus-1.0-5"
find_lib "libibus*"
prep_pkg "ibus-gtk3"
prep_pkg "libcanberra-gtk3-module"
prep_pkg "libxapp-gtk3-module"
conf_app GTK_PATH "/usr" "${LIB_DIR}/${LIB_SUBDIR}gtk-3.0"
conf_app GTK_IM_MODULE_FILE "/usr" "${LIB_DIR}/${LIB_SUBDIR}gtk-3.0/*.*.*"

## Core features
cp -r $INSTALL_ARTIF/* $OPT_PREFIX
conf_app BABL_PATH "$OPT_PREFIX" "${LIB_DIR}/${LIB_SUBDIR}babl-*"
conf_app GEGL_PATH "$OPT_PREFIX" "${LIB_DIR}/${LIB_SUBDIR}gegl-*"
conf_app GIMP3_SYSCONFDIR "$OPT_PREFIX" "etc/gimp/*"
conf_app GIMP3_DATADIR "$OPT_PREFIX" "share/gimp/*"
### Copy localized language list support
find_dat "/usr/share/xml/iso-codes" "iso_639-2.xml" "iso_639.xml"
### Copy system theme support (TODO: remove when our Default theme is mature)
find_bin "find*"
find_bin "gsettings*"
find_bin "sed*"
### Copy GTK inspector support
find_lib "libEGL*"
find_lib "libGL*"
find_lib "dri*"
conf_app LIBGL_DRIVERS_PATH "$OPT_PREFIX" "${LIB_DIR}/${LIB_SUBDIR}dri"

## Plug-ins
find_bin "uname*"
conf_app GIMP3_PLUGINDIR "$OPT_PREFIX" "${LIB_DIR}/${LIB_SUBDIR}gimp/*"
conf_app GI_TYPELIB_PATH "$OPT_PREFIX" "${LIB_DIR}/${LIB_SUBDIR}girepository-*"
### Copy JavaScript plug-ins support
find_bin "gjs*"
### Copy Lua plug-ins support (NOT WORKING)
#find_bin "lua*"
#find_lib "liblua*"
### Copy Python plug-ins support
find_bin "python*"
find_lib "python*.*"
conf_app PYTHONPATH "/usr" "${LIB_DIR}/${LIB_SUBDIR}python*.**"

## Final adjustments
### Auto detect and copy deps of binaries copied above
"./$go_appimagetool" --appimage-extract-and-run -s deploy $OPT_PREFIX/share/applications/org.gimp.GIMP.desktop
### Rearranje babl, GEGL and GIMP (only the needed files)
if [[ -z "$2" ]] || [[ "$2" == "usr" ]]; then
  cp -r $GIMP_DISTRIB/etc $GIMP_PREFIX
  rm -r $GIMP_DISTRIB/etc
  cp -r $GIMP_DISTRIB/lib $GIMP_PREFIX
  rm -r $GIMP_DISTRIB/lib
  cp -r $GIMP_DISTRIB/lib64 $GIMP_PREFIX
  rm -r $GIMP_DISTRIB/lib64
elif [[ "$2" == "AppDir" ]]; then
  cp -r $GIMP_PREFIX/* $GIMP_DISTRIB
  rm -r $GIMP_PREFIX
fi
### Remove unnecessary files
rm -r $OPT_PREFIX/include
rm -r $OPT_PREFIX/${LIB_DIR}/${LIB_SUBDIR}pkgconfig
rm -r $OPT_PREFIX/share/doc
rm -r $OPT_PREFIX/share/man

## Sad adjustments (go-appimagetool don't like when this is made before and don't do for us yet)
sed -i "s|\"/usr/|\"|g" "$OPT_PREFIX/${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-2.0/2.10.0/loaders.cache"
cp -r "/usr/${LIB_DIR}/${LIB_SUBDIR}gtk-3.0/3.0.0/immodules.cache" "$OPT_PREFIX/${LIB_DIR}/${LIB_SUBDIR}gtk-3.0/3.0.0"
sed -i "s|\"/usr/|\"|g" "$OPT_PREFIX/${LIB_DIR}/${LIB_SUBDIR}gtk-3.0/3.0.0/immodules.cache"


# CONFIGURE APPRUN
cp build/linux/appimage/AppRun $GIMP_DISTRIB

if [[ -z "$2" ]] || [[ "$2" == "usr" ]]; then
  sed -i "s|OPT_PREFIX_WILD|usr/|g" $GIMP_DISTRIB/AppRun
elif [[ "$2" == "AppDir" ]]; then
  sed -i "s|OPT_PREFIX_WILD||g" $GIMP_DISTRIB/AppRun
fi

GIMP_APP_VERSION=$(grep GIMP_APP_VERSION $BUILD_ARTIF/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
sed -i "s|GIMP_APP_VERSION|${GIMP_APP_VERSION}|" $GIMP_DISTRIB/AppRun


# CONFIGURE METADATA
sed -i '/kudo/d' $OPT_PREFIX/share/metainfo/org.gimp.GIMP.appdata.xml
if [[ "$2" == "AppDir" ]]; then
  mkdir -p $GIMP_PREFIX/share
  cp -r $GIMP_DISTRIB/share/metainfo $GIMP_PREFIX/share
  cp -r $GIMP_DISTRIB/share/applications $GIMP_PREFIX/share
fi


# CONFIGURE ICON
cp $OPT_PREFIX/share/icons/hicolor/scalable/apps/org.gimp.GIMP.svg $GIMP_DISTRIB/org.gimp.GIMP.svg
if [[ "$2" == "AppDir" ]]; then
  cp -r $GIMP_DISTRIB/share/icons/ $GIMP_PREFIX/share
fi


# MAKE APPIMAGE
"./$legacy_appimagetool" --appimage-extract-and-run $GIMP_DISTRIB # -u "zsync|https://download.gimp.org/gimp/v${GIMP_APP_VERSION}/GIMP-latest-${ARCH}.AppImage.zsync"
mkdir build/linux/appimage/_Output
GIMP_VERSION=$(grep GIMP_VERSION $BUILD_ARTIF/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
mv GNU*.AppImage build/linux/appimage/_Output/GIMP-${GIMP_VERSION}-${ARCH}.AppImage
