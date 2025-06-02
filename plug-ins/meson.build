subdir('common')

if get_option('windows-installer') or get_option('ms-store')
  all_plugins_sources = []
  foreach plugin : common_plugins_list
    if plugin.get('name').startswith('file-') and plugin.get('name') != 'file-desktop-link' and plugin.get('name') != 'file-lnk'
      all_plugins_sources += [ meson.current_source_dir() / 'common' / plugin.get('name') + '.c' ]
    endif
  endforeach
endif


complex_plugins_list = [
  { 'name': 'file-bmp', },
  { 'name': 'file-dds', },
  { 'name': 'file-faxg3', },
  { 'name': 'file-fli', },
  { 'name': 'file-icns', },
  { 'name': 'file-ico', },
  { 'name': 'file-jpeg', },
  { 'name': 'file-psd', },
  { 'name': 'file-raw', },
  { 'name': 'file-sgi', },
  { 'name': 'file-tiff', },
  { 'name': 'flame', },
  { 'name': 'fractal-explorer', },
  { 'name': 'gfig', },
  { 'name': 'gimpressionist', },
  { 'name': 'gradient-flare', },
  { 'name': 'help', },
  { 'name': 'ifs-compose', },
  { 'name': 'imagemap', },
  { 'name': 'lighting', },
  { 'name': 'map-object', },
  { 'name': 'metadata', },
  { 'name': 'pagecurl', },
  { 'name': 'print', },
  { 'name': 'screenshot', },
  { 'name': 'script-fu', },
  { 'name': 'selection-to-path', }
]

if openexr.found()
  complex_plugins_list += {
    'name': 'file-exr',
  }
endif

if cfitsio_dep.found()
  complex_plugins_list += {
    'name': 'file-fits',
  }
endif

if webp_found
  complex_plugins_list += {
    'name': 'file-webp',
  }
endif

if get_option('webkit-unmaintained')
  complex_plugins_list += {
    'name': 'help-browser',
  }
endif
  
if platform_windows and host_cpu_family == 'x86'
  complex_plugins_list += {
    'name': 'twain',
  }
endif

foreach plugin : complex_plugins_list
  subdir(plugin.get('name'))
  if get_option('windows-installer') or get_option('ms-store')
    foreach sourcecode : plugin_sourcecode
      if plugin.get('name').startswith('file-')
        #.ico files are handled in a special way by installer and msix scripts
        if plugin.get('name') == 'file-ico' and platform_windows
          continue
        else
          all_plugins_sources += [ meson.current_source_dir() / plugin.get('name') / sourcecode ]
        endif
      endif
    endforeach
  endif
endforeach


subdir('python')

if get_option('windows-installer') or get_option('ms-store')
  foreach plugin : python_plugins
    if plugin.get('name').startswith('file-') 
      all_plugins_sources += [ meson.current_source_dir() / 'python' / plugin.get('name') + '.py' ]
    endif
  endforeach
endif


if get_option('windows-installer') or get_option('ms-store')
  #List of file associations dynamically created with the plugins lists above
  custom_target('file_associations',
    input : all_plugins_sources,
    output : 'file_associations.list',
    command : [
      python,
      meson.current_source_dir() / 'generate_associations.py',
      '@OUTPUT@'
    ] + all_plugins_sources,
    build_by_default: true
  )
endif
