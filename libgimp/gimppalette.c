/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppalette.c
 * Copyright (C) 2023 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"

#include "gimppalette.h"

struct _GimpPalette
{
  GimpResource parent_instance;
};

G_DEFINE_TYPE (GimpPalette, gimp_palette, GIMP_TYPE_RESOURCE);


static void  gimp_palette_class_init (GimpPaletteClass *klass)
{
}

static void  gimp_palette_init (GimpPalette *palette)
{
}

/**
 * gimp_palette_get_colormap:
 * @palette: The palette.
 * @format: The desired color format.
 * @num_colors: (out) (nullable): The number of colors in the palette.
 * @num_bytes: (out) (nullable): The byte-size of the returned value.
 *
 * This procedure returns a palette's colormap as an array of bytes with
 * all colors converted to a given Babl @format.
 *
 * The byte-size of the returned colormap depends on the number of
 * colors and on the bytes-per-pixel size of @format. E.g. that the
 * following equality is ensured:
 *
 * ```C
 * num_bytes == num_colors * babl_format_get_bytes_per_pixel (format)
 * ```
 *
 * Therefore @num_colors and @num_bytes are kinda redundant since both
 * indicate the size of the return value in a different way. You may
 * both set them to %NULL but not at the same time.
 *
 * Returns: (transfer full) (array length=num_bytes): The palette's colormap.
 *
 * Since: 3.0
 **/
guint8 *
gimp_palette_get_colormap (GimpPalette *palette,
                           const Babl  *format,
                           gint        *num_colors,
                           gsize       *num_bytes)
{
  GBytes *bytes;
  guint8 *colormap;
  gint    n_colors;
  gsize   n_bytes;

  g_return_val_if_fail (GIMP_IS_PALETTE (palette), NULL);
  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (num_colors != NULL || num_bytes != NULL, NULL);

  bytes    = _gimp_palette_get_bytes (palette, format, &n_colors);
  colormap = g_bytes_unref_to_data (bytes, &n_bytes);

  if (num_colors)
    *num_colors = n_colors;
  if (num_bytes)
    *num_bytes = n_bytes;

  return colormap;
}

/**
 * gimp_palette_set_colormap:
 * @palette: The palette.
 * @format: The desired color format.
 * @colormap (array length=num_bytes): The new colormap values.
 * @num_bytes: The byte-size of @colormap.
 *
 * This procedure sets the entries in the specified palette in one go,
 * though they must all be in the same @format.
 *
 * The number of entries depens on the @num_bytes size of @colormap and
 * the bytes-per-pixel size of @format.
 * The procedure will fail if @num_bytes is not an exact multiple of the
 * number of bytes per pixel of @format.
 *
 * Returns: %TRUE on success.
 *
 * Since: 3.0
 **/
gboolean
gimp_palette_set_colormap (GimpPalette *palette,
                           const Babl  *format,
                           guint8      *colormap,
                           gsize        num_bytes)
{
  GBytes   *bytes;
  gboolean  success;

  g_return_val_if_fail (GIMP_IS_PALETTE (palette), FALSE);
  g_return_val_if_fail (format != NULL, FALSE);
  g_return_val_if_fail (colormap != NULL, FALSE);
  g_return_val_if_fail (num_bytes % babl_format_get_bytes_per_pixel (format) == 0, FALSE);

  bytes   = g_bytes_new_static (colormap, num_bytes);
  success = _gimp_palette_set_bytes (palette, format, bytes);
  g_bytes_unref (bytes);

  return success;
}
