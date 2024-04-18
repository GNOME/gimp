#!/usr/bin/env python3

GIMP_TEST_PALETTE      = "Bears"
GIMP_TEST_PALETTE_SIZE = 256
GIMP_TEST_COLOR_IDX    = 3
GIMP_TEST_COLOR_FORMAT = "R'G'B' u8"
GIMP_TEST_COLOR_R_U8   = 72
GIMP_TEST_COLOR_G_U8   = 56
GIMP_TEST_COLOR_B_U8   = 56

pal = Gimp.Palette.get_by_name(GIMP_TEST_PALETTE)
gimp_assert('gimp_palette_get_by_name()', type(pal) == Gimp.Palette)

pal2 = Gimp.Palette.get_by_name(GIMP_TEST_PALETTE)
gimp_assert('gimp_palette_get_by_name() is unique', pal == pal2)

n_colors = pal.get_color_count()
gimp_assert('gimp_palette_get_color_count()', GIMP_TEST_PALETTE_SIZE == n_colors)

colors = pal.get_colors()
gimp_assert('gimp_palette_get_colors()',
            len(colors) == GIMP_TEST_PALETTE_SIZE and type(colors[0]) == Gegl.Color)

f = colors[GIMP_TEST_COLOR_IDX].get_format()
gimp_assert ("Checking fourth palette color's format",
             f == Babl.format (GIMP_TEST_COLOR_FORMAT))

b = colors[GIMP_TEST_COLOR_IDX].get_bytes(f)
rgb = b.get_data()
gimp_assert ("Checking fourth palette color's RGB components",
             int(rgb[0]) == GIMP_TEST_COLOR_R_U8 and int(rgb[1]) == GIMP_TEST_COLOR_G_U8 and int(rgb[2]) == GIMP_TEST_COLOR_B_U8)

# Run the same tests through PDB:

proc   = Gimp.get_pdb().lookup_procedure('gimp-palette-get-by-name')
config = proc.create_config()
config.set_property('name', GIMP_TEST_PALETTE)
result = proc.run(config)
status = result.index(0)
gimp_assert('gimp-palette-get-by-name', status == Gimp.PDBStatusType.SUCCESS)

pal2 = result.index(1)
gimp_assert('gimp-palette-get-by-name and gimp_palette_get_by_name() get identical result', pal == pal2)

proc   = Gimp.get_pdb().lookup_procedure('gimp-palette-get-color-count')
config = proc.create_config()
config.set_property('palette', pal)
result = proc.run(config)
status = result.index(0)
gimp_assert('gimp-palette-get-color-count', status == Gimp.PDBStatusType.SUCCESS)

n_colors = result.index(1)
gimp_assert('gimp_palette_get_color_count()', GIMP_TEST_PALETTE_SIZE == n_colors)

proc   = Gimp.get_pdb().lookup_procedure('gimp-palette-get-colors')
config = proc.create_config()
config.set_property('palette', pal)
result = proc.run(config)
status = result.index(0)
gimp_assert('gimp-palette-get-colors', status == Gimp.PDBStatusType.SUCCESS)

colors = result.index(1)
# XXX This test is actually what happens right now, but not what should happen.
# See: https://gitlab.gnome.org/GNOME/gimp/-/issues/10885#note_2030308
# And: https://gitlab.gnome.org/GNOME/gobject-introspection/-/issues/492
# If some day this test fails, and in particular if it can return a list of
# GeglColor, then we should remove the test and deprecate
# gimp_value_array_get_color_array() in favor of the generic
# gimp_value_array_index().
gimp_assert('gimp-palette-get-colors', type(colors) == GObject.GBoxed)

colors = result.get_color_array(1)
gimp_assert('gimp_palette_get_colors()',
            type(colors) == list and len(colors) == GIMP_TEST_PALETTE_SIZE and type(colors[0]) == Gegl.Color)

f = colors[3].get_format()
gimp_assert ("Checking fourth palette color's format",
             f == Babl.format (GIMP_TEST_COLOR_FORMAT))

b = colors[3].get_bytes(f)
rgb = b.get_data()
gimp_assert ("Checking fourth palette color's RGB components",
             int(rgb[0]) == GIMP_TEST_COLOR_R_U8 and int(rgb[1]) == GIMP_TEST_COLOR_G_U8 and int(rgb[2]) == GIMP_TEST_COLOR_B_U8)
