isl_input = custom_target('setup.isl',
  input : 'setup.isl.in',
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
  { 'code': 'cs',    'encoding': 'WINDOWS-1250', },
  { 'code': 'da',    },
  { 'code': 'de',    },
  { 'code': 'el',    'encoding': 'WINDOWS-1253', },
  { 'code': 'en',    'prefix': '', },
  { 'code': 'en_GB', 'encoding': 'UTF-8',        },
  { 'code': 'eo',    'encoding': 'ISO-8859-3',   },
  { 'code': 'es',    },
  { 'code': 'eu',    },
  { 'code': 'fi',    },
  { 'code': 'fr',    },
  { 'code': 'he',    'encoding': 'WINDOWS-1255', },
  { 'code': 'hu',    'encoding': 'WINDOWS-1250', },
  { 'code': 'id',    'encoding': 'ISO-8859-1',   },
  { 'code': 'is',    },
  { 'code': 'it',    },
  { 'code': 'ja',    'encoding': 'CP932',        },
  { 'code': 'kab',   'encoding': 'UTF-8',        },
  { 'code': 'ko',    'encoding': 'EUC-KR',       },
  { 'code': 'lt',    'encoding': 'ISO-8859-13',  },
  { 'code': 'lv',    'encoding': 'ISO-8859-13',  },
  { 'code': 'mr',    'encoding': 'UTF-8',  },
  { 'code': 'ms',    'encoding': 'UTF-8',  },
  { 'code': 'nl',    },
  { 'code': 'pl',    'encoding': 'WINDOWS-1250', },
  { 'code': 'pt', },
  { 'code': 'pt_BR', },
  { 'code': 'ro',    'encoding': 'WINDOWS-1250', },
  { 'code': 'ru',    'encoding': 'WINDOWS-1251', },
  { 'code': 'sk',    'encoding': 'WINDOWS-1250', },
  { 'code': 'sl',    'encoding': 'WINDOWS-1250', },
  { 'code': 'sv',    },
  { 'code': 'tr',    'encoding': 'WINDOWS-1254', },
  { 'code': 'uk',    'encoding': 'WINDOWS-1251', },
  { 'code': 'vi',    'encoding': 'WINDOWS-1258', },
  { 'code': 'zh_CN', 'encoding': 'WINDOWS-936',  },
  { 'code': 'zh_TW', 'encoding': 'CP950',        },
]

extractlang = find_program('extractlang.sh')

all_isl = []
foreach language : languages
  lang_code    = language.get('code')
  lang_prefix  = language.get('prefix', '[@0@]'.format(lang_code))
  lang_encoding= language.get('encoding', 'WINDOWS-1252')

  islfile = '@0@.setup.isl'.format(lang_code)

  all_isl += custom_target(islfile,
    input : isl_input,
    output: islfile,
    command: [ extractlang, lang_prefix, lang_encoding, '@INPUT@' ],
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
