#!/bin/bash

set -e

if [ -z "$MESON_BUILD_ROOT" ]; then
  # Let's prevent contributors from creating broken bundles
  echo -e "\033[31m(ERROR)\033[0m: Script called standalone. Please, build GIMP targeting installer or msix creation and this script will be called automatically."
  exit 1
fi


if [[ "$CI_JOB_NAME" =~ "cross" ]]; then
  apt-get update
  apt-get install -y --no-install-recommends   \
                     binutils                  \
                     binutils-mingw-w64-x86-64 \
                     file                      \
                     libglib2.0-bin            \
                     python3
fi


# Bundle deps and GIMP files
export GIMP_SOURCE=$(echo $MESON_SOURCE_ROOT | sed 's|\\|/|g')

grep -q '#define GIMP_UNSTABLE' config.h && export GIMP_UNSTABLE=1
gimp_app_version=$(grep GIMP_APP_VERSION config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
if [ "$GIMP_UNSTABLE" ]; then
  gimp_mutex_version="$gimp_app_version"
else
  gimp_mutex_version=$(echo $gimp_app_version | sed 's/^[^0-9]*\([0-9]*\).*$/\1/')
fi

## GIMP prefix: as set at meson configure time
export GIMP_PREFIX=$(echo $MESON_INSTALL_DESTDIR_PREFIX | sed 's|\\|/|g')
## System prefix: it is MSYSTEM_PREFIX
export MSYS_PREFIX=$(grep 'Main binary:' meson-logs/meson-log.txt | sed 's|Main binary: ||' | sed 's|\\bin\\python.exe||' | sed 's|\\|/|g')
if [ "$QUASI_MSYS2_ROOT" ]; then
  export MSYS_PREFIX="$MSYSTEM_PREFIX"
fi
## Bundle dir: we make a "perfect" bundle separated from GIMP_PREFIX
#NOTE: The bundling script need to set $MSYSTEM_PREFIX to our dist scripts
#fallback code be able to identify what arch they are distributing
export GIMP_DISTRIB="$GIMP_SOURCE/gimp-$(echo $MSYSTEM_PREFIX | sed 's|/||')"

bundle ()
{
  ## Tweak paths to avoid expanding
  mkdir -p limbo
  cd limbo
  limited_search_path=$(dirname $(echo $2 | sed "s|${2%%/*}/|$1/${2%%/*}/|g" | sed "s|*|no_scape|g"))
  ## Search for targets in search path
  search_path=$(echo $(echo $limited_search_path | sed "s|no_scape|*|g"))
  bundledArray=($(find "$search_path" -maxdepth 1 -name ${2##*/}))
  ## Copy found targets to bundle path
  for target_path in "${bundledArray[@]}"; do
    bundled_path=$(echo $target_path | sed "s|$1/|$GIMP_DISTRIB/|g")
    parent_path=$(dirname $bundled_path)
    echo "Bundling $target_path to $parent_path"
    mkdir -p "$parent_path"
    cp -fru "$target_path" $parent_path >/dev/null 2>&1 || continue
  done
  ## Undo the tweak done above
  cd ..
  rm -r limbo
}

clean ()
{
  if [[ "$2" =~ '/' ]]; then
    cleanedArray=($(find $1/${2%/*} -iname ${2##*/}))
  else
    cleanedArray=($(find $1/ -iname ${2##*/}))
  fi
  for path_dest_full in "${cleanedArray[@]}"; do
    if [[ "$path_dest_full" = "${cleanedArray[0]}" ]]; then
      echo "Cleaning $1/$2"
    fi
    rm $path_dest_full
  done
}


## Prevent Git going crazy
mkdir -p $GIMP_DISTRIB
echo "*" > $GIMP_DISTRIB/.gitignore

## Add a wrapper at tree root, less messy than having to look for the
## binary inside bin/, in the middle of all the DLLs.
echo "bin\gimp-$gimp_mutex_version.exe" > $GIMP_DISTRIB/gimp.cmd


## Settings.
bundle "$GIMP_PREFIX" etc/gimp
### Needed for fontconfig
bundle "$MSYS_PREFIX" etc/fonts


## Headers (for use of gimptool).
bundle "$GIMP_PREFIX" include/gimp-*
bundle "$GIMP_PREFIX" include/babl-*
bundle "$GIMP_PREFIX" include/gegl-*
bundle "$MSYS_PREFIX" include/exiv*
bundle "$MSYS_PREFIX" include/gexiv*


## Library data.
bundle "$GIMP_PREFIX" lib/gimp
bundle "$GIMP_PREFIX" lib/babl-*
bundle "$GIMP_PREFIX" lib/gegl-*
### Needed to open remote files
bundle "$MSYS_PREFIX" lib/gio
### Needed to loading icons in GUI
bundle "$MSYS_PREFIX" lib/gdk-pixbuf-*/*/loaders.cache
bundle "$MSYS_PREFIX" lib/gdk-pixbuf-*/*/loaders/libpixbufloader-png.dll
bundle "$MSYS_PREFIX" lib/gdk-pixbuf-*/*/loaders/pixbufloader_svg.dll
### Support for legacy Win clipboard images: https://gitlab.gnome.org/GNOME/gimp/-/issues/4802
bundle "$MSYS_PREFIX" lib/gdk-pixbuf-*/*/loaders/libpixbufloader-bmp.dll
### Support for non .PAT patterns: https://gitlab.gnome.org/GNOME/gimp/-/issues/12351
bundle "$MSYS_PREFIX" lib/gdk-pixbuf-*/*/loaders/libpixbufloader-jpeg.dll
bundle "$MSYS_PREFIX" lib/gdk-pixbuf-*/*/loaders/libpixbufloader-gif.dll
bundle "$MSYS_PREFIX" lib/gdk-pixbuf-*/*/loaders/libpixbufloader-tiff.dll
clean "$GIMP_DISTRIB" lib/*.a


## Resources.
bundle "$GIMP_PREFIX" share/gimp
### Needed for file dialogs
bundle "$MSYS_PREFIX" share/glib-*/schemas/gschemas.compiled
### Needed to not crash UI. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/6165
bundle "$MSYS_PREFIX" share/icons/Adwaita
### Needed by GTK to use icon themes. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/5080
bundle "$GIMP_PREFIX" share/icons/hicolor
### Needed for 'th' word breaking in Text tool etc
bundle "$MSYS_PREFIX" share/libthai
### Needed for file-wmf work
bundle "$MSYS_PREFIX" share/libwmf
### Only copy from langs supported in GIMP.
lang_array=($(echo $(ls $GIMP_SOURCE/po/*.po |
              sed -e "s|$GIMP_SOURCE/po/||g" -e 's|.po||g' | sort) |
              tr '\n\r' ' '))
for lang in "${lang_array[@]}"; do
  bundle "$GIMP_PREFIX" share/locale/$lang/LC_MESSAGES/*.mo
  if [ -d "$MSYS_PREFIX/share/locale/$lang/LC_MESSAGES/" ]; then
    # Needed for eventually used widgets, GTK inspector etc
    bundle "$MSYS_PREFIX" share/locale/$lang/LC_MESSAGES/gtk*.mo
    # For language list in text tool options
    bundle "$MSYS_PREFIX" share/locale/$lang/LC_MESSAGES/iso_639_3.mo
  fi
done
### Needed for welcome page
bundle "$GIMP_PREFIX" share/metainfo/org.gimp*.xml
### mypaint brushes
bundle "$MSYS_PREFIX" share/mypaint-data
### Needed for full CJK and Cyrillic support in file-pdf
bundle "$MSYS_PREFIX" share/poppler


## Executables and DLLs.

### We save the list of already copied DLLs to keep a state between 2_bundle-gimp-uni_dep runs.
rm -f done-dll.list

### Minimal (and some additional) executables for the 'bin' folder
bundle "$GIMP_PREFIX" bin/gimp*.exe
bundle "$GIMP_PREFIX" bin/libgimp*.dll
### Bundled just to promote GEGL. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/10580
bundle "$GIMP_PREFIX" bin/gegl.exe
if [ "$GIMP_UNSTABLE" ] && [[ ! "$MSYS_PREFIX" =~ "32" ]]; then
  ### Needed for 'Show image graph'.
  #### See: https://gitlab.gnome.org/GNOME/gimp/-/issues/6045
  bundle "$MSYS_PREFIX" bin/dot.exe
  #### See: https://gitlab.gnome.org/GNOME/gimp/-/issues/12119
  bundle "$MSYS_PREFIX" bin/libgvplugin_dot*.dll
  bundle "$MSYS_PREFIX" bin/libgvplugin_pango*.dll
  bundle "$MSYS_PREFIX" bin/config6
fi
### Needed to not pollute output. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/8877
bundle "$MSYS_PREFIX" bin/gdbus.exe
### Needed for hyperlink support etc... See: https://gitlab.gnome.org/GNOME/gimp/-/issues/12288
#...when running from `gimp*.exe --verbose`
bundle "$MSYS_PREFIX" bin/gspawn*-console.exe
if [ -z "$GIMP_UNSTABLE" ]; then
  #...when running from `gimp*.exe`
  bundle "$MSYS_PREFIX" bin/gspawn*-helper.exe
fi

### Optional binaries for GObject Introspection support
if [ -z "$QUASI_MSYS2_ROOT" ]; then
  bundle "$GIMP_PREFIX" lib/girepository-*
  bundle "$MSYS_PREFIX" lib/girepository-*

  #FIXME: luajit crashes at startup: See: https://gitlab.gnome.org/GNOME/gimp/-/issues/11597
  #bundle "$MSYS_PREFIX" bin/luajit.exe
  #bundle "$MSYS_PREFIX" lib/lua
  #bundle "$MSYS_PREFIX" share/lua

  #python.exe is needed for plug-ins output in `gimp-console*.exe`
  bundle "$MSYS_PREFIX" bin/python.exe
  if [ -z "$GIMP_UNSTABLE" ]; then
    #pythonw.exe is needed to run plug-ins silently in `gimp*.exe`
    bundle "$MSYS_PREFIX" bin/pythonw.exe
  fi
  bundle "$MSYS_PREFIX" lib/python*
  clean "$GIMP_DISTRIB" lib/python*/*.pyc
else
  # Just to ensure there is no introspected files that will output annoying warnings
  # This is needed because meson.build files can have flaws
  clean "$GIMP_DISTRIB" *.lua
  clean "$GIMP_DISTRIB" *.py
  clean "$GIMP_DISTRIB" *.scm
  clean "$GIMP_DISTRIB" *.vala
fi

### Deps (DLLs) of the binaries in 'bin' and 'lib' dirs
echo "Searching for dependencies of $GIMP_DISTRIB/bin in $MSYS_PREFIX and $GIMP_PREFIX"
binArray=($(find "$GIMP_DISTRIB/bin" \( -iname '*.dll' -or -iname '*.exe' \)))
for dep in "${binArray[@]}"; do
  python3 $GIMP_SOURCE/build/windows/2_bundle-gimp-uni_dep.py $dep $MSYS_PREFIX/ $GIMP_PREFIX/ $GIMP_DISTRIB --output-dll-list done-dll.list;
done
echo "Searching for dependencies of $GIMP_DISTRIB/lib in $MSYS_PREFIX and $GIMP_PREFIX"
libArray=($(find "$GIMP_DISTRIB/lib" \( -iname '*.dll' -or -iname '*.exe' \)))
for dep in "${libArray[@]}"; do
  python3 $GIMP_SOURCE/build/windows/2_bundle-gimp-uni_dep.py $dep $MSYS_PREFIX/ $GIMP_PREFIX/ $GIMP_DISTRIB --output-dll-list done-dll.list;
done

### .pdb (CodeView) debug symbols
### crossroad don't have LLVM/Clang backend yet
#if [ "$CI_JOB_NAME" != "gimp-win-x64-cross" ]; then
#  cp -fr ${GIMP_PREFIX}/bin/*.pdb ${GIMP_DISTRIB}/bin/
#fi
