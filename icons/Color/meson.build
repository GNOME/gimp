
if have_vector_icons
  source_icons = {
    'scalable':         [ 'scalable', 'scalable', '.svg' ],
    'bitmap_64-system': [ '64x64',    '64',       '.png' ],
    'bitmap_64-always': [ '64x64',    '64',       '.png' ],
  }
else
  sizes = [ '12', '16', '18', '20', '22', '24', '32',
            '48', '64', '96', '128', '192', '256' ]

  foreach size : sizes
    icon_list = '../icon-lists/bitmap_@0@.list'.format(size)
    cmd = run_command('python3', '-c',
                      'print(",".join([line.strip() for line in open("@0@") if line.strip() != ""]))'.format(icon_list),
                      check: true)
    source_icons = cmd.stdout().strip().split(',')

    if size == '64'
      icon_list = '../icon-lists/bitmap_64-system.list'
      cmd = run_command('python3', '-c',
                        'print(",".join([line.strip() for line in open("@0@") if line.strip() != ""]))'.format(icon_list),
                        check: true)
      source_icons += cmd.stdout().strip().split(',')
    endif

    subdir(size)
  endforeach

  # These are available as PNG directly.
  source_icons = {
    'bitmap_64-always': [ '64x64', '64', '.png' ],
  }
endif

install_data('index.theme',
  install_dir: gimpiconsdir / theme,
)

foreach source, out : source_icons
  outdir = out[0]
  indir  = out[1]
  suffix = out[2]

  icon_list = '../icon-lists/@0@.list'.format(source)
  cmd = run_command('python3', '-c',
                    'print(",".join(["@0@/" + line.strip() + "@1@" for line in open("@2@") if line.strip() != ""]))'.format(indir, suffix, icon_list),
                    check: true)
  source_icons = cmd.stdout().strip().split(',')
  install_data(source_icons, install_dir: gimpiconsdir / theme / outdir / 'apps')
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
