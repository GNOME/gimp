#!/bin/sh

#########################
# Script for daily cron #
#########################

#### Usage ####
if test "$#" -ne 4; then
    echo "Usage: $0 <GPKKEY> <REPO> <LOGDIR> <BUILDDIR>"
    echo
    echo "    GPKKEY: GPG Key ID to sign flatpak commits with"
    echo "    REPO: local path to flatpak repository directory"
    echo "    LOGDIR: path where logs are stored (under date folder)"
    echo "    BUILDDIR: path where build files are stored:"
    echo "      - {BUILDDIR}/{date}/build: the finale built prefix"
    echo "      - {BUILDDIR}/{date}/artifacts: the build artifacts"
    echo "      - {BUILDDIR}/.flatpak-builder: cache, etc. (reused)"
    exit 1
fi

date="`date --iso-8601=minute`"

# Get variables.
GPGKEY="$1"
REPO="$2"

LOGDIR="$3/$date/"

BUILDDIR="$4"
STATEDIR="$BUILDDIR/.flatpak-builder"
BUILDBASE="$BUILDDIR/$date/"
ARTIFACTDIR="$BUILDBASE/artifacts"
BUILDDIR="$BUILDBASE/build/"

flatpak update --user --assumeyes

# Jump to the source directory.
# We assume readlink existence.
DIR="$(dirname "$(readlink -f "$0")")"
cd $DIR

mkdir "$LOGDIR"
if test "$?" -ne "0"; then
  echo "Log directory '$LOGDIR' already exists."
  exit 1
fi

LOGFILE="${LOGDIR}/gimp-flatpak-cron.log"
echo "Flatpak cron for GIMP started at `date --rfc-2822`." > $LOGFILE

print_log() {
  echo "[GIMP-FLATPAK-CRON] (`date --rfc-2822`) $1" >> $LOGFILE
}

mkdir "$BUILDBASE"
if test "$?" -ne "0"; then
  print_log "Build directory '$BUILDBASE' already exists."
  exit 1
fi
mkdir "$ARTIFACTDIR"

flatpak_build_branch() {
  ARCH=$1
  BRANCH=$2
  FORCE=$3
  BUILDLOG="${LOGDIR}/gimp-flatpak-build-${ARCH}-${BRANCH}.log"
  INSTALLDIR="${BUILDDIR}/${ARCH}/${BRANCH}"

  mkdir -p ${INSTALLDIR}

  # Use ccache to improve build speed. Clean previous build directories.
  # Keep build dirs for later debugging if ever any build issue arised.
  BUILD_OPTIONS="--ccache --force-clean --keep-build-dirs --jobs=1"

  SKIP=""
  if test "x$FORCE" != "xyes"; then
    if test -f "$REPO/config"; then
      SKIP="--skip-if-unchanged"
    fi
  fi
  print_log "Building $BRANCH/$ARCH with options: $BUILD_OPTIONS $SKIP"
  flatpak-builder $SKIP $BUILD_OPTIONS --arch="$ARCH" \
                  --state-dir="${STATEDIR}" \
                  "${INSTALLDIR}" org.gimp.GIMP-$BRANCH.json \
                  > "${BUILDLOG}" 2>&1
  ret="$?"
  print_log "Flatpak $BRANCH/$ARCH successfully built in ${INSTALLDIR}"
  if test $ret -eq 0; then
    print_log "Exporting $BRANCH release for $ARCH"
    flatpak-builder --export-only --gpg-sign="$GPGKEY" --repo="$REPO" \
                    --state-dir="${STATEDIR}" \
                    --arch="$ARCH" "${INSTALLDIR}" \
                    org.gimp.GIMP-$BRANCH.json \
                    >> "${BUILDLOG}" 2>&1
    ret="$?"
    if test $ret -eq 0; then
      print_log "Flatpak $BRANCH/$ARCH successfully exported to ${REPO}"
    fi
  fi
  mv ${STATEDIR}/build/ "${ARTIFACTDIR}/${ARCH}"
  print_log "Detailed build logs for $BRANCH/$ARCH: ${BUILDLOG}"

  return $ret
}

#### Build flatpaks ####

for arch in "i386" "x86_64"; do
  flatpak_build_branch $arch nightly yes
done

print_log "Cron file ending at `date --rfc-2822`"
