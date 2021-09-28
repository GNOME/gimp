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

# Basically, the build rules below do nothing (a mere `meson --version` call).
# But because they depends on `git-version.h`, meson ensure that it gets built first,
# Then the result of this targets is used in 35+ resource compiler build rules.
#
# Nasty trick indeed, but it fixes race condition issue described in GNOME/GIMP#6257.

meson_command = find_program('meson')

gimp_plugins_rc = custom_target('git-version.h build-time-dependency for gimp_plugins_rc',
  build_by_default: true,
  build_always_stale: true,
  command: [meson_command, '--version'],
  depends: [gitversion_h],
  output:  ['gimp-plug-ins.rc']
).full_path()

gimp_app_rc = custom_target('git-version.h build-time-dependency for gimp.rc',
  build_by_default: true,
  build_always_stale: true,
  command: [meson_command, '--version'],
  depends: [gitversion_h],
  output:  ['gimp.rc']
).full_path()
