stock_icons = [
  'stock-intensity-ambient-high',
  'stock-intensity-ambient-low',
  'stock-intensity-diffuse-high',
  'stock-intensity-diffuse-low',
  'stock-reflectivity-diffuse-high',
  'stock-reflectivity-diffuse-low',
  'stock-reflectivity-specular-high',
  'stock-reflectivity-specular-low',
  'stock-reflectivity-highlight-high',
  'stock-reflectivity-highlight-low',
]

stock_icons_list = []
foreach name : stock_icons
  stock_icons_list += name.underscorify()
  stock_icons_list += join_paths(meson.current_source_dir(), name + '.png')
endforeach

stock_pixbufs_h = custom_target('stock-pixbufs.h',
  input : [ ],
  output: [ 'stock-pixbufs.h', ],
  command: [
    gdk_pixbuf_csource,
    '--raw',
    '--build-list',
    stock_icons_list,
  ],
  capture: true,
)
