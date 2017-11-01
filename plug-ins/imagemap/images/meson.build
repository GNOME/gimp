stock_images = [
  'stock-circle',
  'stock-coord',
  'stock-dimension',
  'stock-java',
  'stock-polygon',
  'stock-rectangle',
  'stock-to-back',
  'stock-to-front',
]

stock_icons_list = []
foreach image_name : stock_images
  stock_icons_list += [
    image_name.underscorify(),
    join_paths(meson.current_source_dir(), image_name + '.png'),
  ]
endforeach

imap_stock_pixbuf_h = custom_target('imap-stock-pixbufs.h',
  input : [],
  output: [ 'imap-stock-pixbufs.h' ],
  command: [
    'gdk-pixbuf-csource',
    '--raw',
    '--build-list',
    stock_icons_list,
  ],
  capture: true,
)
