gimpiconsdir = gimpdatadir / 'icons'

themes = [
  'Default',
  'Legacy',
  'hicolor'
]

foreach theme : themes
  # Install these always from PNG sources.
  install_icons = [
    {
      'outdir': '64x64',
      'indir':  '64',
      'suffixes': [ '.png' ],
      'files':  [ 'bitmap_64-always', ],
    },
  ]

  if have_vector_icons
    install_icons += [
      {
        'outdir': 'scalable',
        'indir':  'scalable',
        'suffixes': [ '.svg', '-symbolic.svg' ],
        'files':  [ 'scalable',
                    'color-selectors', 'controllers', 'display-filters',
                    'locks', 'prefs', 'templates', 'tools' ],
      },
    ]
  else
    source_list = {
      '12':  [ 'bitmap_12', ],
      '16':  [ 'bitmap_16',
               'color-selectors', 'controllers', 'display-filters',
               'locks', 'prefs', 'templates', 'tools' ],
      '18':  [ 'bitmap_18', ],
      '22':  [ 'bitmap_22', ],
      '24':  [ 'bitmap_24', 'templates', 'tools' ],
      '32':  [ 'bitmap_32', ],
      '48':  [ 'bitmap_48', 'prefs' ],
      '64':  [ 'bitmap_64', 'bitmap_64-system', ],
      '96':  [ 'bitmap_96', ],
      '128': [ 'bitmap_128', ],
      '192': [ 'bitmap_192', ],
      '256': [ 'bitmap_256', ],
    }

    build_icons = {
    }
    foreach size, inputs : source_list
      dup_icons = []
      foreach input : inputs
        icon_list = 'icon-lists/@0@.list'.format(input)
        py_cmd = 'print(",".join([line.strip() for line in open("@0@") if line.strip() != "" and line.strip()[0] != "#"]))'
        cmd = run_command(python, '-c',
                          py_cmd.format(icon_list),
                          check: true)
        dup_icons += cmd.stdout().strip().split(',')
      endforeach
      source_icons = []
      foreach icon : dup_icons
        if not source_icons.contains(icon)
          # remove duplicates.
          source_icons += [icon]
        endif
      endforeach

      build_icons += {size: source_icons}
    endforeach
  endif

  subdir(theme)
endforeach
