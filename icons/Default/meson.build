rtl_icons = [
  ['go-next', 'go-previous'],
  ['go-first', 'go-last'],
]

if have_vector_icons
  # Only process the 64x64 icons for the 2 resource icons.
  size='64'
  subdir('64')
else
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
  install_dir: gimpiconsdir / theme,
)

foreach install : install_icons
  icons  = []

  inputs = install['files']
  outdir = install['outdir']
  indir  = install['indir']

  py_cmd = 'print(",".join(["@0@/" + line.strip() + "@1@" for line in open("@2@") if line.strip() != "" and line.strip()[0] != "#"]))'
  dup_icons = []

  foreach suffix : install['suffixes']
    foreach input : inputs
      input = '../icon-lists/@0@.list'.format(input)
      cmd = run_command(python, '-c',
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

    # gimp-wilber icon in color is the official logo with shadow.
    wilber = '@0@/gimp-wilber.svg'.format(indir)
    if wilber in icons
      newicons = []
      foreach icon : icons
        if icon != wilber
          newicons += icon
        endif
      endforeach
      icons = newicons
      install_data(meson.project_source_root() / 'gimp-data/images/logo/gimp-logo-shadow.svg',
                   rename: 'gimp-wilber.svg',
                   install_dir: gimpiconsdir / theme / outdir / 'logo/unisize')
    endif

    wilber = '@0@/gimp-wilber-symbolic.svg'.format(indir)
    if wilber in icons
      newicons = []
      foreach icon : icons
        if icon != wilber
          newicons += icon
        endif
      endforeach
      icons = newicons
      install_data(meson.project_source_root() / 'gimp-data/images/logo/gimp-wilber-symbolic.svg',
                   rename: 'gimp-wilber-symbolic.svg',
                   install_dir: gimpiconsdir / theme / outdir / 'logo/unisize')
    endif

    install_data(icons, install_dir: gimpiconsdir / theme / outdir / 'apps')

    # Duplicate gimp-wilber-eek as image-missing
    wilber_eek = '@0@/gimp-wilber-eek@1@'.format(indir, suffix)
    if wilber_eek in icons
      # Note: since this is just a copy of gimp-wilber-week, we could just do a
      # symlink with install_symlink(), but this is only available in meson 0.61.0
      # and looking at its code, it would apparently fail if symlinks are not
      # available in the target OS (instead of silently and intelligently falling
      # back to doing a copy).
      install_data(wilber_eek,
                   rename: 'image-missing@0@'.format(suffix),
                   install_dir: gimpiconsdir / theme / outdir / 'apps')
    endif

    # Duplicate RTL relevant icons
    if suffix == '.svg'
      foreach rtl : rtl_icons
        rtl_icon     = '@0@/@1@@2@'.format(indir, rtl[0], suffix)
        rtl_sym_icon = '@0@/@1@-symbolic@2@'.format(indir, rtl[0], suffix)
        install_data(rtl_icon,
                      rename: '@0@-rtl@1@'.format(rtl[1], suffix),
                      install_dir: gimpiconsdir / theme / outdir / 'apps')
        install_data(rtl_sym_icon,
                      rename: '@0@-symbolic-rtl@1@'.format(rtl[1], suffix),
                      install_dir: gimpiconsdir / theme / outdir / 'apps')

        rtl_icon     = '@0@/@1@@2@'.format(indir, rtl[1], suffix)
        rtl_sym_icon = '@0@/@1@-symbolic@2@'.format(indir, rtl[1], suffix)
        install_data(rtl_icon,
                      rename: '@0@-rtl@1@'.format(rtl[0], suffix),
                      install_dir: gimpiconsdir / theme / outdir / 'apps')
        install_data(rtl_sym_icon,
                      rename: '@0@-symbolic-rtl@1@'.format(rtl[0], suffix),
                      install_dir: gimpiconsdir / theme / outdir / 'apps')
      endforeach
    endif
  endforeach
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
  command: [ python,'-c','import sys; sys.stdout.write(sys.argv[1])',xml_content ],
  capture: true,
)
icons_core_sources = gnome.compile_resources(
  resourcename,
  xml_file,
  c_name: resourcename.underscorify(),
  dependencies: icon_dialog_question,
  source_dir: [meson.current_build_dir()]
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
  command: [ python,'-c','import sys; sys.stdout.write(sys.argv[1])',xml_content ],
  capture: true,
)
icons_imgs_sources = gnome.compile_resources(
  resourcename,
  xml_file,
  c_name: resourcename.underscorify(),
  dependencies: icon_wilber_eek,
  source_dir: [meson.current_build_dir()]
)
