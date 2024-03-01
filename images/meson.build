## gimp.ico ##

build_gimp_ico = get_option('windows-installer')

if build_gimp_ico
  gen_gimp_ico_conf = configuration_data()
  gen_gimp_ico_conf.set('PARENT_TOP_SRCDIR', meson.global_source_root())
  gen_gimp_ico_conf.set('SRCDIR', meson.current_source_dir())
  gen_gimp_ico_py = configure_file(
    input : 'gen-gimp-ico.py.in',
    output : 'gen-gimp-ico.py',
    configuration : gen_gimp_ico_conf
  )

  # `gimp.ico` is used in gimp.rc compiled in GIMP for Windows.
  # Since it is a circular dependency (GIMP needs it on Windows and the icon
  # needs GIMP to be created), we committed gimp.ico into this repository and
  # this target is made optional. It must be built explicitly by maintainers as
  # part of the procedure when updating the logo.
  gimp_ico = custom_target('gimp.ico',
    input : [ gen_gimp_ico_py ],
    depend_files: [ meson.global_source_root() / 'desktop/scalable/gimp.svg' ],
    output: [ 'gimp.ico', ],
    # By having 'sh' first instead of gimp_exe_name, we prevent meson to check for
    # the GIMP executable in PATH and fail before installation, since anyway
    # this target is a special-case which won't be built during the main build.
    command: [ 'sh', '-c',
               gimp_exe_name + ' -nidfs --batch-interpreter python-fu-eval -b - --quit'],
    feed: true,
    build_by_default: false,
  )

endif
