dynamics = files(
  'Basic-Dynamics.gdyn',
  'Color-From-Gradient.gdyn',
  'Fade-Tapering.gdyn',
  'Random-Color.gdyn',
  'Tilt-Angle.gdyn',
  'Track-Direction.gdyn',
  'Velocity-Tapering.gdyn',
)

install_data(dynamics,
  install_dir: gimpdatadir / 'dynamics',
)

dynamics_types = [
  'Basic',
  'FX',
]
foreach dynamics_type : dynamics_types
  subdir(dynamics_type)
  install_data(
    dynamics,
    install_dir: gimpdatadir / 'dynamics' / dynamics_type,
  )
endforeach
