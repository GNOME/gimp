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

# Build (part of) GIMP
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
    export GDK_PATH=`echo ${CROSSROAD_PREFIX}/lib/gdk-pixbuf-*/*/`
    GDK_PATH=$(sed "s|${CROSSROAD_PREFIX}/||g" <<< $GDK_PATH)
    wine ${CROSSROAD_PREFIX}/bin/gdk-pixbuf-query-loaders.exe ${CROSSROAD_PREFIX}/${GDK_PATH}loaders/*.dll > ${CROSSROAD_PREFIX}/${GDK_PATH}loaders.cache
    sed -i "s&$CROSSROAD_PREFIX/&&" ${CROSSROAD_PREFIX}/${GDK_PATH}/loaders.cache
    sed -i '/.dll\"/s*/*\\\\*g' ${CROSSROAD_PREFIX}/${GDK_PATH}/loaders.cache

    ## Generator of the glib 'gschemas.compiled'
    export GLIB_PATH=`echo ${CROSSROAD_PREFIX}/share/glib-*/schemas/`
    GLIB_PATH=$(sed "s|${CROSSROAD_PREFIX}/||g" <<< $GLIB_PATH)
    wine glib-compile-schemas --targetdir=${CROSSROAD_PREFIX}/${GLIB_PATH} ${CROSSROAD_PREFIX}/${GLIB_PATH}
fi

## XXX Functional fix to the problem of non-configured interpreters
## XXX Also, functional generator of the pixbuf 'loaders.cache' for GUI image support
gimp_app_version=`grep -rI '\<version *:' ../meson.build | head -1 | sed "s/^.*version *: *'\([0-9]\+\.[0-9]\+\).[0-9]*' *,.*$/\1/"`
echo "@echo off
      echo This is a CI crossbuild of GIMP.
      :: Don't run this under PowerShell since it produces UTF-16 files.
      echo .js   (JavaScript) plug-ins ^|^ NOT supported!
      echo .lua  (Lua) plug-ins        ^|^ NOT supported!
      echo .py   (Python) plug-ins     ^|^ NOT supported!
      echo .scm  (ScriptFu) plug-ins   ^|^ NOT supported!
      echo .vala (Vala) plug-ins       ^|^ NOT supported!
      bin\gdk-pixbuf-query-loaders.exe lib\gdk-pixbuf-2.0\2.10.0\loaders\*.dll > lib\gdk-pixbuf-2.0\2.10.0\loaders.cache
      echo.
      bin\gimp-$gimp_app_version.exe" > ${CROSSROAD_PREFIX}/gimp.cmd
echo "Please run the gimp.cmd file to know the actual plug-in support." > ${CROSSROAD_PREFIX}/README.txt