devel_docs_build_root = meson.current_build_dir()

scan_args_common = [
  '--deprecated-guards=LIGMA_DISABLE_DEPRECATED',
]

mkdb_args_common = [
  '--name-space=ligma',
]

if gi_docgen.found()
  subdir('reference')
endif

if get_option('g-ir-doc')
  subdir('g-ir-docs')
endif
