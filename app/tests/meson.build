apptests_links = [
  libappconfig,
  libappactions,
  libappdialogs,
  libappdisplay,
  libappgui,
  libappmenus,
  libapppropgui,
  libapptools,
  libappwidgets,
  libgimpbase,
  libgimpcolor,
  libgimpconfig,
  libgimpmath,
  libgimpmodule,
  libgimpthumb,
  libgimpwidgets,
]


libapptestutils_sources = [
  'gimp-app-test-utils.c',
  'gimp-test-session-utils.c',
]

libapptestutils = static_library('apptestutils',
  libapptestutils_sources,
  dependencies: libapp_dep,
  link_with: apptests_links,
)

apptests_links += libapptestutils


app_tests = [
  'core',
  'gimpidtable',
  'save-and-export',
#'session-2-8-compatibility-multi-window',
#'session-2-8-compatibility-single-window',
  'single-window-mode',
#'tools',
  'ui',
  'xcf',
]

cmd = run_command('create_test_env.sh', check: false)
if cmd.returncode() != 0
 error(cmd.stderr().strip())
endif

# Prevent parallel builds for the tests
# The tests must not be run in parallel or in a different order as specified

prio = 1000
foreach test_name : app_tests
  test_exe = executable(test_name,
    'test-@0@.c'.format(test_name),
    'tests.c',
    dependencies: [ libapp_dep, appstream_glib ],
    link_with: apptests_links,
  )

  test(test_name,
    test_exe,
    env: [
      'GIMP_TESTING_ABS_TOP_SRCDIR='  + meson.project_source_root(),
      'GIMP_TESTING_ABS_TOP_BUILDDIR='+ meson.project_build_root(),
      'GIMP_TESTING_PLUGINDIRS='      + meson.project_build_root()/'plug-ins'/'common',
      'UI_TEST=yes',
    ],
    suite: 'app',
    timeout: 60,
    is_parallel : false,
    priority: prio,
  )

  prio = prio - 10

endforeach
