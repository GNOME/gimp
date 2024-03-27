# Set GIMP app version (major.minor)
if platform_linux
  json_config = configuration_data()
  json_config.set('GIMP_APP_VERSION', gimp_app_version)

  configure_file(
    input : 'org.gimp.GIMP-nightly.json.in',
    output: 'org.gimp.GIMP-nightly.json',
    configuration: json_config,
  )
endif