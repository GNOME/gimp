# Windows specific

configure_file(
  input : 'gimp-plug-ins.rc.in',
  output: 'gimp-plug-ins.rc',
  configuration: versionconfig,
)

configure_file(
  input : 'gimp.rc.in',
  output: 'gimp.rc',
  configuration: versionconfig,
)

# Basically, the build rules below do nothing (a mere `python -c 'exit()'` call).
# But because they depends on `git-version.h`, meson ensure that it gets built first,
# Then the result of this targets is used in 35+ resource compiler build rules.
#
# Nasty trick indeed, but it fixes race condition issue described in GNOME/GIMP#6257.

gimp_plugins_rc = custom_target('gimp_plugins_rc',
  build_by_default: true,
  build_always_stale: true,
  command: [python, '-c', 'exit()'],
  depends: generate_version_h ? [gitversion_h] : [],
  output:  ['gimp-plug-ins.rc']
)

gimp_app_rc = custom_target('gimp_app_rc',
  build_by_default: true,
  build_always_stale: true,
  command: [python, '-c', 'exit()'],
  depends: generate_version_h ? [gitversion_h] : [],
  output:  ['gimp.rc']
)
