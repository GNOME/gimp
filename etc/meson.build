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
bash = find_program('bash', required : get_option('bash-completion'))
bash_completion = dependency('bash-completion', version: '>=2.0', required: get_option('bash-completion'))
if bash_completion.found()
  bash_comp_inst_dir = ''
  bash_comp_dir_override = bash_completion.version().version_compare('>= 2.10') ? ['datadir', get_option('datadir')] : ['prefix', get_option('prefix')]
  bash_comp_inst_dir = bash_completion.get_variable('completionsdir', pkgconfig_define: bash_comp_dir_override,
                                                    default_value: get_option('datadir') / 'bash-completion' / 'completions')

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
  have_bash_completion = 'true (@0@)'.format(bash_comp_inst_dir)
elif get_option('bash-completion').disabled()
  have_bash_completion = 'disabled'
else
  if not bash.found()
    have_bash_completion = 'false (bash not found)'
  elif not bash_completion.found()
    have_bash_completion = 'false (bash-completion not found)'
  endif
endif
