# Install Inno Setup.
wget https://jrsoftware.org/download.php/is.exe
./is.exe //SILENT //SUPPRESSMSGBOXES //CURRENTUSER //SP- //LOG="innosetup.log"

# Install unofficial language files. These are translations of "unknown
# translation quality or might not be maintained actively".
# Cf. https://jrsoftware.org/files/istrans/
ISCCDIR=`grep "Dest filename:.*ISCC.exe" innosetup.log | sed 's/.*Dest filename: *\|ISCC.exe//g'`
ISCCDIR=`cygpath -u "$ISCCDIR"`
mkdir -p "${ISCCDIR}/Languages/Unofficial"
cd "${ISCCDIR}/Languages/Unofficial"

download_lang ()
{
  langfile="$1"
  rm -f "$langfile"
  wget "https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/Unofficial/$langfile"
  downloaded="$?"
  if [ $downloaded -ne 0 ]; then
    echo "Download of '$langfile' failed."
    exit 1
  fi
}

download_lang_official ()
{
  langfile="$1"
  rm -f "$langfile"
  wget "https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/$langfile"
  downloaded="$?"
  if [ $downloaded -ne 0 ]; then
    echo "Download of '$langfile' failed."
    exit 1
  fi
}

add_bom ()
{
  langfile="$1"
  file "$langfile" |grep "with BOM" 2>&1 > /dev/null
  has_bom="$?"
  if [ $has_bom -ne 0 ]; then
    sed -i "1s/^/\xEF\xBB\xBF/" "$langfile"
  fi
}


download_lang Basque.isl
download_lang ChineseSimplified.isl
download_lang ChineseTraditional.isl
# Supposed to be UTF-8 yet missing BOM.
add_bom ChineseTraditional.isl
download_lang EnglishBritish.isl
download_lang Esperanto.isl
download_lang Galician.isl
download_lang Georgian.isl
download_lang Greek.isl
download_lang Indonesian.isl
download_lang Korean.isl
download_lang Latvian.isl
download_lang Lithuanian.isl
download_lang Malaysian.isl
download_lang Marathi.islu
download_lang Romanian.isl
download_lang Swedish.isl
download_lang Vietnamese.isl
cd -

# Hungarian is not in a release yet, but was moved from Unofficial
cd "${ISCCDIR}/Languages/"
download_lang_official Hungarian.isl
cd -

# Copy generated language files into the source directory.
cp _build/build/windows/installer/lang/*isl build/windows/installer/lang

# Copy generated welcome images into the source directory.
cp _build/build/windows/installer/*bmp build/windows/installer/

# Construct now the installer.
VERSION=`grep -rI '\<version *:' meson.build | head -1 | sed "s/^.*version *: *'\([0-9]\+\.[0-9]\+\.[0-9]\+\)' *,.*$/\1/"`
#MAJOR_VERSION=`echo $VERSION | sed "s/^\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)$/\1/"`
#MINOR_VERSION=`echo $VERSION | sed "s/^\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)$/\2/"`
#MICRO_VERSION=`echo $VERSION | sed "s/^\([0-9]\+\)\.\([0-9]\+\)\.\([0-9]\+\)$/\3/"`
cd build/windows/installer
./compile.bat ${VERSION} ../../.. gimp-w32 gimp-w64 ../../.. gimp-w32 gimp-w64

# Test if the installer was created and return success/failure.
if [ -f "_Output/gimp-${VERSION}-setup.exe" ]; then
  cd _Output/
  INSTALLER="gimp-${VERSION}-setup.exe"
  sha256sum $INSTALLER > ${INSTALLER}.SHA256SUMS
  sha512sum $INSTALLER > ${INSTALLER}.SHA512SUMS
  exit 0
else
  exit 1
fi
