# compositor-level ui tests. drive a real gimp window under headless mutter -
# black box, same path a real mouse takes, no gtk test apis. separate from the
# app/tests + libgimp/tests unit suites.
#
# runs on dogtail's hermetic session (dogtail.hermetic) - do_action over
# AT-SPI, no coordinate math, no injector. needs mutter, dbus-run-session,
# at-spi2-core and dogtail >= 2.1.0 for the configuring python (see
# .gitlab-ci.yml). anything missing disables the suite here, with the
# reason in the setup summary. once a test runs, a missing piece is a hard
# error (exit 99), not a skip.

ui_tests_status = 'yes'

# pointless unless we can actually run the gimp we just built.
if not (meson.can_run_host_binaries() and have_gobject_introspection)
  ui_tests_status = 'no (can\'t run host binaries, or no GObject Introspection)'
  subdir_done()
endif

mutter_prog      = find_program('mutter',            required: false)
dbus_run_session = find_program('dbus-run-session',  required: false)

if not (mutter_prog.found() and dbus_run_session.found())
  ui_tests_status = 'no (need mutter, dbus-run-session)'
  subdir_done()
endif

# check the version from package metadata, importing dogtail pulls in gtk.
dogtail_check = run_command(python, '-c',
  '\n'.join([
    '''import re, sys''',
    '''from importlib.metadata import version, PackageNotFoundError''',
    '''try:''',
    '''    m = re.match(r'(\d+)\.(\d+)', version('dogtail'))''',
    '''except PackageNotFoundError:''',
    '''    sys.exit(1)''',
    '''sys.exit(0 if m and (int(m[1]), int(m[2])) >= (2, 1) else 1)''']),
  check: false)

if dogtail_check.returncode() != 0
  ui_tests_status = 'no (dogtail>=2.1.0 not installed for @0@)'.format(python.full_path())
  subdir_done()
endif

# run the gui binary straight, not the in-build wrapper - that one runs the
# console binary when -Denable-console-bin is on, so no gui and no dialog.

ui_tests = [
  { 'name': 'startup' },
  { 'name': 'new-image' },
  { 'name': 'export-image',   'timeout': 300 },
  { 'name': 'color-swap' },
  { 'name': 'layer-add' },
  { 'name': 'undo-redo' },
  { 'name': 'layer-delete' },
  { 'name': 'zoom' },
  { 'name': 'fg-color' },
]

foreach t : ui_tests
  test(t['name'],
    dbus_run_session,
    args: [ '--',
            python.full_path(),
            meson.current_source_dir() / (t['name'] + '.py'),
            gimpmain_exe.full_path(),
            mutter_prog.full_path(),
          ],
    depends: gimp_exe_depends + [ gimpmain_exe ],
    env: gimp_run_env,
    suite: 'ui',
    timeout: t.get('timeout', 240),
    is_parallel: false,
  )
endforeach
