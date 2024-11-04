#!/bin/bash

set -e

if [ -z "$MESON_BUILD_ROOT" ]; then
  # Let's prevent contributors from creating broken bundles
  echo -e "\033[31m(ERROR)\033[0m: Script called standalone. Please, just build GIMP and this script will be called automatically."
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
grep -q 'windows-installer=true' meson-logs/meson-log.txt && export INSTALLER_OPTION_ON=1
grep -q 'ms-store=true' meson-logs/meson-log.txt && export STORE_OPTION_ON=1
if [ "$GITLAB_CI" ] || [ "$INSTALLER_OPTION_ON" ] || [ "$STORE_OPTION_ON" ]; then
  export PERFECT_BUNDLE=1
fi

## GIMP prefix: as set at meson configure time
export GIMP_PREFIX=$(echo $MESON_INSTALL_DESTDIR_PREFIX | sed 's|\\|/|g')
## System prefix: on Windows shell, it is manually set; on POSIX shell, it is set by crossroad
export MSYS_PREFIX=$(grep 'Main binary:' $MESON_BUILD_ROOT/meson-logs/meson-log.txt | sed 's|Main binary: ||' | sed 's|\\bin\\python.exe||' | sed 's|\\|/|g')
if [ "$CROSSROAD_PLATFORM" ]; then
  export MSYS_PREFIX="$GIMP_PREFIX"
fi
## Bundle dir: normally, we make a quick bundling; on CI, we make a "perfect" bundling
export GIMP_DISTRIB="$GIMP_PREFIX"
if [ "$PERFECT_BUNDLE" ]; then
  #NOTE: The bundling script need to set ARTIFACTS_SUFFIX to our dist scripts
  #fallback code be able to identify what arch they are distributing
  #https://github.com/msys2/MSYS2-packages/issues/4960
  if [[ "$MSYS_PREFIX" =~ "clangarm64" ]]; then
    export ARTIFACTS_SUFFIX="a64"
  elif [[ "$MSYS_PREFIX" =~ "clang64" ]] || [ "$CROSSROAD_PLATFORM" = "w64" ]; then
    export ARTIFACTS_SUFFIX="x64"
  else # [ "$MSYS_PREFIX" =~ "mingw32" ];
    export ARTIFACTS_SUFFIX="x86"
  fi
  export GIMP_DISTRIB="$GIMP_SOURCE/gimp-${ARTIFACTS_SUFFIX}"
fi

bundle ()
{
  cd $GIMP_DISTRIB
  limited_search_path=$(dirname $(echo $2 | sed "s|${2%%/*}/|$1/${2%%/*}/|g" | sed "s|*|no_scape|g"))
  search_path=$(echo $(echo $limited_search_path | sed "s|no_scape|*|g"))
  bundledArray=($(find "$search_path" -maxdepth 1 -name ${2##*/}))
  for target_path in "${bundledArray[@]}"; do
    bundled_path=$(echo $target_path | sed "s|$1/|$GIMP_DISTRIB/|g")
    parent_path=$(dirname $bundled_path)
    if [ "$1" != "$GIMP_PREFIX" ] || [ "$PERFECT_BUNDLE" ]; then
      echo "Bundling $target_path to $parent_path"
    fi
    mkdir -p "$parent_path"
    cp -fru "$target_path" $parent_path >/dev/null 2>&1 || continue
  done
  cd ..
}

clean ()
{
  if [ "$PERFECT_BUNDLE" ]; then
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
  fi
}


## Prevent Git going crazy
mkdir -p $GIMP_DISTRIB
if [ "$PERFECT_BUNDLE" ]; then
  echo "*" > $GIMP_DISTRIB/.gitignore
fi

## Add a wrapper at tree root, less messy than having to look for the
## binary inside bin/, in the middle of all the DLLs.
gimp_app_version=$(grep GIMP_APP_VERSION $MESON_BUILD_ROOT/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
echo "bin\gimp-$gimp_app_version.exe" > $GIMP_DISTRIB/gimp.cmd


## Settings.
bundle "$GIMP_PREFIX" etc/gimp
### Needed for fontconfig
bundle "$MSYS_PREFIX" etc/fonts


## Headers.
bundle "$GIMP_PREFIX" include/gimp-*
bundle "$GIMP_PREFIX" include/babl-*
bundle "$GIMP_PREFIX" include/gegl-*


## Library data.
bundle "$GIMP_PREFIX" lib/gimp
bundle "$GIMP_PREFIX" lib/babl-*
bundle "$GIMP_PREFIX" lib/gegl-*
bundle "$MSYS_PREFIX" lib/gio
bundle "$MSYS_PREFIX" lib/gdk-pixbuf-*/*/loaders.cache
bundle "$MSYS_PREFIX" lib/gdk-pixbuf-*/*/loaders/libpixbufloader-png.dll
bundle "$MSYS_PREFIX" lib/gdk-pixbuf-*/*/loaders/pixbufloader_svg.dll
clean "$GIMP_DISTRIB" lib/*.a


## Resources.
bundle "$GIMP_PREFIX" share/gimp
### Needed for file dialogs
bundle "$MSYS_PREFIX" share/glib-*/schemas/gschemas.compiled
### https://gitlab.gnome.org/GNOME/gimp/-/issues/6165
bundle "$MSYS_PREFIX" share/icons/Adwaita
### https://gitlab.gnome.org/GNOME/gimp/-/issues/5080
bundle "$GIMP_PREFIX" share/icons/hicolor
### Needed for file-wmf work
bundle "$MSYS_PREFIX" share/libwmf
### Only copy from langs supported in GIMP.
lang_array=($(echo $(ls $GIMP_SOURCE/po/*.po |
              sed -e "s|$GIMP_SOURCE/po/||g" -e 's|.po||g' | sort) |
              tr '\n\r' ' '))
for lang in "${lang_array[@]}"; do
  bundle "$GIMP_PREFIX" share/locale/$lang/LC_MESSAGES/*.mo
  if [ -d "$MSYS_PREFIX/share/locale/$lang/LC_MESSAGES/" ]; then
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
### https://gitlab.gnome.org/GNOME/gimp/-/issues/10580
bundle "$GIMP_PREFIX" bin/gegl*.exe
### https://gitlab.gnome.org/GNOME/gimp/-/issues/6045
bundle "$MSYS_PREFIX" bin/dot.exe
### https://gitlab.gnome.org/GNOME/gimp/-/issues/8877
bundle "$MSYS_PREFIX" bin/gdbus.exe

### Optional binaries for GObject Introspection support
if [ "$CI_JOB_NAME" != 'gimp-win-x64-cross' ]; then
  bundle "$GIMP_PREFIX" lib/girepository-*
  bundle "$MSYS_PREFIX" lib/girepository-*

  # https://gitlab.gnome.org/GNOME/gimp/-/issues/11597
  #bundle "$MSYS_PREFIX" bin/luajit.exe
  #bundle "$MSYS_PREFIX" lib/lua
  #bundle "$MSYS_PREFIX" share/lua

  bundle "$MSYS_PREFIX" bin/python*.exe
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
