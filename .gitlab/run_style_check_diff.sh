#!/bin/sh

ancestor_horizon=28  # days (4 weeks)

echo ""
echo "This script may be wrong. You may disregard it if it conflicts with"
echo "https://developer.gimp.org/core/coding_style/"

clang-format --version

. .gitlab/search-common-ancestor.sh

git_diff_log='git-diff.log'
git diff -U0 --no-color "${newest_common_ancestor_sha}" > "$git_diff_log"
git_status=$?
if [ $git_status -ne 0 ]; then
  printf "\033[31m(ERROR)\033[0m: git diff failed with exit code ${git_status}:\n"
  cat "$git_diff_log"
  exit $git_status
fi

format_diff_log="format-diff.log"
clang-format-diff -p1 < "$git_diff_log" > "$format_diff_log"
format_status=$?
if [ $format_status -ne 0 ]; then
  printf "\033[31m(ERROR)\033[0m: clang-format-diff failed with exit code ${format_status}:\n"
  cat "$format_diff_log"
  exit $format_status
fi

if [ -s "$format_diff_log" ]; then
  printf '\033[31m(ERROR)\033[0m: Coding Style check failed. Please make the following changes:\n'
  cat "$format_diff_log"
  exit 1
else
  printf '(INFO): code is alright regarding being style-compliant.\n'
fi

