#!/bin/sh

# Loosely based on:
# https://github.com/AppImage/AppImageSpec/blob/master/draft.md
# https://gitlab.com/inkscape/inkscape/-/commit/b280917568051872793a0c7223b8d3f3928b7d26

set -e


# AGNOSTIC VARIABLES (only touch them to make even more portable, without casuistry)

## This script is "filesystem-agnostic". The packager can quickly choose either
## putting everything in /usr or in AppDir(root) just specifying the 2nd parameter.
GIMP_DISTRIB="$PWD/build/linux/appimage/AppDir"
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

if [ "$GITLAB_CI" ]; then
  export GIMP_PREFIX="$PWD/_install"
elif [ -z "$GITLAB_CI" ] && [ -z "$GIMP_PREFIX" ]; then
  export GIMP_PREFIX="$PWD/../_install"
fi
export SYSTEM_PREFIX="/usr"


#(MOSTLY) AGNOSTIC FUNCTIONS
prepare ()
{
  if [ "$GITLAB_CI" ]; then
    apt-get install -y --no-install-recommends $1
  fi
}

bundle ()
{
  path_origin_full=$(echo $1/$2)
  if [ -z "$3" ]; then
    # '*utils' mode (tends to be slow)
    check_recurse=${2##*/}
    if [[ "$check_recurse" =~ '*' ]] && [[ ! "$check_recurse" =~ '/' ]]; then
      ## Copy files with wildcards
      path_dest_parent=$(echo $1/${2%/*} | sed "s|${1}/||")
      mkdir -p "$OPT_PREFIX/$path_dest_parent"
      bundledArray=($(find $1/${2%/*} -maxdepth 1 -name ${2##*/}))
      for path_origin_full2 in "${bundledArray[@]}"; do
        echo "(INFO): copying $path_origin_full2 to $OPT_PREFIX/$path_dest_parent"
        cp $path_origin_full2 "$OPT_PREFIX/$path_dest_parent"
      done
    else
      ## Copy specific file or specific folder
      if [[ "$2" =~ '/' ]]; then
        path_dest_parent=$(dirname $path_origin_full | sed "s|${1}/||")
      fi
      if [ -d "$path_origin_full" ] || [ -f "$path_origin_full" ]; then
        mkdir -p "$OPT_PREFIX/$path_dest_parent"
        echo "(INFO): copying $path_origin_full to $OPT_PREFIX/$path_dest_parent"
        cp -r "$path_origin_full" "$OPT_PREFIX/$path_dest_parent"
      else
        echo -e "\033[33m(WARNING)\033[0m: $path_origin_full does not exist!"
      fi
    fi
  else
    # 'go-appimage' mode (tends to be fast)
    echo "(INFO): postponing $path_origin_full (will be bundled by 'go-appimage')"
  fi
}

configure ()
{
  if [ -z "$3" ]; then
    if [[ "$1" =~ 'GIMP' ]]; then
      prefix="$GIMP_PREFIX"
    elif [[ ! "$1" =~ 'GIMP' ]] || [ "$3" ]; then
      prefix="$SYSTEM_PREFIX"
    fi
    echo "(INFO): configuring $1"
    VAR_PATH=$(echo $prefix/$2 | sed "s|$prefix/||g")
    sed -i "s|${1}_WILD|OPT_PREFIX_WILD${VAR_PATH}|" build/linux/appimage/AppRun
  else
    echo "(INFO): postponing $1 configuration (will be configured by 'go-appimage')"
  fi
}

clean ()
{
  if [[ "$2" =~ '/' ]]; then
    cleanedArray=($(find $OPT_PREFIX/${1%/*} -iname ${1##*/}))
  else
    cleanedArray=($(find $OPT_PREFIX/ -iname ${1##*/}))
  fi
  for path_dest_full in "${cleanedArray[@]}"; do
    echo "(INFO): cleaning $path_dest_full"
    rm $path_dest_full
  done
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

## Settings.
bundle $GIMP_PREFIX etc/gimp/
configure GIMP3_SYSCONFDIR etc/gimp/*/
### Needed for fontconfig
bundle $SYSTEM_PREFIX etc/fonts/ --go

## Library data.
configure LD_LINUX lib64/ld-*.so* --go
bundle $GIMP_PREFIX ${LIB_DIR}/${LIB_SUBDIR}gimp/
configure GIMP3_PLUGINDIR ${LIB_DIR}/${LIB_SUBDIR}gimp/*/
bundle $GIMP_PREFIX ${LIB_DIR}/${LIB_SUBDIR}babl-*/
configure BABL_PATH ${LIB_DIR}/${LIB_SUBDIR}babl-*/
bundle $GIMP_PREFIX ${LIB_DIR}/${LIB_SUBDIR}gegl-*/
configure GEGL_PATH ${LIB_DIR}/${LIB_SUBDIR}gegl-*/
### Glib needed files (and its commonly required modules)
#prepare gvfs
#bundle $SYSTEM_PREFIX ${LIB_DIR}/${LIB_SUBDIR}gvfs*
bundle $SYSTEM_PREFIX ${LIB_DIR}/${LIB_SUBDIR}gio/
configure GIO_MODULE_DIR ${LIB_DIR}/${LIB_SUBDIR}gio/ --go
### GTK needed files for the UI work
bundle $SYSTEM_PREFIX ${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-*/*/loaders/libpixbufloader-png.so --go
bundle $SYSTEM_PREFIX ${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-*/*/loaders/libpixbufloader-svg.so --go
configure GDK_PIXBUF_MODULEDIR ${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-*/*/ --go
bundle SYSTEM_PREFIX ${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-*/*/loaders.cache --go
configure GDK_PIXBUF_MODULE_FILE ${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-*/*/ --go
### GTK needed files for printing (and its commonly required modules)
prepare libibus-1.0-5
bundle $SYSTEM_PREFIX ${LIB_DIR}/${LIB_SUBDIR}libibus*
prepare ibus-gtk3
prepare libcanberra-gtk3-module
prepare libxapp-gtk3-module
bundle $SYSTEM_PREFIX ${LIB_DIR}/${LIB_SUBDIR}gtk-3.0/ --go
configure GTK_PATH ${LIB_DIR}/${LIB_SUBDIR}gtk-3.0/ --go
configure GTK_IM_MODULE_FILE ${LIB_DIR}/${LIB_SUBDIR}gtk-3.0/*.*.*/ --go
### GTK inspector support
bundle $SYSTEM_PREFIX ${LIB_DIR}/${LIB_SUBDIR}libEGL*
bundle $SYSTEM_PREFIX ${LIB_DIR}/${LIB_SUBDIR}libGL*
bundle $SYSTEM_PREFIX ${LIB_DIR}/${LIB_SUBDIR}dri/
configure LIBGL_DRIVERS_PATH ${LIB_DIR}/${LIB_SUBDIR}dri/
clean ${LIB_DIR}/${LIB_SUBDIR}*.a


## Resources.
bundle $GIMP_PREFIX share/gimp/
configure GIMP3_DATADIR share/gimp/*/
### Glib needed files for file dialogs work
bundle $SYSTEM_PREFIX share/glib-*/schemas/gschemas.compiled
### https://gitlab.gnome.org/GNOME/gimp/-/issues/6165
prepare adwaita-icon-theme
bundle $SYSTEM_PREFIX share/icons/Adwaita/
### https://gitlab.gnome.org/GNOME/gimp/-/issues/5080
bundle $GIMP_PREFIX share/icons/hicolor/
### Needed for file-wmf work (not needed to be bundled)
#bundle $SYSTEM_PREFIX share/fonts/type1
### Only copy from langs supported in GIMP.
lang_array=($(echo $(ls po/*.po |
              sed -e 's|po/||g' -e 's|.po||g' | sort) |
              tr '\n\r' ' '))
for lang in "${lang_array[@]}"; do
  bundle $GIMP_PREFIX share/locale/$lang/LC_MESSAGES/*.mo
  if [ -d "$SYSTEM_PREFIX/share/locale/$lang/LC_MESSAGES/" ]; then
    bundle $SYSTEM_PREFIX share/locale/$lang/LC_MESSAGES/gtk*.mo
    bundle $SYSTEM_PREFIX share/locale/$lang/LC_MESSAGES/iso_639.mo
  fi
done
### Needed for welcome page
bundle "$GIMP_PREFIX" share/metainfo/org.gimp*.xml
### Needed needed files for the UI work
bundle $SYSTEM_PREFIX share/mime/
### mypaint brushes
bundle $SYSTEM_PREFIX share/mypaint-data/
### Needed for lang selection in Preferences
bundle $SYSTEM_PREFIX share/xml/iso-codes/iso_639-2.xml
mv $OPT_PREFIX/share/xml/iso-codes/iso_639-2.xml $OPT_PREFIX/share/xml/iso-codes/iso_639.xml


### Minimal (and some additional) executables for the 'bin' folder
bundle "$GIMP_PREFIX" bin/gimp*
### https://gitlab.gnome.org/GNOME/gimp/-/issues/10580
bundle "$GIMP_PREFIX" bin/gegl*
### https://gitlab.gnome.org/GNOME/gimp/-/issues/6045
bundle $SYSTEM_PREFIX bin/dot


## GObject Introspection support
bundle $SYSTEM_PREFIX bin/uname*
bundle $GIMP_PREFIX ${LIB_DIR}/${LIB_SUBDIR}girepository-*/
bundle $SYSTEM_PREFIX ${LIB_DIR}/${LIB_SUBDIR}girepository-*/
configure GI_TYPELIB_PATH ${LIB_DIR}/${LIB_SUBDIR}girepository-*/
### Copy JavaScript plug-ins support
bundle $SYSTEM_PREFIX bin/gjs*
### Copy Lua plug-ins support (NOT WORKING)
bundle $SYSTEM_PREFIX bin/lua*
#ln -s $OPT_PREFIX/bin/luajit-2.1.0-beta3 $OPT_PREFIX/bin/luajit
bundle $SYSTEM_PREFIX ${LIB_DIR}/${LIB_SUBDIR}liblua*
prepare lua-lgi
bundle $SYSTEM_PREFIX ${LIB_DIR}/${LIB_SUBDIR}lua/
bundle $SYSTEM_PREFIX share/lua/
### Copy Python plug-ins support
bundle $SYSTEM_PREFIX bin/python*
bundle $SYSTEM_PREFIX ${LIB_DIR}/python*.*/
configure PYTHONPATH ${LIB_DIR}/python*.*/
clean ${LIB_DIR}/python*/*.pyc

### Copy system theme support
bundle $SYSTEM_PREFIX "bin/gsettings*"
bundle $SYSTEM_PREFIX "bin/sed*"





## Final adjustments
### Deps (DLLs) of the binaries in 'lib' and 'bin' dirs
bundle $GIMP_PREFIX share/applications
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
#clean include/
#clean ${LIB_DIR}/${LIB_SUBDIR}pkgconfig/
#clean share/doc/
#clean share/man/

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
"./$legacy_appimagetool" --appimage-extract-and-run $GIMP_DISTRIB # -u "zsync|https://download.gimp.org/gimp/v${GIMP_APP_VERSION}/GIMP-latest-$(uname -m).AppImage.zsync"
mkdir build/linux/appimage/_Output
GIMP_VERSION=$(grep GIMP_VERSION _build/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
mv GNU*.AppImage build/linux/appimage/_Output/GIMP-${GIMP_VERSION}-$(uname -m).AppImage
