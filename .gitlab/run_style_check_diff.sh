#!/bin/sh

ancestor_horizon=28  # days (4 weeks)

echo ""
echo "This script may be wrong. You may disregard it if it conflicts with"
echo "https://developer.gimp.org/core/coding_style/"

clang-format --version

# Wrap everything in a subshell so we can propagate the exit status.
(

. .gitlab/search-common-ancestor.sh

git diff -U0 --no-color "${newest_common_ancestor_sha}" | clang-format-diff -p1 > format-diff.log
)
exit_status=$?

if [ ${exit_status} -ne 0 ]; then
  printf '\033[31m(ERROR)\033[0m: git diff or clang-format-diff pipeline failed with exit code ${exit_status}.\n'
  exit ${exit_status}
fi

format_diff="$(cat format-diff.log)"

if [ -n "${format_diff}" ]; then
  printf '\033[31m(ERROR)\033[0m: Coding Style check failed. Please make the following changes:\n'
  cat format-diff.log
  exit 1
else
  printf '(INFO): code is alright regarding being style-compliant.\n'
fi

