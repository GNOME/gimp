crossroad source msys2
mkdir _deps && cd _deps

# babl

crossroad install lcms2 && \
git clone --depth 1 https://gitlab.gnome.org/GNOME/babl.git && cd babl && \
crossroad meson _build/ -Denable-gir=false -Dlibdir=lib && \
ninja -C _build install || exit 1
cd ..

# GEGL

crossroad install cairo json-glib && \
git clone --depth 1 https://gitlab.gnome.org/GNOME/gegl.git && cd gegl && \
crossroad meson _build/ -Dintrospection=false -Dsdl2=disabled -Dlibdir=lib && \
ninja -C _build install || exit 1
cd ..

# preparing GIMP

LIBMNG=
if [ "x$CROSSROAD_PLATFORM" = "xw64" ]; then
  # For some reason, file-mng plug-in fails to link in its i686 build.
  # Just disable it for now on i686 only.
  LIBMNG="libmng"
fi

crossroad install appstream-glib              \
                  atk                         \
                  drmingw                     \
                  gexiv2                      \
                  glib2                       \
                  json-c                      \
                  ghostscript                 \
                  iso-codes                   \
                  libheif                     \
                  $LIBMNG                     \
                  libmypaint mypaint-brushes  \
                  libwebp                     \
                  libwmf                      \
                  openexr ilmbase             \
                  poppler poppler-data        \
                  xpm-nox
