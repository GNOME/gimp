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
  'session-2-8-compatibility-multi-window',
  'session-2-8-compatibility-single-window',
  'single-window-mode',
  # 'tools',
  'ui',
  'xcf',
]

foreach test_name : app_tests
  test_exe = executable(test_name,
    'test-@0@.c'.format(test_name),
    dependencies: [ libapp_dep, appstream_glib ],
    link_with: apptests_links,
  )

  test(test_name,
    test_exe,
    env: [
      'GIMP_TESTING_ABS_TOP_SRCDIR='  + meson.source_root(),
      'GIMP_TESTING_ABS_TOP_BUILDDIR='+ meson.build_root(),
      'GIMP_TESTING_PLUGINDIRS=' +      meson.build_root()/'plug-ins'/'common',
      'GIMP_TESTING_PLUGINDIRS_BASENAME_IGNORES=mkgen.pl',
      'UI_TEST=yes',
    ],
    suite: 'app',
  )

endforeach

run_target('create_test_env', command: find_program('create_test_env.sh'))
