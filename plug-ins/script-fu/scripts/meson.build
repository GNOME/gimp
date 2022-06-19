subdir('images')

# scripts interpreted by extension-script-fu, installed to /scripts
scripts = [
  'add-bevel.scm',
  'addborder.scm',
  'blend-anim.scm',
  'burn-in-anim.scm',
  'carve-it.scm',
  'chrome-it.scm',
  'circuit.scm',
  'clothify.scm',
  'coffee.scm',
  'copy-visible.scm',
  'difference-clouds.scm',
  'distress-selection.scm',
  'drop-shadow.scm',
  'erase-rows.scm',
  'font-map.scm',
  'fuzzyborder.scm',
  'gimp-online.scm',
  'gradient-example.scm',
  'grid-system.scm',
  'guides-from-selection.scm',
  'guides-new-percent.scm',
  'guides-new.scm',
  'guides-remove-all.scm',
  'lava.scm',
  'line-nova.scm',
  'mkbrush.scm',
  'old-photo.scm',
  'palette-export.scm',
  'paste-as-brush.scm',
  'paste-as-pattern.scm',
  'perspective-shadow.scm',
  'plug-in-compat.init',
  'predator.scm',
  'reverse-layers.scm',
  'ripply-anim.scm',
  'round-corners.scm',
  'script-fu-compat.init',
  'script-fu-set-cmap.scm',
  'script-fu-util.scm',
  'script-fu.init',
  'select-to-brush.scm',
  'select-to-image.scm',
  'select-to-pattern.scm',
  'selection-round.scm',
  'slide.scm',
  'spinning-globe.scm',
  'tileblur.scm',
  'unsharp-mask.scm',
  'waves-anim.scm',
  'weave.scm',
  'xach-effect.scm',
]

if not stable
  scripts += [
    'contactsheet.scm',
    'test-sphere.scm',
  ]
endif

install_data(
  scripts,
  install_dir: gimpdatadir / 'scripts',
)

# scripts interpreted by gimp-script-fu-interpreter
# Each installed in subdirectory of /plug-in
# Each have a shebang and executable permission.
# Like other interpreted plugins.
# Lacking a shebang, a .interp file is needed to associate .scm suffix

scripts_independent = [
  { 'name': 'ts-helloworld' },
]

foreach plugin : scripts_independent
  name = plugin.get('name')
  srcs = plugin.get('srcs', name + '.scm')

  install_data(srcs,
               install_dir: gimpplugindir / 'plug-ins' / name,
               install_mode: 'rwxr-xr-x')
endforeach
