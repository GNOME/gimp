# test plugins

# Some, but not all, plugins in this directory are installed in unstable build.
# None should be installed in a release.
# None should be translated: not marked or marked but in potfiles.skip.

# Note this meson.build is excluded from a release build at a higher level,
# so here there are no "if not release statements"

# scripts interpreted by extension-script-fu, installed to /scripts
scripts = [
  'contactsheet.scm',
  'clothify-v3.scm',
  'test-sphere.scm',
  'test-v3.scm',
]


install_data(
  scripts,
  install_dir: gimpdatadir / 'scripts',
)


# scripts interpreted by gimp-script-fu-interpreter
# Each installed in subdirectory of /plug-in
# Each have a shebang and executable permission.
# Like other interpreted plugins.
# Lacking a shebang, a .interp file is needed to associate .scm suffix

scripts_independent = [
  { 'name': 'test-bytes' },
  { 'name': 'test-display' },
]

foreach plugin : scripts_independent
  name = plugin.get('name')
  srcs = plugin.get('srcs', name + '.scm')

  install_data(srcs,
               install_dir: gimpplugindir / 'plug-ins' / name,
               install_mode: 'rwxr-xr-x')
endforeach
