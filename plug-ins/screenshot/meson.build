plugin_name = 'screenshot'

plugin_sourcecode = [
  'screenshot-freedesktop.c',
  'screenshot-osx.c',
  'screenshot-win32.c',
  'screenshot-x11.c',
  'screenshot.c',
]
plugin_sources = plugin_sourcecode

screenshot_icons_images = [
  'screenshot-icon.png',
]

resourcename = 'screenshot-icons'
xml_content  = '<?xml version="1.0" encoding="UTF-8"?>\n'
xml_content += '<gresources>\n'
xml_content += '  <gresource prefix="/org/gimp/screenshot-icons">\n'
foreach file : screenshot_icons_images
  xml_content+='    <file>'+ file +'</file>\n'
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
  plugin_sources += windows.compile_resources('screenshot-win32.rc')
endif

plugin_exe = executable('screenshot',
                        plugin_sources,
                        dependencies: [
                          libgimpui_dep,
                          x11, xmu, xext, xfixes,
                        ],
                        win_subsystem: 'windows',
                        install: true,
                        install_dir: gimpplugindir / 'plug-ins' / plugin_name)

# Ugly trick to add executables to plugin_custom_targets list (see root meson.build)
plugin_custom_targets += custom_target('dummy-' + plugin_name,
              input: plugin_exe,
              output: 'dummy-' + plugin_name + '.dummy',
              command: [python, '-c',
                        'open("@OUTPUT@", "w").write("Nice. I was built before calling in-build-gimp.py\\n")'],
              build_by_default: true)
plugin_executables += [plugin_exe.full_path()]
