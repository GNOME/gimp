# Windows specific

# The declare_dependency() tricks ensures that gimp-plug-ins.rc and
# gimp.rc are never used before git-version.h is built first.

gimp_plugins_rc_dep = configure_file(
  input : 'gimp-plug-ins.rc.in',
  output: 'gimp-plug-ins.rc',
  configuration: versionconfig,
)
gimp_plugins_rc = declare_dependency(sources: [ gimp_plugins_rc_dep, gitversion_h ])

gimp_app_rc_dep = configure_file(
  input : 'gimp.rc.in',
  output: 'gimp.rc',
  configuration: versionconfig,
)
gimp_app_rc = declare_dependency(sources: [ gimp_app_rc_dep, gitversion_h ])
