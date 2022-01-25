
if have_vector_icons
  all_icons = [
    [ 'scalable', symbolic_vector_scalable ],
    [ '24x24',    symbolic_vector_24 ],
    # [ '64x64',    bitmap_64system ],
    [ '64x64',    bitmap_64_always ],
  ]

  foreach icon_info : all_icons
    icons_dir = icon_info[0]
    icons_list= icon_info[1]
    install_data(icons_list,
      install_dir: gimpiconsdir / theme / icons_dir / 'apps',
    )
endforeach
else
  gtk_encode_symbolic_svg = find_program('gtk-encode-symbolic-svg')

  sizes = [ '12', '16', '18', '20', '22', '24', '32',
            '48', '64', '96', '128', '192', '256' ]
  sizes = {
    '12':   bitmap_12,
    '16':   bitmap_16,
    '18':   bitmap_18,
    '20':   bitmap_20,
    '22':   bitmap_22,
    '24':   bitmap_24,
    '32':   bitmap_32,
    '48':   bitmap_48,
    '64':   bitmap_64 + bitmap_64_always + bitmap_64system,
    '96':   bitmap_96,
    '128':  bitmap_128,
    '192':  bitmap_192,
    '256':  bitmap_256,
  }
  foreach size, source_icons: sizes
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
  install_dir: gimpiconsdir / theme,
)

icons_core = [ '64/gimp-question.png', ]
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
# icons_core_sources = gnome.compile_resources(
#   resourcename,
#   xml_file,
#   c_name: resourcename.underscorify(),
# )


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
# icons_imgs_sources = gnome.compile_resources(
#   resourcename,
#   xml_file,
#   c_name: resourcename.underscorify(),
# )
