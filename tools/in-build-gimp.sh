#!/bin/sh

set -e

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
