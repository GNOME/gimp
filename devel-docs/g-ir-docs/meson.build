foreach lang : [ 'python', 'gjs' ]

  # XXX meson does not allow building into subdir:
  # https://github.com/mesonbuild/meson/issues/2320
  # Otherwise I could use '-o', '@OUTDIR@' into following commands if
  # the `output` was subdir-able.
  gir_docs_dir = custom_target('g-ir-docs-' + lang + '-dir',
    depends: [ libligma_gir, libligmaui_gir ],
    input: [ ],
    output: [ 'gir-' + lang + '-dirs' ],
    command: [
      'mkdir', '-p',
      '@OUTDIR@' + '/pages/' + lang + '/LigmaUi-' + ligma_api_version,
      '@OUTDIR@' + '/pages/' + lang + '/Ligma-' + ligma_api_version,
      '@OUTDIR@' + '/html/' + lang + '/LigmaUi-' + ligma_api_version,
      '@OUTDIR@' + '/html/' + lang + '/Ligma-' + ligma_api_version
    ],
    build_by_default: true)

  ## Ligma Module ##

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
  gir_docs_pages = custom_target('g-ir-Ligma-' + lang + '-pages',
    depends: [ gir_docs_dir, libligma_gir ],
    input: [ libligma_gir[0] ],
    output: [ 'Ligma-' + lang + '-pages' ],
    command: [
      gir_doc_tool,
      '-I', prefix / 'share/gir-1.0/',
      '-I', meson.project_build_root() / 'libligma',
      '--language=' + lang,
      '-o', '@OUTDIR@' + '/pages/' + lang + '/Ligma-' + ligma_api_version,
      meson.project_build_root() / '@INPUT@'
    ],
    build_by_default: true)

  # This step is completely an ugly workaround for 2 tool issues. The
  # first issue is that g-ir-doc-tool generates invalid XML by not
  # transforming less-than signs into entities. So I am special-casing
  # the one API documentation where we need to write a less-than (it will
  # need to be updated if this happens again).
  # See LIGMA issue #7685.
  # The second issue is in meson itself which transforms backslash into
  # slashes preventing to write most basic regexp in a 'command'. For
  # this reason, I need to add the sed command as an external script.
  # See meson issue #1564.
  docs_pages_fix_sh = find_program('docs_pages_fix.sh')
  gir_docs_pages_fix = custom_target('g-ir-Ligma-' + lang + '-pages-fix',
    input: [ gir_docs_pages ],
    output: [ 'Ligma-' + lang + '-pages-fix' ],
    command: [
      docs_pages_fix_sh,
      '@OUTDIR@' + '/pages/' + lang + '/Ligma-' + ligma_api_version + '/Ligma.Procedure.add_menu_path.page',
      '@OUTDIR@' + '/pages/' + lang + '/Ligma-' + ligma_api_version + '/Ligma.checks_get_colors.page',
    ],
    build_by_default: true)

  gir_docs_cache = custom_target('g-ir-Ligma-' + lang + '-cache',
    input: [ gir_docs_pages_fix ],
    output: [ 'Ligma-' + lang + '-cache' ],
    command: [
      yelp_build, 'cache',
      '-o', '@OUTDIR@' + '/pages/' + lang + '/Ligma-' + ligma_api_version + '/index.cache',
      '@OUTDIR@' + '/pages/' + lang + '/Ligma-' + ligma_api_version,
    ],
    build_by_default: true)

  gir_docs_html = custom_target('g-ir-Ligma-' + lang + '-html',
    input: [ gir_docs_cache ],
    output: [ 'Ligma-' + lang + '-html' ],
    # TODO: `output` needs to be complete for installation to work. So
    # we need to figure out how to install the generated files (listing
    # all of them is a crazy idea, but maybe we can generate the
    # expected list somehow? Maybe even using the .def file which is
    # already an exhaustive listing would be a good idea?
    # Also where should we install exactly?
    #install_dir: prefix / ligmadatadir / 'g-ir-docs/html/' + lang + '/Ligma',
    #install: true,
    command: [
      'yelp-build', 'html',
      '-o', '@OUTDIR@' + '/html/' + lang + '/Ligma-' + ligma_api_version,
      '@OUTDIR@' + '/pages/' + lang + '/Ligma-' + ligma_api_version,
    ],
    build_by_default: true)

  ## LigmaUi module ##

  gir_ui_docs_pages = custom_target('g-ir-LigmaUi-' + lang + '-pages',
    depends: [ gir_docs_dir, libligmaui_gir ],
    input: [ libligmaui_gir[0] ],
    output: [ 'LigmaUi-' + lang + '-pages' ],
    command: [
      gir_doc_tool,
      '-I', prefix / 'share/gir-1.0/',
      '-I', meson.project_build_root() / 'libligma',
      '--language=' + lang,
      '-o', '@OUTDIR@' + '/pages/' + lang + '/LigmaUi-' + ligma_api_version,
      meson.project_build_root() / '@INPUT@'
    ],
    build_by_default: true)

  gir_ui_docs_cache = custom_target('g-ir-LigmaUi-' + lang + '-cache',
    input: [ gir_ui_docs_pages ],
    output: [ 'LigmaUi-' + lang + '-cache' ],
    command: [
      yelp_build, 'cache',
      '-o', '@OUTDIR@' + '/pages/' + lang + '/LigmaUi-' + ligma_api_version + '/index.cache',
      '@OUTDIR@' + '/pages/' + lang + '/LigmaUi-' + ligma_api_version,
    ],
    build_by_default: true)

  gir_ui_docs_html = custom_target('g-ir-LigmaUi-' + lang + '-html',
    input: [ gir_ui_docs_cache ],
    output: [ 'LigmaUi-' + lang + '-html' ],
    #install_dir: prefix / ligmadatadir / 'g-ir-docs/html/' + lang + '/LigmaUi',
    #install: true,
    command: [
      'yelp-build', 'html',
      '-o', '@OUTDIR@' + '/html/' + lang + '/LigmaUi-' + ligma_api_version,
      '@OUTDIR@' + '/pages/' + lang + '/LigmaUi-' + ligma_api_version,
    ],
    build_by_default: true)

endforeach

## TODO: a unit test using yelp-check would be useful.
