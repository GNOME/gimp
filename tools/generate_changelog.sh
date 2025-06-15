#!/bin/sh

srcdir="$1"
output="$2"

echo "Creating ${output} based on git log"

gitdir="${srcdir}/.git"

if [ ! -d "${gitdir}" ]; then
    echo "A git checkout and git-log is required to write changelog in ${output}." \
    | tee ${output} >&2
    exit 1
fi


CHANGELOG_START=74424325abb54620b370f2595445b2b2a19fe5e7

( \
    git log "${CHANGELOG_START}^.." --stat "${srcdir}" > temp_log.tmp
    status=$?
    cat temp_log.tmp | fmt --split-only > "${output}.tmp" | rm temp_log.tmp \
    && [ "$status" -eq 0 ] \
    && mv "${output}.tmp" "${output}" -f \
    && echo "Appending ChangeLog.pre-git" \
    && cat "${srcdir}/ChangeLog.pre-git" >> "${output}" \
    && exit 0
) \
||\
( \
    rm "${output}.tmp" -f \
    && echo "Failed to generate ChangeLog, your ChangeLog may be outdated" >&2 \
    && (test -f "${output}" \
        || echo "git-log is required to generate this file" >> "${output}") \
    && exit 1
)
