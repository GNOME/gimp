if not stable or not release

# This does not get installed for releases
plugins += {
  'name': 'test-file-plug-ins',
  'srcs': [
    # modules
    'tests/test-file-plug-ins/gimplogger.py',
    'tests/test-file-plug-ins/gimpconfig.py',
    'tests/test-file-plug-ins/gimptestframework.py',
    'tests/test-file-plug-ins/gimpexporttests.py',
    # actual GIMP plug-in
    'tests/test-file-plug-ins/test-file-plug-ins.py',
    # scripts to start the tests in batch mode
    'tests/test-file-plug-ins/batch-import-tests.py',
    'tests/test-file-plug-ins/batch-export-tests.py',
  ],
}

# configuration data for the tests
subdir('tests')

endif
