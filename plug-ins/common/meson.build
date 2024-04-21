common_plugins_list = [
  { 'name': 'align-layers', },
  { 'name': 'animation-optimize', },
  { 'name': 'animation-play', },
  { 'name': 'blinds', },
  { 'name': 'border-average', },
  { 'name': 'busy-dialog', },
  { 'name': 'checkerboard', },
  { 'name': 'cml-explorer', },
  { 'name': 'colormap-remap', },
  { 'name': 'compose', },
  { 'name': 'contrast-retinex', },
  { 'name': 'crop-zealous',
    'deps': [ gtk3, gegl, gdk_pixbuf, cairo, ],
  },
  { 'name': 'curve-bend', },
  { 'name': 'decompose', },
  { 'name': 'depth-merge', },
  { 'name': 'despeckle', },
  { 'name': 'destripe', },
  { 'name': 'file-cel', },
  { 'name': 'file-compressor',
    'deps': [ gegl, gdk_pixbuf, cairo, gio, liblzma, pango, bz2, zlib, ],
  },
  { 'name': 'file-csource', },
  { 'name': 'file-desktop-link',
    'deps': [ gtk3, gegl, gdk_pixbuf, cairo, ],
  },
  { 'name': 'file-dicom', },
  { 'name': 'file-farbfeld', },
  { 'name': 'file-gbr', },
  { 'name': 'file-gegl', },
  { 'name': 'file-gif-load',
    'deps': [ gtk3, gegl, gdk_pixbuf, cairo, ],
  },
  { 'name': 'file-gif-export', },
  { 'name': 'file-gih', },
  { 'name': 'file-glob',
    'deps': [ gtk3, gegl, gdk_pixbuf, cairo,  ],
  },
  { 'name': 'file-header', },
  { 'name': 'file-html-table', },
  { 'name': 'file-pat', },
  { 'name': 'file-pcx', },
  { 'name': 'file-pdf-load',
    'deps': [ gtk3, gegl, gdk_pixbuf, poppler ],
  },
  { 'name': 'file-pix', },
  { 'name': 'file-png',
    'deps': [ gtk3, gegl, libpng, lcms ],
  },
  { 'name': 'file-pnm', },
  { 'name': 'file-psp',
    'deps': [ gtk3, gegl, zlib, ],
  },
  { 'name': 'file-raw-data', },
  { 'name': 'file-sunras', },
  { 'name': 'file-svg',
    'deps': [ gtk3, gegl, rsvg, ],
  },
  { 'name': 'file-tga', },
  { 'name': 'file-wbmp', },
  { 'name': 'file-xbm', },
  { 'name': 'file-xwd', },
  { 'name': 'film', },
  { 'name': 'gradient-map', },
  { 'name': 'grid', },
  { 'name': 'guillotine', },
  { 'name': 'hot', },
  { 'name': 'jigsaw', },
  { 'name': 'nl-filter', },
  { 'name': 'plugin-browser', },
  { 'name': 'procedure-browser', },
  { 'name': 'qbist', },
  { 'name': 'sample-colorize', },
  { 'name': 'smooth-palette', },
  { 'name': 'sparkle', },
  { 'name': 'sphere-designer', },
  { 'name': 'tile-small', },
  { 'name': 'tile', },
  { 'name': 'unit-editor', },
  { 'name': 'van-gogh-lic', },
  { 'name': 'warp', },
  { 'name': 'wavelet-decompose', },
  { 'name': 'web-browser', },
]

if libaa.found()
  common_plugins_list += {
    'name': 'file-aa',
    'deps': [ gtk3, gegl, gdk_pixbuf, libaa, ],
  }
endif

if have_heif
  common_plugins_list += { 'name': 'file-heif',
    'deps': [ gtk3, gegl, libheif, gexiv2, lcms, ],
  }
endif

if openjpeg.found()
  common_plugins_list += { 'name': 'file-jp2-load',
    'deps': [ gtk3, gegl, gdk_pixbuf, openjpeg, ],
  }
endif

if libjxl.found() and libjxl_threads.found()
  common_plugins_list += {
    'name': 'file-jpegxl',
    'deps': [ gtk3, gegl, gexiv2, libjxl, libjxl_threads, ],
  }
endif

if libmng.found()
  mng_cflags = []
  if platform_windows
    mng_cflags = [ '-DMNG_USE_DLL' ]
  endif
  common_plugins_list += { 'name': 'file-mng',
                           'deps': [ gtk3, gegl, libmng, libpng, ],
                           'cflags': mng_cflags,
  }
endif

if cairopdf.found()
  common_plugins_list += { 'name': 'file-pdf-export',
    'deps': [ gtk3, gegl, gdk_pixbuf, poppler, cairo, cairopdf ],
  }
endif

if ghostscript.found()
  common_plugins_list += { 'name': 'file-ps',
    'deps': [ gtk3, gegl, gdk_pixbuf, ghostscript, ],
  }
endif

if wmf.found()
  common_plugins_list += { 'name': 'file-wmf',
    'deps': [ gtk3, gegl, wmf, ],
  }
endif

if xmc.found()
  common_plugins_list += { 'name': 'file-xmc',
    'deps': [ gtk3, gegl, xmc, ],
  }
endif

if libxpm.found()
  common_plugins_list += { 'name': 'file-xpm',
    'deps': [ gtk3, gegl, libxpm, ],
  }
endif

if have_qoi
  common_plugins_list += {
    'name': 'file-qoi',
    'deps': [ gtk3, gegl, ],
  }
endif

if libiff.found() and libilbm.found()
  common_plugins_list += {
    'name': 'file-iff',
    'deps': [ gtk3, gegl, libiff, libilbm, ],
  }
elif have_ilbm
  common_plugins_list += {
    'name': 'file-iff',
    'deps': [ gtk3, gegl, ],
  }
endif

if not platform_windows
  common_plugins_list += { 'name': 'mail', }
endif

if get_option('webkit-unmaintained')
  common_plugins_list += { 'name': 'web-page',
    'deps': [ gtk3, gegl, gdk_pixbuf, webkit, ],
  }
endif

foreach plugin : common_plugins_list
  plugin_name = plugin.get('name')
  plugin_sources = [ plugin.get('sources', plugin_name + '.c') ]
  plugin_deps = plugin.get('deps', [ gtk3, gegl, gdk_pixbuf, ])
  plugin_cflags = plugin.get('cflags', [])

  if platform_windows
    plugin_rc = configure_file(
      input : gimp_plugins_rc.full_path(),
      output: plugin_name + '.rc',
      copy: true,
    )

    # See https://gitlab.gnome.org/GNOME/gimp/-/issues/8537
    if generate_version_h
      compile_resources_depfiles = []
      compile_resources_depends  = [ gitversion_h ]
    else
      compile_resources_depfiles = [ gitversion_h ]
      compile_resources_depends  = []
    endif

    plugin_sources += windows.compile_resources(
      plugin_rc,
      args: [
        '--define', 'ORIGINALFILENAME_STR="@0@"'.format(plugin_name+'.exe'),
        '--define', 'INTERNALNAME_STR="@0@"'    .format(plugin_name),
        '--define', 'TOP_SRCDIR="@0@"'          .format(meson.project_source_root()),
      ],
      include_directories: [
        rootInclude, appInclude,
      ],
      depend_files: compile_resources_depfiles,
      depends: compile_resources_depends
    )
  endif

  plugin_exe = executable(plugin_name,
                          plugin_sources,
                          include_directories: [ rootInclude, ],
                          link_with : [
                            libgimp,
                            libgimpbase,
                            libgimpcolor,
                            libgimpconfig,
                            libgimpmath,
                            libgimpui,
                            libgimpwidgets,
                          ],
                          dependencies: [ plugin_deps, math ],
                          c_args: plugin_cflags,
                          install: true,
                          install_dir: gimpplugindir / 'plug-ins' / plugin_name)

  # Ugly trick to copy executables into subfolders so that we can run GIMP from
  # the build directory without installing it.
  custom_target('test-' + plugin_name,
                input: [ plugin_exe ],
                output: [ plugin_name + '.dummy' ],
                command: [ python, meson.project_source_root() / '.gitlab/cp-plug-in-subfolder.py',
                           plugin_exe, meson.current_build_dir() / 'test-plug-ins' / plugin_name,
                           '@OUTPUT@' ],
                build_by_default: true)
  plugin_executables += [meson.current_build_dir() / 'test-plug-ins' / plugin_name / fs.name(plugin_exe.full_path())]
endforeach
