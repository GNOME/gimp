# XXX: we have a bunch of (manually run?) tests inside libgimp/test/.
# These should either be deleted or transformed into real unit tests.

if not meson.can_run_host_binaries()
  warning('libgimp unit testing disabled in cross-building or similar environments.')
  subdir_done()
endif

tests = {
  'color-parser': {},
  'constants': {},
  'export-options': {},
  'image': {},
  'palette': {
    'PALETTES': [ 'data/palettes/Bears.gpl' ]
  },
  'selection-float': {},
  'unit': {},
}

run_python_test = find_program('./libgimp-run-python-test.py')
run_c_test      = find_program('./libgimp-run-c-test.py')
foreach test_name, test_data : tests
  basename = 'test-' + test_name

  py_test  = meson.current_source_dir() / basename + '.py'
  test_env = gimp_run_env
  test_env.append('GIMP_TESTING_PLUG_INS', 'libgimp/tests' / basename + '.py', separator: ':')
  foreach data_type, data_list : test_data
    foreach data_path : data_list
      test_env.append('GIMP_TESTING_' + data_type, data_path, separator: ':')
    endforeach
  endforeach

  test(test_name, run_python_test,
       args: [ gimp_exe.full_path(), py_test ],
       env: test_env,
       suite: ['libgimp', 'python3'],
       timeout: 90)

  c_test_name  = basename + '.c'
  c_test = custom_target(c_test_name,
                         input: [ 'c-test-header.c', c_test_name ],
                         output: c_test_name,
                         command: [python, '-c', 'import sys; [sys.stdout.write(open(f).read()) for f in sys.argv[1:]]','@INPUT@'],
                         capture: true,
                         install: false)
  c_test_exe = executable(basename,
                          c_test,
                          dependencies: [ libgimp_dep, pango ],
                          install: false)

  # Ugly trick to add executables to plugin_custom_targets list (see root meson.build)
  plugin_custom_targets += custom_target('dummy-c-test' + basename,
                                         input: c_test_exe,
                                         output: basename + '.dummy',
                                         command: [python, '-c',
                                                   'open("@OUTPUT@", "w").write("Nice. I was built before calling in-build-gimp.py\\n")'],
                                         build_by_default: true)

  test_env.append('GIMP_TESTING_PLUG_INS', 'libgimp/tests' / fs.name(c_test_exe.full_path()), separator: ':')

  test(test_name, run_c_test,
       args: [ gimp_exe.full_path(), meson.current_source_dir() / c_test_name, basename ],
       env: test_env,
       suite: ['libgimp', 'C'],
       timeout: 90)
endforeach
