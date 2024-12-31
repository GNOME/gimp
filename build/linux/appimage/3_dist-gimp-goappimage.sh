#!/bin/sh

# Loosely based on:
# https://github.com/AppImage/AppImageSpec/blob/master/draft.md
# https://gitlab.com/inkscape/inkscape/-/commit/b280917568051872793a0c7223b8d3f3928b7d26

set -e

if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
  if [ "$0" != 'build/linux/appimage/3_dist-gimp-goappimage.sh' ] && [ ${PWD/*\//} != 'appimage' ]; then
    echo -e '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, call this script from the root of gimp git dir'
    exit 1
  elif [ ${PWD/*\//} = 'appimage' ]; then
    cd ../../..
  fi
fi


# 1. INSTALL GO-APPIMAGETOOL AND COMPLEMENTARY TOOLS
echo -e "\e[0Ksection_start:`date +%s`:apmg_tlkt\r\e[0KInstalling (go)appimagetool and other tools"
if [ -f "*appimagetool*.AppImage" ]; then
  rm *appimagetool*.AppImage
  rm runtime*
fi
if [ "$GITLAB_CI" ]; then
  apt-get update >/dev/null 2>&1
  apt-get install -y --no-install-recommends ca-certificates wget curl >/dev/null 2>&1
fi
export HOST_ARCH=$(uname -m)
export APPIMAGE_EXTRACT_AND_RUN=1

if [ "$(ls -dq ./AppDir* 2>/dev/null | wc -l)" != '2' ]; then
  ## For now, we always use the latest go-appimagetool for bundling. See: https://github.com/probonopd/go-appimage/issues/275
  if [ "$GITLAB_CI" ]; then
    apt-get install -y --no-install-recommends patchelf >/dev/null 2>&1
  fi
  wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-${HOST_ARCH}.AppImage" | head -n 1 | cut -d '"' -f 2) >/dev/null 2>&1
  go_appimagetool_text="go-appimagetool build: $(echo appimagetool-*.AppImage | sed -e 's/appimagetool-//' -e "s/-${HOST_ARCH}.AppImage//")"
  go_appimagetool='go-appimagetool.AppImage'
  mv appimagetool-*.AppImage $go_appimagetool
  chmod +x "$go_appimagetool"
fi

if [ "$1" != '--bundle-only' ]; then
  ## standard appimagetool is needed for squashing the .appimage file
  if [ "$GITLAB_CI" ]; then
    apt-get install -y --no-install-recommends file zsync >/dev/null 2>&1
  fi
  wget "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-${HOST_ARCH}.AppImage" >/dev/null 2>&1
  standard_appimagetool='legacy-appimagetool.AppImage'
  mv appimagetool-*.AppImage $standard_appimagetool
  chmod +x "$standard_appimagetool"
  standard_appimagetool_version=$("./$standard_appimagetool" --version 2>&1 | sed -e 's/.*version \(.*\)), build.*/\1/')

  ## static runtime to be squashed by appimagetool with the files bundled by go-appimagetool
  static_runtime_version_online=$(curl -s 'https://api.github.com/repos/AppImage/type2-runtime/releases' |
                                  grep -Po '"target_commitish":.*?[^\\]",' | head -1 |
                                  sed -e 's|target_commitish||g' -e 's|"||g' -e 's|:||g' -e 's|,||g' -e 's| ||g')
  wget https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-aarch64 >/dev/null 2>&1
  wget https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64 >/dev/null 2>&1
  chmod +x "./runtime-$HOST_ARCH"
  static_runtime_version_downloaded=$("./runtime-$HOST_ARCH" --appimage-version 2>&1)
  chmod -x "./runtime-$HOST_ARCH"
  if [ "${static_runtime_version_downloaded#*commit/}" != "${static_runtime_version_online:0:7}" ]; then
    exit 1
  fi
  standard_appimagetool_text="appimagetool commit: $standard_appimagetool_version | type2-runtime commit: ${static_runtime_version_downloaded#*commit/}"
fi
if [ "$(ls -dq ./AppDir* 2>/dev/null | wc -l)" != '2' ] && [ "$1" != '--bundle-only' ]; then
  separator=' | '
fi
echo "(INFO): ${go_appimagetool_text}${separator}${standard_appimagetool_text}"
echo -e "\e[0Ksection_end:`date +%s`:apmg_tlkt\r\e[0K"


# 2. GET GLOBAL VARIABLES
echo -e "\e[0Ksection_start:`date +%s`:apmg_info\r\e[0KGetting AppImage global info"
if [ "$1" != '' ] && [[ ! "$1" =~ "--" ]]; then
  export BUILD_DIR="$1"
else
  export BUILD_DIR=$(echo $PWD/_build*$RUNNER)
fi

## Get info about GIMP version
GIMP_VERSION=$(grep GIMP_VERSION $BUILD_DIR/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
grep -q '#define GIMP_UNSTABLE' $BUILD_DIR/config.h && export GIMP_UNSTABLE=1
if [ -z "$CI_COMMIT_TAG" ] && [ "$GIMP_UNSTABLE" ] || [[ "$GIMP_VERSION" =~ 'git' ]]; then
  export APP_ID="org.gimp.GIMP.Continuous"
else
  echo -e "\033[31m(ERROR)\033[0m: AppImage releases are NOT supported yet. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/7661"
  exit 1
fi
echo "(INFO): App ID: $APP_ID | Version: $GIMP_VERSION"
echo -e "\e[0Ksection_end:`date +%s`:apmg_info\r\e[0K"


# 3. GIMP FILES
if [ "$(ls -dq ./AppDir* 2>/dev/null | wc -l)" != '2' ]; then
echo -e "\e[0Ksection_start:`date +%s`:apmg_files[collapsed=true]\r\e[0KPreparing GIMP files in AppDir-$HOST_ARCH/usr"

## 3.1. Special re-building (only if needed)

### Prepare env. Universal variables from .gitlab-ci.yml
if [ -z "$GITLAB_CI" ]; then
  IFS=$'\n' VAR_ARRAY=($(cat .gitlab-ci.yml | sed -n '/export PATH=/,/GI_TYPELIB_PATH}\"/p' | sed 's/    - //'))
  IFS=$' \t\n'
  for VAR in "${VAR_ARRAY[@]}"; do
    eval "$VAR" || continue
  done
fi

### Ensure that GIMP is relocatable
grep -q 'relocatable-bundle=yes' $BUILD_DIR/meson-logs/meson-log.txt && export RELOCATABLE_BUNDLE_ON=1
if [ -z "$RELOCATABLE_BUNDLE_ON" ]; then
  if [ ! -f "$BUILD_DIR/build.ninja" ]; then
    echo -e "\033[31m(ERROR)\033[0m: No GIMP build found. You can configure GIMP with '-Drelocatable-bundle=yes' to make a build suitable for AppImage."
  else
    echo "(INFO): rebuilding GIMP as relocatable"
    meson configure $BUILD_DIR -Drelocatable-bundle=yes >/dev/null 2>&1
  fi
  cd $BUILD_DIR
  ninja &> ninja.log | rm ninja.log || cat ninja.log
  ninja install >/dev/null 2>&1
  ccache --show-stats
  cd ..
fi


## 3.2. Bundle files

#Prefixes to get files to copy
UNIX_PREFIX='/usr'
if [ -z "$GITLAB_CI" ] && [ -z "$GIMP_PREFIX" ]; then
  export GIMP_PREFIX="$PWD/../_install"
fi

#Paths to receive copied files
APP_DIR="$PWD/AppDir-$HOST_ARCH"
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
        output_dest_path="$dest_path"
        if [ "$3" = '--dest' ]; then
          output_dest_path="${USR_DIR}/$4"
          dest_path="$output_dest_path/tmp"
        fi
        mkdir -p $dest_path
        echo "(INFO): bundling $target_path to $output_dest_path"
        cp -ru $target_path $dest_path >/dev/null 2>&1 || continue

        #Additional parameters for special situations
        if [ "$3" = '--dest' ]; then
          mv $dest_path/${2##*/} $(dirname $dest_path)
          rm -r "$dest_path"
        elif [ "$3" = '--rename' ]; then
          mv $dest_path/${2##*/} $dest_path/$4
        fi
      done
    done

    #Undo the tweak done above
    cd ..
    rm -r limbo
  else
    echo "(INFO): skipping $1/$2 (will be bundled by go-appimagetool)"
  fi
}

conf_app ()
{
  #Make backup of AppRun before changing it
  if [ ! -f 'build/linux/appimage/AppRun.bak' ]; then
    cp build/linux/appimage/AppRun build/linux/appimage/AppRun.bak
  fi

  #Prefix from which to expand the var
  prefix=$UNIX_PREFIX
  case $1 in
    *BABL*|*GEGL*|*GIMP*)
      prefix=$GIMP_PREFIX
  esac

  #Set expanded var in AppRun (and temporarely in environ if needed by this script)
  var_path=$(echo $prefix/$2 | sed "s|${prefix}/||g")
  sed -i "s|${1}_WILD|usr/${var_path}|" build/linux/appimage/AppRun
  eval $1="usr/$var_path"
}

wipe_usr ()
{
  find $USR_DIR -iname ${1##*/} -execdir rm -r -f "{}" \;
}

## Prepare AppDir
mkdir -p $APP_DIR
bund_usr "$UNIX_PREFIX" "lib*/ld-*.so.*" --go
if [ "$HOST_ARCH" = 'aarch64' ]; then
  conf_app LD_LINUX "lib/ld-*.so.*"
else
  conf_app LD_LINUX "lib64/ld-*.so.*"
fi

## Bundle base (bare minimum to run GTK apps)
### Glib needed files (to be able to use file dialogs)
bund_usr "$UNIX_PREFIX" "share/glib-*/schemas"
### Glib commonly required modules
prep_pkg "gvfs"
bund_usr "$UNIX_PREFIX" "lib/gvfs*"
bund_usr "$UNIX_PREFIX" "bin/gvfs*" --dest "${LIB_DIR}/gvfs"
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
  # Needed for eventually used widgets, GTK inspector etc
  bund_usr "$UNIX_PREFIX" share/locale/$lang/LC_MESSAGES/gtk3*.mo
  # For language list in text tool options
  bund_usr "$UNIX_PREFIX" share/locale/$lang/LC_MESSAGES/iso_639*3.mo
done
conf_app GIMP3_LOCALEDIR "share/locale"
bund_usr "$GIMP_PREFIX" "etc/gimp"
conf_app GIMP3_SYSCONFDIR "etc/gimp/*"

## Other features and plug-ins
### Needed for welcome page
bund_usr "$GIMP_PREFIX" "share/metainfo/*.xml" --rename $APP_ID.appdata.xml
sed -i '/kudo/d' $USR_DIR/share/metainfo/$APP_ID.appdata.xml
sed -i "s/date=\"TODO\"/date=\"`date --iso-8601`\"/" $USR_DIR/share/metainfo/$APP_ID.appdata.xml
### mypaint brushes
bund_usr "$UNIX_PREFIX" "share/mypaint-data/1.0"
### Needed for 'th' word breaking in Text tool etc
bund_usr "$UNIX_PREFIX" "share/libthai"
conf_app LIBTHAI_DICTDIR "share/libthai"
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
bund_usr "$GIMP_PREFIX" "share/applications/*.desktop" --rename $APP_ID.desktop
#go-appimagetool have too polluted output so we save as log. See: https://github.com/probonopd/go-appimage/issues/314
"./$go_appimagetool" -s deploy $USR_DIR/share/applications/$APP_ID.desktop &> appimagetool.log || cat appimagetool.log

## Manual adjustments (go-appimagetool don't handle Linux FHS gracefully yet)
### Undo the mess which breaks babl and GEGL. See: https://github.com/probonopd/go-appimage/issues/315
cp -r $APP_DIR/lib/* $USR_DIR/${LIB_DIR}
rm -r $APP_DIR/lib
### Ensure that LD is in right dir. See: https://github.com/probonopd/go-appimage/issues/49
if [ "$HOST_ARCH" = 'x86_64' ]; then
  cp -r $APP_DIR/lib64 $USR_DIR
  rm -r $APP_DIR/lib64
fi
chmod +x "$APP_DIR/$LD_LINUX"
exec_array=($(find "$USR_DIR/bin" "$USR_DIR/$LIB_DIR" ! -iname "*.so*" -type f -exec head -c 4 {} \; -exec echo " {}" \;  | grep ^.ELF))
for exec in "${exec_array[@]}"; do
  if [[ ! "$exec" =~ 'ELF' ]]; then
    patchelf --set-interpreter "./$LD_LINUX" "$exec" >/dev/null 2>&1 || continue
  fi
done
## Unnecessary files created or bundled by go-appimagetool
mv build/linux/appimage/AppRun $APP_DIR
mv build/linux/appimage/AppRun.bak build/linux/appimage/AppRun
echo "usr/${LIB_DIR}/${LIB_SUBDIR}gconv
      usr/${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-*/gdk-pixbuf-query-loaders
      usr/share/doc
      usr/share/themes
      etc" > appimageignore-$HOST_ARCH
echo -e "\e[0Ksection_end:`date +%s`:apmg_files\r\e[0K"
fi
if [ "$1" = '--bundle-only' ] || [ "$2" = '--bundle-only' ]; then
  exit 0
fi


# 4. PREPARE .APPIMAGE-SPECIFIC "SOURCE"
appdir_array=($(find . -maxdepth 1 -iname "AppDir*"))
for APP_DIR in "${appdir_array[@]}"; do
export ARCH=$(echo $APP_DIR | sed -e 's|AppDir-||' -e 's|./||')
echo -e "\e[0Ksection_start:`date +%s`:${ARCH}_source[collapsed=true]\r\e[0KMaking AppImage assets for $ARCH"
export USR_DIR="$APP_DIR/usr"

## 4.1. Finish AppRun configuration
echo '(INFO): finishing AppRun configuration'
sed -i '/_WILD/d' "$APP_DIR/AppRun"
sed -i "s/APP_ID/$APP_ID/" "$APP_DIR/AppRun"
chmod +x $APP_DIR/AppRun

## 4.2. Copy icon assets (similarly to flatpaks's 'rename-icon')
echo "(INFO): copying $APP_ID.svg asset to AppDir"
find "$USR_DIR/share/icons/hicolor" -iname *.svg -execdir ln -sf "{}" $APP_ID.svg \;
find "$USR_DIR/share/icons/hicolor" -iname *.png -execdir ln -sf "{}" $APP_ID.png \;
cp -L "$USR_DIR/share/icons/hicolor/scalable/apps/$APP_ID.svg" $APP_DIR

## 4.3. Configure .desktop asset (similarly to flatpaks's 'rename-desktop-file')
echo "(INFO): configuring $APP_ID.desktop"
sed -i "s/Icon=gimp/Icon=$APP_ID/g" "$USR_DIR/share/applications/${APP_ID}.desktop"
gimp_app_version=$(grep GIMP_APP_VERSION $BUILD_DIR/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
ln -sfr "$USR_DIR/bin/gimp-$gimp_app_version" "$USR_DIR/bin/$APP_ID"
sed -i "s/gimp-$gimp_app_version/$APP_ID/g" "$USR_DIR/share/applications/${APP_ID}.desktop"
ln -sfr "$USR_DIR/share/applications/${APP_ID}.desktop" $APP_DIR

## 4.4. Configure appdata asset (similarly to flatpaks's 'rename-appdata-file')
echo "(INFO): configuring $APP_ID.appdata.xml"
sed -i "s/org.gimp.GIMP/${APP_ID}/g" "$USR_DIR/share/metainfo/${APP_ID}.appdata.xml"
sed -i "s/gimp.desktop/${APP_ID}.desktop/g" "$USR_DIR/share/metainfo/${APP_ID}.appdata.xml"
echo -e "\e[0Ksection_end:`date +%s`:${ARCH}_source\r\e[0K"


# 5. CONSTRUCT .APPIMAGE
APPIMAGETOOL_APP_NAME="GIMP-${GIMP_VERSION}-${ARCH}.AppImage"
echo -e "\e[0Ksection_start:`date +%s`:${ARCH}_making[collapsed=true]\r\e[0KSquashing $APPIMAGETOOL_APP_NAME"
"./$standard_appimagetool" $APP_DIR $APPIMAGETOOL_APP_NAME --exclude-file appimageignore-$ARCH \
                                                           --runtime-file runtime-$ARCH #\
                                                          #--updateinformation "zsync|https://gitlab.gnome.org/GNOME/gimp/-/jobs/artifacts/master/raw/build/linux/appimage/_Output/${APPIMAGETOOL_APP_NAME}.zsync?job=dist-appimage-weekly"
file "./$APPIMAGETOOL_APP_NAME"
echo -e "\e[0Ksection_end:`date +%s`:${ARCH}_making\r\e[0K"
done

if [ "$GITLAB_CI" ]; then
  mkdir -p build/linux/appimage/_Output/
  mv GIMP*.AppImage* build/linux/appimage/_Output/
fi
