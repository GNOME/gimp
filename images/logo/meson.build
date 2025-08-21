gen_conf = configuration_data()
gen_conf.set('PARENT_TOP_SRCDIR', meson.global_source_root().replace('\\', '/'))
gen_conf.set('SRCDIR', meson.current_source_dir().replace('\\', '/'))
gen_conf.set('BUILDDIR', meson.current_build_dir().replace('\\', '/'))


################################################################################
# PLATFORM-NEUTRAL ICONS

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

if not stable or gi_docgen.found()
  devel_logo = custom_target('gimp-devel-logo.png',
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
  installed_image = gimpdatadir / 'images' / fs.name(devel_logo.full_path())
  if not fs.is_absolute(installed_image)
    installed_image = prefix / installed_image
  endif

  install_symlink(fs.name(devel_logo.full_path()),
                  install_dir: get_option('datadir') / 'doc' / 'gimp-@0@'.format(gimp_app_version) / 'images',
                  pointing_to: installed_image)
endif


################################################################################
# LINUX ICONS

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
  meson.add_install_script(python, '-c',
                           'from pathlib import Path; from shutil import copy2; import sys, os; target_dir = Path(os.environ.get("MESON_INSTALL_DESTDIR_PREFIX")) / sys.argv[2]; target_dir.mkdir(parents=True, exist_ok=True); copy2(sys.argv[1], target_dir / "gimp.png")', app_icon.full_path(), installdir)
endforeach

install_data('gimp-logo.svg',
             install_dir: get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
             rename: 'gimp.svg')


################################################################################
# WINDOWS ICONS

## Icons used by the .exe installer package ##
if get_option('windows-installer')
  # `.ico` files are used in gimp.rc and gimp-plug-ins.rc compiled in GIMP for
  # Windows. Since these are circular dependencies (GIMP needs them on Windows
  # and the icons need GIMP to be created), we committed both ico files into
  # this repository and this target is made optional. It must be built
  # explicitly by maintainers as part of the procedure when updating the logo.

  ## Icon for gimp.exe
  gen_gimp_ico_py = configure_file(input : 'gen-gimp-ico.py.in',
                                   output : 'gen-gimp-ico.py',
                                   configuration : gen_conf)
  gimp_ico = custom_target('gimp.ico',
                           input : [ gen_gimp_ico_py ],
                           depend_files: [ 'gimp-logo.svg' ],
                           depends: [ gimp_exe_depends ],
                           output: [ 'gimp.ico', ],
                           command: [ gimp_exe,'-nidfs',
                                      '--batch-interpreter', 'python-fu-eval',
                                      '-b', '-', '--quit'],
                           feed: true,
                           build_by_default: false,
                           env: gimp_run_env)


  ## Icon for .xcf and other fileicon
  gen_fileicon_ico_conf = configuration_data()
  gen_fileicon_ico_conf.set('OUTPUT_TYPE','ico')
  gen_fileicon_ico_conf.merge_from(gen_conf)
  gen_fileicon_ico_py = configure_file(input : 'gen-fileicon.py.in',
                                       output : 'gen-fileicon-ico.py',
                                       configuration : gen_fileicon_ico_conf)
  fileicon_ico = custom_target('fileicon.ico',
                               input : [ gen_fileicon_ico_py ],
                               depend_files: [ 'gimp-logo-shadow.svg', 'fileicon-base-xcf.svg', 'fileicon-base.svg' ],
                               depends: [ gimp_exe_depends ],
                               output: [ 'stamp-fileicon.ico', ],
                               command: [ gimp_exe,'-nidfs',
                                          '--batch-interpreter', 'python-fu-eval',
                                          '-b', '-', '--quit'],
                               feed: true,
                               build_by_default: false,
                               env: gimp_run_env)


  ## Similarly `plug-ins.ico` is used in gimp-plug-ins.rc.
  gen_plug_ins_ico_py = configure_file(input : 'gen-plug-ins-ico.py.in',
                                       output : 'gen-plug-ins-ico.py',
                                       configuration : gen_conf)
  fileicon_ico = custom_target('plug-ins.ico',
                               input : [ gen_plug_ins_ico_py ],
                               depend_files: [ 'gimp-logo-shadow.svg', 'plug-ins-base.svg' ],
                               depends: [ gimp_exe_depends ],
                               output: [ 'plug-ins.ico', ],
                               command: [ gimp_exe,'-nidfs',
                                          '--batch-interpreter', 'python-fu-eval',
                                          '-b', '-', '--quit'],
                               feed: true,
                               build_by_default: false,
                               env: gimp_run_env)


  ## Inno installer: setup .exe icon
  gen_setup_ico = configure_file(input : 'gen-setup-ico.py.in',
                                 output : 'gen-setup-ico.py',
                                 configuration : gen_conf)
  setup_ico = custom_target('setup.ico',
                            input : [ gen_setup_ico ],
                            depend_files: [ 'setup-base.svg', 'gimp-logo-shadow.svg', 'setup-hover.svg' ],
                            depends: [ gimp_exe_depends ],
                            output: [ 'setup.ico', ],
                            command: [ gimp_exe,'-nidfs',
                                       '--batch-interpreter', 'python-fu-eval',
                                       '-b', '-', '--quit'],
                            feed: true,
                            build_by_default: true,
                            env: gimp_run_env)


  ## Inno installer: right-top icon (Windows) ##
  gen_gimp_bmp = configure_file(input : 'gen-gimp-bmp.py.in',
                                output : 'gen-gimp-bmp.py',
                                configuration : gen_conf)
  installicon = custom_target('installicon.bmp',
                               input : [ gen_gimp_bmp ],
                               output: [ 'stamp-installicon.bmp', ],
                               depends: [ gimp_exe_depends ],
                               command: [ gimp_exe, '-nidfs',
                                          '--batch-interpreter', 'python-fu-eval',
                                          '-b', '-', '--quit'],
                               feed: true,
                               build_by_default: true,
                               env: gimp_run_env)
endif


if get_option('ms-store')
  ## MS Store: AppList/Square44x44Logo (Windows) ##
  icon_name = 'AppList'
  gen_icon_conf = configuration_data()
  gen_icon_conf.set('NAME',     icon_name)
  gen_icon_conf.set('TILE_WIDTH',  'none')
  gen_icon_conf.set('TILE_HEIGHT', 'none')
  gen_icon_conf.set('LOGO_SIZE',       44)
  gen_icon_conf.merge_from(gen_conf)
  gen_applist_py = configure_file(input : 'gen-gimp-png.py.in',
                                  output : 'gen-' + icon_name + '.py',
                                  configuration : gen_icon_conf)
  applist = custom_target(icon_name,
                          input : [ gen_applist_py ],
                          depend_files: [ 'gimp-logo.svg' ],
                          depends: [ gimp_exe_depends ],
                          output: [ 'stamp-' + icon_name + '.png' ],
                          command: [ gimp_exe, '-nidfs',
                                     '--batch-interpreter', 'python-fu-eval',
                                     '-b', '-', '--quit'],
                          feed: true,
                          build_by_default: true,
                          env: gimp_run_env)


  ## MS Store: StoreLogo (Windows) ##
  icon_name = 'StoreLogo'
  gen_icon_conf = configuration_data()
  gen_icon_conf.set('NAME',     icon_name)
  gen_icon_conf.set('TILE_WIDTH',  'none')
  gen_icon_conf.set('TILE_HEIGHT', 'none')
  gen_icon_conf.set('LOGO_SIZE',       50)
  gen_icon_conf.merge_from(gen_conf)
  gen_storelogo_py = configure_file(input : 'gen-gimp-png.py.in',
                                    output : 'gen-' + icon_name + '.py',
                                    configuration : gen_icon_conf)
  storelogo = custom_target(icon_name,
                            input : [ gen_storelogo_py ],
                            depend_files: [ 'gimp-logo.svg' ],
                            depends: [ gimp_exe_depends ],
                            output: [ 'stamp-' + icon_name + '.png' ],
                            command: [ gimp_exe, '-nidfs',
                                       '--batch-interpreter', 'python-fu-eval',
                                       '-b', '-', '--quit'],
                            feed: true,
                            build_by_default: true,
                            env: gimp_run_env)


  ## MS Store: MedTile/Square150x150Logo (Windows) ##
  icon_name = 'MedTile'
  gen_tile_conf = configuration_data()
  gen_tile_conf.set('NAME',  icon_name)
  gen_tile_conf.set('TILE_WIDTH',  150)
  gen_tile_conf.set('TILE_HEIGHT', 150)
  ## (The wilber size should be 32% of the tile height)
  gen_tile_conf.set('LOGO_SIZE',    48)
  gen_tile_conf.merge_from(gen_conf)
  gen_medtile_py = configure_file(input : 'gen-gimp-png.py.in',
                                  output : 'gen-' + icon_name + '.py',
                                  configuration : gen_tile_conf)
  med_tile = custom_target(icon_name,
                           input : [ gen_medtile_py ],
                           depend_files: [ 'gimp-logo-shadow.svg' ],
                           depends: [ gimp_exe_depends ],
                           output: [ 'stamp-' + icon_name + '.png' ],
                           command: [ gimp_exe, '-nidfs',
                                      '--batch-interpreter', 'python-fu-eval',
                                      '-b', '-', '--quit'],
                           feed: true,
                           build_by_default: true,
                           env: gimp_run_env)


  ## MS Store: SmallTile/Square71x71Logo (Windows 10 only) ##
  icon_name = 'SmallTile'
  gen_tile_conf = configuration_data()
  gen_tile_conf.set('NAME', icon_name)
  gen_tile_conf.set('TILE_WIDTH',  71)
  gen_tile_conf.set('TILE_HEIGHT', 71)
  ## (The wilber size should be 50% of the tile height)
  gen_tile_conf.set('LOGO_SIZE',   36)
  gen_tile_conf.merge_from(gen_conf)
  gen_smalltile_py = configure_file(input : 'gen-gimp-png.py.in',
                                    output : 'gen-' + icon_name + '.py',
                                    configuration : gen_tile_conf)
  small_tile = custom_target(icon_name,
                             input : [ gen_smalltile_py ],
                             depend_files: [ 'gimp-logo-shadow.svg' ],
                             depends: [ gimp_exe_depends ],
                             output: [ 'stamp-' + icon_name + '.png' ],
                             command: [ gimp_exe, '-nidfs',
                                        '--batch-interpreter', 'python-fu-eval',
                                        '-b', '-', '--quit'],
                             feed: true,
                             build_by_default: true,
                             env: gimp_run_env)


  ## MS Store: WideTile/Wide310x150Logo (Windows 10 only) ##
  icon_name = 'WideTile'
  gen_tile_conf = configuration_data()
  gen_tile_conf.set('NAME',  icon_name)
  gen_tile_conf.set('TILE_WIDTH',  310)
  gen_tile_conf.set('TILE_HEIGHT', 150)
  ## (The wilber size should be 32% of the tile height)
  gen_tile_conf.set('LOGO_SIZE',    48)
  gen_tile_conf.merge_from(gen_conf)
  gen_widetile_py = configure_file(input : 'gen-gimp-png.py.in',
                                   output : 'gen-' + icon_name + '.py',
                                   configuration : gen_tile_conf)
  wide_tile = custom_target(icon_name,
                            input : [ gen_widetile_py ],
                            depend_files: [ 'gimp-logo-shadow.svg' ],
                            depends: [ gimp_exe_depends ],
                            output: [ 'stamp-' + icon_name + '.png' ],
                            command: [ gimp_exe, '-nidfs',
                                       '--batch-interpreter', 'python-fu-eval',
                                       '-b', '-', '--quit'],
                            feed: true,
                            build_by_default: true,
                            env: gimp_run_env)


  ## MS Store: LargeTile/Square310x310Logo (Windows 10 only) ##
  icon_name = 'LargeTile'
  gen_tile_conf = configuration_data()
  gen_tile_conf.set('NAME',  icon_name)
  gen_tile_conf.set('TILE_WIDTH',  310)
  gen_tile_conf.set('TILE_HEIGHT', 310)
  ## (The wilber size should be 34% of the tile height)
  gen_tile_conf.set('LOGO_SIZE',   105)
  gen_tile_conf.merge_from(gen_conf)
  gen_largetile_py = configure_file(input : 'gen-gimp-png.py.in',
                                    output : 'gen-' + icon_name + '.py',
                                    configuration : gen_tile_conf)
  large_tile = custom_target(icon_name,
                             input : [ gen_largetile_py ],
                             depend_files: [ 'gimp-logo-shadow.svg' ],
                             depends: [ gimp_exe_depends ],
                             output: [ 'stamp-' + icon_name + '.png' ],
                             command: [ gimp_exe, '-nidfs',
                                        '--batch-interpreter', 'python-fu-eval',
                                        '-b', '-', '--quit'],
                             feed: true,
                             build_by_default: true,
                             env: gimp_run_env)


  ## fileicon icon (for XCF and other filetypes) ##
  gen_fileicon_png_conf = configuration_data()
  gen_fileicon_png_conf.set('OUTPUT_TYPE','png')
  gen_fileicon_png_conf.merge_from(gen_conf)
  gen_fileicon_png_py = configure_file(input : 'gen-fileicon.py.in',
                                       output : 'gen-fileicon-png.py',
                                       configuration : gen_fileicon_png_conf)
  custom_target('fileicon.png',
                input : [ gen_fileicon_png_py ],
                depend_files: [ 'gimp-logo-shadow.svg', 'fileicon-base-xcf.svg', 'fileicon-base.svg' ],
                depends: [ gimp_exe_depends ],
                output: [ 'stamp-fileicon.png' ],
                command: [ gimp_exe, '-nidfs',
                           '--batch-interpreter', 'python-fu-eval',
                           '-b', '-', '--quit'],
                feed: true,
                build_by_default: true,
                env: gimp_run_env)
endif


################################################################################
# MACOS ICONS
# FIXME: All macOS icons are built separately, one at time. Building them
# from Meson would cause parallelism clashes due to install_name_tool on
# tools/in-build-gimp.py (check that file on gimp repo for more info).

if platform_osx
  ## gimp.icns (macOS) ##
  gen_gimp_icns_py = configure_file(input : 'gen-gimp-icns.py.in',
                                   output : 'gen-gimp-icns.py',
                                   configuration : gen_conf)
  gimp_icns = custom_target('gimp.icns',
                            input : [ gen_gimp_icns_py ],
                            depend_files: [ 'gimp-logo.svg' ],
                            depends: [ gimp_exe_depends ],
                            output: [ 'gimp.icns' ],
                            command: [ gimp_exe,'-nidfs',
                                       '--batch-interpreter', 'python-fu-eval',
                                       '-b', '-', '--quit'],
                            feed: true,
                            build_by_default: false,
                            env: gimp_run_env)


  ## fileicon*.icns (macOS) ##
  gen_fileicon_icns_conf = configuration_data()
  gen_fileicon_icns_conf.set('OUTPUT_TYPE','icns')
  gen_fileicon_icns_conf.merge_from(gen_conf)
  gen_fileicon_icns_py = configure_file(input : 'gen-fileicon.py.in',
                                       output : 'gen-fileicon-icns.py',
                                       configuration : gen_fileicon_icns_conf)
  custom_target('fileicon.icns',
                input : [ gen_fileicon_icns_py ],
                depend_files: [ 'gimp-logo-shadow.svg', 'fileicon-base-xcf.svg', 'fileicon-base.svg' ],
                depends: [ gimp_exe_depends ],
                output: [ 'stamp-fileicon.icns' ],
                command: [ gimp_exe, '-nidfs',
                           '--batch-interpreter', 'python-fu-eval',
                           '-b', '-', '--quit'],
                feed: true,
                build_by_default: false,
                env: gimp_run_env)


  ## gimp-dmg.png (macOS) ##
  gen_gimp_dmg_py = configure_file(input : 'gen-gimp-dmg.py.in',
                                   output : 'gen-gimp-dmg.py',
                                   configuration : gen_conf)
  gimp_dmg = custom_target('gimp-dmg.png',
                           input : [ gen_gimp_dmg_py ],
                           depend_files: [ 'gimp-logo-shadow.svg', '../macos/gimp-dmg.xcf.xz' ],
                           depends: [ gimp_exe_depends ],
                           output: [ 'gimp-dmg.png' ],
                           command: [ gimp_exe,'-nidfs',
                                      '--batch-interpreter', 'python-fu-eval',
                                      '-b', '-', '--quit'],
                           feed: true,
                           build_by_default: false,
                           env: gimp_run_env)
endif
