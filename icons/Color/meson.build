if have_vector_icons
  # Only process the 64x64 icons for the 2 resource icons.
  subdir('64')
else
  foreach size, source_icons : build_icons
    subdir(size)
  endforeach
endif

install_data('index.theme',
  install_dir: ligmaiconsdir / theme,
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

  install_data(icons, install_dir: ligmaiconsdir / theme / outdir / 'apps')
endforeach

icons_core = [ '64/dialog-question.png', ]
icons_imgs = [ '64/ligma-wilber-eek.png', ]

resourcename = 'ligma-core-pixbufs'
xml_content  = '<?xml version="1.0" encoding="UTF-8"?>\n'
xml_content += '<gresources>\n'
xml_content += '  <gresource prefix="/org/ligma/icons">\n'
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
  dependencies: icon_dialog_question,
  source_dir: [meson.current_build_dir()]
)


resourcename = 'ligma-icon-pixbufs'
xml_content  = '<?xml version="1.0" encoding="UTF-8"?>\n'
xml_content += '<gresources>\n'
xml_content += '  <gresource prefix="/org/ligma/icons">\n'
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
  dependencies: icon_wilber_eek,
  source_dir: [meson.current_build_dir()]
)
