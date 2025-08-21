#!/bin/sh

# Ensure the script work properly
case $(readlink /proc/$$/exe) in
  *bash)
    set -o posix
    ;;
esac
set -e
if [ "$0" != 'build/macos/2_build-gimp-macports.sh' ] && [ $(basename "$PWD") != 'macos' ]; then
  printf '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, call this script from the root of gimp git dir/\n'
  exit 1
elif [ $(basename "$PWD") = 'macos' ]; then
  cd ../../..
fi
if [ -z "$GITLAB_CI" ]; then
  git submodule update --init

  NON_RELOCATABLE_OPTION='-Drelocatable-bundle=no'
fi


# Install part of the deps (again)
eval "$(sed -n '/Install part/,/End of config/p' build/macos/1_build-deps-macports.sh)"

if [ "$GITLAB_CI" ]; then
  eval "$(sed -n '/deps_install\[/,/deps_install/p' build/macos/1_build-deps-macports.sh)"
fi


# Prepare env (only GIMP_PREFIX is needed for flatpak)
if [ -z "$GIMP_PREFIX" ]; then
  export GIMP_PREFIX="$PWD/../_install"
fi
eval "$(sed -n -e 's/- //' -e '/macos_environ\[/,/macos_environ/p' .gitlab-ci.yml)"


# Build GIMP only
printf "\e[0Ksection_start:`date +%s`:gimp_build[collapsed=true]\r\e[0KBuilding GIMP\n"
if [ ! -f "_build-$(uname -m)/build.ninja" ]; then
  meson setup _build-$(uname -m) -Dprefix="$GIMP_PREFIX" $NON_RELOCATABLE_OPTION \
              $PKGCONF_RELOCATABLE_OPTION $DMG_OPTION \
              -Dbuild-id=org.gimp.GIMP_official \
              -Dc_args="-I${OPT_PREFIX}/include" -Dcpp_args="-I${OPT_PREFIX}/include" -Dc_link_args="-L${OPT_PREFIX}/lib" -Dcpp_link_args="-L${OPT_PREFIX}/lib"
fi
cd _build-$(uname -m)
ninja
printf "\e[0Ksection_end:`date +%s`:gimp_build\r\e[0K\n"


# Bundle GIMP
printf "\e[0Ksection_start:`date +%s`:gimp_bundle[collapsed=true]\r\e[0K$(if ! grep -q "dmg=true" meson-logs/meson-log.txt; then echo "Installing GIMP as non-relocatable on GIMP_PREFIX"; else echo "Creating .app bundle"; fi)\n"
ninja install > ninja_install.log 2>&1 || { cat ninja_install.log; exit 1; };
cd ..
printf "\e[0Ksection_end:`date +%s`:gimp_bundle\r\e[0K\n"
