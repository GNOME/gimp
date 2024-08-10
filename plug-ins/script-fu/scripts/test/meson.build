# test plugins

# All are not for ordinary users.
# None should be translated: not marked or marked but in potfiles.skip.

# Some can be installed by a build, other must be manually installed.

# No plugin in this directory is installed in a "stable" or "release" version,
# even if buildtype==debug.

if (stable or release)
  subdir_done()
elif get_option('buildtype') != 'debug'
  subdir_done()
endif


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
  { 'name': 'ts-helloworld' },
]

foreach plugin : scripts_independent
  name = plugin.get('name')
  srcs = plugin.get('srcs', name + '.scm')

  install_data(srcs,
               install_dir: gimpplugindir / 'plug-ins' / name,
               install_mode: 'rwxr-xr-x')
endforeach
