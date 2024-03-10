## Logo used in About dialog ##

# Always install the main logo. We actually use the variant with a shadow so
# that it always displays properly, even in the extreme case where the
# background was using Wilber's main color.
logo_png = custom_target('gimp-logo.png',
                         input : 'gimp-logo-shadow.svg',
                         output: 'gimp-logo.png',
                         command: [
                           colorsvg2png, '@INPUT@', '@OUTPUT@', '128',
                         ],
                         install: true,
                         install_dir: gimpdatadir / 'images')

if not stable
  logo_png = custom_target('gimp-devel-logo.png',
                           input : 'gimp-devel-logo.svg',
                           output: 'gimp-devel-logo.png',
                           command: [
                             colorsvg2png, '@INPUT@', '@OUTPUT@', '128',
                           ],
                           install: true,
                           install_dir: gimpdatadir / 'images')

endif

## Logo used in API reference documentation ##

if gi_docgen.found()
  install_symlink(fs.name(logo_png.full_path()),
                  install_dir: get_option('datadir') / 'doc' / 'gimp-@0@'.format(gimp_app_version) / 'images',
                  pointing_to: logo_png.full_path())
endif

## Logo used in the desktop (system menus/overview, title bar, etc.) ##

desktop_sizes = [
  '16',
  '22',
  '24',
  '32',
  '36',
  '48',
  '64',
  '72',
  '96',
  '128',
  '192',
  '256',
  '512',
]

foreach size : desktop_sizes
  sizedir = size + 'x' + size
  installdir = get_option('datadir') / 'icons' / 'hicolor' / sizedir / 'apps'
  app_icon = custom_target('gimp' + sizedir + '.png',
                           input : 'gimp-logo.svg',
                           output: 'gimp' + sizedir + '.png',
                           command: [
                             colorsvg2png, '@INPUT@', '@OUTPUT@', size,
                           ],
                           build_by_default: true,
                           install: false)
  meson.add_install_script('sh', '-c',
                           'mkdir -p "$MESON_INSTALL_DESTDIR_PREFIX/@1@/" && cp -f "@0@" "$MESON_INSTALL_DESTDIR_PREFIX/@1@/gimp.png"'.format(app_icon.full_path(), installdir))
endforeach

install_data('gimp-logo.svg',
             install_dir: get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
             rename: 'gimp.svg')

## gimp.ico (Windows) ##

build_gimp_ico = get_option('windows-installer')

if build_gimp_ico
  gen_gimp_ico_conf = configuration_data()
  gen_gimp_ico_conf.set('PARENT_TOP_SRCDIR', meson.global_source_root())
  gen_gimp_ico_conf.set('SRCDIR', meson.current_source_dir())
  gen_gimp_ico_py = configure_file(input : 'gen-gimp-ico.py.in',
                                   output : 'gen-gimp-ico.py',
                                   configuration : gen_gimp_ico_conf)

  # `gimp.ico` is used in gimp.rc compiled in GIMP for Windows.
  # Since it is a circular dependency (GIMP needs it on Windows and the icon
  # needs GIMP to be created), we committed gimp.ico into this repository and
  # this target is made optional. It must be built explicitly by maintainers as
  # part of the procedure when updating the logo.
  gimp_ico = custom_target('gimp.ico',
                           input : [ gen_gimp_ico_py ],
                           depend_files: [ 'gimp-logo.svg' ],
                           output: [ 'gimp.ico', ],
                           command: [ gimp_exe,'-nidfs',
                                      '--batch-interpreter', 'python-fu-eval',
                                      '-b', '-', '--quit'],
                           feed: true,
                           build_by_default: false,
                           env: env)
endif

## gimp.icns (macOS) ##

gen_gimp_icns_conf = configuration_data()
gen_gimp_icns_conf.set('BUILDDIR', meson.current_build_dir())
gen_gimp_icns_conf.set('SRCDIR', meson.current_source_dir())
gen_gimp_icns_py = configure_file(input : 'gen-gimp-icns.py.in',
                                 output : 'gen-gimp-icns.py',
                                 configuration : gen_gimp_icns_conf)

gimp_ico = custom_target('gimp.icns',
                         input : [ gen_gimp_icns_py ],
                         depend_files: [ 'gimp-logo.svg' ],
                         output: [ 'gimp.icns' ],
                         command: [ gimp_exe,'-nidfs',
                                    '--batch-interpreter', 'python-fu-eval',
                                    '-b', '-', '--quit'],
                         feed: true,
                         build_by_default: false,
                         env: env)
