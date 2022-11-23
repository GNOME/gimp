#!/bin/bash

set -e

if [[ "$MSYSTEM" == "MINGW32" ]]; then
    export ARTIFACTS_SUFFIX="-w32"
    export MSYS2_ARCH="i686"
    export MSYS_PREFIX="/c/msys64/mingw32/"
    export PATH="/mingw32/bin:$PATH"
    export LIGMA_DISTRIB=`realpath ./ligma-w32`
else
    export ARTIFACTS_SUFFIX="-w64"
    export MSYS2_ARCH="x86_64"
    export MSYS_PREFIX="/c/msys64/mingw64/"
    export PATH="/mingw64/bin:$PATH"
    export LIGMA_DISTRIB=`realpath ./ligma-w64`
fi

# Update everything
pacman --noconfirm -Suy

# Install the required packages
pacman --noconfirm -S --needed \
    base-devel \
    mingw-w64-$MSYS2_ARCH-binutils \
    mingw-w64-$MSYS2_ARCH-toolchain \
    mingw-w64-$MSYS2_ARCH-ccache \
    \
    mingw-w64-$MSYS2_ARCH-aalib \
    mingw-w64-$MSYS2_ARCH-appstream-glib \
    mingw-w64-$MSYS2_ARCH-atk \
    mingw-w64-$MSYS2_ARCH-brotli \
    mingw-w64-$MSYS2_ARCH-cairo \
    mingw-w64-$MSYS2_ARCH-drmingw \
    mingw-w64-$MSYS2_ARCH-gexiv2 \
    mingw-w64-$MSYS2_ARCH-ghostscript \
    mingw-w64-$MSYS2_ARCH-glib2 \
    mingw-w64-$MSYS2_ARCH-glib-networking \
    mingw-w64-$MSYS2_ARCH-gobject-introspection \
    mingw-w64-$MSYS2_ARCH-gobject-introspection-runtime \
    mingw-w64-$MSYS2_ARCH-graphviz \
    mingw-w64-$MSYS2_ARCH-gtk3 \
    mingw-w64-$MSYS2_ARCH-iso-codes \
    mingw-w64-$MSYS2_ARCH-json-c \
    mingw-w64-$MSYS2_ARCH-json-glib \
    mingw-w64-$MSYS2_ARCH-lcms2 \
    mingw-w64-$MSYS2_ARCH-lensfun \
    mingw-w64-$MSYS2_ARCH-libarchive \
    mingw-w64-$MSYS2_ARCH-libheif \
    mingw-w64-$MSYS2_ARCH-libjxl \
    mingw-w64-$MSYS2_ARCH-libmypaint \
    mingw-w64-$MSYS2_ARCH-libspiro \
    mingw-w64-$MSYS2_ARCH-libwebp \
    mingw-w64-$MSYS2_ARCH-libwmf \
    mingw-w64-$MSYS2_ARCH-lua51-lgi \
    mingw-w64-$MSYS2_ARCH-luajit \
    mingw-w64-$MSYS2_ARCH-maxflow \
    mingw-w64-$MSYS2_ARCH-mypaint-brushes \
    mingw-w64-$MSYS2_ARCH-openexr \
    mingw-w64-$MSYS2_ARCH-pango \
    mingw-w64-$MSYS2_ARCH-poppler \
    mingw-w64-$MSYS2_ARCH-poppler-data \
    mingw-w64-$MSYS2_ARCH-python \
    mingw-w64-$MSYS2_ARCH-python3-gobject \
    mingw-w64-$MSYS2_ARCH-shared-mime-info \
    mingw-w64-$MSYS2_ARCH-suitesparse \
    mingw-w64-$MSYS2_ARCH-vala \
    mingw-w64-$MSYS2_ARCH-xpm-nox

export LIGMA_PREFIX="`realpath ./_install`${ARTIFACTS_SUFFIX}"
export PATH="$LIGMA_PREFIX/bin:$PATH"

# Package ressources.
mkdir -p ${LIGMA_DISTRIB}
cp -fr ${LIGMA_PREFIX}/etc ${LIGMA_DISTRIB}
cp -fr ${LIGMA_PREFIX}/include ${LIGMA_DISTRIB}
#cp -fr ${LIGMA_PREFIX}/ssl ${LIGMA_DISTRIB}
cp -fr ${LIGMA_PREFIX}/share ${LIGMA_DISTRIB}

# Package executables.
mkdir ${LIGMA_DISTRIB}/bin
cp -fr ${LIGMA_PREFIX}/bin/ligma*.exe ${LIGMA_DISTRIB}/bin/

# With the native Windows build, it's directly in bin/
#mkdir ${LIGMA_DISTRIB}/libexec
#cp -fr ${LIGMA_PREFIX}/libexec/ligma*.exe ${LIGMA_DISTRIB}/libexec/

# Add a wrapper at tree root, less messy than having to look for the
# binary inside bin/, in the middle of all the DLLs.
echo "bin\ligma-2.99.exe" > ${LIGMA_DISTRIB}/ligma.cmd

# Package library data and modules.
cp -fr ${MSYS_PREFIX}/etc/fonts ${LIGMA_DISTRIB}/etc/
cp -fr ${MSYS_PREFIX}/etc/gtk-3.0 ${LIGMA_DISTRIB}/etc/

mkdir ${LIGMA_DISTRIB}/lib/
cp -fr ${LIGMA_PREFIX}/lib/ligma ${LIGMA_DISTRIB}/lib/
cp -fr ${LIGMA_PREFIX}/lib/gegl-0.4 ${LIGMA_DISTRIB}/lib/
cp -fr ${LIGMA_PREFIX}/lib/babl-0.1 ${LIGMA_DISTRIB}/lib/

cp -fr ${MSYS_PREFIX}/lib/girepository-1.0 ${LIGMA_DISTRIB}/lib/
cp -fr ${LIGMA_PREFIX}/lib/girepository-1.0/* ${LIGMA_DISTRIB}/lib/girepository-1.0/

cp -fr ${MSYS_PREFIX}/lib/gio ${LIGMA_DISTRIB}/lib/
cp -fr ${MSYS_PREFIX}/lib/gdk-pixbuf-2.0 ${LIGMA_DISTRIB}/lib/
cp -fr ${MSYS_PREFIX}/lib/gtk-3.0 ${LIGMA_DISTRIB}/lib/

cp -fr ${MSYS_PREFIX}/lib/python3.10 ${LIGMA_DISTRIB}/lib/

cp -fr ${MSYS_PREFIX}/share/ghostscript ${LIGMA_DISTRIB}/share/
cp -fr ${MSYS_PREFIX}/share/glib-2.0 ${LIGMA_DISTRIB}/share/
cp -fr ${MSYS_PREFIX}/share/libthai ${LIGMA_DISTRIB}/share/
cp -fr ${MSYS_PREFIX}/share/libwmf ${LIGMA_DISTRIB}/share/
cp -fr ${MSYS_PREFIX}/share/mypaint-data ${LIGMA_DISTRIB}/share/
cp -fr ${MSYS_PREFIX}/share/poppler ${LIGMA_DISTRIB}/share/

# Move these 2 folders into bin/ so that Lua plug-ins find the lua
# dependencies. The alternative was to modify LUA_PATH_DEFAULT and
# LUA_CPATH_DEFAULT macros in lua (which ender used to do, hence
# rebuilding lua). But it's much simpler to just move these 2 folders as
# bin/lua/ and bin/lgi/
cp -fr ${MSYS_PREFIX}/share/lua/5.1/ ${LIGMA_DISTRIB}/bin/lua
cp -fr ${MSYS_PREFIX}/lib/lua/5.1/lgi/ ${LIGMA_DISTRIB}/bin/

# XXX Are these themes really needed?
cp -fr ${MSYS_PREFIX}/share/themes ${LIGMA_DISTRIB}/share/

# Only copy from langs supported in LIGMA.
for dir in ${LIGMA_DISTRIB}/share/locale/*/; do
  lang=`basename "$dir"`;
  # TODO: ideally we could be a bit more accurate and copy only the
  # language files from our dependencies and iso_639.mo. But let's go
  # with this for now, especially as each lang may have different
  # translation availability.
  if [ -d "${MSYS_PREFIX}/share/locale/${lang}/LC_MESSAGES/" ]; then
    cp -fr "${MSYS_PREFIX}/share/locale/${lang}/LC_MESSAGES/"*.mo "${LIGMA_DISTRIB}/share/locale/${lang}/LC_MESSAGES/"
  fi
done;

# Only one iso-codes file is useful.
mkdir -p ${LIGMA_DISTRIB}/share/xml/iso-codes
cp -fr ${MSYS_PREFIX}/share/xml/iso-codes/iso_639.xml ${LIGMA_DISTRIB}/share/xml/iso-codes/

# Adwaita can be used as the base icon set.
cp -fr ${MSYS_PREFIX}/share/icons/Adwaita ${LIGMA_DISTRIB}/share/icons/

# XXX Why are these for exactly?
cp -fr ${MSYS_PREFIX}/bin/gspawn*.exe ${LIGMA_DISTRIB}/bin/

# We save the list of already copied DLLs to keep a state between dll_link runs.
rm -f done-dll.list

python3 build/windows/gitlab-ci/dll_link.py ${LIGMA_DISTRIB}/bin/gspawn-win*-helper.exe ${LIGMA_PREFIX}/ ${MSYS_PREFIX} ${LIGMA_DISTRIB} --output-dll-list done-dll.list
python3 build/windows/gitlab-ci/dll_link.py ${LIGMA_DISTRIB}/bin/gspawn-win*-helper-console.exe ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list

# XXX Does not look like it's needed anymore. Check?
cp -fr ${MSYS_PREFIX}/bin/gdk-pixbuf-query-loaders.exe ${LIGMA_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${LIGMA_DISTRIB}/bin/gdk-pixbuf-query-loaders.exe ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list

# XXX Why is bzip2.exe needed?
cp -fr ${MSYS_PREFIX}/bin/bzip2.exe ${LIGMA_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${LIGMA_DISTRIB}/bin/bzip2.exe ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list

# Executables for supported interpreters.
cp -fr ${MSYS_PREFIX}/bin/pythonw.exe ${LIGMA_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${LIGMA_DISTRIB}/bin/pythonw.exe ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list
cp -fr ${MSYS_PREFIX}/bin/python3w.exe ${LIGMA_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${LIGMA_DISTRIB}/bin/python3w.exe ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list
cp -fr ${MSYS_PREFIX}/bin/python3.exe ${LIGMA_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${LIGMA_DISTRIB}/bin/python3.exe ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list

cp -fr ${MSYS_PREFIX}/bin/luajit.exe ${LIGMA_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${LIGMA_DISTRIB}/bin/luajit.exe ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list

# Executable for "gegl:introspect" from graphviz package.
cp -fr ${MSYS_PREFIX}/bin/dot.exe ${LIGMA_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${LIGMA_DISTRIB}/bin/dot.exe ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list

# Generate share/glib-2.0/schemas/gschemas.compiled
glib-compile-schemas --targetdir=${LIGMA_DISTRIB}/share/glib-2.0/schemas ${LIGMA_DISTRIB}/share/glib-2.0/schemas

# Package needed DLLs only
python3 build/windows/gitlab-ci/dll_link.py ${LIGMA_DISTRIB}/bin/ligma-2.99.exe ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list

python3 build/windows/gitlab-ci/dll_link.py ${LIGMA_DISTRIB}/bin/ligma-console-2.99.exe ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list

python3 build/windows/gitlab-ci/dll_link.py ${LIGMA_DISTRIB}/bin/ligma-debug-resume.exe ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list

python3 build/windows/gitlab-ci/dll_link.py ${LIGMA_DISTRIB}/bin/ligma-debug-tool-2.99.exe ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list

python3 build/windows/gitlab-ci/dll_link.py ${LIGMA_DISTRIB}/bin/ligma-test-clipboard-2.99.exe ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list

python3 build/windows/gitlab-ci/dll_link.py ${LIGMA_DISTRIB}/bin/ligmatool-2.99.exe ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list

for dll in ${LIGMA_DISTRIB}/lib/babl-0.1/*.dll; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list;
done
for dll in ${LIGMA_DISTRIB}/lib/gegl-0.4/*.dll; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list;
done
for dll in ${LIGMA_DISTRIB}/lib/gio/modules/*.dll; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list;
done
for dll in ${LIGMA_DISTRIB}/lib/gdk-pixbuf-2.0/2.10.0/loaders/*.dll; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list;
done
for dll in ${LIGMA_DISTRIB}/lib/ligma/2.99/modules/*.dll; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list;
done
for dll in ${LIGMA_DISTRIB}/lib/ligma/2.99/plug-ins/*/*.exe; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list;
done

# Libraries for GObject Introspection.

cp -fr ${MSYS_PREFIX}/bin/libgirepository-1.0-1.dll ${LIGMA_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${LIGMA_DISTRIB}/bin/libgirepository-1.0-1.dll ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list

for dll in ${LIGMA_DISTRIB}/lib/python3.10/site-packages/*/*.dll; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${LIGMA_PREFIX}/ ${MSYS_PREFIX}/ ${LIGMA_DISTRIB} --output-dll-list done-dll.list;
done
