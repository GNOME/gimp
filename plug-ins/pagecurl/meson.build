plugin_name = 'pagecurl'

plugin_sourcecode = [
  'pagecurl.c',
]
plugin_sources = plugin_sourcecode

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
xml_content += '  <gresource prefix="/org/gimp/pagecurl-icons">\n'
foreach file : pagecurl_icons_images
  xml_content+='    <file preprocess="to-pixdata">'+ file +'</file>\n'
endforeach
xml_content += '  </gresource>\n'
xml_content += '</gresources>\n'

xml_file = configure_file(
  output: resourcename + '.gresource.xml',
  command: [ python,'-c','import sys; sys.stdout.write(sys.argv[1])',xml_content ],
  capture: true,
)

plugin_sources += gnome.compile_resources(
  resourcename,
  xml_file,
  c_name: resourcename.underscorify(),
)


if platform_windows
  plugin_sources += windows.compile_resources(
    gimp_plugins_rc,
    args: [
      '--define', 'ORIGINALFILENAME_STR="@0@"'.format(plugin_name+'.exe'),
      '--define', 'INTERNALNAME_STR="@0@"'    .format(plugin_name),
      '--define', 'TOP_SRCDIR="@0@"'          .format(meson.project_source_root()),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
endif

plugin_exe = executable(plugin_name,
                        plugin_sources,
                        dependencies: [
                          libgimpui_dep,
                          math,
                        ],
                        win_subsystem: 'windows',
                        install: true,
                        install_dir: gimpplugindir / 'plug-ins' / plugin_name)
plugin_executables += [plugin_exe.full_path()]
