
# No include_directories('.') here; use libscriptfuInclude

scriptfu_tinyscheme = static_library('scriptfu-tinyscheme',
  'scheme.c',
  'string-port.c',
  'error-port.c',
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
