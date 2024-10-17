#!/bin/bash

set -e

if [ "$1" = '--force' ] && [ "$MSYSTEM_CARCH" = "i686" ]; then
  echo -e "\033[33m(WARNING)\033[0m: 32-bit builds will be dropped in a future release. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/10922"
fi


# NOTE: The bundling scripts, different from building scripts, need to set
# the ARTIFACTS_SUFFIX, even locally: 1) to avoid confusion (bundle dirs are
# relocatable so can be copied to a machine with other arch); and 2) to our
# dist scripts fallback code be able to identify what they are distributing
UNIX_ARCH=$(uname -m)
if [ "$UNIX_ARCH" = "aarch64" ]; then
  export ARTIFACTS_SUFFIX="-a64"
elif [ "$CI_JOB_NAME" = "gimp-win-x64-cross" ] || [ "$UNIX_ARCH" = "x86_64" ]; then
  export ARTIFACTS_SUFFIX="-x64"
else # [ "$UNIX_ARCH" = "i686" ];
  export ARTIFACTS_SUFFIX="-x86"
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
if [[ "$CI_JOB_NAME" =~ "cross" ]]; then
  export GIMP_PREFIX="`realpath ./_install-cross`"
  export MSYS_PREFIX="$GIMP_PREFIX"
fi
export GIMP_PREFIX="$PWD/../_install"
export GIMP_DISTRIB="`realpath ./gimp`${ARTIFACTS_SUFFIX}"

if [ -z "$MSYSTEM" ]; then
  export MSYS_PREFIX=$(grep '    directx-sdk-dir' $MESON_BUILD_ROOT/meson-logs/meson-log.txt | sed 's/    directx-sdk-dir   : //')
  export GIMP_PREFIX="$MESON_INSTALL_DESTDIR_PREFIX"
  if [ -z "$GITLAB_CI" ]; then
    export GIMP_DISTRIB="$GIMP_PREFIX"
  fi
  export GIMP_SOURCE=$(echo $MESON_SOURCE_ROOT/ | sed 's|\\|/|g')
fi

bundle ()
{
  search_path=$(dirname $(echo $1/$2))
  if [ ! -d "$search_path" ]; then
    search_path=$(echo $1/${2%%/*})
  fi
  bundledArray=($(find "$search_path" -maxdepth 1 -name ${2##*/}))
  for target_path in "${bundledArray[@]}"; do
    bundle_path=$(echo $target_path | sed "s|$1/|$GIMP_DISTRIB/|g")
    parent_path=$(dirname $bundle_path)
    if [ ! "$MSYSTEM" ] && [ "$1" != "$GIMP_PREFIX" ]; then
      echo "Bundling $target_path to $parent_path"
    fi
    mkdir -p "$parent_path"
    cp -fru "$target_path" $parent_path >/dev/null 2>&1 || continue
  done
}

clean ()
{
  if [ "$INSTALLER_OPTION" ] || [ "$STORE_OPTION" ]; then
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
echo "*" > $GIMP_DISTRIB/.gitignore

## Settings.
bundle "$GIMP_PREFIX" etc/gimp
### Needed for fontconfig
bundle "$MSYS_PREFIX" etc/fonts


## Headers (in evaluation): https://gitlab.gnome.org/GNOME/gimp/-/issues/6378.
#bundle $GIMP_PREFIX/include/gimp-*
#bundle $GIMP_PREFIX/include/babl-*
#bundle $GIMP_PREFIX/include/gegl-*


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
lang_array=($(echo $(ls ${GIMP_SOURCE}po/*.po |
              sed -e 's|po/||g' -e 's|.po||g' | sort) |
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
  #bundle "$MSYS_PREFIX" lib/lua/
  #bundle "$MSYS_PREFIX" share/lua/

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

### Deps (DLLs) of the binaries in 'lib' and 'bin' dirs
echo "Searching for dependencies of $GIMP_DISTRIB/lib in $GIMP_PREFIX and $MSYS_PREFIX"
libArray=($(find "$GIMP_DISTRIB/lib" \( -iname '*.dll' -or -iname '*.exe' \)))
for dep in "${libArray[@]}"; do
  python3 ${GIMP_SOURCE}build/windows/2_bundle-gimp-uni_dep.py $dep $GIMP_PREFIX/ $MSYS_PREFIX/ $GIMP_DISTRIB --output-dll-list done-dll.list;
done
echo "Searching for dependencies of $GIMP_DISTRIB/bin in $GIMP_PREFIX and $MSYS_PREFIX"
binArray=($(find "$GIMP_DISTRIB/bin" \( -iname '*.dll' -or -iname '*.exe' \)))
for dep in "${binArray[@]}"; do
  python3 ${GIMP_SOURCE}build/windows/2_bundle-gimp-uni_dep.py $dep $GIMP_PREFIX/ $MSYS_PREFIX/ $GIMP_DISTRIB --output-dll-list done-dll.list;
done

### .pdb (CodeView) debug symbols
### crossroad don't have LLVM/Clang backend yet
#if [ "$CI_JOB_NAME" != "gimp-win-x64-cross" ]; then
#  cp -fr ${GIMP_PREFIX}/bin/*.pdb ${GIMP_DISTRIB}/bin/
#fi
