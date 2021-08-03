foreach lang : [ 'python', 'gjs' ]

  # XXX meson does not allow building into subdir:
  # https://github.com/mesonbuild/meson/issues/2320
  # Otherwise I could use '-o', '@OUTDIR@' into following commands if
  # the `output` was subdir-able.
  gir_docs_dir = custom_target('g-ir-docs-' + lang + '-dir',
    depends: [ libgimp_gir, libgimpui_gir ],
    input: [ ],
    output: [ 'gir-' + lang + '-dirs' ],
    command: [
      'mkdir', '-p',
      '@OUTDIR@' + '/pages/' + lang + '/GimpUi-' + gimp_api_version,
      '@OUTDIR@' + '/pages/' + lang + '/Gimp-' + gimp_api_version,
      '@OUTDIR@' + '/html/' + lang + '/GimpUi-' + gimp_api_version,
      '@OUTDIR@' + '/html/' + lang + '/Gimp-' + gimp_api_version
    ],
    build_by_default: true)

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
  gir_docs_pages = custom_target('g-ir-Gimp-' + lang + '-pages',
    depends: [ gir_docs_dir, libgimp_gir ],
    input: [ libgimp_gir[0] ],
    output: [ 'Gimp-' + lang + '-pages' ],
    command: [
      gir_doc_tool,
      '-I', prefix / 'share/gir-1.0/',
      '-I', meson.build_root() / 'libgimp',
      '--language=' + lang,
      '-o', '@OUTDIR@' + '/pages/' + lang + '/Gimp-' + gimp_api_version,
      meson.build_root() / '@INPUT@'
    ],
    build_by_default: true)

  ## Gimp Module ##

  gir_docs_cache = custom_target('g-ir-Gimp-' + lang + '-cache',
    input: [ gir_docs_pages ],
    output: [ 'Gimp-' + lang + '-cache' ],
    command: [
      yelp_build, 'cache',
      '-o', '@OUTDIR@' + '/pages/' + lang + '/Gimp-' + gimp_api_version + '/index.cache',
      '@OUTDIR@' + '/pages/' + lang + '/Gimp-' + gimp_api_version,
    ],
    build_by_default: true)

  gir_docs_html = custom_target('g-ir-Gimp-' + lang + '-html',
    input: [ gir_docs_cache ],
    output: [ 'Gimp-' + lang + '-html' ],
    # TODO: `output` needs to be complete for installation to work. So
    # we need to figure out how to install the generated files (listing
    # all of them is a crazy idea, but maybe we can generate the
    # expected list somehow? Maybe even using the .def file which is
    # already an exhaustive listing would be a good idea?
    # Also where should we install exactly?
    #install_dir: prefix / gimpdatadir / 'g-ir-docs/html/' + lang + '/Gimp',
    #install: true,
    command: [
      'yelp-build', 'html',
      '-o', '@OUTDIR@' + '/html/' + lang + '/Gimp-' + gimp_api_version,
      '@OUTDIR@' + '/pages/' + lang + '/Gimp-' + gimp_api_version,
    ],
    build_by_default: true)

  ## GimpUi module ##

  gir_ui_docs_pages = custom_target('g-ir-GimpUi-' + lang + '-pages',
    depends: [ gir_docs_dir, libgimpui_gir ],
    input: [ libgimpui_gir[0] ],
    output: [ 'GimpUi-' + lang + '-pages' ],
    command: [
      gir_doc_tool,
      '-I', prefix / 'share/gir-1.0/',
      '-I', meson.build_root() / 'libgimp',
      '--language=' + lang,
      '-o', '@OUTDIR@' + '/pages/' + lang + '/GimpUi-' + gimp_api_version,
      meson.build_root() / '@INPUT@'
    ],
    build_by_default: true)

  gir_ui_docs_cache = custom_target('g-ir-GimpUi-' + lang + '-cache',
    input: [ gir_ui_docs_pages ],
    output: [ 'GimpUi-' + lang + '-cache' ],
    command: [
      yelp_build, 'cache',
      '-o', '@OUTDIR@' + '/pages/' + lang + '/GimpUi-' + gimp_api_version + '/index.cache',
      '@OUTDIR@' + '/pages/' + lang + '/GimpUi-' + gimp_api_version,
    ],
    build_by_default: true)

  gir_ui_docs_html = custom_target('g-ir-GimpUi-' + lang + '-html',
    input: [ gir_ui_docs_cache ],
    output: [ 'GimpUi-' + lang + '-html' ],
    #install_dir: prefix / gimpdatadir / 'g-ir-docs/html/' + lang + '/GimpUi',
    #install: true,
    command: [
      'yelp-build', 'html',
      '-o', '@OUTDIR@' + '/html/' + lang + '/GimpUi-' + gimp_api_version,
      '@OUTDIR@' + '/pages/' + lang + '/GimpUi-' + gimp_api_version,
    ],
    build_by_default: true)

endforeach

## TODO: a unit test using yelp-check would be useful.
