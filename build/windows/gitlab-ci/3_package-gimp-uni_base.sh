#!/bin/bash

set -e

# $MSYSTEM_CARCH, $MSYSTEM_PREFIX and $MINGW_PACKAGE_PREFIX are defined by MSYS2.
# https://github.com/msys2/MSYS2-packages/blob/master/filesystem/msystem
if [[ "$MSYSTEM_CARCH" == "aarch64" ]]; then
  export ARTIFACTS_SUFFIX="-a64"
  export MSYS_PREFIX="c:/msys64${MSYSTEM_PREFIX}"
  export GIMP_DISTRIB=`realpath ./gimp-a64`
elif [[ "$CROSSROAD_PLATFORM" == "w64" ]] || [[ "$MSYSTEM_CARCH" == "x86_64" ]]; then
  export ARTIFACTS_SUFFIX="-x64"
  export MSYS_PREFIX="c:/msys64${MSYSTEM_PREFIX}"
  export GIMP_DISTRIB=`realpath ./gimp-x64`
else # [[ "$CROSSROAD_PLATFORM" == "w32" ]] || [[ "$MSYSTEM_CARCH" == "i686" ]];
  export ARTIFACTS_SUFFIX="-x86"
  export MSYS_PREFIX="c:/msys64${MSYSTEM_PREFIX}"
  export GIMP_DISTRIB=`realpath ./gimp-x86`
fi

if [[ "$BUILD_TYPE" != "CI_CROSS" ]] && [[ "$BUILD_TYPE" != "CI_NATIVE" ]]; then
  pacman --noconfirm -Suy
fi


if [[ "$BUILD_TYPE" != "CI_CROSS" ]]; then
  # Install the required (pre-built) packages again
  # We take code from deps script to better maintenance
  GIMP_DIR=""
  DEPS_CODE=$(cat build/windows/gitlab-ci/1_build-deps-msys2.sh)
  DEPS_CODE=$(sed -n '/# Install the/,/# End of install/p' <<< $DEPS_CODE)
  echo "$DEPS_CODE" | bash
fi


# Package deps and GIMP files
export GIMP_PREFIX="`realpath ./_install`${ARTIFACTS_SUFFIX}"
if [[ "$BUILD_TYPE" == "CI_CROSS" ]]; then
  export GIMP_PREFIX="`realpath ./_install`${ARTIFACTS_SUFFIX}-cross"
  export MSYS_PREFIX="$GIMP_PREFIX"
fi
export PATH="$GIMP_PREFIX/bin:$PATH"

## Copy a previously built wrapper at tree root, less messy than
## having to look inside bin/, in the middle of all the DLLs.
## This utility also configure the interpreters.
## Then, copy a built README that make clear the utility of .cmd.
mkdir -p ${GIMP_DISTRIB}
cp -fr ${GIMP_PREFIX}/*.cmd ${GIMP_DISTRIB}/
cp -fr ${GIMP_PREFIX}/*.txt ${GIMP_DISTRIB}/


## Modules.
mkdir ${GIMP_DISTRIB}/etc
cp -fr ${MSYS_PREFIX}/etc/fonts/ ${GIMP_DISTRIB}/etc/
cp -fr ${GIMP_PREFIX}/etc/gimp/ ${GIMP_DISTRIB}/etc/
cp -fr ${MSYS_PREFIX}/etc/gtk-*/ ${GIMP_DISTRIB}/etc/


## Headers.
mkdir ${GIMP_DISTRIB}/include
cp -fr ${GIMP_PREFIX}/include/babl-*/ ${GIMP_DISTRIB}/include/
cp -fr ${GIMP_PREFIX}/include/gegl-*/ ${GIMP_DISTRIB}/include/
cp -fr ${GIMP_PREFIX}/include/gimp-*/ ${GIMP_DISTRIB}/include/


## Library data.
mkdir ${GIMP_DISTRIB}/lib
cp -fr ${GIMP_PREFIX}/lib/babl-*/ ${GIMP_DISTRIB}/lib/
cp -fr ${MSYS_PREFIX}/lib/gdk-pixbuf-*/ ${GIMP_DISTRIB}/lib/
cp -fr ${GIMP_PREFIX}/lib/gegl-*/ ${GIMP_DISTRIB}/lib/
cp -fr ${GIMP_PREFIX}/lib/gimp/ ${GIMP_DISTRIB}/lib/
cp -fr ${MSYS_PREFIX}/lib/gio/ ${GIMP_DISTRIB}/lib/

aList=$(find ${GIMP_DISTRIB}/lib/ -iname '*.a') && aArray=($aList)
for a in "${aArray[@]}"; do
  rm $a
done
rm ${GIMP_DISTRIB}/lib/gegl-*/*.json


## Resources.
mkdir ${GIMP_DISTRIB}/share
cp -fr ${MSYS_PREFIX}/share/ghostscript/ ${GIMP_DISTRIB}/share/
rm -r ${GIMP_DISTRIB}/share/ghostscript/*/doc
cp -fr ${GIMP_PREFIX}/share/gimp/ ${GIMP_DISTRIB}/share/
GLIB_PATH=$(echo ${MSYS_PREFIX}/share/glib-*/schemas)
GLIB_PATH=$(sed "s|${MSYS_PREFIX}/||g" <<< $GLIB_PATH)
mkdir -p ${GIMP_DISTRIB}/${GLIB_PATH}
cp -fr ${MSYS_PREFIX}/share/glib-*/schemas/ ${GIMP_DISTRIB}/share/glib-*/

### Adwaita can be used as the base icon set.
mkdir -p ${GIMP_DISTRIB}/share/icons
cp -fr ${MSYS_PREFIX}/share/icons/Adwaita/ ${GIMP_DISTRIB}/share/icons/
cp -fr ${GIMP_PREFIX}/share/icons/hicolor/ ${GIMP_DISTRIB}/share/icons/

cp -fr ${MSYS_PREFIX}/share/libthai/ ${GIMP_DISTRIB}/share/
cp -fr ${MSYS_PREFIX}/share/libwmf/ ${GIMP_DISTRIB}/share/

### Only copy from langs supported in GIMP.
cp -fr ${GIMP_PREFIX}/share/locale/ ${GIMP_DISTRIB}/share/
for dir in ${GIMP_DISTRIB}/share/locale/*/; do
  lang=`basename "$dir"`;
  # TODO: ideally we could be a bit more accurate and copy only the
  # language files from our dependencies and iso_639.mo. But let's go
  # with this for now, especially as each lang may have different
  # translation availability.
  if [ -d "${MSYS_PREFIX}/share/locale/${lang}/LC_MESSAGES/" ]; then
    cp -fr "${MSYS_PREFIX}/share/locale/${lang}/LC_MESSAGES/"*.mo "${GIMP_DISTRIB}/share/locale/${lang}/LC_MESSAGES/"
  fi
done;

mkdir -p ${GIMP_DISTRIB}/share/man/man1
mkdir -p ${GIMP_DISTRIB}/share/man/man5
cp -fr ${GIMP_PREFIX}/share/man/man1/gimp* ${GIMP_DISTRIB}/share/man/man1/
cp -fr ${GIMP_PREFIX}/share/man/man5/gimp* ${GIMP_DISTRIB}/share/man/man5/
mkdir ${GIMP_DISTRIB}/share/metainfo
cp -fr ${GIMP_PREFIX}/share/metainfo/org.gimp*.xml ${GIMP_DISTRIB}/share/metainfo/
cp -fr ${MSYS_PREFIX}/share/mypaint-data/ ${GIMP_DISTRIB}/share/
cp -fr ${MSYS_PREFIX}/share/poppler/ ${GIMP_DISTRIB}/share/

### Only one iso-codes file is useful.
mkdir -p ${GIMP_DISTRIB}/share/xml/iso-codes
cp -fr ${MSYS_PREFIX}/share/xml/iso-codes/iso_639.xml ${GIMP_DISTRIB}/share/xml/iso-codes/


## Executables and DLLs.

### We save the list of already copied DLLs to keep a state between 3_package-gimp-uni_dep runs.
rm -f done-dll.list

### Minimal (and some additional) executables for the 'bin' folder
mkdir ${GIMP_DISTRIB}/bin
binArray=("${MSYS_PREFIX}/bin/bzip2.exe"
          "${MSYS_PREFIX}/bin/dot.exe"
          "${MSYS_PREFIX}/bin/gdbus.exe"
          "${MSYS_PREFIX}/bin/gdk-pixbuf-query-loaders.exe"
          "${GIMP_PREFIX}/bin/gegl*.exe"
          "${GIMP_PREFIX}/bin/gimp*.exe"
          "${MSYS_PREFIX}/bin/gspawn*.exe")
for exe in "${binArray[@]}"; do
  cp -fr $exe ${GIMP_DISTRIB}/bin/
done

## Optional executables, .DLLs and resources for GObject Introspection support
if [[ "$BUILD_TYPE" != "CI_CROSS" ]]; then
  cp -fr ${MSYS_PREFIX}/bin/libgirepository-*.dll ${GIMP_DISTRIB}/bin/
  python3 build/windows/gitlab-ci/3_package-gimp-uni_dep.py ${GIMP_DISTRIB}/bin/libgirepository-*.dll ${GIMP_PREFIX}/ ${MSYS_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list
  cp -fr ${MSYS_PREFIX}/lib/girepository-*/ ${GIMP_DISTRIB}/lib/
  cp -fr ${GIMP_PREFIX}/lib/girepository-*/* ${GIMP_DISTRIB}/lib/girepository-*/
  cp -fr ${GIMP_PREFIX}/share/gir-*/ ${GIMP_DISTRIB}/share/

  cp -fr ${MSYS_PREFIX}/bin/luajit.exe ${GIMP_DISTRIB}/bin/
  cp -fr ${MSYS_PREFIX}/lib/lua/ ${GIMP_DISTRIB}/lib/
  cp -fr ${MSYS_PREFIX}/share/lua/ ${GIMP_DISTRIB}/share/

  cp -fr ${MSYS_PREFIX}/bin/python*.exe ${GIMP_DISTRIB}/bin/
  cp -fr ${MSYS_PREFIX}/lib/python*/ ${GIMP_DISTRIB}/lib/

  cp -fr ${GIMP_PREFIX}/share/vala/ ${GIMP_DISTRIB}/share/
else
  # Just to ensure there is no introspected files that will output annoying warnings
  # This is needed because meson.build files can have flaws
  goiList=$(find ${GIMP_DISTRIB} \( -iname '*.lua' -or -iname '*.py' -or -iname '*.scm' -or -iname '*.vala' \)) && goiArray=($goiList)
  for goi in "${goiArray[@]}"; do
    rm $goi
  done
fi

### Needed DLLs for the executables in the 'bin' folder
binList=$(find ${GIMP_DISTRIB}/bin/ -iname '*.exe') && binArray=($binList)
for bin in "${binArray[@]}"; do
  python3 build/windows/gitlab-ci/3_package-gimp-uni_dep.py $bin ${GIMP_PREFIX}/ ${MSYS_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list;
done

### Needed DLLs for the executables and DLLs in the 'lib' sub-folders
libList=$(find ${GIMP_DISTRIB}/lib/ \( -iname '*.dll' -or -iname '*.exe' \)) && libArray=($libList)
for lib in "${libArray[@]}"; do
  python3 build/windows/gitlab-ci/3_package-gimp-uni_dep.py $lib ${GIMP_PREFIX}/ ${MSYS_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list;
done
