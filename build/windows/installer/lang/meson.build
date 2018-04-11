isl_input = custom_target('setup.isl',
  input : 'setup.isl.in',
  output: 'setup.isl',
  command: [
    intltool_merge,
    '--desktop-style',
    '--utf8',
    '--multiple-output',
    '--cache=' + join_paths('@OUTDIR@', 'intltool-merge-cache'),
    po_windows_installer_dir,
    '@INPUT@',
    '@OUTPUT@',
  ]
)

languages = [
  [ 'ca',   '[ca]',     'WINDOWS-1252', ],
  [ 'da',   '[da]',     'WINDOWS-1252', ],
  [ 'de',   '[de]',     'WINDOWS-1252', ],
  [ 'el',   '[el]',     'WINDOWS-1253', ],
  [ 'en',   '',         'WINDOWS-1252', ],
  [ 'eo',   '[eo]',     'ISO-8859-3',   ],
  [ 'es',   '[es]',     'WINDOWS-1252', ],
  [ 'eu',   '[eu]',     'WINDOWS-1252', ],
  [ 'fr',   '[fr]',     'WINDOWS-1252', ],
  [ 'hu',   '[hu]',     'WINDOWS-1250', ],
  [ 'id',   '[id]',     'ISO-8859-1',   ],
  [ 'is',   '[is]',     'WINDOWS-1252', ],
  [ 'it',   '[it]',     'WINDOWS-1252', ],
  [ 'lv',   '[lv]',     'ISO-8859-13',  ],
  [ 'nl',   '[nl]',     'WINDOWS-1252', ],
  [ 'pl',   '[pl]',     'WINDOWS-1250', ],
  [ 'pt_BR','[pt_BR]',  'WINDOWS-1252', ],
  [ 'ru',   '[ru]',     'WINDOWS-1251', ],
  [ 'sl',   '[sl]',     'WINDOWS-1250', ],
  [ 'sv',   '[sv]',     'WINDOWS-1252', ],
  [ 'tr',   '[tr]',     'WINDOWS-1254', ],
  [ 'zh_CN','[zh_CN]',  'WINDOWS-936',  ],
]

extractlang = find_program('extractlang.sh')

all_isl = []
foreach language : languages
  code    = language[0]
  prefix  = language[1]
  encoding= language[2]

  islfile = '@0@.setup.isl'.format(code)

  all_isl += custom_target(islfile,
    input : isl_input,
    output: islfile,
    command: [ extractlang, prefix, encoding, '@INPUT@' ],
    capture: true,
    build_by_default: true,
  )
endforeach
