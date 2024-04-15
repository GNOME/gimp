# XXX: we have a bunch of (manually run?) tests inside libgimp/test/.
# These should either be deleted or transformed into real unit tests.

if not meson.can_run_host_binaries()
  warning('libgimp unit testing disabled in cross-building or similar environments.')
  subdir_done()
endif

tests = [
  'palette',
  'selection-float'
]

# Unit testing environment is based on gimp_run_env with additional environment
# variables and added temporary test plug-ins. Ideally I should copy
# gimp_run_env, but meson doesn't have such function to create an env variable
# from another. So I just modify the main one instead (fortunately here it's not
# a big problem).
# See: https://github.com/mesonbuild/meson/issues/13045
gimp_run_env.append('GIMP_TESTING_PLUGINDIRS', meson.project_build_root() / 'libgimp/tests/c-tests/')
gimp_run_env.set('GIMP_TESTING_ABS_TOP_SRCDIR', meson.project_source_root())

run_python_test = find_program('./libgimp-run-python-test.sh')
run_c_test      = find_program('./libgimp-run-c-test.sh')
cat             = find_program('cat')
foreach test_name : tests
  basename = 'test-' + test_name

  py_test  = meson.current_source_dir() / basename + '.py'
  test(test_name, run_python_test,
       args: [ gimp_exe, py_test ],
       env: gimp_run_env,
       suite: ['libgimp', 'python3'],
       timeout: 60)

  c_test_name  = basename + '.c'
  c_test = custom_target(c_test_name,
                         input: [ 'c-test-header.c', c_test_name ],
                         output: c_test_name,
                         command: [cat, '@INPUT@'],
                         capture: true,
                         install: false)
  c_test_exe = executable(basename,
                          c_test,
                          dependencies: [ libgimp_dep, pango ],
                          install: false)

  # Same ugly trick as in plug-ins/common/meson.build to detect plug-ins in a
  # non-installed build directory.
  custom_target(basename + '.dummy',
                input: [ c_test_exe ],
                output: [ basename + '.dummy' ],
                command: [ python, meson.project_source_root() / '.gitlab/cp-plug-in-subfolder.py',
                           c_test_exe, meson.current_build_dir() / 'c-tests' / basename,
                           '@OUTPUT@' ],
                build_by_default: true,
                install: false)

  test(test_name, run_c_test,
       args: [ gimp_exe, meson.current_source_dir() / c_test_name, basename ],
       env: gimp_run_env,
       suite: ['libgimp', 'C'],
       timeout: 60)
endforeach
