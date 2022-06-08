# First we use the XML file to be able to do gettext translation
ms_installer_config_xml = i18n.merge_file(
  input : 'setup.isl.xml.in',
  output: '@BASENAME@',
  data_dirs: meson.current_source_dir(),
  po_dir: po_windows_installer_dir,
  install: true,
  install_dir: gimpdatadir / 'tags',
)

languages = [
  { 'code': 'ca',    },
  { 'code': 'cs',    },
  { 'code': 'da',    },
  { 'code': 'de',    },
  { 'code': 'el',    },
  { 'code': 'en',    'prefix': '', },
  { 'code': 'en_GB', },
  { 'code': 'eo',    },
  { 'code': 'es',    },
  { 'code': 'eu',    },
  { 'code': 'fi',    },
  { 'code': 'fr',    },
  { 'code': 'gl',    },
  { 'code': 'he',    },
  { 'code': 'hu',    },
  { 'code': 'id',    },
  { 'code': 'is',    },
  { 'code': 'it',    },
  { 'code': 'ja',    },
  { 'code': 'ka',    },
  { 'code': 'kab',   },
  { 'code': 'ko',    },
  { 'code': 'lt',    },
  { 'code': 'lv',    },
  { 'code': 'mr',    },
  { 'code': 'ms',    },
  { 'code': 'nl',    },
  { 'code': 'pl',    },
  { 'code': 'pt',    },
  { 'code': 'pt_BR', },
  { 'code': 'ro',    },
  { 'code': 'ru',    },
  { 'code': 'sk',    },
  { 'code': 'sl',    },
  { 'code': 'sv',    },
  { 'code': 'tr',    },
  { 'code': 'uk',    },
  { 'code': 'vi',    },
  { 'code': 'zh_CN', },
  { 'code': 'zh_TW', },
]

# Then, we generate the .isl file for each language using some xsltproc magic
all_isl = []
foreach language : languages
  lang_code = language.get('code')

  if 'prefix' in language
    lang_check = 'value[lang(\'@0@\')]'.format(lang_code)
  else
    lang_check = 'value'.format(lang_code)
  endif

  gimp_ms_installer_lang_xsl = configure_file(
    input: 'gimp-ms-installer-config.xsl.in',
    output: '@BASENAME@',
    configuration: { 'LANG_CHECK': lang_check },
  )

  setup_isl = '@0@.setup.isl'.format(lang_code)
  all_isl += custom_target(setup_isl,
    input : [ ms_installer_config_xml, gimp_ms_installer_lang_xsl  ],
    output: setup_isl,
    command: [
      xsltproc,
      '--xinclude',
      '--output', '@OUTPUT@',
      '@INPUT1@',
      '@INPUT0@',
    ],
    build_by_default: true,
  )
endforeach

test('windows-installer-langs',
  find_program('test-installer-langs.sh'),
  env: [
    'GIMP_TESTING_ABS_TOP_SRCDIR=' + meson.source_root(),
  ],
  suite: 'build')
