#!/bin/sh

# Ensure the script work properly
case $(readlink /proc/$$/exe) in
  *bash)
    set -o posix
    ;;
esac
set -e
if [ "$0" != 'build/macos/1_build-deps-macports.sh' ] && [ $(basename "$PWD") != 'macos' ]; then
  printf '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, read: https://developer.gimp.org/core/setup/build/macos/\n'
  exit 1
elif [ $(basename "$PWD") = 'macos' ]; then
  cd ../../..
fi
if [ -z "$GITLAB_CI" ]; then
  which git >/dev/null 2>&1 || { printf '\033[31m(ERROR)\033[0m: git is required for this script. Please, read: https://developer.gimp.org/core/setup/git/#source-from-git\n'; exit 1; }

  GIT_DEPTH='1'

  PARENT_DIR='/..'
fi


# Install part of the deps
if [ -z "$OPT_PREFIX" ]; then
  export OPT_PREFIX=$(which port | sed 's|/bin/port||' || brew --prefix)
  if echo "$OPT_PREFIX" | grep -q 'not found'; then
    printf '\033[31m(ERROR)\033[0m: MacPorts installation not found. Please, install it on: https://www.macports.org/install.php\n'
    exit 1
  fi
fi
if [ "$OPT_PREFIX" != '/opt/local' ] && [ "$OPT_PREFIX" != '/opt/homebrew' ]; then
  export MACOSX_DEPLOYMENT_TARGET=$(awk '/LSMinimumSystemVersion/{found=1} found && /<string>/{gsub(/.*<string>|<\/string>.*/, ""); print; exit}' build/macos/Info.plist)
  echo "macosx_deployment_target ${MACOSX_DEPLOYMENT_TARGET}" | tee -a ${OPT_PREFIX}/etc/macports/macports.conf >/dev/null 2>&1 || true
  sed -i .bak "s/^#build_arch.*/build_arch $(uname -m)/" "${OPT_PREFIX}/etc/macports/macports.conf" >/dev/null 2>&1 || true
fi #End of config

printf "\e[0Ksection_start:`date +%s`:deps_install[collapsed=true]\r\e[0KInstalling dependencies provided by $( [ -f "$OPT_PREFIX/bin/port" ] && echo MacPorts || echo Homebrew )\n"
if [ -f "$OPT_PREFIX/bin/port" ]; then
  if [ "$CI_JOB_NAME" ] && [ -d "macports-cached-$(uname -m)" ]; then
    cp -fa macports-cached-$(uname -m)/* $OPT_PREFIX/var/macports || true && rm -fr macports-cached-$(uname -m)
    eval $( [ "$OPT_PREFIX" = /opt/local ] && echo sudo ) port deactivate -fNq installed
  elif echo "$CI_JOB_NAME" | grep -q 'deps'; then
    export first_cache=true
  fi
  eval $( [ "$OPT_PREFIX" = /opt/local ] && echo sudo ) port sync -Nv
  eval $( [ "$OPT_PREFIX" = /opt/local ] && echo sudo ) port install -Nv $(grep -v '^#' build/macos/all-deps-uni.txt | sed 's/#.*//' | sed 's/|homebrew:[^ ]*//g' | tr -d '\\' | xargs)
  #eval $( [ "$OPT_PREFIX" = /opt/local ] && echo sudo ) port install -N $(grep -v '^#' build/macos/all-deps-uni.txt | sed 's/#.*//' | sed 's/|homebrew:[^ ]*//g' | tr -d '\\' | xargs) || if [ "$GITLAB_CI" ]; then eval $( [ "$OPT_PREFIX" = /opt/local ] && echo sudo ) port install -Nv $(grep -v '^#' build/macos/all-deps-uni.txt | sed 's/#.*//' | sed 's/|homebrew:[^ ]*//g' | tr -d '\\' | xargs); else false; fi
  if echo "$CI_JOB_NAME" | grep -q 'deps' && { [ "$CI_COMMIT_BRANCH" = "$CI_DEFAULT_BRANCH" ] || [ "$first_cache" ] }; then
    mkdir -p macports-cached-$(uname -m) && cp -fa $OPT_PREFIX/var/macports/* macports-cached-$(uname -m) || true
  fi
else
  brew upgrade --quiet
  brew install --quiet $(tr '\\' '\n' < build/macos/all-deps-uni.txt | sed 's/#.*//' | sed -n 's/.*|homebrew://p' | awk '{print $1}' | xargs)
  git apply -v build/macos/patches/0001-build-macos-Do-not-require-gexiv2-0.14-on-homebrew.patch || true
fi
printf "\e[0Ksection_end:`date +%s`:deps_install\r\e[0K\n"
if find "$OPT_PREFIX/lib" -maxdepth 1 -name "libX11*.dylib" | grep -q .; then printf "\033[33m(WARNING)\033[0m: X11 is installed on macOS. Please remove the wrong dependency from all-deps-uni.txt or report to $( [ -f "$OPT_PREFIX/bin/port" ] && echo MacPorts || echo Homebrew )\n"; fi


# Prepare env (only GIMP_PREFIX is needed for flatpak)
GIMP_DIR="$PWD"
cd ${GIMP_DIR}${PARENT_DIR}

if [ -z "$GIMP_PREFIX" ]; then
  export GIMP_PREFIX="$PWD/_install"
fi
eval "$(sed -n -e 's/- //' -e '/macos_environ\[/,/macos_environ/p' $GIMP_DIR/.gitlab-ci.yml)"


# Build some deps (including babl and GEGL)
self_build()
{
  dep=$(basename "$1" .git)
  printf "\e[0Ksection_start:`date +%s`:${dep}_build[collapsed=true]\r\e[0KBuilding $dep\n"
  if [ ! -d "$dep" ]; then
    if ( echo $1 | grep -q 'babl' || echo $1 | grep -q 'gegl' ) && [ "$CI_COMMIT_TAG" ]; then
      tag_branch=$(git ls-remote --tags --exit-code --refs $1 | grep -oi "$(echo "$dep" | tr '[:lower:]' '[:upper:]')_[0-9]*_[0-9]*_[0-9]*" | sort --version-sort | tail -1)
    else
      tag_branch=${2:-master}
    fi
    printf "Using tag/branch of ${dep}: ${tag_branch}\n"
    git clone --branch=$tag_branch --depth $GIT_DEPTH $1
  fi
  cd $dep
  git pull

  # Configure and build
  if [ ! -f "_build-$(uname -m)/build.ninja" ]; then
    meson setup _build-$(uname -m) -Dprefix="$GIMP_PREFIX" $PKGCONF_RELOCATABLE_OPTION $WARN_AS_ERROR_ON_CI \
                -Dbuildtype=debugoptimized \
                -Dc_args="-I${OPT_PREFIX}/include" -Dcpp_args="-I${OPT_PREFIX}/include" -Dc_link_args="-L${OPT_PREFIX}/lib" -Dcpp_link_args="-L${OPT_PREFIX}/lib"
  fi
  cd _build-$(uname -m)
  ninja
  ninja install
  cd ../..
  printf "\e[0Ksection_end:`date +%s`:${dep}_build\r\e[0K\n"
}

self_build https://gitlab.gnome.org/GNOME/babl
self_build https://gitlab.gnome.org/GNOME/gegl

cd $GIMP_DIR
