#!/usr/bin/env python3

import struct

GIMP_TEST_PALETTE      = "Bears"
GIMP_TEST_PALETTE_SIZE = 256
GIMP_TEST_COLOR_IDX    = 3
GIMP_TEST_COLOR_FORMAT = "R'G'B' u8"
GIMP_TEST_COLOR_R_U8   = 72
GIMP_TEST_COLOR_G_U8   = 56
GIMP_TEST_COLOR_B_U8   = 56
EPSILON                = 1e-6

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
gimp_assert("Checking fourth palette color's format",
            f == Babl.format (GIMP_TEST_COLOR_FORMAT))

b = colors[GIMP_TEST_COLOR_IDX].get_bytes(f)
rgb = b.get_data()
gimp_assert("Checking fourth palette color's RGB components",
            int(rgb[0]) == GIMP_TEST_COLOR_R_U8 and int(rgb[1]) == GIMP_TEST_COLOR_G_U8 and int(rgb[2]) == GIMP_TEST_COLOR_B_U8)

colormap, n_colormap_colors = pal.get_colormap (f)
format_bpp = Babl.format_get_bytes_per_pixel (f);
gimp_assert("gimp_palette_get_colormap()",
            colormap is not None and n_colormap_colors == n_colors and
            n_colormap_colors * format_bpp == len(colormap))

gimp_assert("Comparing fourth palette color's RGB components from colormap",
            colormap[format_bpp * GIMP_TEST_COLOR_IDX] == GIMP_TEST_COLOR_R_U8     and
            colormap[format_bpp * GIMP_TEST_COLOR_IDX + 1] == GIMP_TEST_COLOR_G_U8 and
            colormap[format_bpp * GIMP_TEST_COLOR_IDX + 2]== GIMP_TEST_COLOR_B_U8)

f2 = Babl.format('RGBA float')
colormap, n_colormap_colors = pal.get_colormap (f2)
f2_bpp = Babl.format_get_bytes_per_pixel (f2);
gimp_assert('gimp_palette_get_colormap() in "RGBA float"',
            colormap is not None and n_colormap_colors == n_colors and
            n_colormap_colors * f2_bpp == len(colormap))


start = f2_bpp * GIMP_TEST_COLOR_IDX
colormap_r = struct.unpack('f', colormap[start:start + 4])[0]
colormap_g = struct.unpack('f', colormap[start + 4:start + 8])[0]
colormap_b = struct.unpack('f', colormap[start + 8:start + 12])[0]

rgb_bytes = colors[GIMP_TEST_COLOR_IDX].get_bytes(f2)
rgb = rgb_bytes.get_data()
palette_r = struct.unpack('f', rgb[:4])[0]
palette_g = struct.unpack('f', rgb[4:8])[0]
palette_b = struct.unpack('f', rgb[8:12])[0]
gimp_assert("Comparing fourth palette color's RGB components from colormap in float format",
            abs(colormap_r - palette_r) < EPSILON and
            abs(colormap_g - palette_g) < EPSILON and
            abs(colormap_b - palette_b) < EPSILON)

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
