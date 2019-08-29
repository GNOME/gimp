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
  { 'code': 'eo',    'encoding': 'ISO-8859-3',   },
  { 'code': 'es',    },
  { 'code': 'eu',    },
  { 'code': 'fr',    },
  { 'code': 'hu',    'encoding': 'WINDOWS-1250', },
  { 'code': 'id',    'encoding': 'ISO-8859-1',   },
  { 'code': 'is',    },
  { 'code': 'it',    },
  { 'code': 'ko',    'encoding': 'EUC-KR',       },
  { 'code': 'lv',    'encoding': 'ISO-8859-13',  },
  { 'code': 'nl',    },
  { 'code': 'pl',    'encoding': 'WINDOWS-1250', },
  { 'code': 'pt_BR', },
  { 'code': 'ru',    'encoding': 'WINDOWS-1251', },
  { 'code': 'sl',    'encoding': 'WINDOWS-1250', },
  { 'code': 'sv',    },
  { 'code': 'tr',    'encoding': 'WINDOWS-1254', },
  { 'code': 'zh_CN', 'encoding': 'WINDOWS-936',  },
  { 'code': 'zh_TW', 'encoding': 'CP950',        },
]

extractlang = find_program('extractlang.sh')

all_isl = []
foreach language : languages
  lang_code    = language.get('code')
  lang_prefix  = language.get('prefix', '[@0@]'.format(code))
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
