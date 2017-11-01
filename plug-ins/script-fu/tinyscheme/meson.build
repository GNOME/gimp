
scriptfu_tinyscheme = static_library('scriptfu-tinyscheme',
  'scheme.c',
  include_directories: [ rootInclude, ],
  dependencies: [
    glib,
  ],
  c_args: [
    '-DSTANDALONE=0',
    '-DUSE_ASCII_NAMES=0',
    '-DUSE_INTERFACE=1',
    '-DUSE_MATH=1',
    '-DUSE_STRLWR=0',
  ],
  install: false,
)
