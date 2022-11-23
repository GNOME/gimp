if not have_vector_icons
  gtk_encode_symbolic_svg = find_program('gtk-encode-symbolic-svg', native: true)

  foreach size, source_icons : build_icons
    # Note: I originally wanted to implement it here in a loop but
    # custom_target() doesn't accept an output path with a folder! So I
    # end up creating as many empty directories as generated sizes and
    # copying the same meson.build everywhere (which is really stupid,
    # let's admit it). Only the 64x64 size has a slightly different
    # meson.build as it also installs some non-symbolic images.
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

icons_core = [ '64/ligma-question.png', ]
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
# icons_core_sources = gnome.compile_resources(
#   resourcename,
#   xml_file,
#   c_name: resourcename.underscorify(),
# )


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
# icons_imgs_sources = gnome.compile_resources(
#   resourcename,
#   xml_file,
#   c_name: resourcename.underscorify(),
# )
