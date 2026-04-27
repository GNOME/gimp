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
  printf '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please, call this script from the root of gimp source dir\n'
  exit 1
elif [ $(basename "$PWD") = 'dmg' ]; then
  cd ../../..
fi


# 1. GET MACOS TOOLS VERSION
printf "\e[0Ksection_start:`date +%s`:mac_tlkt\r\e[0KChecking macOS tools\n"
printf "(INFO): macOS version: $(sw_vers -productVersion) | Python version: $(python3 --version 2>&1 | sed 's/[^0-9.]//g')\n"
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
else
  BUNDLE_IDENTIFIER="org.gimp.gimp"
  BUNDLE_NAME="GIMP"
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
for APP in $(echo "$supported_archs" | tr ' ' '\n'); do
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
### Set GIMP app version (CFBundleExecutable can not be a symlink)
conf_plist "%GIMP_APP_VERSION%" "$GIMP_APP_VERSION"
### List supported filetypes
sed -i '' "s|%FILE_TYPES%|$(tr -d '\n' < $BUILD_DIR/plug-ins/file_associations_mac.list)|g" "$DMG_MOUNT/$BUNDLE_NAME.app/Contents/Info.plist"

## 4.2 Create or copy .DS_Store to set .dmg background and icon layout
printf '(INFO): generating .DS_Store\n'
cp -r "$BG_PATH" "$DMG_MOUNT/.background.png"
ln -s /Applications "$DMG_MOUNT/Applications"
#Ported from https://github.com/salty-salty-studios/first-snow/blob/main/installer/mac/metabuilder.py
python3 -m venv ds-py3-venv
. ds-py3-venv/bin/activate
pip install ds_store biplist mac_alias
python3 <<-EOF
from datetime import datetime, timezone
import ds_store
import biplist
import mac_alias

def create_alias(volname, filename):
  volume = mac_alias.VolumeInfo(
    volname, datetime.now(timezone.utc),
    fs_type=mac_alias.ALIAS_HFS_VOLUME_SIGNATURE, disk_type=mac_alias.ALIAS_FIXED_DISK,
    attribute_flags=0, fs_id=bytes(2),
  )
  target = mac_alias.TargetInfo(
    mac_alias.ALIAS_KIND_FILE, filename,
    folder_cnid=0, cnid=0, creation_date=datetime.now(timezone.utc),
    creator_code=bytes(4), type_code=bytes(4),
    folder_name=volname, carbon_path='{}:{}'.format(volname, filename),
  )
  return mac_alias.Alias(volume=volume, target=target)

if __name__ == '__main__':
  store = ds_store.DSStore.open("$DMG_MOUNT/.DS_Store", 'w+')
  store['.']['vSrn'] = ('long', 1)
  store['.']['icvl'] = ('type', 'icnv')

  #background
  icvp_options = {
    'backgroundType': 2,
    'backgroundColorRed': 1.0,
    'backgroundColorGreen': 1.0,
    'backgroundColorBlue': 1.0,
    'backgroundImageAlias': biplist.Data(create_alias("$APPVER", ".background.png").to_bytes()),
  }
  bwsp_options = {
    'WindowBounds': '{{100, 100}, {%s, %s}}' % (640, 512),
    'ShowToolbar': False,
    'ShowPathbar': False,
    'ShowStatusBar': False,
  }

  #icons
  icvp_options.update({
    'viewOptionsVersion': 1,
    'gridSpacing': 100.0,
    'gridOffsetX': 0.0,
    'gridOffsetY': 0.0,
    'scrollPositionY': 0.0,
    'arrangeBy': 'none',
    'labelOnBottom': True,
    'showItemInfo': False,
    'showIconPreview': False,
    'iconSize': 110.0,
    'textSize': 16.0,
  })
  store["$BUNDLE_NAME.app"]['Iloc'] = (192, 282)
  store["Applications"]['Iloc'] = (448, 282)
  store['.background.png']['Iloc'] = (524, 0)
  store['.fseventsd']['Iloc'] = (524, 0)

  store['.']['icvp'] = icvp_options
  store['.']['bwsp'] = bwsp_options
  store.close()
EOF
rm -fr ds-py3-venv
#Works only interactively
#if [ -z "$GITLAB_CI" ]; then
#  sync
#  sleep 2 #avoid Finder async issues
#  osascript <<EOF
#tell application "Finder"
#    tell disk "$APPVER"
#        open
#        -- background
#        set bounds of container window to {1, 1, 641, 512}
#        set background picture of icon view options of container window to POSIX file "$DMG_MOUNT/.background.png"
#        set toolbar visible of container window to false
#        set pathbar visible of container window to false
#        set statusbar visible of container window to false
#        -- icons
#        set current view of container window to icon view
#        set arrangement of icon view options of container window to not arranged
#        set icon size of icon view options of container window to 110
#        set text size of icon view options of container window to 16
#        set position of item "$BUNDLE_NAME.app" of container window to {192, 282}
#        set position of item "Applications" of container window to {448, 282}
#        set position of item ".background.png" of container window to {545, 0}
#        set position of item ".fseventsd" of container window to {545, 0}
#        update without registering applications
#        close
#    end tell
#end tell
#EOF

## 4.3 Set custom .VolumeIcon.icns
printf '(INFO): copying .VolumeIcon.icns\n'
cp "$BUILD_DIR/build/macos/.VolumeIcon.icns" "$DMG_MOUNT"
SetFile -c icnC "$DMG_MOUNT/.VolumeIcon.icns"
SetFile -a C "$DMG_MOUNT"
printf "\e[0Ksection_end:`date +%s`:${ARCH}_source\r\e[0K\n"


# 5. FINISH .DMG AS COMPRESSED READ-ONLY
DMG_ARTIFACT="gimp-${CUSTOM_GIMP_VERSION}-${ARCH}.dmg"
printf "\e[0Ksection_start:`date +%s`:${ARCH}_making[collapsed=true]\r\e[0KCompressing %s\n" ${DMG_ARTIFACT}
if [ "$GITLAB_CI" ] && [ "$CI_COMMIT_BRANCH" = "$CI_DEFAULT_BRANCH" ]; then
  #Prepare certs to be stored on cert_container
  security delete-keychain cert_container 2>/dev/null || true
  security create-keychain -p "" cert_container
  security set-keychain-settings cert_container
  security unlock-keychain -u cert_container
  security list-keychains -s "${HOME}/Library/Keychains/cert_container-db" "${HOME}/Library/Keychains/login.keychain-db"
  mkdir -p cert_dir
  #Apple certs. See: https://www.apple.com/certificateauthority/
  curl -fsSL 'https://www.apple.com/certificateauthority/DeveloperIDG2CA.cer' > cert_dir/DeveloperIDG2CA.cer
  curl -fsSL 'https://www.apple.com/certificateauthority/AppleWWDRCAG2.cer' > cert_dir/AppleWWDRCAG2.cer
  security import cert_dir/DeveloperIDG2CA.cer -k cert_container -T /usr/bin/codesign
  security import cert_dir/AppleWWDRCAG2.cer -k cert_container -T /usr/bin/codesign
  #GIMP/GNOME cert
  echo "$osx_crt" | base64 -D > cert_dir/gnome.p12
  security import cert_dir/gnome.p12  -k cert_container -P "$osx_crt_pw" -T /usr/bin/codesign
  codesign_subject=$(security find-identity -p codesigning -v | grep "Developer ID Application" | sed 's/.*"\(.*\)"[^"]*$/\1/')
  #Finish cert_container preparation
  security set-key-partition-list -S apple-tool:,apple:,codesign: -k "" cert_container
  identity_output=$(security find-identity cert_container 2>&1)
  printf "$identity_output\n"
  if echo "$identity_output" | grep -q "CSSMERR_TP_NOT_TRUSTED" || echo "$identity_output" | grep -q "0 valid identities"; then
    exit 1
  fi
  rm -rf cert_dir

  printf '(INFO): signing lib/ (except Python.framework)\n'
  find "$DMG_MOUNT/$BUNDLE_NAME.app/Contents/lib/" \
    -type f \( \( -perm -100 -o -perm -010 -o -perm -001 \) -o -name "*.dylib" \) ! -path "*/DWARF/*" ! -path "*/.dSYM/*" ! -path "*/Python.framework/*" -print0 | xargs -0 file | grep ' Mach-O ' | awk -F ':' '{print $1}' | while read -r bin; do
    printf "(INFO): signing $bin\n"
    codesign -s "${codesign_subject}" \
      --options runtime --entitlements 'build/macos/dmg/gimp-hardening.entitlements' "$bin"
    done

  printf '(INFO): signing Python.framework\n'
  if [ "$ARCH" = 'arm64' ]; then
    PYTHON_SIGN_OPT='--launch-constraint-parent'
    PYTHON_SIGN_VAL='build/macos/dmg/python.coderequirement'
    cp build/macos/dmg/python.coderequirement build/macos/dmg/python.coderequirement.bak
    sed -i '' "s|%BUNDLE_IDENTIFIER%|$BUNDLE_IDENTIFIER|" build/macos/dmg/python.coderequirement
    sed -i '' "s|%notarization_teamid%|$notarization_teamid|" build/macos/dmg/python.coderequirement
  else
    PYTHON_SIGN_OPT='--entitlements'
    PYTHON_SIGN_VAL='build/macos/dmg/gimp-hardening.entitlements'
  fi
  find "$DMG_MOUNT/$BUNDLE_NAME.app/Contents/lib/Python.framework/Versions/${PYTHON_VERSION}/lib/" \
    -type f \( \( -perm -100 -o -perm -010 -o -perm -001 \) -o -name "*.dylib" \) -print0 | xargs -0 file | grep ' Mach-O ' | awk -F ':' '{print $1}' | while read -r bin; do
      printf "(INFO): signing $bin\n"
      codesign -s "${codesign_subject}" \
        --options runtime --entitlements 'build/macos/dmg/gimp-hardening.entitlements' "$bin"
    done
  find "$DMG_MOUNT/$BUNDLE_NAME.app/Contents/lib/Python.framework/Versions/${PYTHON_VERSION}/Resources/" \
       "$DMG_MOUNT/$BUNDLE_NAME.app/Contents/lib/Python.framework/Versions/${PYTHON_VERSION}/bin/" \
       "$DMG_MOUNT/$BUNDLE_NAME.app/Contents/lib/Python.framework/Versions/${PYTHON_VERSION}/Python" \
    -type f \( -perm -100 -o -perm -010 -o -perm -001 \) -print0 | xargs -0 file | grep ' Mach-O ' | awk -F ':' '{print $1}' | while read -r bin; do
      printf "(INFO): signing $bin\n"
      codesign -s "${codesign_subject}" \
        --options runtime --timestamp ${PYTHON_SIGN_OPT} ${PYTHON_SIGN_VAL} "$bin"
    done

  printf '(INFO): signing MacOS/ executables called by GIMP\n'
  find "$DMG_MOUNT/$BUNDLE_NAME.app/Contents/MacOS/python3" "$DMG_MOUNT/$BUNDLE_NAME.app/Contents/MacOS/xdg-email" | while read -r bin; do
    if [ -f "$bin" ]; then
      printf "(INFO): signing $bin\n"
      codesign -s "${codesign_subject}" \
        --options runtime --timestamp ${PYTHON_SIGN_OPT} ${PYTHON_SIGN_VAL} "$bin"
    fi
  done
  if [ "$ARCH" = 'arm64' ]; then
    mv -f build/macos/dmg/python.coderequirement.bak build/macos/dmg/python.coderequirement
  fi

  printf '(INFO): signing MacOS/ executables related to gegl\n'
  find "$DMG_MOUNT/$BUNDLE_NAME.app/Contents/MacOS" -type f -perm +111 \
    ! -name "gimp*" ! -name "gimp-console*" ! -name "gimp-debug-tool*" ! -name "gimp-test-clipboard*" ! -name "gimptool*" ! -name "gimp-script-fu-interpreter*" ! -name "python*" ! -name "xdg-email" | while read -r bin; do
    if [ ! -L "$bin" ]; then
      printf "(INFO): signing $bin\n"
      codesign -s "${codesign_subject}" \
        --options runtime --timestamp --entitlements "build/macos/dmg/gimp-hardening.entitlements" "$bin"
    fi
    done

  # This is required for launch-constraint-parent to work (checks parent process identifier)
  printf '(INFO): signing MacOS/ executables auxiliary to GIMP\n'
  for bin in "$DMG_MOUNT/$BUNDLE_NAME.app/Contents/MacOS/gimp-console"* "$DMG_MOUNT/$BUNDLE_NAME.app/Contents/MacOS/gimp-debug-tool"* "$DMG_MOUNT/$BUNDLE_NAME.app/Contents/MacOS/gimp-test-clipboard"* "$DMG_MOUNT/$BUNDLE_NAME.app/Contents/MacOS/gimptool"* "$DMG_MOUNT/$BUNDLE_NAME.app/Contents/MacOS/gimp-script-fu-interpreter"*; do
    if [ -f "$bin" ] && [ ! -L "$bin" ]; then
      printf "(INFO): signing $bin\n"
      codesign -s "${codesign_subject}" \
        --options runtime --timestamp --identifier $BUNDLE_IDENTIFIER --entitlements "build/macos/dmg/gimp-hardening.entitlements" "$bin"
    fi
  done

  printf '(INFO): signing .app\n'
  codesign -s "${codesign_subject}" \
    --options runtime --timestamp --entitlements "build/macos/dmg/gimp-hardening.entitlements" "$DMG_MOUNT/$BUNDLE_NAME.app"
fi
hdiutil detach "$DMG_MOUNT"
hdiutil convert -verbose "temp_$ARCH.dmg" -format ULFO -o "$DMG_ARTIFACT"
rm "temp_$ARCH.dmg"
if [ "$GITLAB_CI" ] && [ "$CI_COMMIT_BRANCH" = "$CI_DEFAULT_BRANCH" ]; then
  printf '(INFO): signing .dmg\n'
  codesign -s "${codesign_subject}" "$DMG_ARTIFACT"
fi
printf "\e[0Ksection_end:`date +%s`:${ARCH}_making\r\e[0K\n"


# 6.A NOTARIZE .DMG
if [ "$GITLAB_CI" ] && [ "$CI_COMMIT_BRANCH" = "$CI_DEFAULT_BRANCH" ]; then
  printf "\e[0Ksection_start:`date +%s`:${ARCH}_notarize[collapsed=true]\r\e[0KNotarizing ${DMG_ARTIFACT}\n"
  printf "(INFO): submitting to Apple servers\n"
  NOTARY_OUT="$(xcrun notarytool submit ${DMG_ARTIFACT} --apple-id ${notarization_login} --team-id ${notarization_teamid} --password ${notarization_password} --wait 2>&1)"
  printf "$NOTARY_OUT\n"

  # Show Request UUID
  REQUEST_UUID=$(echo "$NOTARY_OUT" | grep -oE "id: [0-9a-f-]+" | head -n 1 | awk '{print $2}')
  if [ -z "$REQUEST_UUID" ]; then
    printf "\033[31m(ERROR)\033[0m: Failed finding Request UUID in notarytool output\n"
    exit 1
  fi
  printf "(INFO): Request UUID: $REQUEST_UUID\n"

  # Show log
  NOTARY_STATUS=$(echo "$NOTARY_OUT" | grep status: | awk -F ": " '{print $NF}')
  if ! echo "$NOTARY_STATUS" | grep -q 'Accepted'; then
    printf "\033[31m(ERROR)\033[0m: Notarization failed with status: $NOTARY_STATUS. Showing log\n"
    xcrun notarytool log --apple-id "${notarization_login}" --team-id "${notarization_teamid}" --password "${notarization_password}" "$REQUEST_UUID"
    exit 1
  fi

  printf "(INFO): stapling the notarization ticket to the DMG\n"
  xcrun stapler staple -v ${DMG_ARTIFACT}
  if [ ! $? -eq 0 ]; then
    printf '\033[31m(ERROR)\033[0m: Failed to staple notarization ticket to DMG file\n'
    exit 1
  fi
  printf "\e[0Ksection_end:`date +%s`:${ARCH}_notarize\r\e[0K\n"
fi

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