#!/bin/sh

ancestor_horizon=28  # days (4 weeks)

# We need to add a new remote for the upstream target branch, since this script
# could be running in a personal fork of the repository which has out of date
# branches.
#
# Limit the fetch to a certain date horizon to limit the amount of data we get.
# If the branch was forked from origin/main before this horizon, it should
# probably be rebased.
if ! git ls-remote --exit-code upstream >/dev/null 2>&1 ; then
    git remote add upstream https://gitlab.gnome.org/GNOME/gimp.git
fi
git fetch --shallow-since="$(date --date="${ancestor_horizon} days ago" +%Y-%m-%d)" upstream > ./fetch_upstream.log 2>&1

# Work out the newest common ancestor between the detached HEAD that this CI job
# has checked out, and the upstream target branch (which will typically be
# `upstream/main` or `upstream/glib-2-62`).
# `${CI_MERGE_REQUEST_TARGET_BRANCH_NAME}` or `${CI_MERGE_REQUEST_SOURCE_BRANCH_NAME}`
# are only defined if we’re running in a merge request pipeline,
# fall back to `${CI_DEFAULT_BRANCH}` or `${CI_COMMIT_BRANCH}` respectively
# otherwise.

# add patch-origin
git remote add patch-origin ${CI_MERGE_REQUEST_SOURCE_PROJECT_URL:-${CI_PROJECT_URL}}

source_branch="${CI_MERGE_REQUEST_SOURCE_BRANCH_NAME:-${CI_COMMIT_BRANCH}}"
git fetch --shallow-since="$(date --date="${ancestor_horizon} days ago" +%Y-%m-%d)" patch-origin "${source_branch}" > ./fetch_origin.log 2>&1

git rev-list --first-parent "upstream/${CI_MERGE_REQUEST_TARGET_BRANCH_NAME:-${CI_DEFAULT_BRANCH}}" > "temp_upstream" 2>&1
git rev-list --first-parent "patch-origin/${source_branch}" > "temp_patch-origin" 2>&1
newest_common_ancestor_sha=$(diff --old-line-format='' --new-line-format='' "temp_upstream" "temp_patch-origin" | head -n 1)
if [ -z "${newest_common_ancestor_sha}" ] || [ "${newest_common_ancestor_sha}" = '' ]; then
    echo "Couldn’t find common ancestor with upstream main branch. This typically"
    echo "happens if you branched from main a long time ago. Please update"
    echo "your clone, rebase, and re-push your branch."
    exit 1
fi
