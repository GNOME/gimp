
if have_vector_icons
  install_icons = [
    {
      'outdir': 'scalable',
      'indir':  'scalable',
      'suffix': '.svg',
      'files':  [ 'scalable',
                  'color-selectors', 'controllers', 'display-filters',
                  'prefs', 'templates', 'tools' ],
    },
    {
      'outdir': '64x64',
      'indir':  '64',
      'suffix': '.png',
      'files':  [ 'bitmap_64-always', ],
    },
  ]
else
  build_icons = {
    '12':  [ 'bitmap_12', ],
    '16':  [ 'bitmap_16',
             'color-selectors', 'controllers', 'display-filters',
             'prefs', 'templates', 'tools' ],
    '18':  [ 'bitmap_18', ],
    '20':  [ 'bitmap_20', ],
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

  foreach size, inputs : build_icons
    dup_icons = []
    foreach input : inputs
      icon_list = '../icon-lists/@0@.list'.format(input)
      py_cmd = 'print(",".join([line.strip() for line in open("@0@") if line.strip() != "" and line.strip()[0] != "#"]))'
      cmd = run_command('python3', '-c',
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

    subdir(size)
  endforeach

  # These are available as PNG directly.
  install_icons = [
    {
      'outdir': '64x64',
      'indir':  '64',
      'suffix': '.png',
      'files':  [ 'bitmap_64-always', ],
    },
  ]
endif

install_data('index.theme',
  install_dir: gimpiconsdir / theme,
)

foreach install : install_icons
  icons  = []

  inputs = install['files']
  outdir = install['outdir']
  indir  = install['indir']
  suffix = install['suffix']

  py_cmd = 'print(",".join(["@0@/" + line.strip() + "@1@" for line in open("@2@") if line.strip() != "" and line.strip()[0] != "#"]))'
  dup_icons = []
  foreach input : inputs
    input = '../icon-lists/@0@.list'.format(input)
    cmd = run_command('python3', '-c',
                      py_cmd.format(indir, suffix, input),
                      check: true)
    dup_icons += cmd.stdout().strip().split(',')
  endforeach
  foreach icon : dup_icons
    if not icons.contains(icon)
      # remove duplicates.
      icons += [icon]
    endif
  endforeach

  install_data(icons, install_dir: gimpiconsdir / theme / outdir / 'apps')
endforeach

icons_core = [ '64/dialog-question.png', ]
icons_imgs = [ '64/gimp-wilber-eek.png', ]

resourcename = 'gimp-core-pixbufs'
xml_content  = '<?xml version="1.0" encoding="UTF-8"?>\n'
xml_content += '<gresources>\n'
xml_content += '  <gresource prefix="/org/gimp/icons">\n'
foreach file : icons_core
  xml_content+='    <file preprocess="to-pixdata">'+ file +'</file>\n'
endforeach
xml_content += '  </gresource>\n'
xml_content += '</gresources>\n'

xml_file = configure_file(
  output: resourcename + '.gresource.xml',
  command: [ 'echo', xml_content ],
  capture: true,
)
icons_core_sources = gnome.compile_resources(
  resourcename,
  xml_file,
  c_name: resourcename.underscorify(),
)


resourcename = 'gimp-icon-pixbufs'
xml_content  = '<?xml version="1.0" encoding="UTF-8"?>\n'
xml_content += '<gresources>\n'
xml_content += '  <gresource prefix="/org/gimp/icons">\n'
foreach file : icons_imgs
  xml_content+='    <file preprocess="to-pixdata">'+ file +'</file>\n'
endforeach
xml_content += '  </gresource>\n'
xml_content += '</gresources>\n'

xml_file = configure_file(
  output: resourcename + '.gresource.xml',
  command: [ 'echo', xml_content ],
  capture: true,
)
icons_imgs_sources = gnome.compile_resources(
  resourcename,
  xml_file,
  c_name: resourcename.underscorify(),
)
