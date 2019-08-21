dynamics = files(
  'Tilt-Angle.gdyn',
  'Fade-Tapering.gdyn',
  'Basic-Dynamics.gdyn',
  'Random-Color.gdyn',
  'Velocity-Tapering.gdyn',
  'Color-From-Gradient.gdyn',
  'Dynamics-Off.gdyn',
  'Track-Direction.gdyn',
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
