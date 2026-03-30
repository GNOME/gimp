
# Configure_file is just here to rename the files
manconf = configuration_data()
manconf.set('GIMP_VERSION',   gimp_version)
manconf.set('GIMP_APP_VERSION', gimp_app_version)
manconf.set('gimpdatadir',    prefix / gimpdatadir)
manconf.set('gimplocaledir',  prefix / localedir)
manconf.set('gimpplugindir',  prefix / gimpplugindir)
manconf.set('manpage_gimpdir', '$XDG_CONFIG_HOME/' + gimpdir / gimp_app_version)
manconf.set('gimpsysconfdir', prefix / gimpsysconfdir)
manconf.set('mypaint_brushes_dir', mypaint_brushes_dir)

gimprc_man = custom_target('gimprc-' + gimp_app_version + '.5',
                           output: 'gimprc-' + gimp_app_version + '.5',
                           depends: [ gimp_exe_depends ],
                           command: [ gimp_exe, '--no-interface',
                                      '--dump-gimprc-manpage' ],
                           capture: true,
                           build_by_default: true,
                           env: gimp_run_env,
                           install: true,
                           install_dir: get_option('mandir') / 'man5')

gimptool_man = configure_file(input : 'gimptool.1.in',
                              output: 'gimptool-' + gimp_app_version + '.1',
                              configuration: manconf)
man_files = [ gimptool_man ]

if enable_console_bin
  gimp_console_man = configure_file(input : 'gimp.1.in',
                                    output: 'gimp-console-' + gimp_app_version + '.1',
                                    configuration: manconf)
  man_files += [ gimp_console_man ]
endif

gimp_man = configure_file(input : 'gimp.1.in',
                          output: 'gimp-' + gimp_app_version + '.1',
                          configuration: manconf)
man_files += [ gimp_man ]

install_man(man_files)

if enable_default_bin and not platform_windows
  install_symlink('gimptool.1',
                  pointing_to: 'gimptool-' + gimp_app_version + '.1',
                  install_dir: get_option('mandir') + '/man1')
  install_symlink('gimptool-@0@.1'.format(api_version_major),
                  pointing_to: 'gimptool-' + gimp_app_version + '.1',
                  install_dir: get_option('mandir') + '/man1')

  install_symlink('gimprc.5',
                  pointing_to: 'gimprc-' + gimp_app_version + '.5',
                  install_dir: get_option('mandir') + '/man5')
  install_symlink('gimprc-@0@.5'.format(api_version_major),
                  pointing_to: 'gimprc-' + gimp_app_version + '.5',
                  install_dir: get_option('mandir') + '/man5')

  if enable_console_bin
    install_symlink('gimp-console.1',
                    pointing_to: 'gimp-console-' + gimp_app_version + '.1',
                    install_dir: get_option('mandir') + '/man1')
    install_symlink('gimp-console-@0@.1'.format(api_version_major),
                    pointing_to: 'gimp-console-' + gimp_app_version + '.1',
                    install_dir: get_option('mandir') + '/man1')
  endif

  install_symlink('gimp.1',
                  pointing_to: 'gimp-' + gimp_app_version + '.1',
                  install_dir: get_option('mandir') + '/man1')
  install_symlink('gimp-@0@.1'.format(api_version_major),
                  pointing_to: 'gimp-' + gimp_app_version + '.1',
                  install_dir: get_option('mandir') + '/man1')
endif

# Optional Markdown versions for the website.
pandoc = find_program('pandoc', required: false)
custom_target('Markdown GIMP man page',
              input: [ gimp_man ],
              output: 'gimp.md',
              command: [ '../tools/man2md.py', '@INPUT@', '@OUTPUT@', prefix ],
              build_by_default: false)
custom_target('Markdown gimprc man page',
              input: [ gimprc_man ],
              output: 'gimprc.md',
              command: [ '../tools/man2md.py', '@INPUT@', '@OUTPUT@', prefix ],
              build_by_default: false)
custom_target('Markdown gimptool man page',
              input: [ gimptool_man ],
              output: 'gimptool.md',
              command: [ '../tools/man2md.py', '@INPUT@', '@OUTPUT@', prefix ],
              build_by_default: false)
