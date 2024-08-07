# Manually patches .isl to mimic AppVerName
# https://groups.google.com/g/innosetup/c/w0sebw5YAeg

langsArray_local=($(ls *.isl*))
for langfile in "${langsArray_local[@]}"; do
  #echo "(INFO): patching $langfile with AppVer"
  before=$(cat $langfile | grep -a 'SetupWindowTitle')
  after=$(cat $langfile | grep -a 'SetupWindowTitle' | sed 's|%1|%1 AppVer|')
  sed -i "s|$before|$after|" $langfile >/dev/null 2>&1
  before=$(cat $langfile | grep -a 'UninstallAppFullTitle')
  after=$(cat $langfile | grep -a 'UninstallAppFullTitle' | sed 's|%1|%1 AppVer|')
  sed -i "s|$before|$after|" $langfile >/dev/null 2>&1
done
