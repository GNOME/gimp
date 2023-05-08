devel_docs_build_root = meson.current_build_dir()

scan_args_common = [
  '--deprecated-guards=GIMP_DISABLE_DEPRECATED',
]

mkdb_args_common = [
  '--name-space=gimp',
]

if gi_docgen.found() and have_gobject_introspection
  subdir('reference')
endif

if get_option('g-ir-doc')
  subdir('g-ir-docs')
endif
