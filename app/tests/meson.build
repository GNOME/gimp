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
  libligmabase,
  libligmacolor,
  libligmaconfig,
  libligmamath,
  libligmamodule,
  libligmathumb,
  libligmawidgets,
]


libapptestutils_sources = [
  'ligma-app-test-utils.c',
  'ligma-test-session-utils.c',
]

libapptestutils = static_library('apptestutils',
  libapptestutils_sources,
  dependencies: libapp_dep,
  link_with: apptests_links,
)

apptests_links += libapptestutils


app_tests = [
  'core',
  'ligmaidtable',
  'save-and-export',
  'session-2-8-compatibility-multi-window',
  'session-2-8-compatibility-single-window',
  'single-window-mode',
  # 'tools',
  'ui',
  'xcf',
]

cmd = run_command('create_test_env.sh', check: false)
if cmd.returncode() != 0
 error(cmd.stderr().strip())
endif

foreach test_name : app_tests
  test_exe = executable(test_name,
    'test-@0@.c'.format(test_name),
    dependencies: [ libapp_dep, appstream_glib ],
    link_with: apptests_links,
  )

  test(test_name,
    test_exe,
    env: [
      'LIGMA_TESTING_ABS_TOP_SRCDIR='  + meson.project_source_root(),
      'LIGMA_TESTING_ABS_TOP_BUILDDIR='+ meson.project_build_root(),
      'LIGMA_TESTING_PLUGINDIRS=' +      meson.project_build_root()/'plug-ins'/'common',
      'LIGMA_TESTING_PLUGINDIRS_BASENAME_IGNORES=mkgen.pl',
      'UI_TEST=yes',
    ],
    suite: 'app',
    timeout: 60,
  )

endforeach
