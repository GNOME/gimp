#!/bin/sh

# Loosely based on:
# https://github.com/AppImage/AppImageSpec/blob/master/draft.md
# https://gitlab.com/inkscape/inkscape/-/commit/b280917568051872793a0c7223b8d3f3928b7d26

set -e


# AGNOSTIC VARIABLES (only touch them to make even more portable, without casuistry)

## This script is arch-agnostic. The packager can specify it when calling the script
if [ -z "$1" ]; then
  export ARCH=$(uname -m)
else
  export ARCH=$1
fi
INSTALL_ARTIF=$(echo _install*)
BUILD_ARTIF=$(echo _build*)

## This script is "filesystem-agnostic". The packager can quickly choose either
## putting everything in /usr or in AppDir(root) just specifying the 2nd parameter.
GIMP_DISTRIB="$CI_PROJECT_DIR/build/linux/appimage/AppDir"
if [ "$GITLAB_CI" ] || [ -z "$GIMP_PREFIX" ]; then
  GIMP_PREFIX="$GIMP_DISTRIB/usr"
fi
if [ -z "$2" ] || [ "$2" = "usr" ]; then
  OPT_PREFIX="${GIMP_PREFIX}"
elif [ "$2" = "AppDir" ]; then
  OPT_PREFIX="${GIMP_DISTRIB}"
fi

## This script is distro-agnostic too. We take universal variables from .gitlab-ci.yml
IFS=$'\n' VAR_ARRAY=($(cat .gitlab-ci.yml | sed -n '/export PATH=/,/GI_TYPELIB_PATH}\"/p' | sed 's/    - //'))
IFS=$' \t\n'
for VAR in "${VAR_ARRAY[@]}"; do
  eval "$VAR" || continue
done


#(MOSTLY) AGNOSTIC FUNCTIONS
prepare ()
{
  apt-get install -y --no-install-recommends $1
}

bundle ()
{
  mkdir -p $OPT_PREFIX/$(echo $1 | sed 's|--||')
  if [ "$1" = '--bin' ]; then
    binArray=($(find /usr/bin /bin -name $2))
    for bin in "${binArray[@]}"; do
      echo "(INFO): copying $bin to $OPT_PREFIX/bin"
      cp -r $bin $OPT_PREFIX/bin
    done
  elif [ "$1" = '--lib' ]; then
    libArray=($(find /usr/$LIB_DIR/$LIB_SUBDIR /$LIB_DIR/$LIB_SUBDIR -maxdepth 1 -name $2))
    for lib in "${libArray[@]}"; do
      echo "(INFO): copying $lib to $OPT_PREFIX/$LIB_DIR/$LIB_SUBDIR"
      cp -r $lib $OPT_PREFIX/$LIB_DIR/$LIB_SUBDIR
    done
  elif [ "$1" = '--share' ]; then
    path=$(echo /usr/share/$2 | sed 's|/usr/share/||g')
    dest_path=$(echo /usr/share/$2 | sed -e 's|/usr/share/||g' -e "s|${path##*/}||g")
    mkdir -p $OPT_PREFIX/share/$dest_path
    echo "(INFO): copying /usr/share/$path to $(dirname $OPT_PREFIX/share/$path)"
    cp -r /usr/share/$path $OPT_PREFIX/share/$dest_path
  fi
}

configure ()
{
  echo "(INFO): configuring $1"
  VAR_PATH=$(echo $2/$3 | sed "s|${2}/||g")
  sed -i "s|${1}_WILD|OPT_PREFIX_WILD${VAR_PATH}|" build/linux/appimage/AppRun
}


# PREPARE ENVIRONMENT
prepare wget

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
configure LD_LINUX "/usr" "lib64/ld-*.so.*"
### Glib needed files
bundle --share "glib-*/schemas/"
### Glib commonly required modules
prepare "gvfs"
bundle --lib "gvfs*"
bundle --lib "gio*"
configure GIO_MODULE_DIR "/usr" "${LIB_DIR}/${LIB_SUBDIR}gio"
### GTK needed files
prepare "gnome-icon-theme"
bundle --share "icons/gnome/"
bundle --share "mime/"
configure GDK_PIXBUF_MODULEDIR "/usr" "${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-*/*.*.*"
configure GDK_PIXBUF_MODULE_FILE "/usr" "${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-*/*.*.*"
### GTK commonly required modules
prepare "libibus-1.0-5"
bundle --lib "libibus*"
prepare "ibus-gtk3"
prepare "libcanberra-gtk3-module"
prepare "libxapp-gtk3-module"
configure GTK_PATH "/usr" "${LIB_DIR}/${LIB_SUBDIR}gtk-3.0"
configure GTK_IM_MODULE_FILE "/usr" "${LIB_DIR}/${LIB_SUBDIR}gtk-3.0/*.*.*"

## Core features
cp -r $INSTALL_ARTIF/* $OPT_PREFIX
configure BABL_PATH "$OPT_PREFIX" "${LIB_DIR}/${LIB_SUBDIR}babl-*"
configure GEGL_PATH "$OPT_PREFIX" "${LIB_DIR}/${LIB_SUBDIR}gegl-*"
configure GIMP3_SYSCONFDIR "$OPT_PREFIX" "etc/gimp/*"
configure GIMP3_DATADIR "$OPT_PREFIX" "share/gimp/*"
### Copy localized language list support
bundle --share "xml/iso-codes/iso_639-2.xml"
mv $OPT_PREFIX/share/xml/iso-codes/iso_639-2.xml $OPT_PREFIX/share/xml/iso-codes/iso_639.xml
### Copy system theme support
bundle --bin "gsettings*"
bundle --bin "sed*"
### Copy GTK inspector support
bundle --lib "libEGL*"
bundle --lib "libGL*"
bundle --lib "dri*"
configure LIBGL_DRIVERS_PATH "$OPT_PREFIX" "${LIB_DIR}/${LIB_SUBDIR}dri"

## Plug-ins
bundle --bin "uname*"
configure GIMP3_PLUGINDIR "$OPT_PREFIX" "${LIB_DIR}/${LIB_SUBDIR}gimp/*"
configure GI_TYPELIB_PATH "$OPT_PREFIX" "${LIB_DIR}/${LIB_SUBDIR}girepository-*"
### Copy JavaScript plug-ins support
bundle --bin "gjs*"
### Copy Lua plug-ins support (NOT WORKING)
#bundle --bin "lua*"
#bundle --lib "liblua*"
### Copy Python plug-ins support
bundle --bin "python*"
bundle --lib "python*.*"
configure PYTHONPATH "/usr" "${LIB_DIR}/${LIB_SUBDIR}python3.11"

## Final adjustments
### Auto detect and copy deps of binaries copied above
"./$go_appimagetool" --appimage-extract-and-run -s deploy $OPT_PREFIX/share/applications/org.gimp.GIMP.desktop
### Rearranje babl, GEGL and GIMP (only the needed files)
if [ -z "$2" ] || [ "$2" = "usr" ]; then
  cp -r $GIMP_DISTRIB/etc $GIMP_PREFIX
  rm -r $GIMP_DISTRIB/etc
  cp -r $GIMP_DISTRIB/lib $GIMP_PREFIX
  rm -r $GIMP_DISTRIB/lib
  cp -r $GIMP_DISTRIB/lib64 $GIMP_PREFIX
  rm -r $GIMP_DISTRIB/lib64
elif [ "$2" = "AppDir" ]; then
  cp -r $GIMP_PREFIX/* $GIMP_DISTRIB
  rm -r $GIMP_PREFIX
fi
### Remove unnecessary files
rm -r $OPT_PREFIX/include
rm -r $OPT_PREFIX/${LIB_DIR}/${LIB_SUBDIR}pkgconfig
rm -r $OPT_PREFIX/share/doc
rm -r $OPT_PREFIX/share/man

## Sad adjustments (appimagetool don't handle this gracefully when done before deploy)
### https://github.com/probonopd/go-appimage/issues/284
sed -i "s|\"/usr/|\"|g" "$OPT_PREFIX/${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-2.0/2.10.0/loaders.cache"
### https://github.com/probonopd/go-appimage/issues/282
cp -r "/usr/${LIB_DIR}/${LIB_SUBDIR}gtk-3.0/3.0.0/immodules.cache" "$OPT_PREFIX/${LIB_DIR}/${LIB_SUBDIR}gtk-3.0/3.0.0"
sed -i "s|\"/usr/|\"|g" "$OPT_PREFIX/${LIB_DIR}/${LIB_SUBDIR}gtk-3.0/3.0.0/immodules.cache"


# CONFIGURE APPRUN
cp build/linux/appimage/AppRun $GIMP_DISTRIB

if [ -z "$2" ] || [ "$2" = "usr" ]; then
  sed -i "s|OPT_PREFIX_WILD|usr/|g" $GIMP_DISTRIB/AppRun
elif [ "$2" = "AppDir" ]; then
  sed -i "s|OPT_PREFIX_WILD||g" $GIMP_DISTRIB/AppRun
fi

GIMP_APP_VERSION=$(grep GIMP_APP_VERSION $BUILD_ARTIF/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
sed -i "s|GIMP_APP_VERSION|${GIMP_APP_VERSION}|" $GIMP_DISTRIB/AppRun


# CONFIGURE METADATA
sed -i '/kudo/d' $OPT_PREFIX/share/metainfo/org.gimp.GIMP.appdata.xml
if [ "$2" = "AppDir" ]; then
  mkdir -p $GIMP_PREFIX/share
  cp -r $GIMP_DISTRIB/share/metainfo $GIMP_PREFIX/share
  cp -r $GIMP_DISTRIB/share/applications $GIMP_PREFIX/share
fi


# CONFIGURE ICON
cp $OPT_PREFIX/share/icons/hicolor/scalable/apps/org.gimp.GIMP.svg $GIMP_DISTRIB/org.gimp.GIMP.svg
if [ "$2" = "AppDir" ]; then
  cp -r $GIMP_DISTRIB/share/icons/ $GIMP_PREFIX/share
fi


# MAKE APPIMAGE
"./$legacy_appimagetool" --appimage-extract-and-run $GIMP_DISTRIB # -u "zsync|https://download.gimp.org/gimp/v${GIMP_APP_VERSION}/GIMP-latest-${ARCH}.AppImage.zsync"
mkdir build/linux/appimage/_Output
GIMP_VERSION=$(grep GIMP_VERSION $BUILD_ARTIF/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
mv GNU*.AppImage build/linux/appimage/_Output/GIMP-${GIMP_VERSION}-${ARCH}.AppImage
