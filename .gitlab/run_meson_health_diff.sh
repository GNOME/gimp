#!/bin/bash

source .gitlab/search-common-ancestor.sh

diff=$(git diff -U0 --no-color "${newest_common_ancestor_sha}" -- '*.build' | grep -E '^\+[^+]' | sed 's/^+//')


#List of commonly used utilities on Unix world
#See the context: https://gitlab.gnome.org/GNOME/gimp/-/issues/11385
coreutils_array=(
  ".sh"
  "'sh'"
  "'bash'"
  "'\['"
  "'arch'"
  "'awk'"
  "'b2sum'"
  "'base32'"
  "'base64'"
  "'basename'"
  "'basenc'"
  "'cat'"
  "'chcon'"
  "'chgrp'"
  "'chmod'"
  "'chown'"
  "'chroot'"
  "'cksum'"
  "'comm'"
  "'cp'"
  "'csplit'"
  "'cut'"
  "'date'"
  "'dd'"
  "'df'"
  "'dir'"
  "'dircolors'"
  "'dirname'"
  "'du'"
  "'echo'"
  "'env'"
  "'expand'"
  "'expr'"
  "'factor'"
  "'false'"
  "'find'"
  "'fmt'"
  "'fold'"
  "'gkill'"
  "'grep'"
  "'groups'"
  "'head'"
  "'hostid'"
  "'hostname'"
  "'id'"
  "'install'"
  "'join'"
  "'link'"
  "'ln'"
  "'logname'"
  "'ls'"
  "'md5sum'"
  "'mkdir'"
  "'mkfifo'"
  "'mknod'"
  "'mktemp'"
  "'mv'"
  "'nice'"
  "'nl'"
  "'nohup'"
  "'nproc'"
  "'numfmt'"
  "'od'"
  "'paste'"
  "'pathchk'"
  "'pinky'"
  "'pr'"
  "'printenv'"
  "'printf'"
  "'ptx'"
  "'pwd'"
  "'readlink'"
  "'realpath'"
  "'rm'"
  "'rmdir'"
  "'runcon'"
  "'sed'"
  "'seq'"
  "'sha1sum'"
  "'sha224sum'"
  "'sha256sum'"
  "'sha384sum'"
  "'sha512sum'"
  "'shred'"
  "'shuf'"
  "'sleep'"
  "'sort'"
  "'split'"
  "'stat'"
  "'stdbuf'"
  "'stty'"
  "'sum'"
  "'sync'"
  "'tac'"
  "'tail'"
  "'tee'"
  "'test'"
  "'timeout'"
  "'touch'"
  "'tr'"
  "'true'"
  "'truncate'"
  "'tsort'"
  "'tty'"
  "'uname'"
  "'unexpand'"
  "'uniq'"
  "'unlink'"
  "'users'"
  "'vdir'"
  "'wc'"
  "'who'"
  "'whoami'"
  "'yes'"
)

for coreutil in "${coreutils_array[@]}"; do
  if echo "$diff" | grep -q "$coreutil"; then
    found_coreutils+=" $coreutil"
  fi
done

if [ "$found_coreutils" ]; then
  echo -e '\033[31m(ERROR)\033[0m: Seems that you are trying to add an Unix-specific dependency to be called by Meson.'
  echo "         Please, port to Python (which is crossplatform), your use of:${found_coreutils}."
fi


#Limited list of commonly used utilities on Windows world
ntutils_array=(
  ".bat"
  ".cmd"
  ".ps1"
  "'cmd'"
  "'powershell'"
)

for ntutil in "${ntutils_array[@]}"; do
  if echo "$diff" | grep -q "$ntutil"; then
    found_ntutils+=" $ntutil"
  fi
done

if [ "$found_ntutils" ]; then
  echo -e '\033[31m(ERROR)\033[0m: Seems that you are trying to add a NT-specific dependency to be called by Meson.'
  echo "         Please, port to Python (which is crossplatform), your use of:${found_ntutils}."
fi


if [ "$found_coreutils" ] || [ "$found_ntutils" ]; then
  exit 1
fi

echo 'Meson .build files are alright regarding crossplatform.'
exit 0
