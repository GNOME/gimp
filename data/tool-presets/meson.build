tool_presets_types = [
  'Crop',
  'FX',
  'Paint',
  'Selection',
  'Sketch',
]

foreach tool_presets_type : tool_presets_types
  subdir(tool_presets_type)
  install_data(
    tool_presets,
    install_dir: gimpdatadir / 'tool-presets' / tool_presets_type,
  )
endforeach
