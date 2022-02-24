crossroad source msys2
mkdir _deps && cd _deps

# babl

crossroad install lcms2 && \
git clone --depth 1 https://gitlab.gnome.org/GNOME/babl.git && cd babl && \
crossroad meson _build/ -Denable-gir=false -Dlibdir=lib && \
ninja -C _build install || exit 1
cd ..

# GEGL

crossroad install cairo graphviz json-glib && \
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

if [ $? -ne 0 ]; then
  echo "Installation of pre-built dependencies failed.";
  exit 1;
fi

if [ "x$CROSSROAD_PLATFORM" = "xw64" ]; then
    # build libjxl (not available in MSYS2 yet)
    crossroad install brotli && \
    git clone --depth=1 --branch v0.6.x --recursive https://github.com/libjxl/libjxl.git libjxl && cd libjxl && \
    mkdir _build && cd _build && \
    crossroad cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DJPEGXL_ENABLE_PLUGINS=OFF -DBUILD_TESTING=OFF -DJPEGXL_WARNINGS_AS_ERRORS=OFF -DJPEGXL_ENABLE_SJPEG=OFF -DJPEGXL_ENABLE_BENCHMARK=OFF -DJPEGXL_ENABLE_EXAMPLES=OFF -DJPEGXL_ENABLE_MANPAGES=OFF -DJPEGXL_ENABLE_SKCMS=ON -DJPEGXL_FORCE_SYSTEM_BROTLI=ON -DJPEGXL_FORCE_SYSTEM_HWY=OFF -DJPEGXL_ENABLE_JNI=OFF -DJPEGXL_ENABLE_TCMALLOC=OFF -DJPEGXL_ENABLE_TOOLS=OFF -DCMAKE_CXX_FLAGS="-DHWY_COMPILE_ONLY_SCALAR" .. && \
    ninja && ninja install || exit 1

    # move DLLs into correct location
    if [ -f ${CROSSROAD_PREFIX}/lib/libjxl.dll ]; then
      mv --target-directory=${CROSSROAD_PREFIX}/bin ${CROSSROAD_PREFIX}/lib/libjxl.dll
    fi

    if [ -f ${CROSSROAD_PREFIX}/lib/libjxl_threads.dll ]; then
      mv --target-directory=${CROSSROAD_PREFIX}/bin ${CROSSROAD_PREFIX}/lib/libjxl_threads.dll
    fi

    # install image/jxl mime type
    mkdir -p ${CROSSROAD_PREFIX}/share/mime/packages
    cp --target-directory=${CROSSROAD_PREFIX}/share/mime/packages ../plugins/mime/image-jxl.xml

    cd ../..

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
