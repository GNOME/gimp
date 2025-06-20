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

[ ${exit_status} = 0 ] || exit ${exit_status}

format_diff="$(cat format-diff.log)"

if [ -n "${format_diff}" ]; then
    cat format-diff.log
    exit 1
fi

