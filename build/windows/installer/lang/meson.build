# First we use the XML file to be able to do gettext translation
ms_installer_config_xml = i18n.merge_file(
  input : 'setup.isl.xml.in',
  output: '@BASENAME@',
  data_dirs: meson.current_source_dir(),
  po_dir: po_windows_installer_dir,
  install: false,
)

languages = [
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

addbom = find_program(meson.current_source_dir() / 'addbom.sh')

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

  nobom_setup_isl = '@0@-nobom.setup.isl'.format(lang_code)
  nobom_setup_isl_tmp = custom_target(nobom_setup_isl,
    input : [ ms_installer_config_xml, gimp_ms_installer_lang_xsl  ],
    output: nobom_setup_isl,
    command: [
      xsltproc,
      '--xinclude',
      '--output', '@OUTPUT@',
      '@INPUT1@',
      '@INPUT0@',
    ],
    build_by_default: true,
  )

  setup_isl = '@0@.setup.isl'.format(lang_code)
  # Inno-Setup absolutely requires a BOM to recognize UTF-8 files.
  # Here I am working around 3 issues in meson:
  # 1. We can't easily combine commands in meson. So either we combine
  # them in an external script, or we run several custom_target(). I do
  # a mix of both because custom_target() has too many bugs.
  # 2. To concat a BOM to the file, I'd like to simply call this Python
  # code:
  # 'open("@OUTPUT@", "wb").write(b"\\xEF\\xBB\\xBF" + open("@INPUT@", "rb").read())'
  # Unfortunately we can't pass several @INPUT@/@OUTPUT@ in a same arg.
  # See: https://github.com/mesonbuild/meson/issues/7696
  # 3. Antislashes are replaced with slashes in custom_target(). I tried
  # some weird trick encoding a string to UTF-8 instead of using the
  # well known BOM bytes directly. Unfortunately result was not
  # consistent on all platforms (at least Linux and Windows).
  # See https://github.com/mesonbuild/meson/issues/1564
  custom_target(setup_isl,
    input : [ nobom_setup_isl_tmp, 'addbom.sh' ],
    output: [ setup_isl ],
    command: [
      addbom, '@INPUT0@', '@OUTPUT@'
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

test('windows-installer-langs',
  find_program('test-installer-langs.sh'),
  env: [
    'GIMP_TESTING_ABS_TOP_SRCDIR=' + meson.source_root(),
  ],
  suite: 'build')
