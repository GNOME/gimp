#!/bin/sh

# Parameters
REVISION="$1"
if [[ "$GIMP_CI_APPIMAGE" =~ [1-9] ]] && [ "$CI_PIPELINE_SOURCE" != 'schedule' ]; then
  export REVISION="$GIMP_CI_APPIMAGE"
fi
MODE="$2"
if [ "$REVISION" = '--bundle-only' ]; then
  export MODE="$REVISION"
fi
BUILD_DIR="$3"

set -e

if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
  if [ "$0" != 'build/linux/appimage/3_dist-gimp-goappimage.sh' ] && [ ${PWD/*\//} != 'appimage' ]; then
    echo -e '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, call this script from the root of gimp git dir'
    exit 1
  elif [ ${PWD/*\//} = 'appimage' ]; then
    cd ../../..
  fi

  export PARENT_DIR='../'
fi


# 1. INSTALL BUNDLING TOOL AND STANDARD APPIMAGE DISTRIBUTION TOOLS
echo -e "\e[0Ksection_start:`date +%s`:apmg_tlkt\r\e[0KInstalling appimage tools"
GIMP_DIR="$PWD/"
cd ${GIMP_DIR}${PARENT_DIR}
if [ "$GITLAB_CI" ]; then
  apt-get update >/dev/null 2>&1
  apt-get install -y --no-install-recommends ca-certificates wget curl binutils debuginfod >/dev/null 2>&1
fi
export HOST_ARCH=$(uname -m)
export APPIMAGE_EXTRACT_AND_RUN=1

if [ ! "$(find $GIMP_DIR -maxdepth 1 -iname "AppDir*")" ] || [ "$MODE" = '--bundle-only' ]; then
  ## For now, we always use the latest go-appimagetool for bundling. See: https://github.com/probonopd/go-appimage/issues/275
  if [ "$GITLAB_CI" ]; then
    apt-get install -y --no-install-recommends file patchelf >/dev/null 2>&1
  fi
  bundler="$PWD/go-appimagetool.AppImage"
  rm -f "$bundler" >/dev/null
  wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-${HOST_ARCH}.AppImage" | head -n 1 | cut -d '"' -f 2) >/dev/null 2>&1
  bundler_text="go-appimagetool build: $(echo appimagetool-*.AppImage | sed -e 's/appimagetool-//' -e "s/-${HOST_ARCH}.AppImage//")"
  mv appimagetool-*.AppImage $bundler
  chmod +x "$bundler"
fi

if [ "$MODE" != '--bundle-only' ]; then
  ## standard appimagetool is needed for squashing the .appimage file
  if [ "$GITLAB_CI" ]; then
    apt-get install -y --no-install-recommends file zsync appstream >/dev/null 2>&1
  fi
  standard_appimagetool="$PWD/standard-appimagetool.AppImage"
  wget "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-${HOST_ARCH}.AppImage" -O $standard_appimagetool >/dev/null 2>&1
  chmod +x "$standard_appimagetool"
  standard_appimagetool_version=$("$standard_appimagetool" --version 2>&1 | sed -e 's/.*version \(.*\)), build.*/\1/')

  ## static runtime to be squashed by appimagetool with the files bundled by the bundler
  static_runtime_version_online=$(curl -s 'https://api.github.com/repos/AppImage/type2-runtime/releases' |
                                  grep -Po '"target_commitish":.*?[^\\]",' | head -1 |
                                  sed -e 's|target_commitish||g' -e 's|"||g' -e 's|:||g' -e 's|,||g' -e 's| ||g')
  wget https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-aarch64 -O runtime-aarch64 >/dev/null 2>&1
  wget https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64 -O runtime-x86_64 >/dev/null 2>&1
  chmod +x "./runtime-$HOST_ARCH"
  static_runtime_version_downloaded=$("./runtime-$HOST_ARCH" --appimage-version 2>&1)
  chmod -x "./runtime-$HOST_ARCH"
  if [ "${static_runtime_version_downloaded#*commit/}" != "${static_runtime_version_online:0:7}" ]; then
    echo -e '\033[31m(ERROR)\033[0m: Downloaded runtime version differs from the one released online. Please, run again this script.'
    exit 1
  fi
  standard_appimagetool_text="appimagetool commit: $standard_appimagetool_version | type2-runtime commit: ${static_runtime_version_downloaded#*commit/}"
fi
if [ ! "$(find $GIMP_DIR -maxdepth 1 -iname "AppDir*")" ] && [ "$MODE" != '--bundle-only' ]; then
  separator=' | '
fi
cd $GIMP_DIR
echo "(INFO): ${bundler_text}${separator}${standard_appimagetool_text}"
echo -e "\e[0Ksection_end:`date +%s`:apmg_tlkt\r\e[0K"


# 2. GET GLOBAL VARIABLES
echo -e "\e[0Ksection_start:`date +%s`:apmg_info\r\e[0KGetting AppImage global info"
if [ "$BUILD_DIR" = '' ]; then
  export BUILD_DIR=$(find $PWD -maxdepth 1 -iname "_build*$RUNNER" | head -n 1)
fi
if [ ! -f "$BUILD_DIR/config.h" ]; then
  echo -e "\033[31m(ERROR)\033[0m: config.h file not found. You can configure GIMP with meson to generate it."
  exit 1
fi
eval $(sed -n 's/^#define  *\([^ ]*\)  *\(.*\) *$/export \1=\2/p' $BUILD_DIR/config.h)

## Set proper AppImage update channel and App ID
if [ -z "$GIMP_RELEASE" ] || [ "$GIMP_IS_RC_GIT" ]; then
  export CHANNEL="Continuous"
elif [ "$GIMP_RELEASE" ] && [ "$GIMP_UNSTABLE" ] || [ "$GIMP_RC_VERSION" ]; then
  export CHANNEL="Pre"
else
  export CHANNEL="Stable"
fi
export APP_ID="org.gimp.GIMP.$CHANNEL"

## Get info about GIMP version
export CUSTOM_GIMP_VERSION="$GIMP_VERSION"
if [[ ! "$REVISION" =~ [1-9] ]]; then
  export REVISION="0"
else
  export CUSTOM_GIMP_VERSION="${GIMP_VERSION}-${REVISION}"
fi
echo "(INFO): App ID: $APP_ID | Version: $CUSTOM_GIMP_VERSION"
echo -e "\e[0Ksection_end:`date +%s`:apmg_info\r\e[0K"


# 3. GIMP FILES (IN APPDIR)
if [ ! "$(find . -maxdepth 1 -iname "AppDir*")" ] || [ "$MODE" = '--bundle-only' ]; then
echo -e "\e[0Ksection_start:`date +%s`:apmg_files[collapsed=true]\r\e[0KPreparing GIMP files in AppDir-$HOST_ARCH/usr"
grep -q 'relocatable-bundle=yes' $BUILD_DIR/meson-logs/meson-log.txt && export RELOCATABLE_BUNDLE_ON=1
if [ -z "$RELOCATABLE_BUNDLE_ON" ]; then
  echo -e "\033[31m(ERROR)\033[0m: No relocatable GIMP build found. You can build GIMP with '-Drelocatable-bundle=yes' to make a build suitable for AppImage."
  exit 1
fi

#Prefixes to get files to copy
UNIX_PREFIX='/usr'
if [ -z "$GITLAB_CI" ] && [ -z "$GIMP_PREFIX" ]; then
  export GIMP_PREFIX="$PWD/../_install"
fi
if [ -z "$GITLAB_CI" ]; then
  IFS=$'\n' VAR_ARRAY=($(cat .gitlab-ci.yml | sed -n '/multi-os/,/multiarch/p' | sed 's/- //'))
  IFS=$' \t\n'
  for VAR in "${VAR_ARRAY[@]}"; do
    eval "$VAR"
  done
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
  #Prevent unwanted expansion
  mkdir -p limbo
  cd limbo

  #Paths where to search
  case $2 in
    bin*)
      search_path=("$1/bin" "$1/sbin" "$1/libexec")
      ;;
    lib*)
      search_path=("$(dirname $(echo $2 | sed "s|lib/|$1/${LIB_DIR}/${LIB_SUBDIR}|g" | sed "s|*|no_scape|g"))"
                   "$(dirname $(echo $2 | sed "s|lib/|/usr/${LIB_DIR}/|g" | sed "s|*|no_scape|g"))")
      ;;
    share*|include*|etc*)
      search_path=("$(dirname $(echo $2 | sed "s|${2%%/*}|$1/${2%%/*}|g" | sed "s|*|no_scape|g"))")
      ;;
  esac
  for path in "${search_path[@]}"; do
    expanded_path=$(echo $(echo $path | sed "s|no_scape|*|g"))
    if [ ! -d "$expanded_path" ]; then
      continue
    fi

    #Copy found targets from search_path to bundle dir
    target_array=($(find $expanded_path -maxdepth 1 -name ${2##*/}))
    for target_path in "${target_array[@]}"; do
      dest_path="$(dirname $(echo $target_path | sed "s|$1/|${USR_DIR}/|g"))"
      output_dest_path="$dest_path"
      if [ "$3" = '--dest' ] || [ "$3" = '--rename' ]; then
        if [ "$3" = '--dest' ]; then
          output_dest_path="${USR_DIR}/$4"
        elif [ "$3" = '--rename' ]; then
          output_dest_path="${USR_DIR}/${4%/*}"
        fi
        dest_path="$output_dest_path/tmp"
      fi
      
      if [ "$3" != '--bundler' ] && [ "$5" != '--bundler' ]; then
        echo "(INFO): bundling $target_path to $output_dest_path"
        mkdir -p $dest_path
        cp -ru $target_path $dest_path >/dev/null 2>&1 || continue

        #Additional parameters for special situations
        if [ "$3" = '--dest' ] || [ "$3" = '--rename' ]; then
          mv $dest_path/${2##*/} $USR_DIR/$4
          rm -r "$dest_path"
        fi
      else
        echo "(INFO): skipping $target_path (will be bundled by the tool)"
        if [[ "$target_path" =~ 'bin' ]] || [[ "$target_path" =~ '.so' ]]; then
          export APPENDED_LIST+="$target_path "
        fi
      fi
    done
  done

  #Undo the tweak done above
  cd ..
  rm -r limbo
}

conf_app ()
{
  #Prefix from which to expand the var
  prefix=$UNIX_PREFIX
  case $1 in
    *BABL*|*GEGL*|*GIMP*)
      prefix=$GIMP_PREFIX
  esac

  #Get expanded var
  if [ "$3" != '--no-expand' ]; then
    appdir_path='${APPDIR}/usr/'
    var_path="$(echo $prefix/$2 | sed "s|${prefix}/||g")"
  else
    unset appdir_path
    var_path="$2"
  fi
  
  #Set expanded var in AppRun (and in environ if needed by this script or by the bundler)
  if [ "$3" != '--bundler' ] && [ "$4" != '--bundler' ]; then
    apprun="build/linux/appimage/AppRun"
    if [ ! -f "$apprun.bak" ]; then
      cp $apprun "$apprun.bak"
    fi
    echo "export $1=\"${appdir_path}${var_path}\"" >> "$apprun"
  fi
  eval export $1="${prefix}/$var_path" || $true
}

wipe_usr ()
{
  find $USR_DIR -iname ${1##*/} -execdir rm -r -f "{}" \;
}

## Prepare AppDir
mkdir -p $APP_DIR
echo '*' > $APP_DIR/.gitignore
bund_usr "$UNIX_PREFIX" "lib*/ld-*.so.*" --bundler
if [ "$HOST_ARCH" = 'aarch64' ]; then
  conf_app LD_LINUX "lib/ld-*.so.*"
else
  conf_app LD_LINUX "lib64/ld-*.so.*"
fi

## Bundle base (bare minimum to run GTK apps)
### Glib needed files (to be able to use URIs and file dialogs). See: #12937 and #13082
bund_usr "$UNIX_PREFIX" "lib/glib-*/gio-launch-desktop" --dest "bin"
prep_pkg "xapps-common" 
bund_usr "$UNIX_PREFIX" "share/glib-*/schemas" 
### Glib commonly required modules
prep_pkg "gvfs"
bund_usr "$UNIX_PREFIX" "bin/gvfs*" --dest "${LIB_DIR}/gvfs"
bund_usr "$UNIX_PREFIX" "lib/gvfs/*.so"
bund_usr "$UNIX_PREFIX" "lib/gio/modules/*"
conf_app GIO_MODULE_DIR "${LIB_DIR}/${LIB_SUBDIR}gio/modules"
### GTK needed files (to be able to load icons)
bund_usr "$UNIX_PREFIX" "share/icons/Adwaita"
bund_usr "$GIMP_PREFIX" "share/icons/hicolor"
bund_usr "$UNIX_PREFIX" "share/mime"
bund_usr "$UNIX_PREFIX" "lib/gdk-pixbuf-*/*.*.*/loaders/*.so" --bundler
conf_app GDK_PIXBUF_MODULEDIR "${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-*/*.*.*/loaders"
conf_app GDK_PIXBUF_MODULE_FILE "${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-*/*.*.*/loaders.cache"
### GTK commonly required modules
prep_pkg "libcanberra-gtk3-module"
prep_pkg "libxapp-gtk3-module"
prep_pkg "packagekit-gtk3-module"
bund_usr "$UNIX_PREFIX" "lib/gtk-3.0/modules/*.so" --bundler
bund_usr "$UNIX_PREFIX" "lib/gtk-3.0/*.*.*/printbackends/*.so" --bundler
conf_app GTK_PATH "${LIB_DIR}/${LIB_SUBDIR}gtk-3.0"
prep_pkg "ibus-gtk3"
bund_usr "$UNIX_PREFIX" "lib/gtk-3.0/*.*.*/immodules/*.so" --bundler
conf_app GTK_IM_MODULE_FILE "${LIB_DIR}/${LIB_SUBDIR}gtk-3.0/*.*.*/immodules.cache"

## Core features
bund_usr "$GIMP_PREFIX" "lib/libbabl*"
bund_usr "$GIMP_PREFIX" "lib/babl-*/*.so"
conf_app BABL_PATH "${LIB_DIR}/${LIB_SUBDIR}babl-*"
bund_usr "$GIMP_PREFIX" "lib/libgegl*"
bund_usr "$GIMP_PREFIX" "lib/gegl-*/*.so"
conf_app GEGL_PATH "${LIB_DIR}/${LIB_SUBDIR}gegl-*"
bund_usr "$GIMP_PREFIX" "lib/libgimp*"
bund_usr "$GIMP_PREFIX" "lib/gimp"
bund_usr "$GIMP_PREFIX" "share/gimp"
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
bund_usr "$GIMP_PREFIX" "etc/gimp"

## Other features and plug-ins
### Needed for welcome page
bund_usr "$GIMP_PREFIX" "share/metainfo/*.xml"
### mypaint brushes
bund_usr "$UNIX_PREFIX" "share/mypaint-data/1.0"
### Needed for 'th' word breaking in Text tool etc
bund_usr "$UNIX_PREFIX" "share/libthai"
conf_app LIBTHAI_DICTDIR "share/libthai"
### Needed for full CJK and Cyrillic support in file-pdf
bund_usr "$UNIX_PREFIX" "share/poppler"
### file-wmf support
bund_usr "$UNIX_PREFIX" "share/fonts/type1/urw-base35/Nimbus*" --dest "share/libwmf/fonts"
bund_usr "$UNIX_PREFIX" "share/fonts/type1/urw-base35/StandardSymbols*" --dest "share/libwmf/fonts"
# Note: we want the same test as around the global variable
# show_debug_menu in app/main.c
if [ "$GIMP_UNSTABLE" ] || [ -z "$GIMP_RELEASE" ]; then
  ### Image graph support
  bund_usr "$UNIX_PREFIX" "bin/libgvc*" --rename "bin/dot"
  bund_usr "$UNIX_PREFIX" "lib/graphviz/config*"
  bund_usr "$UNIX_PREFIX" "lib/graphviz/libgvplugin_dot*"
  bund_usr "$UNIX_PREFIX" "lib/graphviz/libgvplugin_pango*"
  ### Needed for GTK inspector
  bund_usr "$UNIX_PREFIX" "lib/libEGL*"
  bund_usr "$UNIX_PREFIX" "lib/libGL*"
  bund_usr "$UNIX_PREFIX" "lib/dri*"
  #TODO: remove this on Debian Trixie (which have Mesa 24.2)
  conf_app LIBGL_DRIVERS_PATH "${LIB_DIR}/${LIB_SUBDIR}dri"
fi
### Debug dialog
bund_usr "$GIMP_PREFIX" "bin/gimp-debug-tool*" --dest "libexec"
### headers and .pc files for help building filters and plug-ins
bund_usr "$GIMP_PREFIX" "include/gimp-*"
bund_usr "$GIMP_PREFIX" "include/babl-*"
bund_usr "$GIMP_PREFIX" "include/gegl-*"
bund_usr "$GIMP_PREFIX" "lib/pkgconfig/gimp-*"
bund_usr "$GIMP_PREFIX" "lib/pkgconfig/babl-*"
bund_usr "$GIMP_PREFIX" "lib/pkgconfig/gegl-*"
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
conf_app PYTHONDONTWRITEBYTECODE "1" --no-expand
####FIXME: lua crashes with loop: See: #11895
#bund_usr "$UNIX_PREFIX" "bin/luajit" --rename "lua"
#bund_usr "$UNIX_PREFIX" "lib/liblua5.1-lgi*"
#bund_usr "$UNIX_PREFIX" "lib/lua/5.1"
#conf_app LUA_CPATH "\${APPDIR}/usr/${LIB_DIR}/${LIB_SUBDIR}lua/5.1/?.so" --no-expand
#bund_usr "$UNIX_PREFIX" "share/lua/5.1"
#conf_app LUA_PATH "\${APPDIR}/usr/share/lua/5.1/?.lua;\${APPDIR}/usr/share/lua/5.1/lgi/?.lua;\${APPDIR}/usr/share/lua/5.1/lgi/override/?.lua" --no-expand

## Other binaries and deps (bundle them and do fine-tuning with bundling tool)
bund_usr "$GIMP_PREFIX" 'bin/gimp*'
bund_usr "$GIMP_PREFIX" "bin/gegl"
bund_usr "$GIMP_PREFIX" "share/applications/*.desktop"
#go-appimagetool have too polluted output so we save as log. See: https://github.com/probonopd/go-appimage/issues/314
"$bundler" -s deploy $(echo "$USR_DIR/share/applications/*.desktop") &> appimagetool.log || cat appimagetool.log

## Manual adjustments after running the bundling tool
### Undo the mess which breaks babl and GEGL. See: https://github.com/probonopd/go-appimage/issues/315
cp -r $APP_DIR/lib/* $USR_DIR/${LIB_DIR}
rm -r $APP_DIR/lib
### Fix not fully bundled GTK canberra module. See: https://github.com/probonopd/go-appimage/issues/332
find "$USR_DIR/${LIB_DIR}/${LIB_SUBDIR}gtk-3.0/modules" -iname *canberra*.so -execdir ln -sf "{}" libcanberra-gtk-module.so \;
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
### We can't set LD_LIBRARY_PATH partly to not break patchelf trick so we need 'ln' for Lua
#cd $APP_DIR
#lua_cpath_tweaked="$(echo $LUA_CPATH | sed -e 's|$HERE/||' -e 's|/?.so||')/lgi"
#find "usr/${LIB_DIR}/${LIB_SUBDIR}" -maxdepth 1 -iname *.so* -exec ln -sf $(realpath "{}" --relative-to "$lua_cpath_tweaked") "$lua_cpath_tweaked" \;
#cd ..

## Debug symbols
#if [ "$GITLAB_CI" ]; then
#  export DEBUGINFOD_URLS="https://debuginfod.debian.net"
#fi
#bin_array=($(find "$USR_DIR/bin" "$USR_DIR/$LIB_DIR" "$(dirname $APP_DIR/$LD_LINUX)" ! -iname "*.dumb*" -type f -exec head -c 4 {} \; -exec echo " {}" \;  | grep ^.ELF))
#for bin in "${bin_array[@]}"; do
#  if [[ ! "$bin" =~ 'ELF' ]] && [[ ! "$bin" =~ '.debug' ]]; then
#    grep -a -q '.gnu_debuglink' $bin && echo "(INFO): bundling $bin debug symbols to $(dirname $bin)" && cp -f $(debuginfod-find debuginfo $bin) "$(dirname $bin)/$(readelf --string-dump=.gnu_debuglink $bin | sed -n '/]/{s/.* //;p;q}')" || $true
#  fi
#done

## Files unnecessarily created or bundled by the tool
mv build/linux/appimage/AppRun $APP_DIR
mv build/linux/appimage/AppRun.bak build/linux/appimage/AppRun
rm $APP_DIR/*.desktop
echo "usr/${LIB_DIR}/${LIB_SUBDIR}gconv
      usr/${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-*/gdk-pixbuf-query-loaders
      usr/${LIB_DIR}/${LIB_SUBDIR}gdk-pixbuf-*/*.debug
      usr/share/doc
      usr/share/themes
      etc
      .gitignore" > appimageignore-$HOST_ARCH

## Revision (this does the same as '-Drevision' build option)
before=$(cat "$(echo $USR_DIR/share/gimp/*/gimp-release)" | grep 'revision')
after="revision=$REVISION"
sed -i "s|$before|$after|" "$(echo $USR_DIR/share/gimp/*/gimp-release)"
echo -e "\e[0Ksection_end:`date +%s`:apmg_files\r\e[0K"
fi
if [ "$MODE" = '--bundle-only' ]; then
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
ln -sfr "$USR_DIR/bin/gimp-$GIMP_APP_VERSION" "$USR_DIR/bin/$APP_ID"
printf "\nexec \"\${LD_LINUX}\" --inhibit-cache \"\$APPDIR\"/usr/bin/$APP_ID \"\$@\"" >> "$APP_DIR/AppRun"
chmod +x $APP_DIR/AppRun

## 4.2. Copy icon assets (similarly to flatpaks's 'rename-icon')
echo "(INFO): copying $APP_ID.svg asset to AppDir"
find "$USR_DIR/share/icons/hicolor" \( -iname *.svg -and ! -iname $APP_ID*.svg \) -execdir ln -sf "{}" $APP_ID.svg \;
find "$USR_DIR/share/icons/hicolor" \( -iname *.png -and ! -iname $APP_ID*.png \) -execdir ln -sf "{}" $APP_ID.png \;
cp -L "$USR_DIR/share/icons/hicolor/scalable/apps/$APP_ID.svg" $APP_DIR
ln -sfr "$APP_DIR/$APP_ID.svg" $APP_DIR/.DirIcon

## 4.3. Configure .desktop asset (similarly to flatpaks's 'rename-desktop-file')
echo "(INFO): configuring $APP_ID.desktop"
find "$USR_DIR/share/applications" \( -iname *.desktop -and ! -iname $APP_ID*.desktop \) -execdir mv "{}" $APP_ID.desktop \;
sed -i "s/gimp-$GIMP_APP_VERSION/$APP_ID/g" "$USR_DIR/share/applications/${APP_ID}.desktop"
sed -i "s/Icon=gimp/Icon=$APP_ID/g" "$USR_DIR/share/applications/${APP_ID}.desktop"
ln -sfr "$USR_DIR/share/applications/${APP_ID}.desktop" $APP_DIR

## 4.4. Configure appdata asset (similarly to flatpaks's 'rename-appdata-file')
echo "(INFO): configuring $APP_ID.appdata.xml"
find "$USR_DIR/share/metainfo" \( -iname *.appdata.xml -and ! -iname $APP_ID*.appdata.xml \) -execdir mv "{}" $APP_ID.appdata.xml \;
sed -i "s/org.gimp.GIMP</${APP_ID}</g" "$USR_DIR/share/metainfo/${APP_ID}.appdata.xml"
sed -i "s/gimp.desktop/${APP_ID}.desktop/g" "$USR_DIR/share/metainfo/${APP_ID}.appdata.xml"
sed -i "s/date=\"TODO\"/date=\"`date --iso-8601`\"/" "$USR_DIR/share/metainfo/${APP_ID}.appdata.xml"
echo -e "\e[0Ksection_end:`date +%s`:${ARCH}_source\r\e[0K"


# 5. CONSTRUCT .APPIMAGE
APPIMAGETOOL_APP_NAME="GIMP-${CUSTOM_GIMP_VERSION}-${ARCH}.AppImage"
echo -e "\e[0Ksection_start:`date +%s`:${ARCH}_making[collapsed=true]\r\e[0KSquashing $APPIMAGETOOL_APP_NAME"
if [ "$GIMP_RELEASE" ] && [ -z "$GIMP_IS_RC_GIT" ]; then
  update_info="--updateinformation zsync|https://download.gimp.org/gimp/GIMP-${CHANNEL}-${ARCH}.AppImage.zsync"
fi
"$standard_appimagetool" $APP_DIR $APPIMAGETOOL_APP_NAME --exclude-file appimageignore-$ARCH \
                                                         --runtime-file ${PARENT_DIR}runtime-$ARCH $update_info
file "./$APPIMAGETOOL_APP_NAME"
#updateinformation is not compatible with our server. See: https://github.com/AppImage/appimagetool/issues/91
if [ -f "${APPIMAGETOOL_APP_NAME}.zsync" ]; then
  before=$(cat "$APPIMAGETOOL_APP_NAME.zsync" | grep -a "URL: ")
  after=$(cat "$APPIMAGETOOL_APP_NAME.zsync" | grep -a "URL: " | sed "s|$APPIMAGETOOL_APP_NAME|v$GIMP_APP_VERSION/linux/$APPIMAGETOOL_APP_NAME|")
  sed -i "s|$before|$after|" "$APPIMAGETOOL_APP_NAME.zsync" >/dev/null 2>&1
  mv ${APPIMAGETOOL_APP_NAME}.zsync GIMP-${CHANNEL}-${ARCH}.AppImage.zsync
fi
echo -e "\e[0Ksection_end:`date +%s`:${ARCH}_making\r\e[0K"


# 6. GENERATE SHASUMS
echo -e "\e[0Ksection_start:`date +%s`:${ARCH}_trust[collapsed=true]\r\e[0KChecksumming $APPIMAGETOOL_APP_NAME"
if [ "$GIMP_RELEASE" ] && [ -z "$GIMP_IS_RC_GIT" ]; then
  sha256sum $APPIMAGETOOL_APP_NAME > $APPIMAGETOOL_APP_NAME.SHA256SUMS
fi
echo "(INFO): $APPIMAGETOOL_APP_NAME SHA-256: $(sha256sum $APPIMAGETOOL_APP_NAME | cut -d ' ' -f 1)"
if [ "$GIMP_RELEASE" ] && [ -z "$GIMP_IS_RC_GIT" ]; then
  sha512sum $APPIMAGETOOL_APP_NAME > $APPIMAGETOOL_APP_NAME.SHA512SUMS
fi
echo "(INFO): $APPIMAGETOOL_APP_NAME SHA-512: $(sha512sum $APPIMAGETOOL_APP_NAME | cut -d ' ' -f 1)"
echo -e "\e[0Ksection_end:`date +%s`:${ARCH}_trust\r\e[0K"


if [ "$GITLAB_CI" ]; then
  output_dir='build/linux/appimage/_Output'
  mkdir -p $output_dir
  mv GIMP*.AppImage* $output_dir
fi
done
