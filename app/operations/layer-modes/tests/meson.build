if not meson.can_run_host_binaries()
  warning('Legacy layer mode unit testing disabled in cross-building or similar environments.')
  subdir_done()
endif

pycode = ''
foreach mode, submodes : layer_modes_own_ops + layer_modes_shared_op
  op_name = 'gimp:' + mode
  op_enum = 'Gimp.LayerMode.' + mode.replace('-', '_').to_upper()
  pycode += '  "@0@": {\n'.format(op_name) +\
            '    "enum": @0@,\n'.format(op_enum)
  foreach spaces, sha256sums : submodes
    pycode += '    "@0@": [\n'.format(spaces)
    foreach sha256sum : sha256sums
      pycode += '      "@0@",\n'.format(sha256sum)
    endforeach
    pycode += '    ],\n'
  endforeach
  pycode += '  },\n'
endforeach


test_ops_conf = configuration_data()
test_ops_conf.set('OPS', pycode)
test_ops_conf.set('OUTDIR', meson.project_build_root())

test_ops_py = configure_file(input : '../../layer-modes-legacy/tests/test-operations.py.in',
                             output : 'test-operations.py',
                             configuration : test_ops_conf)

test('layer-modes', run_python_test,
     args: [ gimp_exe.full_path(), test_ops_py ],
     env: test_env,
     suite: ['operations/layer-modes', 'python3'],
     timeout: 1000)

