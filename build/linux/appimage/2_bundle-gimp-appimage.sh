#!/bin/sh

# Loosely based on:
# https://github.com/AppImage/AppImageSpec/blob/master/draft.md
# https://gitlab.com/inkscape/inkscape/-/commit/b280917568051872793a0c7223b8d3f3928b7d26

set -e

if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
  if [ "$0" != 'build/linux/appimage/2_bundle-gimp-appimage.sh' ] && [ ${PWD/*\//} != 'appimage' ]; then
    echo -e '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, call this script from the root of gimp git dir'
    exit 1
  elif [ ${PWD/*\//} = 'appimage' ]; then
    cd ../../..
  fi
fi


# SPECIAL BUILDING

## We apply these patches otherwise appstream-cli get confused with
## (non-reverse) DNS naming and fails. That's NOT a GIMP bug, see: #6798
echo '(INFO): patching GIMP with reverse DNS naming'
git apply -v build/linux/appimage/patches/0001-desktop-po-Use-reverse-DNS-naming.patch >/dev/null 2>&1
cd gimp-data
git apply -v ../build/linux/appimage/patches/0001-images-logo-Use-reverse-DNS-naming.patch >/dev/null 2>&1
cd ..

## Prepare env. Universal variables from .gitlab-ci.yml
IFS=$'\n' VAR_ARRAY=($(cat .gitlab-ci.yml | sed -n '/export PATH=/,/GI_TYPELIB_PATH}\"/p' | sed 's/    - //'))
IFS=$' \t\n'
for VAR in "${VAR_ARRAY[@]}"; do
  eval "$VAR" || continue
done

## Rebuild GIMP
echo '(INFO): rebuilding GIMP as relocatable'
meson configure _build -Drelocatable-bundle=yes -Dvector-icons=true >/dev/null 2>&1
mkdir -p build/linux/appimage/_Output
cd _build
ninja &> ../build/linux/appimage/_Output/ninja.log
if [ $? -ne 0 ]; then
  cat ../build/linux/appimage/_Output/ninja.log
else
  rm ../build/linux/appimage/_Output/ninja.log
fi
ninja install >/dev/null 2>&1
ccache --show-stats
cd ..


# AGNOSTIC VARIABLES (only touch them to make even more portable, without casuistry)

## This script is "filesystem-agnostic". The packager can quickly choose either
## putting everything in /usr or in AppDir(root) just specifying the 2nd parameter.
GIMP_DISTRIB="$CI_PROJECT_DIR/build/linux/appimage/AppDir"
if [ "$GITLAB_CI" ] || [ -z "$GIMP_PREFIX" ]; then
  GIMP_PREFIX="$GIMP_DISTRIB/usr"
fi
if [ -z "$1" ] || [ "$1" = "usr" ]; then
  OPT_PREFIX="${GIMP_PREFIX}"
elif [ "$1" = "AppDir" ]; then
  OPT_PREFIX="${GIMP_DISTRIB}"
fi

#(MOSTLY) AGNOSTIC FUNCTIONS
prep_pkg ()
{
  apt-get install -y --no-install-recommends $1 >/dev/null 2>&1
}

find_bin ()
{
  find /usr/bin -name ${1} -execdir cp -r '{}' $OPT_PREFIX/bin \;
  find /bin -name ${1} -execdir cp -r '{}' $OPT_PREFIX/bin \;
}

find_lib ()
{
  mkdir -p $OPT_PREFIX/${LIB_DIR}/${LIB_SUBDIR}
  find /usr/${LIB_DIR}/${LIB_SUBDIR} -maxdepth 1 -name ${1} -execdir cp -r '{}' $OPT_PREFIX/${LIB_DIR}/${LIB_SUBDIR} \;
  find /usr/${LIB_DIR} -maxdepth 1 -name ${1} -execdir cp -r '{}' $OPT_PREFIX/${LIB_DIR}/${LIB_SUBDIR} \;
  find /${LIB_DIR}/${LIB_SUBDIR} -maxdepth 1 -name ${1} -execdir cp -r '{}' $OPT_PREFIX/${LIB_DIR}/${LIB_SUBDIR} \;
  find /${LIB_DIR} -maxdepth 1 -name ${1} -execdir cp -r '{}' $OPT_PREFIX/${LIB_DIR}/${LIB_SUBDIR} \;
}

find_dat ()
{
  DAT_PATH=$(echo $1 | sed 's|/usr/||g')
  mkdir -p $OPT_PREFIX/$DAT_PATH
  cp -r $1/$2 $OPT_PREFIX/$DAT_PATH/$3
}

conf_app ()
{
  VAR_PATH=$(echo $2/$3 | sed "s|${2}/||g")
  sed -i "s|${1}_WILD|OPT_PREFIX_WILD${VAR_PATH}|" build/linux/appimage/AppRun
}


# PREPARE ENVIRONMENT
apt-get install -y --no-install-recommends wget >/dev/null 2>&1

## For now, we always use the latest version of go-appimagetool
wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-x86_64.AppImage" | head -n 1 | cut -d '"' -f 2) >/dev/null 2>&1
mv *.AppImage appimagetool.appimage
go_appimagetool=appimagetool.appimage
chmod +x "$go_appimagetool"

## go-appimagetool have buggy appstreamcli so we need to use the legacy one
legacy_appimagetool="appimagetool-x86_64.AppImage"
wget "https://github.com/AppImage/AppImageKit/releases/download/continuous/$legacy_appimagetool" >/dev/null 2>&1
chmod +x "$legacy_appimagetool"


# BUNDLE FILES
echo '(INFO): making .appimage bundle'

## System base (needed to use GIMP or to avoid polluting the terminal output)
conf_app LD_LINUX "/usr" "lib64/ld-*.so.*"
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
cp -r _install/* $OPT_PREFIX
conf_app BABL_PATH "$OPT_PREFIX" "${LIB_DIR}/${LIB_SUBDIR}babl-*"
conf_app GEGL_PATH "$OPT_PREFIX" "${LIB_DIR}/${LIB_SUBDIR}gegl-*"
conf_app GIMP3_SYSCONFDIR "$OPT_PREFIX" "etc/gimp/*"
conf_app GIMP3_DATADIR "$OPT_PREFIX" "share/gimp/*"
### Copy system theme support
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
conf_app PYTHONPATH "/usr" "${LIB_DIR}/${LIB_SUBDIR}python3.11"

## Final adjustments
### Auto detect and copy deps of binaries copied above
"./$go_appimagetool" --appimage-extract-and-run -s deploy $OPT_PREFIX/share/applications/org.gimp.GIMP.desktop >/dev/null 2>&1
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

GIMP_APP_VERSION=$(grep GIMP_APP_VERSION _build/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
sed -i "s|GIMP_APP_VERSION|${GIMP_APP_VERSION}|" $GIMP_DISTRIB/AppRun


# CONFIGURE METADATA
sed -i '/kudo/d' $OPT_PREFIX/share/metainfo/org.gimp.GIMP.appdata.xml
sed -i "s/date=\"TODO\"/date=\"`date --iso-8601`\"/" $OPT_PREFIX/share/metainfo/org.gimp.GIMP.appdata.xml
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
"./$legacy_appimagetool" --appimage-extract-and-run $GIMP_DISTRIB &> build/linux/appimage/_Output/appimagetool.log # -u "zsync|https://download.gimp.org/gimp/v${GIMP_APP_VERSION}/GIMP-latest-$(uname -m).AppImage.zsync"
GIMP_VERSION=$(grep GIMP_VERSION _build/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
mv GNU*.AppImage build/linux/appimage/_Output/GIMP-${GIMP_VERSION}-$(uname -m).AppImage
