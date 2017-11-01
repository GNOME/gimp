plugin_name = 'pagecurl'

pagecurl_icons_images = [
  'curl0.png',
  'curl1.png',
  'curl2.png',
  'curl3.png',
  'curl4.png',
  'curl5.png',
  'curl6.png',
  'curl7.png',
]
resourcename = 'pagecurl-icons'
xml_content  = '<?xml version="1.0" encoding="UTF-8"?>\n'
xml_content += '<gresources>\n'
xml_content += '  <gresource prefix="/org/gimp/icons">\n'
foreach file : pagecurl_icons_images
  xml_content+='    <file preprocess="to-pixdata">'+ file +'</file>\n'
endforeach
xml_content += '  </gresource>\n'
xml_content += '</gresources>\n'

xml_file = configure_file(
  output: resourcename + '.gresource.xml',
  command: [ 'echo', xml_content ],
  capture: true,
)

pagecurl_icons_sources = gnome.compile_resources(
  resourcename,
  xml_file,
  c_name: resourcename.underscorify(),
)


rc_file = platform_win32 ? windows.compile_resources(
  gimp_plugins_rc,
  args: [
    '--define', 'ORIGINALFILENAME_STR="@0@"'.format(plugin_name+'.exe'),
    '--define', 'INTERNALNAME_STR="@0@"'    .format(plugin_name),
    '--define', 'TOP_SRCDIR="@0@"'          .format(meson.source_root()),
  ],
  include_directories: [
    rootInclude, appInclude,
  ],
) : []

executable(plugin_name,
  [
    'pagecurl.c',
    pagecurl_icons_sources,
    rc_file,
  ],
  include_directories: [ rootInclude, ],
  dependencies: [
    gtk2, gegl, math,
  ],
  link_with: [
    libgimp,
    libgimpbase,
    libgimpcolor,
    libgimpconfig,
    libgimpmath,
    libgimpui,
    libgimpwidgets,
  ],
  install: true,
  install_dir: join_paths(gimpplugindir, 'plug-ins'),
)
