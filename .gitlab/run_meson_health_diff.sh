#!/bin/sh

. .gitlab/search-common-ancestor.sh


# CHECK SCRIPTS RUNNED BY MESON (ALL OSes)
printf "\e[0Ksection_start:`date +%s`:nonunix_test[collapsed=false]\r\e[0KChecking for non-Unix compatibility\n"
diff=$(git diff -U0 --no-color --submodule=diff "${newest_common_ancestor_sha}" \
  | awk '
    /^diff --git a\/.*\.(build|py)/ {
      sub(/^diff --git a\//, "", $0)
      sub(/ b\/.*$/, "", $0)
      file = $0
      next
    }
    /^\+[^+]/ && file != "" {
      print file ":" substr($0, 2)
    }
  ')

## List of commonly used utilities on Unix world
## See the context: https://gitlab.gnome.org/GNOME/gimp/-/issues/11385
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
  printf "$diff\n"
  printf '\033[31m(ERROR)\033[0m: Seems that you are trying to add an Unix-specific dependency to be called by Meson.\n'
  printf "         Please, port to Python (which is crossplatform), your use of:${found_coreutils}.\n"
fi

## Limited list of commonly used utilities on Windows world
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
  printf "$diff\n"
  printf '\033[31m(ERROR)\033[0m: Seems that you are trying to add a NT-specific dependency to be called by Meson.\n'
  printf "         Please, port to Python (which is crossplatform), your use of:${found_ntutils}.\n"
fi

if [ -z "$found_coreutils" ] && [ -z "$found_ntutils" ]; then
  printf '(INFO): Meson .build and .py files are alright regarding being crossplatform.\n'
fi
printf "\e[0Ksection_end:`date +%s`:nonunix_test\r\e[0K\n"


# CHECK SCRIPTS NOT RUN BY MESON (UNIX ONLY)
# Shell scripts have potential portability issues if:
# 1) contain bash shebang or are called by bash;
# 2) contain bashisms.
printf "\e[0Ksection_start:`date +%s`:unix_test[collapsed=false]\r\e[0KChecking for Unix portability (optional)\n"
diff=$(git diff -U0 --no-color --submodule=diff "${newest_common_ancestor_sha}" \
  | awk '
    /^diff --git a\// {
      sub(/^diff --git a\//, "", $0)
      sub(/ b\/.*$/, "", $0)
      file = $0
      next
    }
    /^\+[^+]/ && file != "" {
      print file ":" substr($0, 2)
    }
  ')

## Check shebang and external call (1)
echo "$diff" | grep -E '#!\s*/usr/bin/env\s+bash|#!\s*/(usr/bin|bin)/bash(\s|$)' && found_bashism='extrinsic_bashism'
echo "$diff" | grep -E "bash\s+.*\.sh" && found_bashism='extrinsic_bashism'

## Check content with shellcheck and checkbashisms (2)
for sh_script in $(find "$CI_PROJECT_DIR" -type d -name .git -prune -o -type f \( ! -name '*.ps1' ! -name '*.c' -exec grep -lE '^#!\s*/usr/bin/env\s+(sh|bash)|^#!\s*/(usr/bin|bin)/(sh|bash)(\s|$)' {} \; -o -name '*.sh' ! -exec grep -q '^#!' {} \; -print \)); do
  shellcheck --severity=warning --shell=sh -x "$sh_script" | grep -v 'set option posix is' | grep -vE '.*http.*SC[0-9]+.*POSIX' | grep --color=always -B 2 -E 'SC[0-9]+.*POSIX' && found_bashism='intrinsic_bashism'
  checkbashisms -f $sh_script || found_bashism='intrinsic_bashism'
done

if [ "$found_bashism" ]; then
  printf '\033[33m(WARNING)\033[0m: Seems that you added a Bash-specific code (aka "bashism").\n'
  printf "           It is recommended to make it POSIX-compliant (which is portable).\n"
else
  printf '(INFO): Shell .sh files are alright regarding being portable.\n'
fi
printf "\e[0Ksection_end:`date +%s`:unix_test\r\e[0K\n"


if [ "$found_coreutils" ] || [ "$found_ntutils" ] || [ "$found_bashism" ]; then
  exit 1
fi

exit 0
