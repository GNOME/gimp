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
download_lang Hungarian.isl
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

# Copy generated language files into the source directory.
cp _build-w64/build/windows/installer/lang/*isl build/windows/installer/lang

# Construct now the installer.
MAJOR_VERSION=`grep 'm4_define(\[gimp_major_version' configure.ac |sed 's/m4_define(\[gimp_major_version.*\[\([0-9]*\)\].*/\1/'`
MINOR_VERSION=`grep 'm4_define(\[gimp_minor_version' configure.ac |sed 's/m4_define(\[gimp_minor_version.*\[\([0-9]*\)\].*/\1/'`
MICRO_VERSION=`grep 'm4_define(\[gimp_micro_version' configure.ac |sed 's/m4_define(\[gimp_micro_version.*\[\([0-9]*\)\].*/\1/'`
cd build/windows/installer
./compile.bat ${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION} ../../.. gimp-w32 gimp-w64 ../../.. gimp-w32 gimp-w64

# Test if the installer was created and return success/failure.
if [ -f "_Output/gimp-${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}-setup.exe" ]; then
  cd _Output/
  INSTALLER="gimp-${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}-setup.exe"
  sha256sum $INSTALLER > ${INSTALLER}.SHA256SUMS
  sha512sum $INSTALLER > ${INSTALLER}.SHA512SUMS
  exit 0
else
  exit 1
fi
