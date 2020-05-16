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
    bash_comp_inst_dir = bash_comp_dep.get_variable('completionsdir', define_variable: bash_comp_dir_override)
  endif

  if bash_comp_inst_dir == ''
    message('Found bash-completion but the .pc file did not set \'completionsdir\', fallback to a predefined path')
    bash_comp_inst_dir = get_option('datadir') / 'bash-completion' / 'completions'
  endif

  install_data('bash_completion',
    install_dir: bash_comp_inst_dir,
    rename: 'gimp',
  )
endif
