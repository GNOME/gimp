if [[ "x$CROSSROAD_PLATFORM" = "xw64" ]]; then
    export ARTIFACTS_SUFFIX="-x64"
else # [[ "x$CROSSROAD_PLATFORM" = "xw32" ]];
    export ARTIFACTS_SUFFIX="-x86"
fi


# Install the required (pre-built) packages for babl and GEGL
crossroad source msys2
crossroad install cairo     \
                  graphviz  \
                  json-glib \
                  lcms2

# Clone babl and GEGL (follow master branch)
mkdir _deps && cd _deps
git clone --depth 1 https://gitlab.gnome.org/GNOME/babl.git _babl 
git clone --depth 1 https://gitlab.gnome.org/GNOME/gegl.git _gegl

# Build babl and GEGL
mkdir _babl/_build${ARTIFACTS_SUFFIX}/ && cd _babl/_build${ARTIFACTS_SUFFIX}/
crossroad meson setup .. -Denable-gir=false \
                         -Dlibdir=lib
ninja && ninja install

mkdir ../../_gegl/_build${ARTIFACTS_SUFFIX}/ && cd ../../_gegl/_build${ARTIFACTS_SUFFIX}/
crossroad meson setup .. -Dintrospection=false \
                         -Dlibdir=lib          \
                         -Dsdl2=disabled  
ninja && ninja install
cd ../../


# Install the required (pre-built) packages for GIMP
export DEPS_PATH="../build/windows/gitlab-ci/all-deps-uni.txt"
sed -i "s/DEPS_ARCH_//g" $DEPS_PATH
export GIMP_DEPS=`cat $DEPS_PATH`
crossroad install libmng $GIMP_DEPS

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
