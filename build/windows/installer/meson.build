## This file shouldn't even exist, but Meson unfortunately do
## not support C++ style comments. See: https://github.com/mesonbuild/meson/issues/14260

installer_conf = configuration_data()
installer_conf.set('GIMP_APP_VERSION',  gimp_app_version)
installer_conf.set('GIMP_VERSION',  gimp_version)
installer_conf.set('GIMP_PKGCONFIG_VERSION',  gimp_api_version)

installer_conf.set('GIMP_RELEASE', release ? 1 : '')
if gimp_unstable != false
  installer_conf.set('GIMP_UNSTABLE', gimp_unstable)
endif
if gimp_rc_version != false
  installer_conf.set('GIMP_RC_VERSION', gimp_rc_version)
endif

configure_file(
  input : 'config_clean.h.in',
  output: 'config_clean.h',
  configuration: installer_conf
)


subdir('lang')
