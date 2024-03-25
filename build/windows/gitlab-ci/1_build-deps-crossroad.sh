if [[ "x$CROSSROAD_PLATFORM" = "xw64" ]]; then
  export ARTIFACTS_SUFFIX="-x64"
else # [[ "x$CROSSROAD_PLATFORM" = "xw32" ]];
  export ARTIFACTS_SUFFIX="-x86"
fi


# Install the required (pre-built) packages for babl, GEGL and GIMP
crossroad source msys2
DEPS_LIST=$(cat build/windows/gitlab-ci/all-deps-uni.txt)
DEPS_LIST=$(sed "s/\${MINGW_PACKAGE_PREFIX}-//g" <<< $DEPS_LIST)
DEPS_LIST=$(sed 's/\\//g' <<< $DEPS_LIST)

crossroad install $DEPS_LIST

if [ $? -ne 0 ]; then
  echo "Installation of pre-built dependencies failed.";
  exit 1;
fi


# Clone babl and GEGL (follow master branch)
mkdir _deps && cd _deps
git clone --depth 1 https://gitlab.gnome.org/GNOME/babl.git _babl
git clone --depth 1 https://gitlab.gnome.org/GNOME/gegl.git _gegl

# Build babl and GEGL
mkdir _babl/_build${ARTIFACTS_SUFFIX}/ && cd _babl/_build${ARTIFACTS_SUFFIX}/
crossroad meson setup .. -Denable-gir=false
ninja && ninja install

mkdir ../../_gegl/_build${ARTIFACTS_SUFFIX}/ && cd ../../_gegl/_build${ARTIFACTS_SUFFIX}/
crossroad meson setup .. -Dintrospection=false
ninja && ninja install
cd ../../


# "Build" part of deps
if [ "x$CROSSROAD_PLATFORM" = "xw64" ]; then
  ## Generator of the gio 'giomodule.cache' to fix error about
  ## libgiognutls.dll that prevents generating loaders.cache
  gio=''
  gio+="libgiognomeproxy.dll: gio-proxy-resolver\n"
  gio+="libgiognutls.dll: gio-tls-backend\n"
  gio+="libgiolibproxy.dll: gio-proxy-resolver\n"
  gio+="libgioopenssl.dll: gio-tls-backend\n"
  printf "%b" "$gio" > ${CROSSROAD_PREFIX}/lib/gio/modules/giomodule.cache

  ## NOT WORKING: Fallback generator of the pixbuf 'loaders.cache' for GUI image support
  GDK_PATH=$(echo ${CROSSROAD_PREFIX}/lib/gdk-pixbuf-*/*/)
  wine ${CROSSROAD_PREFIX}/bin/gdk-pixbuf-query-loaders.exe ${GDK_PATH}loaders/*.dll > ${GDK_PATH}loaders.cache
  sed -i "s&$CROSSROAD_PREFIX/&&" ${CROSSROAD_PREFIX}/${GDK_PATH}/loaders.cache
  sed -i '/.dll\"/s*/*\\\\*g' ${CROSSROAD_PREFIX}/${GDK_PATH}/loaders.cache

  ## Generator of the glib 'gschemas.compiled'
  GLIB_PATH=$(echo ${CROSSROAD_PREFIX}/share/glib-*/schemas/)
  wine ${CROSSROAD_PREFIX}/bin/glib-compile-schemas.exe --targetdir=${GLIB_PATH} ${GLIB_PATH}
fi
