
if have_vector_icons
  source_icons = {
    'scalable':         [ 'scalable', 'scalable', '.svg' ],
    'vector_24':        [ '24x24',    '24',       '.svg' ],
    'bitmap_64-system': [ '64x64',    '64',       '.png' ],
    'bitmap_64-always': [ '64x64',    '64',       '.png' ],
  }
else
  source_icons = {
    'bitmap_12':        [ '12x12',   '12',  '.png' ],
    'bitmap_16':        [ '16x16',   '16',  '.png' ],
    'bitmap_18':        [ '18x18',   '18',  '.png' ],
    'bitmap_20':        [ '20x20',   '20',  '.png' ],
    'bitmap_22':        [ '22x22',   '22',  '.png' ],
    'bitmap_24':        [ '24x24',   '24',  '.png' ],
    'bitmap_32':        [ '32x32',   '32',  '.png' ],
    'bitmap_48':        [ '48x48',   '48',  '.png' ],
    'bitmap_64':        [ '64x64',   '64',  '.png' ],
    'bitmap_96':        [ '96x96',   '96',  '.png' ],
    'bitmap_128':       [ '128x128', '128', '.png' ],
    'bitmap_192':       [ '192x192', '192', '.png' ],
    'bitmap_256':       [ '256x256', '256', '.png' ],
    'bitmap_64-system': [ '64x64',    '64', '.png' ],
    'bitmap_64-always': [ '64x64',    '64', '.png' ],
  }

  all_icons = [
    [ '12x12',    bitmap_12 ],
    [ '16x16',    bitmap_16 ],
    [ '18x18',    bitmap_18 ],
    [ '20x20',    bitmap_20 ],
    [ '22x22',    bitmap_22 ],
    [ '24x24',    bitmap_24 ],
    [ '32x32',    bitmap_32 ],
    [ '48x48',    bitmap_48 ],
    [ '64x64',    bitmap_64 + bitmap_64_always + bitmap_64system ],
    [ '96x96',    bitmap_96 ],
    [ '128x128',  bitmap_128 ],
    [ '192x192',  bitmap_192 ],
    [ '256x256',  bitmap_256 ],
  ]
endif

install_data('index.theme',
  install_dir: gimpiconsdir / theme,
)

foreach source, out : source_icons
  outdir = out[0]
  indir  = out[1]
  suffix = out[2]

  icon_list = '../icon-lists/@0@.list'.format(source)
  cmd = run_command('python3', '-c',
                    'print(",".join(["@0@/" + line.strip() + "@1@" for line in open("@2@")]))'.format(indir, suffix, icon_list),
                    check: true)
  source_icons = cmd.stdout().strip().split(',')
  install_data(source_icons, install_dir: gimpiconsdir / theme / outdir / 'apps')
endforeach

icons_core = [ '64/dialog-question.png', ]
icons_imgs = [ '64/gimp-wilber-eek.png', ]

resourcename = 'gimp-core-pixbufs'
xml_content  = '<?xml version="1.0" encoding="UTF-8"?>\n'
xml_content += '<gresources>\n'
xml_content += '  <gresource prefix="/org/gimp/icons">\n'
foreach file : icons_core
  xml_content+='    <file preprocess="to-pixdata">'+ file +'</file>\n'
endforeach
xml_content += '  </gresource>\n'
xml_content += '</gresources>\n'

xml_file = configure_file(
  output: resourcename + '.gresource.xml',
  command: [ 'echo', xml_content ],
  capture: true,
)
icons_core_sources = gnome.compile_resources(
  resourcename,
  xml_file,
  c_name: resourcename.underscorify(),
)


resourcename = 'gimp-icon-pixbufs'
xml_content  = '<?xml version="1.0" encoding="UTF-8"?>\n'
xml_content += '<gresources>\n'
xml_content += '  <gresource prefix="/org/gimp/icons">\n'
foreach file : icons_imgs
  xml_content+='    <file preprocess="to-pixdata">'+ file +'</file>\n'
endforeach
xml_content += '  </gresource>\n'
xml_content += '</gresources>\n'

xml_file = configure_file(
  output: resourcename + '.gresource.xml',
  command: [ 'echo', xml_content ],
  capture: true,
)
icons_imgs_sources = gnome.compile_resources(
  resourcename,
  xml_file,
  c_name: resourcename.underscorify(),
)
