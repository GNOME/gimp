if [[ "x$CROSSROAD_PLATFORM" = "xw64" ]]; then
    export ARTIFACTS_SUFFIX="-x64"
    export MESON_OPTIONS=""
else # [[ "x$CROSSROAD_PLATFORM" = "xw32" ]];
    export ARTIFACTS_SUFFIX="-x86"
    export MESON_OPTIONS="-Dwmf=disabled -Dmng=disabled"
fi


# Build GIMP
mkdir _build${ARTIFACTS_SUFFIX} && cd _build${ARTIFACTS_SUFFIX}
crossroad meson setup .. -Dgi-docgen=disabled \
                         -Djavascript=disabled $MESON_OPTIONS
ninja && ninja install
cd ..

# Copy the required packages and (part of) GIMP from the deps artifact
cp -fr $CROSSROAD_PREFIX/ _install${ARTIFACTS_SUFFIX}/

if [[ "x$CROSSROAD_PLATFORM" = "xw32" ]]; then
    # We fail to install wine32 in x86 dep job and Gitlab "needs" field
    # requires jobs to be from a prior stage, so we take from the x64 dep job
    export CROSSROAD_PREFIX=".local/share/crossroad/roads/w64/gimp"
    
    cp ${CROSSROAD_PREFIX}/lib/gio/modules/giomodule.cache _install${ARTIFACTS_SUFFIX}/lib/gio/modules
    
    export GDK_PATH=`echo ${CROSSROAD_PREFIX}/lib/gdk-pixbuf-*/*/`
    GDK_PATH=$(sed "s|${CROSSROAD_PREFIX}/||g" <<< $GDK_PATH)
    cp ${CROSSROAD_PREFIX}/${GDK_PATH}loaders.cache _install${ARTIFACTS_SUFFIX}/${GDK_PATH}
    
    export GLIB_PATH=`echo ${CROSSROAD_PREFIX}/share/glib-*/schemas/`
    GLIB_PATH=$(sed "s|${CROSSROAD_PREFIX}/||g" <<< $GLIB_PATH)
    cp ${CROSSROAD_PREFIX}/${GLIB_PATH}gschemas.compiled _install${ARTIFACTS_SUFFIX}/${GLIB_PATH}
fi
