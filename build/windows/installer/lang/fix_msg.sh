# Manually patches .isl to mimic AppVerName
# https://groups.google.com/g/innosetup/c/w0sebw5YAeg

echo "(INFO): temporarily patching $(echo $1 | sed 's|\/|\\|g') with $2"

before=$(cat "$1" | grep -a 'SetupWindowTitle')
after=$(cat "$1" | grep -a 'SetupWindowTitle' | sed "s|%1|%1 $2|")
sed -i "s|$before|$after|" "$1" >/dev/null 2>&1

before=$(cat "$1" | grep -a 'UninstallAppFullTitle')
after=$(cat "$1" | grep -a 'UninstallAppFullTitle' | sed "s|%1|%1 $2|")
sed -i "s|$before|$after|" "$1" >/dev/null 2>&1
