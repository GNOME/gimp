# test i18n plugins

# Not for translators: translation data is mocked up

# scripts interpreted by gimp-script-fu-interpreter
scripts_independent = [
  { 'name': 'test-i18n' },
  { 'name': 'test-i18n-more' },
]

foreach plugin : scripts_independent
  name = plugin.get('name')
  srcs = plugin.get('srcs', name + '.scm')

  install_data(srcs,
               install_dir: gimpplugindir / 'plug-ins' / name,
               install_mode: 'rwxr-xr-x')
endforeach


# Install test translations for a suite of PDB procedures
# under two i18n domain names: "script-fu-test" and "scriptfu-test-more"

# See "Internationalizing" at the gimp developer website
# Two plugin files define three PDB procedures
# that share the same translations data.
# "Suite" means: sharing the same translations data files.
# suite is one-to-many with i18n domain names, unfortunately.

# ! install_dir: does NOT accept a list

# test-i18n.scm defines two PDB procedures
i18n.gettext ('scriptfu-test',
              preset: 'glib',
              install_dir: gimpplugindir / 'plug-ins' / 'test-i18n' / 'locale'
             )

# test-i18n-more.scm defines one PDB procedure
# Duplicate, install i18n data in a second place
# and give it a different domain name, "scriptfu-test-more !!!
i18n.gettext ('scriptfu-test-more',
              preset: 'glib',
              install_dir: gimpplugindir / 'plug-ins' / 'test-i18n-more' / 'locale'
             )