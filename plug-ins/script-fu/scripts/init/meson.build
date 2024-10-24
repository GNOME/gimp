
# scripts loaded by ScriptFu to initialize the embedded interpreter
init_scripts = [
  'plug-in-compat.scm',
  'init.scm',
  'script-fu-compat.scm',
]

# compatibility scripts distributed but not automatically loaded by ScriptFu.
# Loaded by third-party plugin scripts from well-know directory, on demand.
compat_scripts = [
  'SIOD-compat.scm',
  'PDB-compat-v2.scm',
]


install_data(
  init_scripts,
  install_dir: gimpdatadir / 'scripts' / 'scriptfu-init',
)

install_data(
  compat_scripts,
  install_dir: gimpdatadir / 'scripts' / 'scriptfu-init',
)

