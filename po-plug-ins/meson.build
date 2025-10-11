po_plug_ins_dir = meson.current_source_dir()
i18n.gettext(gettext_package + '-std-plug-ins', preset: 'glib')

# Special-casing, we just reuse the same locale dictionary for our demo
# extension. We could have separated the strings but just for the sake
# of demo on code-side, it's simpler to do it this way.
extension_i18n = 'org.gimp.extension.goat-exercises'
i18n.gettext(extension_i18n, preset: 'glib',
             install_dir: gimpplugindir /  'extensions' / extension_i18n / 'locale')
