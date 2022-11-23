#!/bin/sh

# Make sure that the languages specified in the installer match the
# translations present. This check step is necessary to not forget new
# installer translations because we have a manual step.

INSTALLER_LANGS=`grep -rI '^Name:.*MessagesFile' ${LIGMA_TESTING_ABS_TOP_SRCDIR}/build/windows/installer/ligma3264.iss | \
                 sed 's/^Name: *"\([a-zA-Z_]*\)".*$/\1/' | sort`
# 'en' doesn't have a gettext file because it is the default.
INSTALLER_LANGS=`echo "$INSTALLER_LANGS" | tr '\n\r' ' ' | sed 's/\<en\> //'`

PO_LANGS=`ls ${LIGMA_TESTING_ABS_TOP_SRCDIR}/po-windows-installer/*.po | \
          sed 's%.*/po-windows-installer/\([a-zA-Z_]*\).po%\1%' | sort`
PO_LANGS=`echo "$PO_LANGS" | tr '\n\r' ' '`

if [ "$PO_LANGS" != "$INSTALLER_LANGS" ]; then
  echo "Error: languages listed in the Windows installer script do not match the .po files in po-windows-installer/."
  echo "- PO languages:        $PO_LANGS"
  echo "- Installer languages: $INSTALLER_LANGS"
  echo "Please verify: build/windows/installer/ligma3264.iss"
  echo "Base language files can be found in: https://github.com/jrsoftware/issrc/tree/main/Files/Languages"
  echo "If a new language is in Unofficial/, also edit/download it from:"
  echo "build/windows/gitlab-ci/installer-ligma-msys2.sh"
  exit 1
fi

AUTOTOOLS_LANGS=`grep '^\s*[a-zA-Z_]*:[a-zA-Z_]*\s*\\\\\?' ${LIGMA_TESTING_ABS_TOP_SRCDIR}/build/windows/installer/lang/Makefile.am | \
                 sed 's/^\t*\([a-zA-Z_]*\):.*$/\1/' |sort`
AUTOTOOLS_LANGS=`echo "$AUTOTOOLS_LANGS" | tr '\n\r' ' ' | sed 's/\<en\> //'`

if [ "$PO_LANGS" != "$AUTOTOOLS_LANGS" ]; then
  echo "Error: languages listed in the autotools script do not match the .po files in po-windows-installer/."
  echo "- PO languages:        $PO_LANGS"
  echo "- Autotools languages: $AUTOTOOLS_LANGS"
  echo "Please verify: build/windows/installer/lang/Makefile.am"
  exit 1
fi

MESON_LANGS=`grep "'code':" ${LIGMA_TESTING_ABS_TOP_SRCDIR}/build/windows/installer/lang/meson.build | \
            sed "s/^.*'code': *'\([^']*\)'.*$/\1/" |sort`
MESON_LANGS=`echo "$MESON_LANGS" | tr '\n\r' ' ' | sed 's/\<en\> //'`

if [ "$PO_LANGS" != "$MESON_LANGS" ]; then
  echo "Error: languages listed in the meson script do not match the .po files in po-windows-installer/."
  echo "- PO languages:    $PO_LANGS"
  echo "- Meson languages: $MESON_LANGS"
  echo "Please verify: build/windows/installer/lang/meson.build"
  exit 1
fi

INSTALLER_LANGS=`grep -rI '^Name:.*MessagesFile.*Unofficial' ${LIGMA_TESTING_ABS_TOP_SRCDIR}/build/windows/installer/ligma3264.iss | \
                 sed 's/^.*Unofficial\\\\\([a-zA-Z_.]*\),.*$/\1/' | sort`
INSTALLER_LANGS=`echo "$INSTALLER_LANGS" | tr '\n\r' ' '`

PULLED_UNOFFICIAL=`grep '^download_lang [^(]' ${LIGMA_TESTING_ABS_TOP_SRCDIR}/build/windows/gitlab-ci/installer-ligma-msys2.sh | \
                   sed 's$^download_lang \([^.]*.isl.\?\)$\1$' | sort`
PULLED_UNOFFICIAL=`echo "$PULLED_UNOFFICIAL" | tr '\n\r' ' '`

if [ "$INSTALLER_LANGS" = "$PULLED_UNOFFICIAL" ]; then
  exit 0
else
  echo "Error: unofficial languages listed in the Windows installer script do not match the pulled InnoSetup files."
  echo "- Pulled files:        $PULLED_UNOFFICIAL"
  echo "- Installer languages: $INSTALLER_LANGS"
  echo "Please verify: build/windows/gitlab-ci/installer-ligma-msys2.sh"
  exit 1
fi
