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
#cd build/flatpak/

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

# Prepare build commands.
flatpak_build() {
  # 1/ Update and export the BaseApp only if the json changed.
  # 3/ Update the local flatpak install. This assumes that the baseapp has
  #    already been locally installed with --user.
  # 4/ Update and export the dev flatpak only if the json changed.
  # 5/ Update and export the nightly flatpak even if the json hasn't
  #    changed.
  ARCH=$1
  # Use ccache to improve build speed. Clean previous build directories.
  # Keep build dirs for later debugging if ever any build issue arised.
  BUILD_OPTIONS="--ccache --force-clean --keep-build-dirs"
  echo "[GIMP-FLATPAK-CRON] (`date`) Building BaseApp for $ARCH"
  log_title "Building BaseApp for $ARCH"
  if test -f "baseapp-tmp-repo/config"; then
    SKIP="--skip-if-unchanged"
  else
    SKIP=""
  fi
  flatpak-builder $SKIP $BUILD_OPTIONS --arch="$ARCH" \
                  "builds/gimp-flatpak-${ARCH}-baseapp" flathub/org.gimp.BaseApp.json \
                  > "logs/gimp-flatpak-build-${ARCH}-baseapp.log" 2>&1
  ret="$?"
  if test $ret -eq 0; then
    echo "[GIMP-FLATPAK-CRON] (`date`) Exporting BaseApp for $ARCH"
    log_title "Exporting BaseApp for $ARCH"
    flatpak-builder --export-only --gpg-sign="$GPGKEY" --repo="baseapp-tmp-repo" \
                    --arch="$ARCH" \
                    "builds/gimp-flatpak-${ARCH}-baseapp" flathub/org.gimp.BaseApp.json \
                    >> "logs/gimp-flatpak-build-${ARCH}-baseapp.log" 2>&1
    log_title "Installing/Updating BaseApp for $ARCH"
    echo "\t* Installing/Updating BaseApp for $ARCH"
    flatpak remote-add --user --no-gpg-verify --if-not-exists baseapp-repo baseapp-tmp-repo \
               >> "logs/gimp-flatpak-build-${ARCH}-baseapp.log" 2>&1
    flatpak install --user -y --arch="$ARCH" baseapp-repo org.gimp.BaseApp \
               >> "logs/gimp-flatpak-build-${ARCH}-baseapp.log" 2>&1
    flatpak update --user >> "logs/gimp-flatpak-build-${ARCH}-baseapp.log" 2>&1
  fi
  # 0 is success build and 42 is skipping for unchanged json.
  if test $ret -eq 0 || test $ret -eq 42; then
    echo "[GIMP-FLATPAK-CRON] (`date`) Building Dev release for $ARCH"
    log_title "Building Dev release for $ARCH"
    if test -f "$REPO/config"; then
      SKIP="--skip-if-unchanged"
    else
      SKIP=""
    fi
    flatpak-builder $SKIP $BUILD_OPTIONS --arch="$ARCH" \
                    "builds/gimp-flatpak-${ARCH}-dev" org.gimp.GIMP-dev.json \
                    > "logs/gimp-flatpak-build-$ARCH-dev.log" 2>&1
    ret="$?"
    if test $ret -eq 0; then
      log_title "Exporting Dev release for $ARCH"
      echo "[GIMP-FLATPAK-CRON] (`date`) Exporting Dev release for $ARCH"
        flatpak-builder --export-only --gpg-sign="$GPGKEY" --repo="$REPO" \
                        --arch="$ARCH" \
                        "builds/gimp-flatpak-${ARCH}-dev" org.gimp.GIMP-dev.json \
                        >> "logs/gimp-flatpak-build-$ARCH-dev.log" 2>&1
    fi
    log_title "Building Nightly release for $ARCH"
    echo "[GIMP-FLATPAK-CRON] (`date`) Building Nightly release for $ARCH"
    flatpak-builder $BUILD_OPTIONS --arch="$ARCH" \
                    "builds/gimp-flatpak-${ARCH}-nightly" org.gimp.GIMP-nightly.json \
                    > "logs/gimp-flatpak-build-${ARCH}-nightly.log" 2>&1
    ret="$?"
    if test $ret -eq 0; then
      log_title "Exporting Nightly release for $ARCH"
      echo "[GIMP-FLATPAK-CRON] (`date`) Exporting Nightly release for $ARCH"
      flatpak-builder --export-only --gpg-sign="$GPGKEY" --repo="$REPO" \
                      --arch="$ARCH" \
                      "builds/gimp-flatpak-${ARCH}-nightly" org.gimp.GIMP-nightly.json \
                      >> "logs/gimp-flatpak-build-${ARCH}-nightly.log" 2>&1
    else
      return $ret
    fi
  fi
}

#### Build flatpaks ####

## x86-64
log_title "Building x86-64 flatpak..."
echo Building x86-64 flatpak...
flatpak_build "x86_64"
x86_64_success=$?
## i386
log_title "Building i386 flatpak..."
echo "Building i386 flatpak..."
flatpak_build "i386"
i386_success=$?

#### End notification ####

end_date=`date --rfc-2822`
log_archive="gimp-flatpak-logs-`date --iso-8601`.tar.xz"
rm -f "$log_archive"
tar cf "$log_archive" logs/

message="Flatpak build started at $start_date, ended at $end_date."
message="$message\nBuild for x86-64 returned: $x86_64_success."
message="$message\nBuild for i386 returned: $i386_success."

printf "$message" |mailx -s "GIMP Flatpak build" -A "$LOGFILE" $MAIL >> $LOGFILE 2>&1
#rm -f "$log_archive"

log_title "Email sent. All done!"
