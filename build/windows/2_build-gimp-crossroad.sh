#!/bin/sh

set -e


# SHELL ENV
if [ -z "$CROSSROAD_PLATFORM" ]; then

if [ -z "$GITLAB_CI" ]; then
  # Make the script work locally
  if [ "$0" != 'build/windows/2_build-gimp-crossroad.sh' ] && [ ${PWD/*\//} != 'windows' ]; then
    echo -e '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/windows/'
    exit 1
  elif [ ${PWD/*\//} = 'windows' ]; then
    cd ../..
  fi
  git submodule update --init
  PARENT_DIR='../'
fi

# FIXME: We need native/Linux gimp-console.
# https://gitlab.gnome.org/GNOME/gimp/-/issues/6393
if [ -z "$GITLAB_CI" ] && [ -z "$GIMP_PREFIX" ]; then
  export GIMP_PREFIX="$PWD/../_install"
fi
if [ ! -d '_build' ]; then
  echo -e '\033[31m(ERROR)\033[0m: Before running this script, first build GIMP natively in _build'
fi
if [ ! -d "$GIMP_PREFIX" ]; then
  echo -e "\033[31m(ERROR)\033[0m: Before running this script, first install GIMP natively in $GIMP_PREFIX"
fi
if [ ! -d '_build' ] || [ ! -d "$GIMP_PREFIX" ]; then
  echo 'Patches are very welcome: https://gitlab.gnome.org/GNOME/gimp/-/issues/6393'
  exit 1
fi
GIMP_APP_VERSION=$(grep GIMP_APP_VERSION _build/config.h | head -1 | sed 's/^.*"\([^"]*\)"$/\1/')
GIMP_CONSOLE_PATH=$PWD/${PARENT_DIR}.local/bin/gimp-console-$GIMP_APP_VERSION
echo "#!/bin/sh" > $GIMP_CONSOLE_PATH
IFS=$'\n' VAR_ARRAY=($(cat .gitlab-ci.yml | sed -n '/export PATH=/,/GI_TYPELIB_PATH}\"/p' | sed 's/    - //'))
IFS=$' \t\n'
for VAR in "${VAR_ARRAY[@]}"; do
  echo $VAR >> $GIMP_CONSOLE_PATH
done
echo "$GIMP_PREFIX/bin/gimp-console-$GIMP_APP_VERSION \"\$@\"" >> $GIMP_CONSOLE_PATH
chmod u+x $GIMP_CONSOLE_PATH

if [ "$GITLAB_CI" ]; then
  # Install crossroad deps again
  # We take code from deps script to better maintenance
  echo "$(cat build/windows/1_build-deps-crossroad.sh             |
          sed -n '/# Beginning of install/,/# End of install/p')" | bash
fi


# CROSSROAD ENV
export PATH="$PWD/${PARENT_DIR}.local/bin:$PWD/bin:$PATH"
export XDG_DATA_HOME="$PWD/${PARENT_DIR}.local/share"
crossroad w64 gimp --run="build/windows/2_build-gimp-crossroad.sh"
else

## The required packages for GIMP are taken from the result of previous script

## Prepare env (no env var is needed, all are auto set to CROSSROAD_PREFIX)
export ARTIFACTS_SUFFIX="-cross"

## Build GIMP
if [ ! -f "_build$ARTIFACTS_SUFFIX/build.ninja" ]; then
  mkdir -p _build$ARTIFACTS_SUFFIX && cd _build$ARTIFACTS_SUFFIX
  crossroad meson setup .. -Dgi-docgen=disabled                 \
                           -Djavascript=disabled -Dvala=disabled
else
  cd _build$ARTIFACTS_SUFFIX
fi
ninja
ninja install
ccache --show-stats
cd ..
fi # END OF CROSSROAD ENV
