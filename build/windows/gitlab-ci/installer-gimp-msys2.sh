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

wget https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/Unofficial/Basque.isl
wget https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/Unofficial/ChineseSimplified.isl
wget https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/Unofficial/ChineseTraditional.isl
wget https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/Unofficial/EnglishBritish.isl
wget https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/Unofficial/Esperanto.isl
wget https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/Unofficial/Greek.isl
wget https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/Unofficial/Hungarian.isl
wget https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/Unofficial/Indonesian.isl
wget https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/Unofficial/Korean.isl
wget https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/Unofficial/Latvian.isl
wget https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/Unofficial/Lithuanian.isl
wget https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/Unofficial/Malaysian.isl
wget https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/Unofficial/Marathi.islu
wget https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/Unofficial/Romanian.isl
wget https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/Unofficial/Swedish.isl
wget https://raw.githubusercontent.com/jrsoftware/issrc/main/Files/Languages/Unofficial/Vietnamese.isl
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
