# Windows specific

configure_file(
  input : 'ligma-plug-ins.rc.in',
  output: 'ligma-plug-ins.rc',
  configuration: versionconfig,
)

configure_file(
  input : 'ligma.rc.in',
  output: 'ligma.rc',
  configuration: versionconfig,
)

# Basically, the build rules below do nothing (a mere `python -c 'exit()'` call).
# But because they depends on `git-version.h`, meson ensure that it gets built first,
# Then the result of this targets is used in 35+ resource compiler build rules.
#
# Nasty trick indeed, but it fixes race condition issue described in GNOME/LIGMA#6257.

ligma_plugins_rc = custom_target('ligma_plugins_rc',
  build_by_default: true,
  build_always_stale: true,
  command: [python, '-c', 'exit()'],
  depends: generate_version_h ? [gitversion_h] : [],
  output:  ['ligma-plug-ins.rc']
)

ligma_app_rc = custom_target('ligma_app_rc',
  build_by_default: true,
  build_always_stale: true,
  command: [python, '-c', 'exit()'],
  depends: generate_version_h ? [gitversion_h] : [],
  output:  ['ligma.rc']
)
