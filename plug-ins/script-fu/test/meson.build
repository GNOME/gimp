
# Install ScriptFu testing framework and test scripts

if not stable
  test_framework_scripts = [
    'frameworks' / 'testing.scm',
  ]

  test_scripts = [
    # test binding to the PDB

    'tests' / 'PDB' / 'image' / 'image-new.scm',
    'tests' / 'PDB' / 'image' / 'image-precision.scm',
    'tests' / 'PDB' / 'image' / 'image-indexed.scm',
    'tests' / 'PDB' / 'image' / 'image-grayscale.scm',
    'tests' / 'PDB' / 'image' / 'image-ops.scm',
    'tests' / 'PDB' / 'image' / 'image-layers.scm',
    'tests' / 'PDB' / 'image' / 'image-set-selected.scm',
    'tests' / 'PDB' / 'image' / 'image-mode.scm',
    'tests' / 'PDB' / 'image' / 'image-color-profile.scm',

    'tests' / 'PDB' / 'paint' / 'paint.scm',
    'tests' / 'PDB' / 'paint' / 'paint-methods.scm',
    'tests' / 'PDB' / 'paint' / 'dynamics.scm',

    'tests' / 'PDB' / 'layer' / 'layer-new.scm',
    'tests' / 'PDB' / 'layer' / 'layer-ops.scm',
    'tests' / 'PDB' / 'layer' / 'layer-mask.scm',

    'tests' / 'PDB' / 'text-layer' / 'text-layer-new.scm',

    'tests' / 'PDB' / 'item' / 'item.scm',
    'tests' / 'PDB' / 'item' / 'item-position.scm',
    'tests' / 'PDB' / 'item' / 'item-layer-group.scm',

    'tests' / 'PDB' / 'drawable' / 'drawable.scm',
    'tests' / 'PDB' / 'drawable' / 'drawable-attributes.scm',
    'tests' / 'PDB' / 'drawable' / 'drawable-ops.scm',

    'tests' / 'PDB' / 'channel' / 'channel-new.scm',
    'tests' / 'PDB' / 'channel' / 'channel-attributes.scm',
    'tests' / 'PDB' / 'channel' / 'channel-ops.scm',

    'tests' / 'PDB' / 'vectors' / 'vectors-new.scm',
    'tests' / 'PDB' / 'vectors' / 'vectors-stroke.scm',

    'tests' / 'PDB' / 'selection' / 'selection.scm',
    'tests' / 'PDB' / 'selection' / 'selection-from.scm',
    'tests' / 'PDB' / 'selection' / 'selection-by.scm',
    'tests' / 'PDB' / 'selection' / 'selection-by-shape.scm',

    'tests' / 'PDB' / 'resource' / 'resource.scm',
    'tests' / 'PDB' / 'resource' / 'resource-ops.scm',
    'tests' / 'PDB' / 'resource' / 'brush.scm',
    'tests' / 'PDB' / 'resource' / 'palette.scm',

    'tests' / 'PDB' / 'context' / 'context-get-set.scm',
    'tests' / 'PDB' / 'context' / 'context-resource.scm',
    'tests' / 'PDB' / 'context' / 'context-stack.scm',

    'tests' / 'PDB' / 'display' / 'display.scm',

    'tests' / 'PDB' / 'edit' / 'buffer.scm',
    'tests' / 'PDB' / 'edit' / 'edit.scm',
    'tests' / 'PDB' / 'edit' / 'edit-multi-layer.scm',

    'tests' / 'PDB' / 'gimp' / 'gimp.scm',
    'tests' / 'PDB' / 'gimp' / 'PDB.scm',
    'tests' / 'PDB' / 'gimp' / 'refresh.scm',
    'tests' / 'PDB' / 'gimp' / 'procedures.scm',

    # uncategorized tests
    'tests' / 'PDB' / 'misc.scm',
    'tests' / 'PDB' / 'enums.scm',
    'tests' / 'PDB' / 'bind-args.scm',
    'tests' / 'PDB' / 'pixel.scm',
    'tests' / 'PDB' / 'gegl.scm',

    # comprehensive, total test
    'tests' / 'PDB' / 'pdb.scm',

    # test TinyScheme embedded interpreter

    'tests' / 'TS' / 'sharp-expr.scm',
    'tests' / 'TS' / 'sharp-expr-char.scm',
    'tests' / 'TS' / 'sharp-expr-unichar.scm',
    'tests' / 'TS' / 'unichar.scm',
    'tests' / 'TS' / 'cond-expand.scm',
    'tests' / 'TS' / 'atom2string.scm',
    'tests' / 'TS' / 'integer2char.scm',
    'tests' / 'TS' / 'string-escape.scm',
    'tests' / 'TS' / 'file-port.scm',
    'tests' / 'TS' / 'string-port-input.scm',
    'tests' / 'TS' / 'string-port-output.scm',
    # WIP 'tests' / 'TS' / 'string-port-unichar.scm',
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



