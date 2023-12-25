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
