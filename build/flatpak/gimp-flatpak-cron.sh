#!/bin/sh

#########################
# Script for daily cron #
#########################

#### Usage ####
if test "$#" -ne 3; then
    echo "Usage: $0 <GPKKEY> <REPO> <MAIL>"
    echo
    echo "    GPKKEY: GPG Key ID to sign flatpak commits with"
    echo "    REPO: local path to flatpak repository directory"
    echo "    MAIL: email address to send report to"
    exit 1
fi

#### Setup ####
start_date=`date --rfc-2822`

# Get variables.
GPGKEY=$1
REPO=$2
MAIL=$3

flatpak update --user

# Jump to the source directory.
# We assume readlink existence.
DIR="$(dirname "$(readlink -f "$0")")"
cd $DIR
git pull --force

mkdir -p logs
mkdir -p builds

# Delete any previous build dirs.
rm -fr .flatpak-builder/build/*
rm -fr logs/*
rm -fr builds/*

#### Init the logs ####
LOGFILE="gimp-flatpak-logs-`date --iso-8601`.log"
rm -fr $LOGFILE
echo "Flatpak cron for GIMP started at `date`." > $LOGFILE

log_title() {
  echo "[GIMP-FLATPAK-CRON] (`date`) $1" >> $LOGFILE
}

flatpak_build_baseapp() {
  ARCH=$1

  BUILD_OPTIONS="--ccache --force-clean --keep-build-dirs"
  log_title "Building BaseApp for $ARCH"
  if test -f "baseapp-tmp-repo/config"; then
    # Update and export the BaseApp only if the json changed.
    SKIP="--skip-if-unchanged"
  else
    SKIP=""
  fi
  flatpak-builder $SKIP $BUILD_OPTIONS --arch="$ARCH" \
                  "builds/gimp-flatpak-${ARCH}-baseapp" flathub/org.gimp.BaseApp.json \
                  > "logs/gimp-flatpak-build-${ARCH}-baseapp.log" 2>&1
  ret="$?"

  if test $ret -eq 0; then
    log_title "Exporting BaseApp for $ARCH"
    flatpak-builder --export-only --gpg-sign="$GPGKEY" --repo="baseapp-tmp-repo" \
                    --arch="$ARCH" \
                    "builds/gimp-flatpak-${ARCH}-baseapp" flathub/org.gimp.BaseApp.json \
                    >> "logs/gimp-flatpak-build-${ARCH}-baseapp.log" 2>&1
    ret="$?"
    log_title "Installing/Updating BaseApp for $ARCH"
    # Update as a local flatpak repository.
    flatpak remote-add --user --no-gpg-verify --if-not-exists baseapp-repo baseapp-tmp-repo \
               >> "logs/gimp-flatpak-build-${ARCH}-baseapp.log" 2>&1
    flatpak install --user -y --arch="$ARCH" baseapp-repo org.gimp.BaseApp \
               >> "logs/gimp-flatpak-build-${ARCH}-baseapp.log" 2>&1
    flatpak update --user >> "logs/gimp-flatpak-build-${ARCH}-baseapp.log" 2>&1
  fi

  # 0 is success build and 42 is skipping for unchanged json.
  return $ret
}

flatpak_build_branch() {
  ARCH=$1
  BRANCH=$2
  FORCE=$3

  # Use ccache to improve build speed. Clean previous build directories.
  # Keep build dirs for later debugging if ever any build issue arised.
  BUILD_OPTIONS="--ccache --force-clean --keep-build-dirs"

  log_title "Building $BRANCH release for $ARCH"
  SKIP=""
  if test "x$FORCE" != "xyes"; then
    if test -f "$REPO/config"; then
      SKIP="--skip-if-unchanged"
    fi
  fi
  flatpak-builder $SKIP $BUILD_OPTIONS --arch="$ARCH" \
                  "builds/gimp-flatpak-${ARCH}-$BRANCH" \
                  org.gimp.GIMP-$BRANCH.json \
                  > "logs/gimp-flatpak-build-$ARCH-$BRANCH.log" 2>&1
  ret="$?"
  if test $ret -eq 0; then
    log_title "Exporting $BRANCH release for $ARCH"
    flatpak-builder --export-only --gpg-sign="$GPGKEY" --repo="$REPO" \
                    --arch="$ARCH" \
                    "builds/gimp-flatpak-${ARCH}-$BRANCH" \
                    org.gimp.GIMP-$BRANCH.json \
                    >> "logs/gimp-flatpak-build-$ARCH-$BRANCH.log" 2>&1
    ret="$?"
  fi

  return $ret
}

flatpak_build() {
  ARCH=$1

  flatpak_build_baseapp $ARCH
  ret=$?
  if test $ret -eq 0 || test $ret -eq 42; then
    flatpak_build_branch $ARCH dev no
    flatpak_build_branch $ARCH nightly yes
  fi
}

#### Build flatpaks ####

success=""
for arch in "i386" "x86_64"; do
  log_title "Building $arch flatpak..."
  flatpak_build $arch
  success="$success\nBuild for $arch returned: $?."
done

#### End notification ####

end_date=`date --rfc-2822`
log_archive="gimp-flatpak-logs-`date --iso-8601`.tar.xz"
rm -f "$log_archive"
tar cf "$log_archive" logs/

message="Flatpak build started at $start_date, ended at $end_date.\n$success"

printf "$message" |mailx -s "GIMP Flatpak build" -A "$LOGFILE" $MAIL >> $LOGFILE 2>&1
#rm -f "$log_archive"

log_title "Email sent. All done!"
