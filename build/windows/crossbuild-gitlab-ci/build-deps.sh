crossroad source msys2
mkdir _deps && cd _deps

# babl

crossroad install lcms2 && \
git clone --depth 1 https://gitlab.gnome.org/GNOME/babl.git && cd babl && \
crossroad meson _build/ -Denable-gir=false && \
ninja -C _build install || exit 1
cd ..

# GEGL

crossroad install json-glib && \
git clone --depth 1 https://gitlab.gnome.org/GNOME/gegl.git && cd gegl && \
crossroad meson _build/ -Dintrospection=false -Dsdl2=disabled && \
ninja -C _build install || exit 1
cd ..

# preparing GIMP

crossroad install appstream-glib              \
                  drmingw                     \
                  gexiv2                      \
                  json-c                      \
                  ghostscript                 \
                  iso-codes                   \
                  libheif                     \
                  libmng                      \
                  libmypaint mypaint-brushes  \
                  libwebp                     \
                  libwmf                      \
                  openexr                     \
                  poppler poppler-data        \
                  xpm-nox
