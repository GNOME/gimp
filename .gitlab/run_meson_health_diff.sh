#!/bin/sh

. .gitlab/search-common-ancestor.sh

diff=$(git diff -U0 --no-color "${newest_common_ancestor_sha}" -- '*.build' '*.py' | grep -E '^\+[^+]' | sed 's/^+//')


#List of commonly used utilities on Unix world
#See the context: https://gitlab.gnome.org/GNOME/gimp/-/issues/11385
coreutils_list="
   \.sh \
   'sh' \
   'bash' \
   '\[' \
   'arch' \
   'awk' \
   'b2sum' \
   'base32' \
   'base64' \
   'basename' \
   'basenc' \
   'cat' \
   'chcon' \
   'chgrp' \
   'chmod' \
   'chown' \
   'chroot' \
   'cksum' \
   'cmp' \
   'comm' \
   'cp' \
   'csplit' \
   'cut' \
   'date' \
   'dd' \
   'df' \
   'diff' \
   'dir' \
   'dircolors' \
   'dirname' \
   'du' \
   'echo' \
   'env' \
   'expand' \
   'expr' \
   'factor' \
   'false' \
   'find' \
   'fmt' \
   'fold' \
   'gkill' \
   'grep' \
   'groups' \
   'head' \
   'hostid' \
   'hostname' \
   'id' \
   'install' \
   'join' \
   'link' \
   'ln' \
   'logname' \
   'ls' \
   'md5sum' \
   'mkdir' \
   'mkfifo' \
   'mknod' \
   'mktemp' \
   'mv' \
   'nice' \
   'nl' \
   'nohup' \
   'nproc' \
   'numfmt' \
   'od' \
   'paste' \
   'pathchk' \
   'pinky' \
   'pr' \
   'printenv' \
   'printf' \
   'ptx' \
   'pwd' \
   'readlink' \
   'realpath' \
   'rm' \
   'rmdir' \
   'runcon' \
   'sed' \
   'seq' \
   'sha1sum' \
   'sha224sum' \
   'sha256sum' \
   'sha384sum' \
   'sha512sum' \
   'shred' \
   'shuf' \
   'sleep' \
   'sort' \
   'split' \
   'stat' \
   'stdbuf' \
   'stty' \
   'sum' \
   'sync' \
   'tac' \
   'tail' \
   'tee' \
   'test' \
   'timeout' \
   'touch' \
   'tr' \
   'true' \
   'truncate' \
   'tsort' \
   'tty' \
   'uname' \
   'unexpand' \
   'uniq' \
   'unlink' \
   'users' \
   'vdir' \
   'wc' \
   'who' \
   'whoami' \
   'yes'
"

for coreutil in $coreutils_list; do
  if echo "$diff" | grep -q "$coreutil"; then
    found_coreutils="$(echo "$found_coreutils $coreutil" | sed 's|\\||g')"
  fi
done

if [ "$found_coreutils" ]; then
  printf '\033[31m(ERROR)\033[0m: Seems that you are trying to add an Unix-specific dependency to be called by Meson.\n'
  printf "         Please, port to Python (which is crossplatform), your use of:${found_coreutils}.\n"
fi


#Limited list of commonly used utilities on Windows world
ntutils_list="
   \.bat \
   \.cmd \
   \.ps1 \
   'cmd' \
   'powershell'
"

for ntutil in $ntutils_list; do
  if echo "$diff" | grep -q "$ntutil"; then
    found_ntutils="$(echo "$found_ntutils $ntutil" | sed 's|\\||g')"
  fi
done

if [ "$found_ntutils" ]; then
  printf '\033[31m(ERROR)\033[0m: Seems that you are trying to add a NT-specific dependency to be called by Meson.\n'
  printf "         Please, port to Python (which is crossplatform), your use of:${found_ntutils}.\n"
fi


if [ "$found_coreutils" ] || [ "$found_ntutils" ]; then
  echo "$diff"
  exit 1
fi

printf 'Meson .build files are alright regarding crossplatform.\n'
exit 0
