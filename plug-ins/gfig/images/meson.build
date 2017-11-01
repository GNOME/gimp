stock_icons = [
  'stock-bezier',
  'stock-circle',
  'stock-copy-object',
  'stock-curve',
  'stock-delete-object',
  'stock-ellipse',
  'stock-line',
  'stock-logo',
  'stock-move-object',
  'stock-move-point',
  'stock-polygon',
  'stock-rectangle',
  'stock-select-object',
  'stock-show-all',
  'stock-spiral',
  'stock-star',
]

stock_icons_list = []
foreach name : stock_icons
  stock_icons_list += name.underscorify()
  stock_icons_list += join_paths(meson.current_source_dir(), name + '.png')
endforeach

stock_pixbufs_h = custom_target('gfig-stock-pixbufs.h',
  input : [ ],
  output: [ 'gfig-stock-pixbufs.h', ],
  command: [
    gdk_pixbuf_csource,
    '--raw',
    '--build-list',
    stock_icons_list,
  ],
  capture: true,
)
