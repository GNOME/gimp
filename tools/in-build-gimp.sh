#!/bin/sh

set -e

export GIMP3_DIRECTORY=$(mktemp -d ${GIMP_GLOBAL_BUILD_ROOT}/.GIMP3-build-config-XXX)
echo INFO: temporary GIMP configuration directory: $GIMP3_DIRECTORY

if [ -n "$GIMP_TEMP_UPDATE_RPATH" ]; then
  # Earlier code used to set DYLD_LIBRARY_PATH environment variable instead, but
  # it didn't work on contributor's builds because of System Integrity
  # Protection (SIP), though it did work in the CI.
  export IFS=":"
  for bin in $GIMP_TEMP_UPDATE_RPATH;
  do
    install_name_tool -add_rpath ${GIMP_GLOBAL_BUILD_ROOT}/libgimp $bin
    install_name_tool -add_rpath ${GIMP_GLOBAL_BUILD_ROOT}/libgimpbase $bin
    install_name_tool -add_rpath ${GIMP_GLOBAL_BUILD_ROOT}/libgimpcolor $bin
    install_name_tool -add_rpath ${GIMP_GLOBAL_BUILD_ROOT}/libgimpconfig $bin
    install_name_tool -add_rpath ${GIMP_GLOBAL_BUILD_ROOT}/libgimpmath $bin
    install_name_tool -add_rpath ${GIMP_GLOBAL_BUILD_ROOT}/libgimpmodule $bin
    install_name_tool -add_rpath ${GIMP_GLOBAL_BUILD_ROOT}/libgimpthumb $bin
    install_name_tool -add_rpath ${GIMP_GLOBAL_BUILD_ROOT}/libgimpwidgets $bin
  done;
  unset IFS
fi

cat /dev/stdin | $GIMP_SELF_IN_BUILD "$@"

if [ -n "$GIMP_TEMP_UPDATE_RPATH" ]; then
  export IFS=":"
  for bin in $GIMP_TEMP_UPDATE_RPATH;
  do
    install_name_tool -delete_rpath ${GIMP_GLOBAL_BUILD_ROOT}/libgimp $bin
    install_name_tool -delete_rpath ${GIMP_GLOBAL_BUILD_ROOT}/libgimpbase $bin
    install_name_tool -delete_rpath ${GIMP_GLOBAL_BUILD_ROOT}/libgimpcolor $bin
    install_name_tool -delete_rpath ${GIMP_GLOBAL_BUILD_ROOT}/libgimpconfig $bin
    install_name_tool -delete_rpath ${GIMP_GLOBAL_BUILD_ROOT}/libgimpmath $bin
    install_name_tool -delete_rpath ${GIMP_GLOBAL_BUILD_ROOT}/libgimpmodule $bin
    install_name_tool -delete_rpath ${GIMP_GLOBAL_BUILD_ROOT}/libgimpthumb $bin
    install_name_tool -delete_rpath ${GIMP_GLOBAL_BUILD_ROOT}/libgimpwidgets $bin
  done;
  unset IFS
fi

# Clean-up the temporary config directory after each usage, yet making sure we
# don't get tricked by weird redirections or anything of the sort. In particular
# we check that this is a directory with user permission, not a symlink, and
# that it's inside inside the project build's root.
if [ -n "$GIMP3_DIRECTORY" ] && [ -d "$GIMP3_DIRECTORY" ] && [ -O "$GIMP3_DIRECTORY" ]; then
  if [ -L "$GIMP3_DIRECTORY" ]; then
    echo "ERROR: \$GIMP3_DIRECTORY ($GIMP3_DIRECTORY) should not be a symlink."
    exit 1
  fi
  used_dir_prefix=$(realpath "$GIMP3_DIRECTORY")
  used_dir_prefix=${used_dir_prefix%???}
  tmpl_dir_prefix=$(realpath "$GIMP_GLOBAL_BUILD_ROOT")/.GIMP3-build-config-
  if [ "$used_dir_prefix" != "$tmpl_dir_prefix" ]; then
    echo "ERROR: \$GIMP3_DIRECTORY ($GIMP3_DIRECTORY) should be under the build directory with a specific prefix."
    echo "       \"$used_dir_prefix\" != \"$tmpl_dir_prefix\""
    exit 1
  fi
  echo INFO: Running: rm -fr --preserve-root --one-file-system \"$GIMP3_DIRECTORY\"
  if [ -n "$GIMP_TEMP_UPDATE_RPATH" ]; then
    # macOS doesn't have the additional security options.
    rm -fr "$GIMP3_DIRECTORY"
  else
    rm -fr --preserve-root --one-file-system "$GIMP3_DIRECTORY"
  fi
else
  echo "ERROR: \$GIMP3_DIRECTORY ($GIMP3_DIRECTORY) is not a directory or does not belong to the user"
  exit 1
fi
