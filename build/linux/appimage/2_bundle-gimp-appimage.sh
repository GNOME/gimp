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
echo '(INFO): temporarily patching GIMP with reverse DNS naming'
patch_app_id ()
{
  git apply $1 build/linux/appimage/patches/0001-desktop-po-Use-reverse-DNS-naming.patch >/dev/null 2>&1 || true
  cd gimp-data
  git apply $1 ../build/linux/appimage/patches/0001-images-logo-Use-reverse-DNS-naming.patch >/dev/null 2>&1 || true
  cd ..
}
patch_app_id

## Prepare env. Universal variables from .gitlab-ci.yml
IFS=$'\n' VAR_ARRAY=($(cat .gitlab-ci.yml | sed -n '/export PATH=/,/GI_TYPELIB_PATH}\"/p' | sed 's/    - //'))
IFS=$' \t\n'
for VAR in "${VAR_ARRAY[@]}"; do
  eval "$VAR" || continue
done

## Ensure that GIMP is relocatable
grep -q 'relocatable-bundle=yes' _build/meson-logs/meson-log.txt && export RELOCATABLE_BUNDLE_ON=1
if [ -z "$RELOCATABLE_BUNDLE_ON" ]; then
  echo '(INFO): rebuilding GIMP as relocatable'
  ### FIXME: GIMP tests fails with raster icons in relocatable mode
  meson configure _build -Drelocatable-bundle=yes -Dvector-icons=true >/dev/null 2>&1
  cd _build
  ninja &> ninja.log | rm ninja.log || cat ninja.log
  ninja install >/dev/null 2>&1
  ccache --show-stats
  cd ..
fi

## Revert previously applied patches
patch_app_id '-R'


# INSTALL GO-APPIMAGETOOL AND COMPLEMENTARY TOOLS
echo '(INFO): downloading go-appimagetool'
if [ -f "*appimagetool*.AppImage" ]; then
  rm *appimagetool*.AppImage
fi
if [ "$GITLAB_CI" ]; then
  apt-get install -y --no-install-recommends wget >/dev/null 2>&1
  apt-get install -y --no-install-recommends patchelf >/dev/null 2>&1
fi
export ARCH=$(uname -m)
export APPIMAGE_EXTRACT_AND_RUN=1

## For now, we always use the latest version of go-appimagetool
wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-${ARCH}.AppImage" | head -n 1 | cut -d '"' -f 2) >/dev/null 2>&1
echo "(INFO): Downloaded go-appimagetool: $(echo appimagetool-*.AppImage | sed -e 's/appimagetool-//' -e "s/-${ARCH}.AppImage//")"
go_appimagetool='go-appimagetool.AppImage'
mv appimagetool-*.AppImage $go_appimagetool
chmod +x "$go_appimagetool"

## go-appimagetool have buggy appstreamcli so we need to use the legacy one
wget "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-${ARCH}.AppImage" >/dev/null 2>&1
legacy_appimagetool='legacy-appimagetool.AppImage'
mv appimagetool-*.AppImage $legacy_appimagetool
chmod +x "$legacy_appimagetool"


# BUNDLE FILES
grep -q '#define GIMP_UNSTABLE' _build/config.h && export GIMP_UNSTABLE=1

UNIX_PREFIX='/usr'
if [ -z "$GITLAB_CI" ] && [ -z "$GIMP_PREFIX" ]; then
  export GIMP_PREFIX="$PWD/../_install"
fi
APP_DIR="$PWD/AppDir"
USR_DIR="$APP_DIR/usr"

prep_pkg ()
{
  if [ "$GITLAB_CI" ]; then
    apt-get install -y --no-install-recommends $1 >/dev/null 2>&1
  fi
}

bund_usr ()
{
  if [ "$3" != '--go' ]; then
    #Prevent unwanted expansion
    mkdir -p limbo
    cd limbo

    #Paths where to search
    case $2 in
      bin*)
        search_path=("$1/bin" "/usr/sbin" "/usr/libexec")
        ;;
      lib*)
        search_path=("$(dirname $(echo $2 | sed "s|lib/|$1/${LIB_DIR}/${LIB_SUBDIR}|g" | sed "s|*|no_scape|g"))"
                     "$(dirname $(echo $2 | sed "s|lib/|/usr/${LIB_DIR}/|g" | sed "s|*|no_scape|g"))")
        ;;
      share*|etc*)
        search_path=("$(dirname $(echo $2 | sed "s|${2%%/*}|$1/${2%%/*}|g" | sed "s|*|no_scape|g"))")
        ;;
    esac
    for path in "${search_path[@]}"; do
      expanded_path=$(echo $(echo $path | sed "s|no_scape|*|g"))
      if [ ! -d "$expanded_path" ]; then
        break
      fi

      #Copy found targets from search_path to bundle dir
      target_array=($(find $expanded_path -maxdepth 1 -name ${2##*/}))
      for target_path in "${target_array[@]}"; do
        dest_path="$(dirname $(echo $target_path | sed "s|$1/|${USR_DIR}/|g"))"
        mkdir -p $dest_path
        if [ -d "$target_path" ] || [ -f "$target_path" ]; then
          cp -ru $target_path $dest_path >/dev/null 2>&1 || continue
        fi

        #Additional parameters for special situations
        if [ "$3" = '--dest' ] || [ "$3" = '--rename' ]; then
          if [ "$3" = '--dest' ]; then
            mkdir -p ${USR_DIR}/$4
          elif [ "$3" = '--rename' ]; then
            mkdir -p $(dirname ${USR_DIR}/$4)
          fi
          mv $dest_path/${2##*/} ${USR_DIR}/$4
          if [ -z "$(ls -A "$dest_path")" ]; then
            rm -r "$dest_path"
          fi
        fi
      done
    done

    #Undo the tweak done above
    cd ..
    rm -r limbo
  fi
}

conf_app ()
{
  prefix=$UNIX_PREFIX
  case $1 in
    *BABL*|*GEGL*|*GIMP*)
      prefix=$GIMP_PREFIX
  esac
  var_path=$(echo $prefix/$2 | sed "s|${prefix}/||g")
  sed -i "s|${1}_WILD|usr/${var_path}|" build/linux/appimage/AppRun
  eval $1="usr/$var_path"
}

wipe_usr ()
{
  if [[ ! "$1" =~ '*' ]]; then
    rm -r $USR_DIR/$1
  else
    cleanedArray=($(find $USR_DIR -iname ${1##*/}))
    for path_dest_full in "${cleanedArray[@]}"; do
      rm -r -f $path_dest_full
    done
  fi
}

## Prepare AppDir
echo '(INFO): copying files to AppDir'
if [ ! -f 'build/linux/appimage/AppRun.bak' ]; then
  cp build/linux/appimage/AppRun build/linux/appimage/AppRun.bak
fi
mkdir -p $APP_DIR
bund_usr "$UNIX_PREFIX" "lib64/ld-*.so.*" --go
conf_app LD_LINUX "lib64/ld-*.so.*"

## Bundle base (bare minimum to run GTK apps)
### Glib needed files (to be able to use file dialogs)
bund_usr "$UNIX_PREFIX" "share/glib-*/schemas"
### Glib commonly required modules
prep_pkg "gvfs"
bund_usr "$UNIX_PREFIX" "lib/gvfs*"
bund_usr "$UNIX_PREFIX" "bin/gvfs*" --dest "lib/gvfs"
bund_usr "$UNIX_PREFIX" "lib/gio*"
conf_app GIO_MODULE_DIR "${LIB_DIR}/${LIB_SUBDIR}gio"
### GTK needed files (to be able to load icons)
bund_usr "$UNIX_PREFIX" "share/icons/Adwaita"
bund_usr "$GIMP_PREFIX" "share/icons/hicolor"
bund_usr "$UNIX_PREFIX" "share/mime"
bund_usr "$UNIX_PREFIX" "lib/gdk-pixbuf-*" --go
conf_app GDK_PIXBUF_MODULEDIR "${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-*/*.*.*"
conf_app GDK_PIXBUF_MODULE_FILE "${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-*/*.*.*"
### GTK commonly required modules
prep_pkg "libibus-1.0-5"
bund_usr "$UNIX_PREFIX" "lib/libibus*"
prep_pkg "ibus-gtk3"
prep_pkg "libcanberra-gtk3-module"
prep_pkg "libxapp-gtk3-module"
bund_usr "$UNIX_PREFIX" "lib/gtk-*" --go
conf_app GTK_PATH "${LIB_DIR}/${LIB_SUBDIR}gtk-3.0"
conf_app GTK_IM_MODULE_FILE "${LIB_DIR}/${LIB_SUBDIR}gtk-3.0/*.*.*"

## Core features
bund_usr "$GIMP_PREFIX" "lib/libbabl*"
bund_usr "$GIMP_PREFIX" "lib/babl-*"
conf_app BABL_PATH "${LIB_DIR}/${LIB_SUBDIR}babl-*"
bund_usr "$GIMP_PREFIX" "lib/libgegl*"
bund_usr "$GIMP_PREFIX" "lib/gegl-*"
conf_app GEGL_PATH "${LIB_DIR}/${LIB_SUBDIR}gegl-*"
bund_usr "$GIMP_PREFIX" "lib/libgimp*"
bund_usr "$GIMP_PREFIX" "lib/gimp"
conf_app GIMP3_PLUGINDIR "${LIB_DIR}/${LIB_SUBDIR}gimp/*"
bund_usr "$GIMP_PREFIX" "share/gimp"
conf_app GIMP3_DATADIR "share/gimp/*"
lang_array=($(echo $(ls po/*.po |
              sed -e 's|po/||g' -e 's|.po||g' | sort) |
              tr '\n\r' ' '))
for lang in "${lang_array[@]}"; do
  bund_usr "$GIMP_PREFIX" share/locale/$lang/LC_MESSAGES
  bund_usr "$UNIX_PREFIX" share/locale/$lang/LC_MESSAGES/gtk3*.mo
  # For language list in text tool options
  bund_usr "$UNIX_PREFIX" share/locale/$lang/LC_MESSAGES/iso_639*3.mo
done
conf_app GIMP3_LOCALEDIR "share/locale"
bund_usr "$GIMP_PREFIX" "etc/gimp"
conf_app GIMP3_SYSCONFDIR "etc/gimp/*"

## Other features and plug-ins
### Needed for welcome page
bund_usr "$GIMP_PREFIX" "share/metainfo/org.gimp*.xml"
sed -i '/kudo/d' $USR_DIR/share/metainfo/org.gimp.GIMP.appdata.xml
sed -i "s/date=\"TODO\"/date=\"`date --iso-8601`\"/" $USR_DIR/share/metainfo/org.gimp.GIMP.appdata.xml
### mypaint brushes
bund_usr "$UNIX_PREFIX" "share/mypaint-data/1.0"
### Needed for full CJK and Cyrillic support in file-pdf
bund_usr "$UNIX_PREFIX" "share/poppler"
### file-wmf support (not portable, depends on how the distro deals with PS fonts)
#bund_usr "$UNIX_PREFIX" "share/libwmf"
if [ "$GIMP_UNSTABLE" ]; then
  ### Image graph support
  bund_usr "$UNIX_PREFIX" "bin/libgvc*" --rename "bin/dot"
  bund_usr "$UNIX_PREFIX" "lib/graphviz/config*"
  bund_usr "$UNIX_PREFIX" "lib/graphviz/libgvplugin_dot*"
  bund_usr "$UNIX_PREFIX" "lib/graphviz/libgvplugin_pango*"
  ### Needed for GTK inspector
  bund_usr "$UNIX_PREFIX" "lib/libEGL*"
  bund_usr "$UNIX_PREFIX" "lib/libGL*"
  bund_usr "$UNIX_PREFIX" "lib/dri*"
fi
### FIXME: Debug dialog (NOT WORKING)
#bund_usr "$UNIX_PREFIX" "bin/lldb*"
#bund_usr "$GIMP_PREFIX" "libexec/gimp-debug-tool*"
### Introspected plug-ins
bund_usr "$GIMP_PREFIX" "lib/girepository-*"
bund_usr "$UNIX_PREFIX" "lib/girepository-*"
conf_app GI_TYPELIB_PATH "${LIB_DIR}/${LIB_SUBDIR}girepository-*"
#### JavaScript plug-ins support
bund_usr "$UNIX_PREFIX" "bin/gjs*"
bund_usr "$UNIX_PREFIX" "lib/gjs/girepository-1.0/Gjs*" --dest "${LIB_DIR}/${LIB_SUBDIR}girepository-1.0"
#### Python plug-ins support
bund_usr "$UNIX_PREFIX" "bin/python*"
bund_usr "$UNIX_PREFIX" "lib/python*"
wipe_usr ${LIB_DIR}/*.pyc
#### Lua plug-ins support (NOT WORKING and buggy)
#bund_usr "$UNIX_PREFIX" "bin/luajit*"
#bund_usr "$UNIX_PREFIX" "lib/lua"
#bund_usr "$UNIX_PREFIX" "share/lua"

## Other binaries and deps
bund_usr "$GIMP_PREFIX" 'bin/gimp*'
bund_usr "$GIMP_PREFIX" "bin/gegl"
bund_usr "$GIMP_PREFIX" "share/applications/org.gimp.GIMP.desktop"
"./$go_appimagetool" -s deploy $USR_DIR/share/applications/org.gimp.GIMP.desktop &> appimagetool.log

## Manual adjustments (go-appimagetool don't handle Linux FHS gracefully)
### Ensure that LD is in right dir
cp -r $APP_DIR/lib64 $USR_DIR
rm -r $APP_DIR/lib64
chmod +x "$APP_DIR/$LD_LINUX"
exec_array=($(find "$USR_DIR/bin" "$USR_DIR/$LIB_DIR" ! -iname "*.so*" -type f -exec head -c 4 {} \; -exec echo " {}" \;  | grep ^.ELF))
for exec in "${exec_array[@]}"; do
  if [[ ! "$exec" =~ 'ELF' ]]; then
    patchelf --set-interpreter "./$LD_LINUX" "$exec" >/dev/null 2>&1 || continue
  fi
done
### Undo the mess that go-appimagetool makes on the prefix which breaks babl and GEGL
cp -r $APP_DIR/lib/* $USR_DIR/${LIB_DIR}
rm -r $APP_DIR/lib
### Remove unnecessary files bunbled by go-appimagetool
wipe_usr ${LIB_DIR}/${LIB_SUBDIR}gconv
wipe_usr ${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-*/gdk-pixbuf-query-loaders
wipe_usr share/doc
wipe_usr share/themes
rm -r $APP_DIR/etc


# FINISH APPIMAGE

## Configure AppRun
echo '(INFO): configuring AppRun'
sed -i '/_WILD/d' build/linux/appimage/AppRun
mv build/linux/appimage/AppRun $APP_DIR
chmod +x $APP_DIR/AppRun
mv build/linux/appimage/AppRun.bak build/linux/appimage/AppRun

## Copy icon to proper place
echo "(INFO): copying org.gimp.GIMP.svg asset to AppDir"
cp $GIMP_PREFIX/share/icons/hicolor/scalable/apps/org.gimp.GIMP.svg $APP_DIR/org.gimp.GIMP.svg

## Construct .appimage
gimp_app_version=$(grep GIMP_APP_VERSION _build/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
gimp_version=$(grep GIMP_VERSION _build/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
appimage="GIMP-${gimp_version}-${ARCH}.AppImage"
echo "(INFO): making $appimage"
"./$legacy_appimagetool" -n $APP_DIR $appimage &>> appimagetool.log # -u "zsync|https://download.gimp.org/gimp/v${gimp_app_version}/GIMP-latest-${ARCH}.AppImage.zsync"
rm -r $APP_DIR

if [ "$GITLAB_CI" ]; then
  mkdir -p build/linux/appimage/_Output/
  mv GIMP*.AppImage build/linux/appimage/_Output/
fi
