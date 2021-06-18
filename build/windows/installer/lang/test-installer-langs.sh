#!/bin/sh

# Make sure that the languages specified in the installer match the
# translations present. This check step is necessary to not forget new
# installer translations because we have a manual step.

INSTALLER_LANGS=`grep -rI '^Name:.*MessagesFile' ${GIMP_TESTING_ABS_TOP_SRCDIR}/build/windows/installer/gimp3264.iss | \
                 sed 's/^Name: *"\([a-zA-Z_]*\)".*$/\1/' | sort`
# 'en' doesn't have a gettext file because it is the default.
INSTALLER_LANGS=`echo "$INSTALLER_LANGS" | tr '\n\r' ' ' | sed 's/\<en\> //'`

PO_LANGS=`ls ${GIMP_TESTING_ABS_TOP_SRCDIR}/po-windows-installer/*.po |sed 's%.*/po-windows-installer/\([a-zA-Z_]*\).po%\1%'`
PO_LANGS=`echo "$PO_LANGS" | tr '\n\r' ' '`

if [ "$PO_LANGS" != "$INSTALLER_LANGS" ]; then
  echo "Error: languages listed in the Windows installer script do not match the .po files in po-windows-installer/."
  echo "- PO languages:        $PO_LANGS"
  echo "- Installer languages: $INSTALLER_LANGS"
  echo "Please verify: build/windows/installer/gimp3264.iss"
  exit 1
fi

AUTOTOOLS_LANGS=`grep '[a-zA-Z_]*:\[[a-zA-Z_]*\]:[A-Z0-9-]*' ${GIMP_TESTING_ABS_TOP_SRCDIR}/build/windows/installer/lang/Makefile.am | \
                 sed 's/^\t*\([a-zA-Z_]*\):.*$/\1/' |sort`
AUTOTOOLS_LANGS=`echo "$AUTOTOOLS_LANGS" | tr '\n\r' ' ' | sed 's/\<en\> //'`

if [ "$PO_LANGS" != "$AUTOTOOLS_LANGS" ]; then
  echo "Error: languages listed in the autotools script do not match the .po files in po-windows-installer/."
  echo "- PO languages:        $PO_LANGS"
  echo "- Autotools languages: $AUTOTOOLS_LANGS"
  echo "Please verify: build/windows/installer/lang/Makefile.am"
  exit 1
fi
