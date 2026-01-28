#!/bin/sh

# Parameters
REVISION="$1"
if echo "$GIMP_CI_MACOS" | grep -q '[1-9]' && [ "$CI_PIPELINE_SOURCE" != 'schedule' ]; then
  export REVISION="$GIMP_CI_MACOS"
fi
BUILD_DIR="$2"
ARM64_BUNDLE=${3:-gimp-arm64.app}
X86_64_BUNDLE=${4:-gimp-x86_64.app}

# Ensure the script work properly
case $(readlink /proc/$$/exe) in
  *bash)
    set -o posix
    ;;
esac
set -e
if [ "$0" != 'build/macos/dmg/3_dist-gimp-apple.sh' ] && [ $(basename "$PWD") != 'dmg' ]; then
  printf '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, call this script from the root of gimp git dir\n'
  exit 1
elif [ $(basename "$PWD") = 'dmg' ]; then
  cd ../../..
fi


# 1. GET MACOS TOOLS VERSION
printf "\e[0Ksection_start:`date +%s`:mac_tlkt\r\e[0KChecking macOS tools\n"
printf "(INFO): macOS version: $(sw_vers -productVersion)\n"
printf "\e[0Ksection_end:`date +%s`:mac_tlkt\r\e[0K\n"


# 2. GLOBAL INFO
printf "\e[0Ksection_start:`date +%s`:mac_info\r\e[0KGetting DMG global info\n"
if [ "$BUILD_DIR" = '' ]; then
  export BUILD_DIR=$(find $PWD -maxdepth 1 -iname "_build*$(uname -m)" | head -n 1)
fi
if [ ! -f "$BUILD_DIR/config.h" ]; then
  printf "\033[31m(ERROR)\033[0m: config.h file not found. You can configure GIMP with meson to generate it.\n"
  exit 1
fi
eval $(sed -n 's/^#define  *\([^ ]*\)  *\(.*\) *$/export \1=\2/p' $BUILD_DIR/config.h)

# Set proper identifier and bundle name
if [ -z "$GIMP_RELEASE" ] || [ -n "$GIMP_IS_RC_GIT" ]; then
  BUNDLE_IDENTIFIER="org.gimp.gimp.internal"
  BUNDLE_NAME="GIMP (Nightly)"
elif { [ -n "$GIMP_RELEASE" ] && [ -n "$GIMP_UNSTABLE" ]; } || [ -n "$GIMP_RC_VERSION" ]; then
  BUNDLE_IDENTIFIER="org.gimp.gimp.seed"
  BUNDLE_NAME="GIMP (Beta)"
  MUTEX_SUFFIX="-${GIMP_MUTEX_VERSION}"
else
  BUNDLE_IDENTIFIER="org.gimp.gimp"
  BUNDLE_NAME="GIMP"
  MUTEX_SUFFIX="-${GIMP_MUTEX_VERSION}"
fi

## Get info about GIMP version
export CUSTOM_GIMP_VERSION=$(echo $GIMP_VERSION | tr -d '\r')
if ! echo "$REVISION" | grep -q '[1-9]'; then
  export REVISION="0"
else
  export CUSTOM_GIMP_VERSION="${GIMP_VERSION}-${REVISION}"
fi
printf "(INFO): .app namespace: ${BUNDLE_NAME} | Version: $CUSTOM_GIMP_VERSION\n"

## Autodetects what archs will be packaged
if [ ! -d "$ARM64_BUNDLE" ] && [ ! -d "$X86_64_BUNDLE" ]; then
    printf "\033[31m(ERROR)\033[0m: No bundle found. You can tweak 'build/macos/2_build-gimp-macports.sh' or configure GIMP with '-Ddmg=true' to make one.\n"
    exit 1
elif [ -d "$ARM64_BUNDLE" ] && [ ! -d "$X86_64_BUNDLE" ]; then
    printf '(INFO): Arch: arm64\n'
    supported_archs="$ARM64_BUNDLE"
elif [ ! -d "$ARM64_BUNDLE" ] && [ -d "$X86_64_BUNDLE" ]; then
    printf '(INFO): Arch: x86_64'
    supported_archs="$X86_64_BUNDLE"
elif [ -d "$ARM64_BUNDLE" ] && [ -d "$X86_64_BUNDLE" ]; then
    printf '(INFO): Arch: arm64 and x86_64'
    supported_archs="$ARM64_BUNDLE $X86_64_BUNDLE"
fi
printf "\e[0Ksection_end:`date +%s`:mac_info\r\e[0K\n"


# 3. PREPARE FILES
for APP in $supported_archs; do
export ARCH=$(echo $APP | sed -e 's|gimp-||' -e 's|./||' -e 's|.app||')
printf "\e[0Ksection_start:`date +%s`:${ARCH}_files[collapsed=true]\r\e[0KPreparing GIMP files in $ARCH .dmg\n"
## Create temporary .dmg
APPVER="GIMP $CUSTOM_GIMP_VERSION install"
hdiutil create -volname "$APPVER" -srcfolder "$APP" -ov -format UDRW "temp_$ARCH.dmg"
#(we do not use 'hdiutil attach -mountrandom' because dangling mounts would last if the script fails)
existing_dmg_mount=$(hdiutil info | grep "$APPVER" | cut -f3-)
if [ -d "$existing_dmg_mount" ]; then
  hdiutil detach "$existing_dmg_mount" -force
fi
DMG_MOUNT=$(hdiutil attach "temp_$ARCH.dmg" | grep "$APPVER" | cut -f3-)
mv -f "$DMG_MOUNT/$APP" "$DMG_MOUNT/$BUNDLE_NAME.app"

## Set revision (this does the same as '-Drevision' build option)
gimp_release="$(echo "$DMG_MOUNT/$BUNDLE_NAME.app"/Contents/Resources/gimp/*/gimp-release)"
before=$(cat "$gimp_release" | grep 'revision')
after="revision=$REVISION"
sed -i '' "s|$before|$after|" "$gimp_release"

## Minor adjustments
rm -f "$DMG_MOUNT/$BUNDLE_NAME.app/.gitignore"
printf "\e[0Ksection_end:`date +%s`:${ARCH}_files\r\e[0K\n"


# 4. CREATE ASSETS
printf "\e[0Ksection_start:`date +%s`:${ARCH}_source[collapsed=true]\r\e[0KMaking .dmg assets for $ARCH .dmg\n"
# (We test the existence of the background here (and not on section 4.2.) to avoid configuring Info.plist for nothing)
BG_PATH="$BUILD_DIR/gimp-data/images/logo/gimp-dmg.png"
if [ ! -f "$BG_PATH" ]; then
  printf "\033[31m(ERROR)\033[0m: DMG background image not found. You can tweak 'build/macos/2_build-gimp-macports.sh' or configure GIMP with '-Ddmg=true' to build it.\n"
  exit 1
fi

## 4.1 Configure Info.plist
printf '(INFO): configuring Info.plist\n'
conf_plist()
{
  sed -i '' "s|$1|$2|g" "$DMG_MOUNT/$BUNDLE_NAME.app/Contents/Info.plist"
}
### Set Identifier
conf_plist "%BUNDLE_IDENTIFIER%" "$BUNDLE_IDENTIFIER"
### Set Name (the name shown on Finder etc)
conf_plist "%BUNDLE_NAME%" "$BUNDLE_NAME"
### Set custom GIMP version
conf_plist "%GIMP_VERSION%" "$CUSTOM_GIMP_VERSION"
#### Needed to differentiate on zsh etc
conf_plist "%MUTEX_SUFFIX%" "$MUTEX_SUFFIX"
### List supported filetypes
sed -i '' "s|%FILE_TYPES%|$(tr -d '\n' < $BUILD_DIR/plug-ins/file_associations_mac.list)|g" "$DMG_MOUNT/$BUNDLE_NAME.app/Contents/Info.plist"

## 4.2 Create or copy .DS_Store to set .dmg background and icon layout
printf '(INFO): handling .DS_Store\n'
mkdir -p "$DMG_MOUNT/.background"
cp -r "$BG_PATH" "$DMG_MOUNT/.background/"
ln -s /Applications "$DMG_MOUNT/Applications"
sync
sleep 2 #avoid Finder async issues
if [ -z "$GITLAB_CI" ]; then
  osascript <<EOF
tell application "Finder"
    tell disk "$APPVER"
        open
        -- background
        set bounds of container window to {1, 1, 641, 512}
        set background picture of icon view options of container window to POSIX file "$DMG_MOUNT/.background/gimp-dmg.png"
        set toolbar visible of container window to false
        set pathbar visible of container window to false
        set statusbar visible of container window to false
        -- icons
        set current view of container window to icon view
        set arrangement of icon view options of container window to not arranged
        set icon size of icon view options of container window to 110
        set position of item "$BUNDLE_NAME.app" of container window to {192, 282}
        set position of item "Applications" of container window to {448, 282}
        set position of item ".background" of container window to {545, 0}
        set position of item ".fseventsd" of container window to {545, 0}
        update without registering applications
        close
    end tell
end tell
EOF
bsdtar -cf build/macos/dmg/DS_Store.tar.xz --xz --options compression-level=9 -C "$DMG_MOUNT" .DS_Store
printf "(INFO): Generated .DS_Store file. If it have a new layout, please commit it on 'build/macos/dmg/'.\n"
fi
bsdtar -xf build/macos/dmg/DS_Store.tar.xz -C "$DMG_MOUNT" .DS_Store

## 4.3 Set custom .VolumeIcon.icns
printf '(INFO): copying .VolumeIcon.icns\n'
cp "$BUILD_DIR/build/macos/.VolumeIcon.icns" "$DMG_MOUNT"
SetFile -c icnC "$DMG_MOUNT/.VolumeIcon.icns"
SetFile -a C "$DMG_MOUNT"
hdiutil detach "$DMG_MOUNT"
printf "\e[0Ksection_end:`date +%s`:${ARCH}_source\r\e[0K\n"


# 5. FINISH .DMG AS COMPRESSED READ-ONLY
DMG_ARTIFACT="gimp-${CUSTOM_GIMP_VERSION}-${ARCH}.dmg"
printf "\e[0Ksection_start:`date +%s`:${ARCH}_making[collapsed=true]\r\e[0KCompressing %s\n" ${DMG_ARTIFACT}
hdiutil convert "temp_$ARCH.dmg" -format ULFO -o "$DMG_ARTIFACT"
rm "temp_$ARCH.dmg"
printf "\e[0Ksection_end:`date +%s`:${ARCH}_making\r\e[0K\n"


# 6.A NOTARIZE .APP (TODO)


# 6.B GENERATE SHASUMS FOR .DMG
printf "\e[0Ksection_start:`date +%s`:${ARCH}_trust[collapsed=true]\r\e[0KChecksumming ${DMG_ARTIFACT}\n"
printf "(INFO): ${DMG_ARTIFACT} SHA-256: $(shasum -a 256 ${DMG_ARTIFACT} | cut -d ' ' -f 1)\n"
printf "(INFO): ${DMG_ARTIFACT} SHA-512: $(shasum -a 512 ${DMG_ARTIFACT} | cut -d ' ' -f 1)\n"
printf "\e[0Ksection_end:`date +%s`:${ARCH}_trust\r\e[0K\n"


if [ "$GITLAB_CI" ]; then
  output_dir='build/macos/dmg/_Output'
  mkdir -p $output_dir
  mv -f ${DMG_ARTIFACT} $output_dir
fi
done