etcconf = configuration_data()
etcconf.set('GIMP_APP_VERSION', gimp_app_version)
etcconf.set('mypaint_brushes_dir', mypaint_brushes_dir)


install_data(
  [
    'controllerrc',
    configure_file(
      input : 'gimprc.in',
      output: 'gimprc',
      configuration: etcconf,
    ),
    'gimp.css',
    'sessionrc',
    'templaterc',
    'toolrc',
    'unitrc',
  ],
  install_dir: gimpsysconfdir,
)

# Bash completion
have_bash = find_program('bash', required : false).found() # For completion scripts
bash_comp_dep = dependency('bash-completion', version: '>=2.0', required: false)
if have_bash

  bash_comp_inst_dir = ''
  if bash_comp_dep.found()
    bash_comp_dir_override = bash_comp_dep.version().version_compare('>= 2.10') ? ['datadir', get_option('datadir')] : ['prefix', get_option('prefix')]
    bash_comp_inst_dir = bash_comp_dep.get_variable('completionsdir', pkgconfig_define: bash_comp_dir_override)
  endif

  if bash_comp_inst_dir == ''
    message('Found bash-completion but the .pc file did not set \'completionsdir\', fallback to a predefined path')
    bash_comp_inst_dir = get_option('datadir') / 'bash-completion' / 'completions'
  endif

  bash_completion_conf = configuration_data()
  bash_completion_conf.set('GIMP_MAIN_EXE', gimpmain_exe_name)
  bash_completion_conf.set('GIMP_SYMLINK1', 'gimp-' + gimp_app_version_major.to_string())
  bash_completion_conf.set('GIMP_SYMLINK2', 'gimp')

  bash_completion_conf.set('GIMP_CONSOLE_EXE',      gimpconsole_exe_name)
  bash_completion_conf.set('GIMP_CONSOLE_SYMLINK1', 'gimp-console-' + gimp_app_version_major.to_string())
  bash_completion_conf.set('GIMP_CONSOLE_SYMLINK2', 'gimp-console')

  configure_file(input : 'bash_completion.in',
                 output: 'gimp-' + gimp_app_version,
                 configuration: bash_completion_conf,
                 install: true,
                 install_dir: bash_comp_inst_dir)
endif
