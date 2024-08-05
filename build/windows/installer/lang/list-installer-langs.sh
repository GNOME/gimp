#!/bin/bash

if [ "$1" = 'cmp' ]; then
  ## Get list of GIMP supported languages
  PO_ARRAY=($(echo $(ls ../po/*.po |
              sed -e 's|../po/||g' -e 's|.po||g' | sort) |
              tr '\n\r' ' '))

  ## Create list of lang [Components]
  for PO in "${PO_ARRAY[@]}"; do
    CMP_LINE='Name: loc\PO_CLEAN; Description: "DESC"; Types: full custom'
    # Change po
    PO_CLEAN=$(echo $PO | sed "s/@/_/g")
    CMP_LINE=$(sed "s/PO_CLEAN/$PO_CLEAN/g" <<< $CMP_LINE)
    # Change desc
    DESC=$(echo "cat //iso_639_entries/iso_639_entry[@dl_code=\"$PO\"]/@name" |
           xmllint --shell ../build/windows/installer/lang/iso_639_custom.xml |
           awk -F'[="]' '!/>/{print $(NF-1)}')
    CMP_LINE=$(sed "s/DESC/$DESC/g" <<< $CMP_LINE)
    # Create line
    NL=$'\n'
    CMP_LIST+="${CMP_LINE}${NL}"
  done
  echo "$CMP_LIST" > build/windows/installer/base_po-cmp.list
fi


if [ "$1" = 'files' ]; then
  ## Get list of GIMP supported languages
  PO_ARRAY=($(echo $(ls ../po/*.po |
              sed -e 's|../po/||g' -e 's|.po||g' | sort) |
              tr '\n\r' ' '))

  ## Create list of lang [Files]
  for PO in "${PO_ARRAY[@]}"; do
    FILES_LINE='Source: "{#GIMP_DIR32}\share\locale\PO\*"; DestDir: "{app}\share\locale\PO"; Components: loc\PO_CLEAN; Flags: recursesubdirs restartreplace uninsrestartdelete ignoreversion'
    # Change po
    PO_CLEAN=$(echo $PO | sed "s/@/_/g")
    FILES_LINE=$(sed "s/PO_CLEAN/$PO_CLEAN/g" <<< $FILES_LINE)
    FILES_LINE=$(sed "s/PO/$PO/g" <<< $FILES_LINE)
    # Create line
    NL=$'\n'
    FILES_LIST+="${FILES_LINE}${NL}"
  done
  echo "$FILES_LIST" > build/windows/installer/base_po-files.list
fi


if [ "$1" = 'msg' ]; then
  ## Get list of Inno supported languages
  PO_INNO_ARRAY=($(echo $(ls ../po-windows-installer/*.po |
                  sed -e 's|../po-windows-installer/||g' -e 's|.po||g' | sort) |
                  tr '\n\r' ' '))

  ## Create list of lang [Languages]
  if [ "$1" = 'msg' ]; then
    for PO in "${PO_INNO_ARRAY[@]}"; do
      MSG_LINE='Name: "PO"; MessagesFile: "compiler:INNO_CODE,{#ASSETS_DIR}\lang\PO.setup.isl"'
      # Change po
      MSG_LINE=$(sed "s/PO/$PO/g" <<< $MSG_LINE)
      # Change isl
      INNO_CODE=$(echo "cat //iso_639_entries/iso_639_entry[@dl_code=\"$PO\"]/@inno_code" |
                  xmllint --shell ../build/windows/installer/lang/iso_639_custom.xml |
                  awk -F'[="]' '!/>/{print $(NF-1)}')
      MSG_LINE=$(sed "s/INNO_CODE/$INNO_CODE/g" <<< $MSG_LINE)
      # Create line
      if [ "$INNO_CODE" != '' ]; then
        NL=$'\n'
        MSG_LIST+="${MSG_LINE}${NL}"
      fi
    done
    echo "$MSG_LIST" > build/windows/installer/base_po-msg.list
  fi
fi