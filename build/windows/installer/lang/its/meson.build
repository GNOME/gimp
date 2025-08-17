#Here we generate rules for XML translation extraction by 'i18n.merge_file' on the parent lang/meson.build)

xgettext = find_program('xgettext', version: '>=0.23', required: false)

its_conf = configuration_data()
its_conf.set('ESCAPE_ENTITY', xgettext.found() ? 'yes' : 'no')
gimp_ms_installer_config_its = configure_file(input : 'gimp-ms-installer-config.its.in',
                                              output : 'gimp-ms-installer-config.its',
                                              configuration : its_conf)
