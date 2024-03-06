set -e

if [[ "x$CROSSROAD_PLATFORM" = "xw64" ]]; then
  export ARTIFACTS_SUFFIX="-x64"
  export MESON_OPTIONS=""
else # [[ "x$CROSSROAD_PLATFORM" = "xw32" ]];
  export ARTIFACTS_SUFFIX="-x86"
  export MESON_OPTIONS="-Dwmf=disabled -Dmng=disabled"
fi


# The required packages for GIMP are taken from the previous job


# Build GIMP
mkdir _build${ARTIFACTS_SUFFIX} && cd _build${ARTIFACTS_SUFFIX}
crossroad meson setup .. -Dgi-docgen=disabled                 \
                         -Djavascript=disabled -Dlua=disabled \
                         -Dpython=disabled -Dvala=disabled    \
                         $MESON_OPTIONS
(ninja && ninja install) || exit 1
cd ..


## XXX Functional fix to the problem of non-configured interpreters
## XXX Also, functional generator of the pixbuf 'loaders.cache' for GUI image support
GIMP_APP_VERSION=$(grep GIMP_APP_VERSION _build${ARTIFACTS_SUFFIX}/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
GDK_PATH=$(echo ${CROSSROAD_PREFIX}/lib/gdk-pixbuf-*/*/)
GDK_PATH=$(sed "s|${CROSSROAD_PREFIX}/||g" <<< $GDK_PATH)
GDK_PATH=$(sed 's|/|\\|g' <<< $GDK_PATH)
echo "@echo off
      echo This is a CI crossbuild of GIMP.
      :: Don't run this under PowerShell since it produces UTF-16 files.
      echo .js   (JavaScript) plug-ins ^|^ NOT supported!
      echo .lua  (Lua) plug-ins        ^|^ NOT supported!
      echo .py   (Python) plug-ins     ^|^ NOT supported!
      echo .scm  (ScriptFu) plug-ins   ^|^ NOT supported!
      echo .vala (Vala) plug-ins       ^|^ NOT supported!
      bin\gdk-pixbuf-query-loaders.exe ${GDK_PATH}loaders\*.dll > ${GDK_PATH}loaders.cache
      echo.
      bin\gimp-$GIMP_APP_VERSION.exe" > ${CROSSROAD_PREFIX}/gimp.cmd
echo "Please run the gimp.cmd file to know the actual plug-in support." > ${CROSSROAD_PREFIX}/README.txt


# Copy built GIMP, babl and GEGL and pre-built packages to GIMP_PREFIX
cp -fr $CROSSROAD_PREFIX/ _install${ARTIFACTS_SUFFIX}/

if [[ "x$CROSSROAD_PLATFORM" = "xw32" ]]; then
  # We fail to install wine32 in x86 dep job and Gitlab "needs" field
  # requires jobs to be from a prior stage, so we take from the x64 dep job
  export CROSSROAD_PREFIX=".local/share/crossroad/roads/w64/gimp"

  cp ${CROSSROAD_PREFIX}/lib/gio/modules/giomodule.cache _install${ARTIFACTS_SUFFIX}/lib/gio/modules

  GDK_PATH=$(echo ${CROSSROAD_PREFIX}/lib/gdk-pixbuf-*/*/)
  GDK_PATH=$(sed "s|${CROSSROAD_PREFIX}/||g" <<< $GDK_PATH)
  cp ${CROSSROAD_PREFIX}/${GDK_PATH}loaders.cache _install${ARTIFACTS_SUFFIX}/${GDK_PATH}

  GLIB_PATH=$(echo ${CROSSROAD_PREFIX}/share/glib-*/schemas/)
  GLIB_PATH=$(sed "s|${CROSSROAD_PREFIX}/||g" <<< $GLIB_PATH)
  cp ${CROSSROAD_PREFIX}/${GLIB_PATH}gschemas.compiled _install${ARTIFACTS_SUFFIX}/${GLIB_PATH}
fi
