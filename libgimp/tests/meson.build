# XXX: we have a bunch of (manually run?) tests inside libgimp/test/.
# These should either be deleted or transformed into real unit tests.

tests = [
  'palette',
]

# make GIMP runnable without being installed.
env=environment()

menu_paths=meson.project_build_root() / 'menus:' + meson.project_source_root() / 'menus'
env.set('GIMP_TESTING_MENUS_PATH', menu_paths)

env.set('GIMP_TESTING_PLUGINDIRS', meson.project_build_root() / 'plug-ins:')
env.append('GIMP_TESTING_PLUGINDIRS', meson.project_build_root() / 'plug-ins/python')
env.append('GIMP_TESTING_PLUGINDIRS', meson.project_build_root() / 'plug-ins/common/test-plug-ins/')
env.append('GIMP_TESTING_PLUGINDIRS', meson.project_build_root() / 'libgimp/tests/c-tests/')

env.prepend('GI_TYPELIB_PATH', meson.project_build_root() / 'libgimp')
env.prepend('LD_LIBRARY_PATH', meson.project_build_root() / 'libgimp')
env.prepend('LD_LIBRARY_PATH', meson.project_build_root() / 'libgimpbase')
env.prepend('LD_LIBRARY_PATH', meson.project_build_root() / 'libgimpcolor')
env.prepend('LD_LIBRARY_PATH', meson.project_build_root() / 'libgimpconfig')
env.prepend('LD_LIBRARY_PATH', meson.project_build_root() / 'libgimpmath')
env.prepend('LD_LIBRARY_PATH', meson.project_build_root() / 'libgimpmodule')
env.prepend('LD_LIBRARY_PATH', meson.project_build_root() / 'libgimpthumb')
env.prepend('LD_LIBRARY_PATH', meson.project_build_root() / 'libgimpwidgets')

env.set('GIMP_TESTING_ABS_TOP_SRCDIR', meson.project_source_root())

if enable_console_bin
  gimp_exe=gimpconsole_exe
else
  gimp_exe=gimpmain_exe
endif

run_python_test = find_program('./libgimp-run-python-test.sh')
run_c_test      = find_program('./libgimp-run-c-test.sh')
cat             = find_program('cat')
foreach test_name : tests
  basename = 'test-' + test_name

  py_test  = meson.current_source_dir() / basename + '.py'
  test(test_name, run_python_test,
       args: [ gimp_exe, py_test ],
       env: env,
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
       env: env,
       suite: ['libgimp', 'C'],
       timeout: 60)
endforeach
