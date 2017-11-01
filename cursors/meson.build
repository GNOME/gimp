tool_cursors = [
  'cursor-bad.png',
  'cursor-color-picker.png',
  'cursor-corner-bottom-left.png',
  'cursor-corner-bottom-right.png',
  'cursor-corner-bottom.png',
  'cursor-corner-left.png',
  'cursor-corner-right.png',
  'cursor-corner-top-left.png',
  'cursor-corner-top-right.png',
  'cursor-corner-top.png',
  'cursor-crosshair-small.png',
  'cursor-crosshair.png',
  'cursor-mouse.png',
  'cursor-move.png',
  'cursor-none.png',
  'cursor-side-bottom-left.png',
  'cursor-side-bottom-right.png',
  'cursor-side-bottom.png',
  'cursor-side-left.png',
  'cursor-side-right.png',
  'cursor-side-top-left.png',
  'cursor-side-top-right.png',
  'cursor-side-top.png',
  'cursor-zoom.png',

  'modifier-anchor.png',
  'modifier-background.png',
  'modifier-bad.png',
  'modifier-control.png',
  'modifier-foreground.png',
  'modifier-intersect.png',
  'modifier-join.png',
  'modifier-minus.png',
  'modifier-move.png',
  'modifier-pattern.png',
  'modifier-plus.png',
  'modifier-resize.png',
  'modifier-select.png',

  'tool-airbrush.png',
  'tool-blend.png',
  'tool-blur.png',
  'tool-bucket-fill.png',
  'tool-burn.png',
  'tool-clone.png',
  'tool-color-picker.png',
  'tool-crop.png',
  'tool-dodge.png',
  'tool-ellipse-select.png',
  'tool-eraser.png',
  'tool-flip-horizontal.png',
  'tool-flip-vertical.png',
  'tool-free-select.png',
  'tool-fuzzy-select.png',
  'tool-hand.png',
  'tool-heal.png',
  'tool-ink.png',
  'tool-iscissors.png',
  'tool-measure.png',
  'tool-move.png',
  'tool-paintbrush.png',
  'tool-paths-anchor.png',
  'tool-paths-control.png',
  'tool-paths-segment.png',
  'tool-paths.png',
  'tool-pencil.png',
  'tool-perspective.png',
  'tool-polygon-select.png',
  'tool-rect-select.png',
  'tool-resize.png',
  'tool-rotate.png',
  'tool-shear.png',
  'tool-smudge.png',
  'tool-text.png',
  'tool-warp.png',
  'tool-zoom.png',
]

color_picker_cursors = [
  'cursor-color-picker.png',
]

# Build xml files, in source dir, at Meson-runtime
# TODO : versionning, or write in build dir

cursors_resourcename = 'gimp-tool-cursors'
cursors_xml_content  = '<?xml version="1.0" encoding="UTF-8"?>\n'
cursors_xml_content += '<gresources>\n'
cursors_xml_content += '  <gresource prefix="/org/gimp/tool-cursors">\n'
foreach cursor : tool_cursors
  cursors_xml_content+='    <file preprocess="to-pixdata">'+ cursor +'</file>\n'
endforeach
cursors_xml_content += '  </gresource>\n'
cursors_xml_content += '</gresources>\n'

cursors_xml = configure_file(
  output: cursors_resourcename + '.gresource.xml',
  command: [ 'echo', cursors_xml_content ],
  capture: true,
)


pickers_resourcename = 'gimp-color-picker-cursors'
pickers_xml_content  = '<?xml version="1.0" encoding="UTF-8"?>\n'
pickers_xml_content += '<gresources>\n'
pickers_xml_content += '  <gresource prefix="/org/gimp/color-picker-cursors">\n'
foreach cursor : color_picker_cursors
  pickers_xml_content+='    <file preprocess="to-pixdata">'+ cursor +'</file>\n'
endforeach
pickers_xml_content += '  </gresource>\n'
pickers_xml_content += '  <gresource prefix="/org/gimp/color-picker-cursors-raw">\n'
foreach cursor : color_picker_cursors
  pickers_xml_content+='    <file                        >'+ cursor +'</file>\n'
endforeach
pickers_xml_content += '  </gresource>\n'
pickers_xml_content += '</gresources>\n'

pickers_xml = configure_file(
  output: pickers_resourcename + '.gresource.xml',
  command: [ 'echo', pickers_xml_content ],
  capture: true,
)

cursors_sources = gnome.compile_resources(
  cursors_resourcename,
  cursors_xml,
  c_name: cursors_resourcename.underscorify(),
)

pickers_sources = gnome.compile_resources(
  pickers_resourcename,
  pickers_xml,
  c_name: pickers_resourcename.underscorify(),
)
