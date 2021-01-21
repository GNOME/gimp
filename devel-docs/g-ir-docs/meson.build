if gir_doc_tool.found() and yelp_build.found()

  # XXX meson does not allow building into subdir:
  # https://github.com/mesonbuild/meson/issues/2320
  # Otherwise I could use '-o', '@OUTDIR@' into following commands if
  # the `output` was subdir-able.
  gir_docs_dir = custom_target('g-ir-docs-dir',
    depends: [ libgimp_gir, libgimpui_gir ],
    input: [ ],
    output: [ 'html-dirs' ],
    command: [
      'mkdir', '-p',
      '@OUTDIR@' + '/html/Python/GimpUi-' + gimp_api_version,
      '@OUTDIR@' + '/html/Python/Gimp-' + gimp_api_version
    ],
    build_by_default: false)

  # XXX `output` is bogus. g-ir-doc-tool produces a lot of output,
  # basically one page per public symbol, which is more than 1000 so
  # it's not usable. Since custom_target() requires an 'output', I could
  # just set one output such as 'index.page` as a trick, but since we
  # have another issue on subdir anyway (cf. above), I use some bogus
  # file instead. The fact the bogus file is not even created does not
  # even seem to be a problem for meson.
  # Moreover I realized that the targets listed by ninja are only the
  # output files (not the target name), so I basically ends up using
  # this field to create understandable names).
  gir_docs_python = custom_target('g-ir-Gimp-python-pages',
    depends: [ gir_docs_dir, libgimp_gir ],
    input: [ libgimp_gir[0] ],
    output: [ 'Gimp-python-pages' ],
    command: [
      gir_doc_tool,
      '-I', prefix / 'share/gir-1.0/',
      '-I', meson.build_root() / 'libgimp',
      '--language=Python',
      '-o', '@OUTDIR@' + '/html/Python/Gimp-' + gimp_api_version,
      '@INPUT@'
    ],
    build_by_default: false)

  ## Gimp Module ##

  gir_cache_python = custom_target('g-ir-Gimp-python-cache',
    input: [ gir_docs_python ],
    output: [ 'Gimp-python-cache' ],
    command: [
      yelp_build, 'cache',
      '-o', '@OUTDIR@' + '/html/Python/Gimp-' + gimp_api_version + '/index.cache',
      '@OUTDIR@' + '/html/Python/Gimp-' + gimp_api_version,
    ],
    build_by_default: false)

  gir_html_python = custom_target('g-ir-Gimp-python-html',
    input: [ gir_cache_python ],
    output: [ 'Gimp-python-html' ],
    # TODO: `output` needs to be complete for installation to work. So
    # we need to figure out how to install the generated files (listing
    # all of them is a crazy idea, but maybe we can generate the
    # expected list somehow? Maybe even using the .def file which is
    # already an exhaustive listing would be a good idea?
    # Also where should we install exactly?
    #install_dir: prefix / gimpdatadir / 'g-ir-docs/html/Python/Gimp',
    #install: true,
    command: [
      'yelp-build', 'html',
      '-o', '@OUTDIR@' + '/html/Python/Gimp-' + gimp_api_version,
      '@OUTDIR@' + '/html/Python/Gimp-' + gimp_api_version,
    ])

  ## GimpUi module ##

  gir_docs_python = custom_target('g-ir-GimpUi-python-pages',
    depends: [ gir_docs_dir, libgimpui_gir ],
    input: [ libgimpui_gir[0] ],
    output: [ 'GimpUi-python-pages' ],
    command: [
      gir_doc_tool,
      '-I', prefix / 'share/gir-1.0/',
      '-I', meson.build_root() / 'libgimp',
      '--language=Python',
      '-o', '@OUTDIR@' + '/html/Python/GimpUi-' + gimp_api_version,
      '@INPUT@'
    ],
    build_by_default: false)

  gir_cache_python = custom_target('g-ir-GimpUi-python-cache',
    input: [ gir_docs_python ],
    output: [ 'GimpUi-python-cache' ],
    command: [
      yelp_build, 'cache',
      '-o', '@OUTDIR@' + '/html/Python/GimpUi-' + gimp_api_version + '/index.cache',
      '@OUTDIR@' + '/html/Python/GimpUi-' + gimp_api_version,
    ],
    build_by_default: false)

  gir_html_python = custom_target('g-ir-GimpUi-python-html',
    input: [ gir_cache_python ],
    output: [ 'GimpUi-python-html' ],
    #install_dir: prefix / gimpdatadir / 'g-ir-docs/html/Python/GimpUi',
    #install: true,
    command: [
      'yelp-build', 'html',
      '-o', '@OUTDIR@' + '/html/Python/GimpUi-' + gimp_api_version,
      '@OUTDIR@' + '/html/Python/GimpUi-' + gimp_api_version,
    ])

  ## TODO: a unit test using yelp-check would be useful.

endif
