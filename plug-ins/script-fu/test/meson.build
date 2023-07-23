
# Install ScriptFu testing framework and test scripts

if not stable
  test_framework_scripts = [
    'frameworks' / 'testing.scm',
  ]

  test_scripts = [
    'tests' / 'PDB' / 'image' / 'image-new.scm',
    'tests' / 'PDB' / 'image' / 'image-precision.scm',
    'tests' / 'PDB' / 'image' / 'image-indexed.scm',
    'tests' / 'PDB' / 'image' / 'image-grayscale.scm',
    'tests' / 'PDB' / 'image' / 'image-ops.scm',
    'tests' / 'PDB' / 'layer' / 'layer-new.scm',
    'tests' / 'PDB' / 'layer' / 'layer-ops.scm',
    'tests' / 'PDB' / 'layer' / 'layer-mask.scm',
    'tests' / 'PDB' / 'text-layer' / 'text-layer-new.scm',
    'tests' / 'PDB' / 'item' / 'item.scm',
    'tests' / 'PDB' / 'channel' / 'channel-new.scm',
    'tests' / 'PDB' / 'vectors-new.scm',
    'tests' / 'PDB' / 'selection' / 'selection.scm',
    'tests' / 'PDB' / 'selection' / 'selection-from.scm',
    'tests' / 'PDB' / 'resource.scm',
    'tests' / 'PDB' / 'brush.scm',
    'tests' / 'PDB' / 'misc.scm',
    'tests' / 'PDB' / 'enums.scm',
    # comprehensive, total test
    'tests' / 'PDB' / 'pdb.scm',

    'tests' / 'TS' / 'sharp-expr.scm',
    'tests' / 'TS' / 'sharp-expr-char.scm',
    'tests' / 'TS' / 'sharp-expr-unichar.scm',
    'tests' / 'TS' / 'unichar.scm',
    'tests' / 'TS' / 'cond-expand.scm',
    'tests' / 'TS' / 'atom2string.scm',
    'tests' / 'TS' / 'integer2char.scm',
    'tests' / 'TS' / 'string-port.scm',
    'tests' / 'TS' / 'testing.scm',
    'tests' / 'TS' / 'vector.scm',
    'tests' / 'TS' / 'no-memory.scm',
    # comprehensive, total test
    'tests' / 'TS' / 'tinyscheme.scm',
  ]

endif

# Install test framework to shared /scripts
# Assert: SFConsole, SFExtension, and standalong SFInterpreter will read them.
# SFConsole is primary testing interface.
# Some plugins in /scripts (SFExtension)
# and in /plug-ins (SFInterpreter) may also be interface for testing.

install_data(
  test_framework_scripts,
  install_dir: gimpdatadir / 'scripts',
)

# Install test scripts to shared /tests
# Install flattening the dir structure.

install_data(
  test_scripts,
  install_dir: gimpdatadir / 'tests',
)



