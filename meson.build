project('gimp-data',
        'c',
        version: '2.99.19',
        meson_version: '>=0.59.0')

if not meson.is_subproject()
  error('''
    gimp-data is meant to be run as a subproject of GIMP source repository only:
    https://gitlab.gnome.org/GNOME/gimp/
  ''')
endif

# Options passed by subproject() call:
gimpdatadir = get_option('gimp-datadir')

if gimpdatadir == ''
  error('gimp-data was not called with expected options: gimpdatadir')
endif

# Yielded options:
have_vector_icons = get_option('vector-icons')

## Tools ##

native_glib_minver = '2.2.0'
native_rsvg_minver = '2.40.6'
native_glib = dependency('glib-2.0',    version: '>='+native_glib_minver, native: true)
native_rsvg = dependency('librsvg-2.0', version: '>='+native_rsvg_minver, native: true)

subdir('tools')

pythonmod = import('python')
python    = pythonmod.find_installation('python3')

gnome     = import('gnome')

## Data ##

subdir('brushes')
subdir('cursors')
subdir('icons')
subdir('patterns')
