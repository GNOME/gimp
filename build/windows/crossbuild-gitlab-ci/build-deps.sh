mkdir _deps && cd _deps

# babl

crossroad install lcms2 && \
git clone --depth 1 https://gitlab.gnome.org/GNOME/babl.git && cd babl && \
crossroad meson _build/ -Denable-gir=false -Dwith-lcms=false && \
ninja -C _build install || exit 1
cd ..

# GEGL

crossroad install cairo json-glib && \
git clone --depth 1 https://gitlab.gnome.org/GNOME/gegl.git && cd gegl && \
crossroad meson _build/ -Dintrospection=false -Dsdl2=disabled && \
ninja -C _build install || exit 1
cd ..

# gexiv2

crossroad install exiv2 && \
git clone --depth 1 https://gitlab.gnome.org/GNOME/gexiv2.git && cd gexiv2 && \
crossroad meson _build/ -Dintrospection=false -Dvapi=false && \
ninja -C _build install || exit 1
cd ..

# appstream-glib

crossroad install libsoup libarchive gdk-pixbuf && \
git clone --depth 1 https://github.com/hughsie/appstream-glib.git && cd appstream-glib && \
crossroad meson _build/ -Dintrospection=false -Drpm=false -Dbuilder=false \
                        -Dstemmer=false -Dman=false -Ddep11=false && \
ninja -C _build install || exit 1
cd ..

# json-c

# TODO: implicit-fallthrough and no-return-type warnings must be
# reenabled once the following patch has been merged into json-c source:
# https://github.com/json-c/json-c/pull/556
git clone --depth 1 https://github.com/json-c/json-c.git && \
mkdir json-c/_build && cd json-c/_build && \
CFLAGS="-Wno-expansion-to-defined -Wimplicit-fallthrough=0 -Wno-return-type" crossroad ../configure && make install || exit 1
cd ../..

# libmypaint

git clone --depth 1 --branch libmypaint-v1 https://github.com/mypaint/libmypaint.git && \
mkdir libmypaint/_build && cd libmypaint/_build && \
crossroad ../configure --enable-introspection=no && make install || exit 1
cd ../..

# mypaint-brushes

git clone --depth 1 --branch v1.3.x https://github.com/mypaint/mypaint-brushes.git
mkdir mypaint-brushes/_build && cd mypaint-brushes/_build
crossroad ../configure && make install || exit 1
cd ../..

# poppler-data

git clone --depth 1 https://anongit.freedesktop.org/git/poppler/poppler-data.git && \
mkdir poppler-data/_build && cd poppler-data/_build && \
crossroad cmake .. && make install || exit 1
cd ../..

# Glib (available in crossroad but bumped Pango needs higher version)

git clone --depth 1 https://gitlab.gnome.org/GNOME/glib.git && cd glib && \
crossroad meson _build && ninja -C _build install || exit 1
crossroad mask glib
cd ..

# Fix a bug in Mingw-w64 headers when building with strict
# C90-compliance.
# See my commit fb232993 in mingw-w64-mingw-w64 repository.
sed -i 's%^//\(.*\)$%/*\1 */%' $CROSSROAD_PREFIX/include/stdlib.h

# Pango (available in crossroad but too old)

crossroad install harfbuzz && \
git clone --depth 1 https://gitlab.gnome.org/GNOME/pango.git && cd pango && \
crossroad meson _build -Dintrospection=false && \
ninja -C _build install || exit 1
crossroad mask pango
cd ..

# DrMingw

git clone --depth 1 https://github.com/jrfonseca/drmingw.git && \
mkdir drmingw/_build && cd drmingw/_build && \
crossroad cmake .. -DPYTHON_EXECUTABLE=`which python3` && make install || exit 1
cd ../..

# preparing GIMP

crossroad install atk gtk3 libtiff xz-libs librsvg2 poppler-glib dbus-glib
