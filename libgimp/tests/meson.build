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
foreach test_name : tests
  basename = 'test-' + test_name
  py_test  = meson.current_source_dir() / basename + '.py'

  test(test_name, run_python_test,
       args: [ gimp_exe, py_test ],
       env: env,
       suite: ['libgimp', 'python3'],
       timeout: 60)
endforeach
