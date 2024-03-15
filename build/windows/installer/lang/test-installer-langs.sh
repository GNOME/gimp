#!/bin/sh

# Make sure that the languages specified in the installer match the
# translations present. This check step is necessary to not forget new
# installer translations because we have a manual step.

INSTALLER_LANGS=`grep -rI '^Name:.*MessagesFile' build/windows/installer/base_po-msg.list | \
                 sed 's/^Name: *"\([a-zA-Z_]*\)".*$/\1/' | sort`
# 'en' doesn't have a gettext file because it is the default.
INSTALLER_LANGS=`echo "$INSTALLER_LANGS" | tr '\n\r' ' ' | sed 's/\<en\> //'`

PO_LANGS=`ls ${GIMP_TESTING_ABS_TOP_SRCDIR}/po-windows-installer/*.po | \
          sed 's%.*/po-windows-installer/\([a-zA-Z_]*\).po%\1%' | sort`
PO_LANGS=`echo "$PO_LANGS" | tr '\n\r' ' '`

if [ "$PO_LANGS" != "$INSTALLER_LANGS" ]; then
  echo "Error: languages listed in generated 'base_po-msg.list' do not match the .po files in po-windows-installer/."
  echo "- PO languages:        $PO_LANGS"
  echo "- Installer languages: $INSTALLER_LANGS"
  echo "Please verify: build/windows/installer/lang/iso_639_custom.xml"
  echo "You probably will need to add an 'inno_path' to the faulting lang"
  exit 1
fi


MESON_LANGS=`grep "'code':" ${GIMP_TESTING_ABS_TOP_SRCDIR}/build/windows/installer/lang/meson.build | \
            sed "s/^.*'code': *'\([^']*\)'.*$/\1/" |sort`
MESON_LANGS=`echo "$MESON_LANGS" | tr '\n\r' ' ' | sed 's/\<en\> //'`

if [ "$PO_LANGS" != "$MESON_LANGS" ]; then
  echo "Error: languages listed in the meson script do not match the .po files in po-windows-installer/."
  echo "- PO languages:    $PO_LANGS"
  echo "- Meson languages: $MESON_LANGS"
  echo "Please verify: build/windows/installer/lang/meson.build"
  exit 1
fi
