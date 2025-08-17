# 1. First we use the XML file to be able to do gettext translation
# (The 'its' subdir contains rules for XML translation extraction by 'i18n.merge_file')
subdir('its')
ms_installer_config_xml = i18n.merge_file(
  input : 'setup.isl.xml.in',
  output: '@BASENAME@',
  data_dirs: [meson.current_source_dir(), meson.current_build_dir()],
  po_dir: po_windows_installer_dir,
  install: false,
)


# 2. Then, we generate the .isl file for each language using some xsltproc magic
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

foreach language : languages
  lang_code = language.get('code')
  # Since gettext 0.23, outputted xml:lang by msgfmt conforms to W3C
  # standard. Try both with underscore and hyphens.
  web_lang_code = lang_code.replace('_', '-')
  if 'default' in language
    lang_check = 'value[not(@xml:lang)]'
  else
    lang_check = 'value[lang(\'@0@\')] | value[lang(\'@1@\')]'.format(lang_code, web_lang_code)
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


# 3. Generate lang lists for Inno
gen_list = find_program('list-installer-langs.py')

custom_target('base_po-msg',
  input : [ 'iso_639_custom.xml' ],
  output: [ 'base_po-msg.list', ],
  command: [
    gen_list, 'msg', '@SOURCE_ROOT@'
  ],
  build_by_default: true,
)

custom_target('base_po-cmp',
  input : [ 'iso_639_custom.xml' ],
  output: [ 'base_po-cmp.list', ],
  command: [
    gen_list, 'cmp', '@SOURCE_ROOT@'
  ],
  build_by_default: true,
)

custom_target('base_po-files',
  input : [ 'iso_639_custom.xml' ],
  output: [ 'base_po-files.list', ],
  command: [
    gen_list, 'files', '@SOURCE_ROOT@'
  ],
  build_by_default: true,
)
