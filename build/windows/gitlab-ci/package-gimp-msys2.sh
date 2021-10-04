#!/bin/bash

set -e

if [[ "$MSYSTEM" == "MINGW32" ]]; then
    export ARTIFACTS_SUFFIX="-w32"
    export MSYS2_ARCH="i686"
    export MSYS_PREFIX="/c/msys64/mingw32/"
    export PATH="/mingw32/bin:$PATH"
    export GIMP_DISTRIB=`realpath ./gimp-w32`
else
    export ARTIFACTS_SUFFIX="-w64"
    export MSYS2_ARCH="x86_64"
    export MSYS_PREFIX="/c/msys64/mingw64/"
    export PATH="/mingw64/bin:$PATH"
    export GIMP_DISTRIB=`realpath ./gimp-w64`
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
    mingw-w64-$MSYS2_ARCH-gtk-doc \
    mingw-w64-$MSYS2_ARCH-iso-codes \
    mingw-w64-$MSYS2_ARCH-json-c \
    mingw-w64-$MSYS2_ARCH-json-glib \
    mingw-w64-$MSYS2_ARCH-lcms2 \
    mingw-w64-$MSYS2_ARCH-lensfun \
    mingw-w64-$MSYS2_ARCH-libarchive \
    mingw-w64-$MSYS2_ARCH-libheif \
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

export GIMP_PREFIX="`realpath ./_install`${ARTIFACTS_SUFFIX}"
export PATH="$GIMP_PREFIX/bin:$PATH"

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
cp -fr ${MSYS_PREFIX}/etc/fonts ${GIMP_DISTRIB}/etc/
cp -fr ${GIMP_PREFIX}/etc/gtk-3.0 ${GIMP_DISTRIB}/etc/

mkdir ${GIMP_DISTRIB}/lib/
cp -fr ${GIMP_PREFIX}/lib/gimp ${GIMP_DISTRIB}/lib/
cp -fr ${GIMP_PREFIX}/lib/gegl-0.4 ${GIMP_DISTRIB}/lib/
cp -fr ${GIMP_PREFIX}/lib/babl-0.1 ${GIMP_DISTRIB}/lib/

cp -fr ${MSYS_PREFIX}/lib/girepository-1.0 ${GIMP_DISTRIB}/lib/
cp -fr ${GIMP_PREFIX}/lib/girepository-1.0/* ${GIMP_DISTRIB}/lib/girepository-1.0/

cp -fr ${MSYS_PREFIX}/lib/gio ${GIMP_DISTRIB}/lib/
cp -fr ${MSYS_PREFIX}/lib/gdk-pixbuf-2.0 ${GIMP_DISTRIB}/lib/
cp -fr ${GIMP_PREFIX}/lib/gtk-3.0 ${GIMP_DISTRIB}/lib/

cp -fr ${MSYS_PREFIX}/lib/python3.9 ${GIMP_DISTRIB}/lib/

cp -fr ${MSYS_PREFIX}/share/ghostscript ${GIMP_DISTRIB}/share/
cp -fr ${MSYS_PREFIX}/share/glib-2.0 ${GIMP_DISTRIB}/share/
cp -fr ${MSYS_PREFIX}/share/libthai ${GIMP_DISTRIB}/share/
cp -fr ${MSYS_PREFIX}/share/libwmf ${GIMP_DISTRIB}/share/
cp -fr ${MSYS_PREFIX}/share/mypaint-data ${GIMP_DISTRIB}/share/
cp -fr ${MSYS_PREFIX}/share/poppler ${GIMP_DISTRIB}/share/

# Move these 2 folders into bin/ so that Lua plug-ins find the lua
# dependencies. The alternative was to modify LUA_PATH_DEFAULT and
# LUA_CPATH_DEFAULT macros in lua (which ender used to do, hence
# rebuilding lua). But it's much simpler to just move these 2 folders as
# bin/lua/ and bin/lgi/
cp -fr ${MSYS_PREFIX}/share/lua/5.1/ ${GIMP_DISTRIB}/bin/lua
cp -fr ${MSYS_PREFIX}/lib/lua/5.1/lgi/ ${GIMP_DISTRIB}/bin/

# XXX Are these themes really needed?
cp -fr ${MSYS_PREFIX}/share/themes ${GIMP_DISTRIB}/share/

# Only copy from langs supported in GIMP.
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

# Only one iso-codes file is useful.
mkdir -p ${GIMP_DISTRIB}/share/xml/iso-codes
cp -fr ${MSYS_PREFIX}/share/xml/iso-codes/iso_639.xml ${GIMP_DISTRIB}/share/xml/iso-codes/

# Adwaita can be used as the base icon set.
cp -fr ${MSYS_PREFIX}/share/icons/Adwaita ${GIMP_DISTRIB}/share/icons/

# XXX Why are these for exactly?
cp -fr ${MSYS_PREFIX}/bin/gspawn*.exe ${GIMP_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gspawn-win*-helper.exe ${GIMP_PREFIX}/ ${GIMP_DISTRIB}
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gspawn-win*-helper.exe ${MSYS_PREFIX}/ ${GIMP_DISTRIB}
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gspawn-win*-helper-console.exe ${GIMP_PREFIX}/ ${GIMP_DISTRIB}
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gspawn-win*-helper-console.exe ${MSYS_PREFIX}/ ${GIMP_DISTRIB}

# XXX Does not look like it's needed anymore. Check?
cp -fr ${MSYS_PREFIX}/bin/gdk-pixbuf-query-loaders.exe ${GIMP_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gdk-pixbuf-query-loaders.exe ${GIMP_PREFIX}/ ${GIMP_DISTRIB}
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gdk-pixbuf-query-loaders.exe ${MSYS_PREFIX}/ ${GIMP_DISTRIB}

# XXX Why is bzip2.exe needed?
cp -fr ${MSYS_PREFIX}/bin/bzip2.exe ${GIMP_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/bzip2.exe ${GIMP_PREFIX}/ ${GIMP_DISTRIB}
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/bzip2.exe ${MSYS_PREFIX}/ ${GIMP_DISTRIB}

# Executables for supported interpreters.
cp -fr ${MSYS_PREFIX}/bin/pythonw.exe ${GIMP_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/pythonw.exe ${GIMP_PREFIX}/ ${GIMP_DISTRIB}
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/pythonw.exe ${MSYS_PREFIX}/ ${GIMP_DISTRIB}
cp -fr ${MSYS_PREFIX}/bin/python3w.exe ${GIMP_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/python3w.exe ${GIMP_PREFIX}/ ${GIMP_DISTRIB}
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/python3w.exe ${MSYS_PREFIX}/ ${GIMP_DISTRIB}
cp -fr ${MSYS_PREFIX}/bin/python3.exe ${GIMP_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/python3.exe ${GIMP_PREFIX}/ ${GIMP_DISTRIB}
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/python3.exe ${MSYS_PREFIX}/ ${GIMP_DISTRIB}

cp -fr ${MSYS_PREFIX}/bin/luajit.exe ${GIMP_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/luajit.exe ${GIMP_PREFIX}/ ${GIMP_DISTRIB}
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/luajit.exe ${MSYS_PREFIX}/ ${GIMP_DISTRIB}

# Executable for "gegl:introspect" from graphviz package.
cp -fr ${MSYS_PREFIX}/bin/dot.exe ${GIMP_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/dot.exe ${GIMP_PREFIX}/ ${GIMP_DISTRIB}
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/dot.exe ${MSYS_PREFIX}/ ${GIMP_DISTRIB}

# Generate share/glib-2.0/schemas/gschemas.compiled
glib-compile-schemas --targetdir=${GIMP_DISTRIB}/share/glib-2.0/schemas ${GIMP_DISTRIB}/share/glib-2.0/schemas

# Package needed DLLs only
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gimp-2.99.exe ${GIMP_PREFIX}/ ${GIMP_DISTRIB}
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gimp-2.99.exe ${MSYS_PREFIX}/ ${GIMP_DISTRIB}

python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gimp-console-2.99.exe ${GIMP_PREFIX}/ ${GIMP_DISTRIB}
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gimp-console-2.99.exe ${MSYS_PREFIX}/ ${GIMP_DISTRIB}

python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gimp-debug-resume.exe ${GIMP_PREFIX}/ ${GIMP_DISTRIB}
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gimp-debug-resume.exe ${MSYS_PREFIX}/ ${GIMP_DISTRIB}

python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gimp-debug-tool-2.99.exe ${GIMP_PREFIX}/ ${GIMP_DISTRIB}
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gimp-debug-tool-2.99.exe ${MSYS_PREFIX}/ ${GIMP_DISTRIB}

python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gimp-test-clipboard-2.99.exe ${GIMP_PREFIX}/ ${GIMP_DISTRIB}
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gimp-test-clipboard-2.99.exe ${MSYS_PREFIX}/ ${GIMP_DISTRIB}

python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gimptool-2.99.exe ${GIMP_PREFIX}/ ${GIMP_DISTRIB}
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/gimptool-2.99.exe ${MSYS_PREFIX}/ ${GIMP_DISTRIB}

for dll in ${GIMP_DISTRIB}/lib/babl-0.1/*.dll; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${GIMP_PREFIX}/ ${GIMP_DISTRIB};
  python3 build/windows/gitlab-ci/dll_link.py $dll ${MSYS_PREFIX}/ ${GIMP_DISTRIB};
done
for dll in ${GIMP_DISTRIB}/lib/gegl-0.4/*.dll; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${GIMP_PREFIX}/ ${GIMP_DISTRIB};
  python3 build/windows/gitlab-ci/dll_link.py $dll ${MSYS_PREFIX}/ ${GIMP_DISTRIB};
done
for dll in ${GIMP_DISTRIB}/lib/gio/modules/*.dll; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${GIMP_PREFIX}/ ${GIMP_DISTRIB};
  python3 build/windows/gitlab-ci/dll_link.py $dll ${MSYS_PREFIX}/ ${GIMP_DISTRIB};
done
for dll in ${GIMP_DISTRIB}/lib/gdk-pixbuf-2.0/2.10.0/loaders/*.dll; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${GIMP_PREFIX}/ ${GIMP_DISTRIB};
  python3 build/windows/gitlab-ci/dll_link.py $dll ${MSYS_PREFIX}/ ${GIMP_DISTRIB};
done
for dll in ${GIMP_DISTRIB}/lib/gimp/2.99/modules/*.dll; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${GIMP_PREFIX}/ ${GIMP_DISTRIB};
  python3 build/windows/gitlab-ci/dll_link.py $dll ${MSYS_PREFIX}/ ${GIMP_DISTRIB};
done
for dll in ${GIMP_DISTRIB}/lib/gimp/2.99/plug-ins/*/*.exe; do
  python3 build/windows/gitlab-ci/dll_link.py --debug $dll ${GIMP_PREFIX}/ ${GIMP_DISTRIB};
  python3 build/windows/gitlab-ci/dll_link.py --debug $dll ${MSYS_PREFIX}/ ${GIMP_DISTRIB};
done

# Libraries for GObject Introspection.

cp -fr ${MSYS_PREFIX}/bin/libgirepository-1.0-1.dll ${GIMP_DISTRIB}/bin/
python3 build/windows/gitlab-ci/dll_link.py ${GIMP_DISTRIB}/bin/libgirepository-1.0-1.dll ${GIMP_PREFIX}/ ${MSYS_PREFIX}/ ${GIMP_DISTRIB}

for dll in ${GIMP_DISTRIB}/lib/python3.9/site-packages/*/*.dll; do
  python3 build/windows/gitlab-ci/dll_link.py $dll ${GIMP_PREFIX}/ ${GIMP_DISTRIB};
  python3 build/windows/gitlab-ci/dll_link.py $dll ${MSYS_PREFIX}/ ${GIMP_DISTRIB};
done
