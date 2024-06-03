scales = [
  '100',
  '125',
  '150',
  '200',
  '400',
]

sizes = [
  '16',
  '20',
  '24',
  '30',
  '32',
  '36',
  '40',
  '48',
  '60',
  '64',
  '72',
  '80',
  '96',
  '128',
  '256',
]


# UNIVERSAL ICONS (INTRODUCED SINCE WINDOWS 8)
wilber_path = meson.project_source_root() / 'gimp-data/images/logo/' + 'gimp-logo' + '.svg'

## Generate AppList (aka Square44x44Logo) icon
foreach scale : scales
  if scale == '100'
    icon_name = 'AppList.png'
  else
    icon_name = 'AppList.scale-' + scale + '.png'
  endif
  scale_int = scale.to_int()
  size = 44 * scale_int / 100
  size_str = size.to_string()
  custom_target(icon_name,
    input : [ wilber_path ],
    output: [ icon_name ],
    command: [
      colorsvg2png, '@INPUT@', '@OUTPUT@', size_str,
    ],
    build_by_default: true,
  )
endforeach

foreach size : sizes
  icon_name = 'AppList.targetsize-' + size + '.png'
  custom_target(icon_name,
    input : [ wilber_path ],
    output: [ icon_name ],
    command: [
      colorsvg2png, '@INPUT@', '@OUTPUT@', size,
    ],
    build_by_default: true,
  )

  icon_name = 'AppList.targetsize-' + size + '_altform-unplated.png'
  custom_target(icon_name,
    input : [ wilber_path ],
    output: [ icon_name ],
    command: [
      colorsvg2png, '@INPUT@', '@OUTPUT@', size,
    ],
    build_by_default: true,
  )

  icon_name = 'AppList.targetsize-' + size + '_altform-lightunplated.png'
  custom_target(icon_name,
    input : [ wilber_path ],
    output: [ icon_name ],
    command: [
      colorsvg2png, '@INPUT@', '@OUTPUT@', size,
    ],
    build_by_default: true,
  )
endforeach


## Generate StoreLogo icon
foreach scale : scales
if scale == '100'
    icon_name = 'StoreLogo.png'
  else
    icon_name = 'StoreLogo.scale-' + scale + '.png'
  endif
  scale_int = scale.to_int()
  size = 50 * scale_int / 100
  size_str = size.to_string()
  custom_target(icon_name,
    input : [ wilber_path ],
    output: [ icon_name ],
    command: [
      colorsvg2png, '@INPUT@', '@OUTPUT@', size_str,
    ],
    build_by_default: true,
  )
endforeach


# LIMBO ICONS (DISCONTINUED IN WINDOWS 11 BUT MICROSOFT OBLIES DEVS TO SHIP)
wilber_path = meson.project_source_root() / 'gimp-data/images/logo/' + 'gimp-logo-shadow' + '.svg'

## Generate MedTile (aka Square150x150Logo) icon
## (The wilber size should be 32% of the icon height)
foreach scale : scales
  icon_name = 'MedTile.scale-' + scale + '.png'

  scale_int = scale.to_int()
  size = 150 * scale_int / 100
  size_str = size.to_string()

  dim_size = 48 * scale_int / 100
  dim_str = dim_size.to_string()

  gen_tile_conf = configuration_data()
  gen_tile_conf.set('INPUT',      wilber_path)
  gen_tile_conf.set('OUTPUT',     meson.current_build_dir() / icon_name)
  gen_tile_conf.set('IMG_WIDTH',  size_str)
  gen_tile_conf.set('IMG_HEIGHT', size_str)
  gen_tile_conf.set('LOGO_DIM',   dim_str)
  gen_medtile_py = configure_file(input : 'gen-tile.py.in',
                                  output : 'gen-medtile-' + scale + '.py',
                                  configuration : gen_tile_conf)

  med_tile = custom_target(icon_name,
                           input : [ gen_medtile_py ],
                           depend_files: [ wilber_path ],
                           depends: [ gimp_exe_depends ],
                           output: [ icon_name ],
                           command: [ gimp_exe, '-nidfs',
                                      '--batch-interpreter', 'python-fu-eval',
                                      '-b', '-', '--quit'],
                           feed: true,
                           build_by_default: true,
                           env: gimp_run_env)
endforeach


# LEGACY ICONS (DISCONTINUED IN WINDOWS 11)

## Generate SmallTile (aka Square71x71Logo) icon
## (The wilber size should be 50% of the icon height)
foreach scale : scales
  icon_name = 'SmallTile.scale-' + scale + '.png'

  scale_int = scale.to_int()
  size = 71 * scale_int / 100
  size_str = size.to_string()

  dim_size = 36 * scale_int / 100
  dim_str = dim_size.to_string()

  gen_tile_conf = configuration_data()
  gen_tile_conf.set('INPUT',      wilber_path)
  gen_tile_conf.set('OUTPUT',     meson.current_build_dir() / icon_name)
  gen_tile_conf.set('IMG_WIDTH',  size_str)
  gen_tile_conf.set('IMG_HEIGHT', size_str)
  gen_tile_conf.set('LOGO_DIM',   dim_str)
  gen_smalltile_py = configure_file(input : 'gen-tile.py.in',
                                    output : 'gen-smalltile-' + scale + '.py',
                                    configuration : gen_tile_conf)

  small_tile = custom_target(icon_name,
                             input : [ gen_smalltile_py ],
                             depend_files: [ wilber_path ],
                             depends: [ gimp_exe_depends ],
                             output: [ icon_name ],
                             command: [ gimp_exe, '-nidfs',
                                        '--batch-interpreter', 'python-fu-eval',
                                        '-b', '-', '--quit'],
                             feed: true,
                             build_by_default: true,
                             env: gimp_run_env)
endforeach


## Generate LargeTile (aka Square310x310Logo) icon
## (The wilber size should be 34% of the icon height)
foreach scale : scales
  icon_name = 'LargeTile.scale-' + scale + '.png'

  scale_int = scale.to_int()
  size = 310 * scale_int / 100
  size_str = size.to_string()

  dim_size = 105 * scale_int / 100
  dim_str = dim_size.to_string()

  gen_tile_conf = configuration_data()
  gen_tile_conf.set('INPUT',      wilber_path)
  gen_tile_conf.set('OUTPUT',     meson.current_build_dir() / icon_name)
  gen_tile_conf.set('IMG_WIDTH',  size_str)
  gen_tile_conf.set('IMG_HEIGHT', size_str)
  gen_tile_conf.set('LOGO_DIM',   dim_str)
  gen_largetile_py = configure_file(input : 'gen-tile.py.in',
                                    output : 'gen-largetile-' + scale + '.py',
                                    configuration : gen_tile_conf)

  large_tile = custom_target(icon_name,
                             input : [ gen_largetile_py ],
                             depend_files: [ wilber_path ],
                             depends: [ gimp_exe_depends ],
                             output: [ icon_name ],
                             command: [ gimp_exe, '-nidfs',
                                        '--batch-interpreter', 'python-fu-eval',
                                        '-b', '-', '--quit'],
                             feed: true,
                             build_by_default: true,
                             env: gimp_run_env)
endforeach


## Generate WideTile (aka Wide310x150Logo) icon
## (The wilber size should be 32% of the icon height)
foreach scale : scales
  icon_name = 'WideTile.scale-' + scale + '.png'

  scale_int = scale.to_int()
  w_size = 310 * scale_int / 100
  w_size_str = w_size.to_string()

  h_size = 150 * scale_int / 100
  h_size_str = h_size.to_string()

  dim_size = 48 * scale_int / 100
  dim_str = dim_size.to_string()

  gen_tile_conf = configuration_data()
  gen_tile_conf.set('INPUT',      wilber_path)
  gen_tile_conf.set('OUTPUT',     meson.current_build_dir() / icon_name)
  gen_tile_conf.set('IMG_WIDTH',  w_size_str)
  gen_tile_conf.set('IMG_HEIGHT', h_size_str)
  gen_tile_conf.set('LOGO_DIM',   dim_str)
  gen_widetile_py = configure_file(input : 'gen-tile.py.in',
                                   output : 'gen-widetile-' + scale + '.py',
                                   configuration : gen_tile_conf)

  wide_tile = custom_target(icon_name,
                            input : [ gen_widetile_py ],
                            depend_files: [ wilber_path ],
                            depends: [ gimp_exe_depends ],
                            output: [ icon_name ],
                            command: [ gimp_exe, '-nidfs',
                                       '--batch-interpreter', 'python-fu-eval',
                                       '-b', '-', '--quit'],
                            feed: true,
                            build_by_default: true,
                            env: gimp_run_env)
endforeach


# OTHER ASSETS

## fileicon icon (for XCF and other filetypes)
fileicon_path = meson.project_source_root() / 'gimp-data/images/logo/' / 'fileicon-base' + '.svg'
foreach scale : scales
  if scale == '100'
    icon_name = 'fileicon.png'
  else
    icon_name = 'fileicon.scale-' + scale + '.png'
  endif
  scale_int = scale.to_int()
  size = 44 * scale_int / 100
  size_str = size.to_string()
  conf = configuration_data()
  conf.set('TOP_SRCDIR', meson.project_source_root())
  conf.set('OUTPUT',     meson.current_build_dir() / icon_name)
  conf.set('LOGO_DIM',   size_str)
  gen_fileicon_py = configure_file(input : 'gen-fileicon-png.py.in',
                                   output : 'gen-fileicon-png-' + size_str + '.py',
                                   configuration : conf)
  custom_target(icon_name,
                input : [ gen_fileicon_py ],
                depend_files: [ wilber_path, fileicon_path ],
                depends: [ gimp_exe_depends ],
                output: [ icon_name ],
                command: [ gimp_exe, '-nidfs',
                           '--batch-interpreter', 'python-fu-eval',
                           '-b', '-', '--quit'],
                feed: true,
                build_by_default: true,
                env: gimp_run_env)
endforeach

foreach size : sizes
  icon_name = 'fileicon.targetsize-' + size + '.png'
  conf = configuration_data()
  conf.set('TOP_SRCDIR', meson.project_source_root())
  conf.set('OUTPUT',     meson.current_build_dir() / icon_name)
  conf.set('LOGO_DIM',   size)
  gen_fileicon_py = configure_file(input : 'gen-fileicon-png.py.in',
                                   output : 'gen-fileicon-png-' + size + '.py',
                                   configuration : conf)
  custom_target(icon_name,
                input : [ gen_fileicon_py ],
                depend_files: [ wilber_path, fileicon_path ],
                depends: [ gimp_exe_depends ],
                output: [ icon_name ],
                command: [ gimp_exe, '-nidfs',
                            '--batch-interpreter', 'python-fu-eval',
                            '-b', '-', '--quit'],
                feed: true,
                build_by_default: true,
                env: gimp_run_env)
endforeach
