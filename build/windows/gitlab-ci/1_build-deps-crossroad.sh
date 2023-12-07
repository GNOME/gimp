crossroad source msys2
mkdir _deps && cd _deps

# babl

crossroad install lcms2 && \
git clone --depth 1 https://gitlab.gnome.org/GNOME/babl.git && cd babl && \
crossroad meson setup _build/ -Denable-gir=false -Dlibdir=lib && \
ninja -C _build install || exit 1
cd ..

# GEGL

crossroad install cairo graphviz json-glib && \
git clone --depth 1 https://gitlab.gnome.org/GNOME/gegl.git && cd gegl && \
crossroad meson setup _build/ -Dintrospection=false -Dsdl2=disabled -Dlibdir=lib && \
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
                  aalib                       \
                  atk                         \
                  cfitsio                     \
                  drmingw                     \
                  gexiv2                      \
                  glib2                       \
                  glib-networking             \
                  json-c                      \
                  ghostscript                 \
                  gobject-introspection       \
                  gobject-introspection-runtime \
                  iso-codes                   \
                  libheif                     \
                  libiff                      \
                  libilbm                     \
                  libjxl                      \
                  $LIBMNG                     \
                  libmypaint mypaint-brushes  \
                  libwebp                     \
                  libwmf                      \
                  openexr                     \
                  poppler poppler-data        \
                  qoi                         \
                  xpm-nox

if [ $? -ne 0 ]; then
  echo "Installation of pre-built dependencies failed.";
  exit 1;
fi

if [ "x$CROSSROAD_PLATFORM" = "xw64" ]; then
    # Generate the loaders.cache file for GUI image support.
    # Note: this is mostly for distribution so I initially wanted to
    # have these in "win64-nightly" job but "win32-nightly" also
    # requires the same file (and I fail to install wine32) whereas
    # Gitlab "needs" field requires jobs to be from a prior stage. So I
    # generate this here, with dependencies.
    wine ${CROSSROAD_PREFIX}/bin/gdk-pixbuf-query-loaders.exe ${CROSSROAD_PREFIX}/lib/gdk-pixbuf-2.0/2.10.0/loaders/*.dll > ${CROSSROAD_PREFIX}/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache
    sed -i "s&$CROSSROAD_PREFIX/&&" ${CROSSROAD_PREFIX}/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache
    sed -i '/.dll\"/s*/*\\\\*g' ${CROSSROAD_PREFIX}/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache
fi
