
if have_vector_icons
  all_icons = [
    [ 'scalable', color_vector_scalable ],
    [ '24x24',    color_vector_24 ],
    [ '64x64',    bitmap_64_always + bitmap_64system ],
  ]
else
  all_icons = [
    [ '12x12',    bitmap_12 ],
    [ '16x16',    bitmap_16 ],
    [ '18x18',    bitmap_18 ],
    [ '20x20',    bitmap_20 ],
    [ '22x22',    bitmap_22 ],
    [ '24x24',    bitmap_24 ],
    [ '32x32',    bitmap_32 ],
    [ '48x48',    bitmap_48 ],
    [ '64x64',    bitmap_64 + bitmap_64_always + bitmap_64system ],
    [ '96x96',    bitmap_96 ],
    [ '128x128',  bitmap_128 ],
    [ '192x192',  bitmap_192 ],
    [ '256x256',  bitmap_256 ],
  ]
endif


install_data('index.theme',
  install_dir: gimpiconsdir / theme,
)

foreach icon_info : all_icons
  icons_dir = icon_info[0]
  icons_list= icon_info[1]
  install_data(icons_list,
    install_dir: gimpiconsdir / theme / icons_dir / 'apps',
  )
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
