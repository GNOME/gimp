#!/bin/sh

# Make sure that the languages specified in the installer match the
# translations present. This check step is necessary to not forget new
# installer translations because we have a manual step.

MESON_LANGS=`grep "'code':" ${GIMP_TESTING_ABS_TOP_SRCDIR}/build/windows/installer/lang/meson.build | \
            sed "s/^.*'code': *'\([^']*\)'.*$/\1/" |sort`
MESON_LANGS=`echo "$MESON_LANGS" | tr '\n\r' ' ' | sed 's/\<en\> //'`

if [ "$PO_INSTALLER_LANGS" != "$MESON_LANGS" ]; then
  echo "Error: languages listed in the meson script do not match the .po files in po-windows-installer/."
  echo "- PO languages:    $PO_INSTALLER_LANGS"
  echo "- Meson languages: $MESON_LANGS"
  echo "Please verify: build/windows/installer/lang/meson.build"
  exit 1
fi
