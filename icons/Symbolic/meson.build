
install_data('index.theme',
  install_dir: join_paths(gimpiconsdir, theme)
)

foreach icon_info : all_icons
  icons_dir = icon_info[0]
  icons_list= icon_info[1]
  install_data(icons_list,
    install_dir: join_paths(gimpiconsdir, theme, icons_dir, 'apps')
  )
endforeach


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
