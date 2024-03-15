# First we use the XML file to be able to do gettext translation
ms_installer_config_xml = i18n.merge_file(
  input : 'setup.isl.xml.in',
  output: '@BASENAME@',
  data_dirs: meson.current_source_dir(),
  po_dir: po_windows_installer_dir,
  install: false,
)

languages = [
  { 'code': 'be',    },
  { 'code': 'bg',    },
  { 'code': 'ca',    },
  { 'code': 'cs',    },
  { 'code': 'da',    },
  { 'code': 'de',    },
  { 'code': 'el',    },
  { 'code': 'en',    'default': true, },
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
  { 'code': 'kab',   'langname': 'Taqbaylit' },
  { 'code': 'ko',    },
  { 'code': 'lt',    },
  { 'code': 'lv',    },
  { 'code': 'mr',    },
  { 'code': 'ms',    },
  { 'code': 'nl',    },
  { 'code': 'nn',    },
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
foreach language : languages
  lang_code = language.get('code')

  if 'default' in language
    lang_check = 'value[not(@xml:lang)]'
  else
    lang_check = 'value[lang(\'@0@\')]'.format(lang_code)
  endif

  gimp_ms_installer_lang_xsl = configure_file(
    input: 'gimp-ms-installer-config.xsl.in',
    output: 'gimp-ms-installer-config-@0@.xsl'.format(lang_code),
    configuration: { 'LANG_CHECK': lang_check },
  )

  setup_isl = '@0@.setup.isl'.format(lang_code)
  custom_target(setup_isl,
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

  if 'langname' in language
    # Some languages may have no default language file provided by
    # InnoSetup. When this happens, we must at least complete a very
    # basic file showing the language name, otherwise it shows as
    # "English".
    newlang_isl = '@0@.isl'.format(language.get('code'))
    newlang_conf = configuration_data()
    newlang_conf.set('LANGNAME', language.get('langname'))
    configure_file(
      input : 'newlang.isl.in',
      output : newlang_isl,
      configuration : newlang_conf
    )
  endif
endforeach

# Generate lang lists for Inno
gen_list = find_program('list-installer-langs.sh')

custom_target('base_po-cmp',
  input : [ 'iso_639_custom.xml' ],
  output: [ 'base_po-cmp.list', ],
  command: [
    gen_list, 'cmp',
  ],
  build_by_default: true,
)

custom_target('base_po-files',
  input : [ 'iso_639_custom.xml' ],
  output: [ 'base_po-files.list', ],
  command: [
    gen_list, 'files',
  ],
  build_by_default: true,
)

custom_target('base_po-msg',
  input : [ 'iso_639_custom.xml' ],
  output: [ 'base_po-msg.list', ],
  command: [
    gen_list, 'msg',
  ],
  build_by_default: true,
)

test('windows-installer-langs',
  find_program('test-installer-langs.sh'),
  env: [
    'GIMP_TESTING_ABS_TOP_SRCDIR=' + meson.project_source_root(),
  ],
  suite: 'build')
