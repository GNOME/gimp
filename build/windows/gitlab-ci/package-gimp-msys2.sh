#!/bin/bash

set -e

# $MINGW_PREFIX, $MINGW_PACKAGE_PREFIX and $MSYSTEM_ARCH are environment
# variables defined by MSYS2 filesytem package.
# https://github.com/msys2/MSYS2-packages/blob/master/filesystem/msystem

if [[ "$MSYSTEM_CARCH" == "i686" ]]; then
    export ARTIFACTS_SUFFIX="-w32"
    export GIMP_DISTRIB=`realpath ./gimp-w32`
elif [[ "$MSYSTEM_CARCH" == "x86_64" ]]; then
    export ARTIFACTS_SUFFIX="-w64"
    export GIMP_DISTRIB=`realpath ./gimp-w64`
else # [[ "$MSYSTEM_CARCH" == "aarch64" ]];
    export ARTIFACTS_SUFFIX="-arm64"
    export GIMP_DISTRIB=`realpath ./gimp-arm64`
fi

export OPTIONAL_PACKAGES=""
if [[ "$MSYSTEM" == "CLANGARM64" ]]; then
  export OPTIONAL_PACKAGES="${MINGW_PACKAGE_PREFIX}-lua51"
else
  export OPTIONAL_PACKAGES="${MINGW_PACKAGE_PREFIX}-luajit"
fi

# Update everything
pacman --noconfirm -Suy

# Install the required packages
pacman --noconfirm -S --needed \
    base-devel \
    ${MINGW_PACKAGE_PREFIX}-binutils \
    ${MINGW_PACKAGE_PREFIX}-toolchain \
    ${MINGW_PACKAGE_PREFIX}-ccache \
    \
    $OPTIONAL_PACKAGES \
    ${MINGW_PACKAGE_PREFIX}-lua51-lgi \
    \
    ${MINGW_PACKAGE_PREFIX}-aalib \
    ${MINGW_PACKAGE_PREFIX}-appstream-glib \
    ${MINGW_PACKAGE_PREFIX}-atk \
    ${MINGW_PACKAGE_PREFIX}-brotli \
    ${MINGW_PACKAGE_PREFIX}-cairo \
    ${MINGW_PACKAGE_PREFIX}-cfitsio \
    ${MINGW_PACKAGE_PREFIX}-drmingw \
    ${MINGW_PACKAGE_PREFIX}-gexiv2 \
    ${MINGW_PACKAGE_PREFIX}-ghostscript \
    ${MINGW_PACKAGE_PREFIX}-glib2 \
    ${MINGW_PACKAGE_PREFIX}-glib-networking \
    ${MINGW_PACKAGE_PREFIX}-gobject-introspection \
    ${MINGW_PACKAGE_PREFIX}-gobject-introspection-runtime \
    ${MINGW_PACKAGE_PREFIX}-graphviz \
    ${MINGW_PACKAGE_PREFIX}-gtk3 \
    ${MINGW_PACKAGE_PREFIX}-iso-codes \
    ${MINGW_PACKAGE_PREFIX}-json-c \
    ${MINGW_PACKAGE_PREFIX}-json-glib \
    ${MINGW_PACKAGE_PREFIX}-lcms2 \
    ${MINGW_PACKAGE_PREFIX}-lensfun \
    ${MINGW_PACKAGE_PREFIX}-libarchive \
    ${MINGW_PACKAGE_PREFIX}-libheif \
    ${MINGW_PACKAGE_PREFIX}-libiff \
    ${MINGW_PACKAGE_PREFIX}-libilbm \
    ${MINGW_PACKAGE_PREFIX}-libjxl \
    ${MINGW_PACKAGE_PREFIX}-libmypaint \
    ${MINGW_PACKAGE_PREFIX}-libspiro \
    ${MINGW_PACKAGE_PREFIX}-libwebp \
    ${MINGW_PACKAGE_PREFIX}-libwmf \
    ${MINGW_PACKAGE_PREFIX}-maxflow \
    ${MINGW_PACKAGE_PREFIX}-mypaint-brushes \
    ${MINGW_PACKAGE_PREFIX}-openexr \
    ${MINGW_PACKAGE_PREFIX}-pango \
    ${MINGW_PACKAGE_PREFIX}-poppler \
    ${MINGW_PACKAGE_PREFIX}-poppler-data \
    ${MINGW_PACKAGE_PREFIX}-python \
    ${MINGW_PACKAGE_PREFIX}-python3-gobject \
    ${MINGW_PACKAGE_PREFIX}-qoi \
    ${MINGW_PACKAGE_PREFIX}-shared-mime-info \
    ${MINGW_PACKAGE_PREFIX}-suitesparse \
    ${MINGW_PACKAGE_PREFIX}-vala \
    ${MINGW_PACKAGE_PREFIX}-xpm-nox

export MSYS2_PREFIX="/c/msys64${MINGW_PREFIX}/"
export GIMP_PREFIX="`realpath ./_install`${ARTIFACTS_SUFFIX}"
export PATH="$GIMP_PREFIX/bin:${MINGW_PREFIX}/bin:$PATH"

# Package ressources.
mkdir -p ${GIMP_DISTRIB}
cp -fr ${GIMP_PREFIX}/etc ${GIMP_DISTRIB}
cp -fr ${GIMP_PREFIX}/include ${GIMP_DISTRIB}
#cp -fr ${GIMP_PREFIX}/ssl ${GIMP_DISTRIB}
cp -fr ${GIMP_PREFIX}/share ${GIMP_DISTRIB}

# Package executables.
mkdir ${GIMP_DISTRIB}/bin
cp -fr ${GIMP_PREFIX}/bin/gimp*.exe ${GIMP_DISTRIB}/bin/

# With the native Windows build, it's directly in bin/
#mkdir ${GIMP_DISTRIB}/libexec
#cp -fr ${GIMP_PREFIX}/libexec/gimp*.exe ${GIMP_DISTRIB}/libexec/

# Add a wrapper at tree root, less messy than having to look for the
# binary inside bin/, in the middle of all the DLLs.
echo "bin\gimp-2.99.exe" > ${GIMP_DISTRIB}/gimp.cmd

# Package library data and modules.
cp -fr ${MSYS2_PREFIX}/etc/fonts ${GIMP_DISTRIB}/etc/
cp -fr ${MSYS2_PREFIX}/etc/gtk-3.0 ${GIMP_DISTRIB}/etc/

mkdir ${GIMP_DISTRIB}/lib/
cp -fr ${GIMP_PREFIX}/lib/gimp ${GIMP_DISTRIB}/lib/
cp -fr ${GIMP_PREFIX}/lib/gegl-0.4 ${GIMP_DISTRIB}/lib/
cp -fr ${GIMP_PREFIX}/lib/babl-0.1 ${GIMP_DISTRIB}/lib/

cp -fr ${MSYS2_PREFIX}/lib/girepository-1.0 ${GIMP_DISTRIB}/lib/
cp -fr ${GIMP_PREFIX}/lib/girepository-1.0/* ${GIMP_DISTRIB}/lib/girepository-1.0/

cp -fr ${MSYS2_PREFIX}/lib/gio ${GIMP_DISTRIB}/lib/
cp -fr ${MSYS2_PREFIX}/lib/gdk-pixbuf-2.0 ${GIMP_DISTRIB}/lib/
cp -fr ${MSYS2_PREFIX}/lib/gtk-3.0 ${GIMP_DISTRIB}/lib/

cp -fr ${MSYS2_PREFIX}/lib/python3.11 ${GIMP_DISTRIB}/lib/

cp -fr ${MSYS2_PREFIX}/share/ghostscript ${GIMP_DISTRIB}/share/
cp -fr ${MSYS2_PREFIX}/share/glib-2.0 ${GIMP_DISTRIB}/share/
cp -fr ${MSYS2_PREFIX}/share/libthai ${GIMP_DISTRIB}/share/
cp -fr ${MSYS2_PREFIX}/share/libwmf ${GIMP_DISTRIB}/share/
cp -fr ${MSYS2_PREFIX}/share/mypaint-data ${GIMP_DISTRIB}/share/
cp -fr ${MSYS2_PREFIX}/share/poppler ${GIMP_DISTRIB}/share/

cp -fr ${MSYS2_PREFIX}/share/lua/ ${GIMP_DISTRIB}/share/
cp -fr ${MSYS2_PREFIX}/lib/lua/ ${GIMP_DISTRIB}/lib/

# XXX Are these themes really needed?
cp -fr ${MSYS2_PREFIX}/share/themes ${GIMP_DISTRIB}/share/

# Only copy from langs supported in GIMP.
for dir in ${GIMP_DISTRIB}/share/locale/*/; do
  lang=`basename "$dir"`;
  # TODO: ideally we could be a bit more accurate and copy only the
  # language files from our dependencies and iso_639.mo. But let's go
  # with this for now, especially as each lang may have different
  # translation availability.
  if [ -d "${MSYS2_PREFIX}/share/locale/${lang}/LC_MESSAGES/" ]; then
    cp -fr "${MSYS2_PREFIX}/share/locale/${lang}/LC_MESSAGES/"*.mo "${GIMP_DISTRIB}/share/locale/${lang}/LC_MESSAGES/"
  fi
done;

# Only one iso-codes file is useful.
mkdir -p ${GIMP_DISTRIB}/share/xml/iso-codes
cp -fr ${MSYS2_PREFIX}/share/xml/iso-codes/iso_639.xml ${GIMP_DISTRIB}/share/xml/iso-codes/

# Adwaita can be used as the base icon set.
cp -fr ${MSYS2_PREFIX}/share/icons/Adwaita ${GIMP_DISTRIB}/share/icons/

# XXX Why are these for exactly?
cp -fr ${MSYS2_PREFIX}/bin/gspawn*.exe ${GIMP_DISTRIB}/bin/

# We save the list of already copied DLLs to keep a state between dll_link runs.
rm -f done-dll.list

python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gspawn-win*-helper.exe ${GIMP_PREFIX}/ ${MSYS2_PREFIX} ${GIMP_DISTRIB} --output-dll-list done-dll.list
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gspawn-win*-helper-console.exe ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list

# XXX Does not look like it's needed anymore. Check?
cp -fr ${MSYS2_PREFIX}/bin/gdk-pixbuf-query-loaders.exe ${GIMP_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gdk-pixbuf-query-loaders.exe ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list

# XXX Why is bzip2.exe needed?
cp -fr ${MSYS2_PREFIX}/bin/bzip2.exe ${GIMP_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/bzip2.exe ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list

# Executables for supported interpreters.
cp -fr ${MSYS2_PREFIX}/bin/pythonw.exe ${GIMP_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/pythonw.exe ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list
cp -fr ${MSYS2_PREFIX}/bin/python3w.exe ${GIMP_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/python3w.exe ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list
cp -fr ${MSYS2_PREFIX}/bin/python3.exe ${GIMP_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/python3.exe ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list

if [[ "$MSYSTEM" == "CLANGARM64" ]]; then
  cp -fr ${MSYS2_PREFIX}/bin/lua5.1.exe ${GIMP_DISTRIB}/bin/
  python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/lua5.1.exe ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list
else
  cp -fr ${MSYS2_PREFIX}/bin/luajit.exe ${GIMP_DISTRIB}/bin/
  python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/luajit.exe ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list
fi

# Executable for "gegl:introspect" from graphviz package.
cp -fr ${MSYS2_PREFIX}/bin/dot.exe ${GIMP_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/dot.exe ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list

# Generate share/glib-2.0/schemas/gschemas.compiled
glib-compile-schemas --targetdir=${GIMP_DISTRIB}/share/glib-2.0/schemas ${GIMP_DISTRIB}/share/glib-2.0/schemas

# Package needed DLLs only
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gimp-2.99.exe ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list

python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gimp-console-2.99.exe ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list

python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gimp-debug-resume.exe ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list

python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gimp-debug-tool-2.99.exe ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list

python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gimp-test-clipboard-2.99.exe ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list

python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gimptool-2.99.exe ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list

for dll in ${GIMP_DISTRIB}/lib/babl-0.1/*.dll; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list;
done
for dll in ${GIMP_DISTRIB}/lib/gegl-0.4/*.dll; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list;
done
for dll in ${GIMP_DISTRIB}/lib/gio/modules/*.dll; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list;
done
for dll in ${GIMP_DISTRIB}/lib/gdk-pixbuf-2.0/2.10.0/loaders/*.dll; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list;
done
for dll in ${GIMP_DISTRIB}/lib/gimp/2.99/modules/*.dll; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list;
done
for dll in ${GIMP_DISTRIB}/lib/gimp/2.99/plug-ins/*/*.exe; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list;
done

# Libraries for GObject Introspection.

cp -fr ${MSYS2_PREFIX}/bin/libgirepository-1.0-1.dll ${GIMP_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/libgirepository-1.0-1.dll ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list

for dll in ${GIMP_DISTRIB}/lib/python3.11/site-packages/*/*.dll; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${GIMP_PREFIX}/ ${MSYS2_PREFIX}/ ${GIMP_DISTRIB} --output-dll-list done-dll.list;
done
