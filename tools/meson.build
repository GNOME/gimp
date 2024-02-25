colorsvg2png = executable('colorsvg2png',
  'colorsvg2png.c',
  native: true,
  dependencies: [
    native_glib,
    native_rsvg
  ],
  # In case b_sanitize was set, we don't really care if the tool has issues (in
  # particular we experienced some memory leaks with b_sanitize=address, within
  # librsvg and we don't want this to break the build).
  override_options: [ 'b_sanitize=none' ],
  install: false,
)
