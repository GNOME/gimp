#!/bin/bash

set -e

# $MSYSTEM_CARCH and $MSYSTEM_PREFIX are defined by MSYS2.
# https://github.com/msys2/MSYS2-packages/blob/master/filesystem/msystem
if [ "$MSYSTEM_CARCH" = "aarch64" ]; then
  export ARTIFACTS_SUFFIX="-a64"
elif [ "$CI_JOB_NAME" = "gimp-win-x64-cross" ] || [ "$MSYSTEM_CARCH" = "x86_64" ]; then
  export ARTIFACTS_SUFFIX="-x64"
else # [ "$MSYSTEM_CARCH" = "i686" ];
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
  export GIMP_PREFIX="`realpath ./_install`${ARTIFACTS_SUFFIX}-cross"
  export MSYS_PREFIX="$GIMP_PREFIX"
else
  export GIMP_PREFIX="`realpath ./_install`${ARTIFACTS_SUFFIX}"
  export MSYS_PREFIX="$MSYSTEM_PREFIX"
fi
export GIMP_DISTRIB="`realpath ./gimp`${ARTIFACTS_SUFFIX}"

bundle ()
{
  check_recurse=${2##*/}
  if [[ "$check_recurse" =~ '*' ]] && [[ ! "$check_recurse" =~ '/' ]]; then
    path_dest_parent=$(echo $1/${2%/*} | sed "s|${1}/||")
    mkdir -p "$GIMP_DISTRIB/$path_dest_parent"
    bundledArray=($(find $1/${2%/*} -maxdepth 1 -name ${2##*/}))
    for path_origin_full2 in "${bundledArray[@]}"; do
      echo "(INFO): copying $path_origin_full2 to $GIMP_DISTRIB/$path_dest_parent"
      cp $path_origin_full2 "$GIMP_DISTRIB/$path_dest_parent"
    done
  else
    path_origin_full=$(echo $1/$2)
    if [[ "$2" =~ '/' ]]; then
      path_dest_parent=$(dirname $path_origin_full | sed "s|${1}/||")
    fi
    mkdir -p "$GIMP_DISTRIB/$path_dest_parent"
    echo "(INFO): copying $path_origin_full to $GIMP_DISTRIB/$path_dest_parent"
    cp -r "$path_origin_full" "$GIMP_DISTRIB/$path_dest_parent"
  fi
}

clean ()
{
  if [[ "$2" =~ '/' ]]; then
    cleanedArray=($(find $1/${2%/*} -iname ${2##*/}))
  else
    cleanedArray=($(find $1/ -iname ${2##*/}))
  fi
  for path_dest_full in "${cleanedArray[@]}"; do
    rm $path_dest_full
    echo "(INFO): cleaning $path_dest_full"
  done
}


## Copy a previously built wrapper at tree root, less messy than
## having to look inside bin/, in the middle of all the DLLs.
## This also configure the interpreters for local builds as courtesy.
bundle "$GIMP_PREFIX" gimp.cmd


## Settings.
bundle "$GIMP_PREFIX" etc/gimp/
### Needed for fontconfig
bundle "$MSYS_PREFIX" etc/fonts/


## Headers (in evaluation): https://gitlab.gnome.org/GNOME/gimp/-/issues/6378.
#bundle $GIMP_PREFIX/include/gimp-*/
#bundle $GIMP_PREFIX/include/babl-*/
#bundle $GIMP_PREFIX/include/gegl-*/


## Library data.
bundle "$GIMP_PREFIX" lib/gimp/
bundle "$GIMP_PREFIX" lib/babl-*/
bundle "$GIMP_PREFIX" lib/gegl-*/
bundle "$MSYS_PREFIX" lib/gio/
bundle "$MSYS_PREFIX" lib/gdk-pixbuf-*/*/loaders.cache
bundle "$MSYS_PREFIX" lib/gdk-pixbuf-*/*/loaders/libpixbufloader-png.dll
bundle "$MSYS_PREFIX" lib/gdk-pixbuf-*/*/loaders/libpixbufloader-svg.dll
clean "$GIMP_DISTRIB" lib/*.a


## Resources.
bundle "$GIMP_PREFIX" share/gimp/
### Needed for file dialogs
bundle "$MSYS_PREFIX" share/glib-*/schemas/
### https://gitlab.gnome.org/GNOME/gimp/-/issues/6165
bundle "$MSYS_PREFIX" share/icons/Adwaita/
### https://gitlab.gnome.org/GNOME/gimp/-/issues/5080
bundle "$GIMP_PREFIX" share/icons/hicolor/
### Needed for wmf
bundle "$MSYS_PREFIX" share/libwmf/
### Only copy from langs supported in GIMP.
bundle "$GIMP_PREFIX" share/locale/
for dir in $GIMP_DISTRIB/share/locale/*/; do
  lang=`basename "$dir"`;
  if [ -d "$MSYS_PREFIX/share/locale/$lang/LC_MESSAGES/" ]; then
    bundle "$MSYS_PREFIX" share/locale/$lang/LC_MESSAGES/gtk*.mo
    bundle "$MSYS_PREFIX" share/locale/$lang/LC_MESSAGES/iso_639*.mo
  fi
done
### Needed for welcome page
bundle "$GIMP_PREFIX" share/metainfo/org.gimp*.xml
### mypaint brushes
bundle "$MSYS_PREFIX" share/mypaint-data/
### Needed for lang selection in Preferences
bundle "$MSYS_PREFIX" share/xml/iso-codes/iso_639.xml


## Executables and DLLs.

### We save the list of already copied DLLs to keep a state between 3_bundle-gimp-uni_dep runs.
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
  bundle "$GIMP_PREFIX" lib/girepository-*/
  bundle "$MSYS_PREFIX" lib/girepository-*/

  bundle "$MSYS_PREFIX" bin/luajit.exe
  bundle "$MSYS_PREFIX" lib/lua/
  bundle "$MSYS_PREFIX" share/lua/

  bundle "$MSYS_PREFIX" bin/python*.exe
  bundle "$MSYS_PREFIX" lib/python*/
else
  # Just to ensure there is no introspected files that will output annoying warnings
  # This is needed because meson.build files can have flaws
  clean "$GIMP_DISTRIB" *.lua
  clean "$GIMP_DISTRIB" *.py
  clean "$GIMP_DISTRIB" *.scm
  clean "$GIMP_DISTRIB" *.vala
fi

### Deps (DLLs) of the binaries in 'lib' and 'bin' dirs
echo "(INFO): searching for dependencies of $GIMP_DISTRIB/lib in $GIMP_PREFIX and $MSYS_PREFIX"
libArray=($(find "$GIMP_DISTRIB/lib" \( -iname '*.dll' -or -iname '*.exe' \)))
for dep in "${libArray[@]}"; do
  python3 build/windows/gitlab-ci/3_bundle-gimp-uni_dep.py $dep $GIMP_PREFIX/ $MSYS_PREFIX/ $GIMP_DISTRIB --output-dll-list done-dll.list;
done
echo "(INFO): searching for dependencies of $GIMP_DISTRIB/bin in $GIMP_PREFIX and $MSYS_PREFIX"
binArray=($(find "$GIMP_DISTRIB/bin" \( -iname '*.dll' -or -iname '*.exe' \)))
for dep in "${binArray[@]}"; do
  python3 build/windows/gitlab-ci/3_bundle-gimp-uni_dep.py $dep $GIMP_PREFIX/ $MSYS_PREFIX/ $GIMP_DISTRIB --output-dll-list done-dll.list;
done

### .pdb (CodeView) debug symbols
### crossroad don't have LLVM/Clang backend yet
#if [ "$CI_JOB_NAME" != "gimp-win-x64-cross" ]; then
#  cp -fr ${GIMP_PREFIX}/bin/*.pdb ${GIMP_DISTRIB}/bin/
#fi
