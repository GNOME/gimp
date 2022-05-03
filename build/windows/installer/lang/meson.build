isl_in_input = custom_target('setup.isl.in',
  input : 'setup.isl.ini',
  output: 'setup.isl.in',
  capture: true,
  command: [
    'sed', 's/^#/;/', '@INPUT@',
  ]
)

isl_input = custom_target('setup.isl',
  input : isl_in_input,
  output: 'setup.isl',
  command: [
    intltool_merge,
    '--desktop-style',
    '--utf8',
    '--multiple-output',
    '--cache=' + '@OUTDIR@' / 'intltool-merge-cache',
    po_windows_installer_dir,
    '@INPUT@',
    '@OUTPUT@',
  ]
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

extractlang = find_program('extractlang.sh')

all_isl = []
foreach language : languages
  lang_code    = language.get('code')
  lang_prefix  = language.get('prefix', '[@0@]'.format(lang_code))

  islfile = '@0@.setup.isl'.format(lang_code)

  all_isl += custom_target(islfile,
    input : isl_input,
    output: islfile,
    command: [ extractlang, lang_prefix, '@INPUT@' ],
    capture: true,
    build_by_default: true,
  )
endforeach

test('windows-installer-langs',
  find_program('test-installer-langs.sh'),
  env: [
    'GIMP_TESTING_ABS_TOP_SRCDIR=' + meson.source_root(),
  ],
  suite: 'build')
