
scriptfu_ftx = static_library('scriptfu-ftx',
  'ftx.c',
  include_directories: [ rootInclude, libscriptfuInclude, ],
  dependencies: [
    glib,
  ],
  c_args: [
    '-DUSE_INTERFACE=1',
  ],
  install: false,
)
