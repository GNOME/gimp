/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimppalette.h"
#include "gimppalette-save.h"

#include "gimp-intl.h"


gboolean
gimp_palette_save (GimpData       *data,
                   GOutputStream  *output,
                   GError        **error)
{
  GimpPalette *palette = GIMP_PALETTE (data);
  GString     *string;
  GList       *list;

  string = g_string_new ("GIMP Palette\n");

  g_string_append_printf (string,
                          "Name: %s\n"
                          "Columns: %d\n"
                          "#\n",
                          gimp_object_get_name (palette),
                          CLAMP (gimp_palette_get_columns (palette), 0, 256));

  for (list = gimp_palette_get_colors (palette);
       list;
       list = g_list_next (list))
    {
      GimpPaletteEntry *entry = list->data;
      guchar            rgb[3];

      /* TODO: we should create a GIMP Palette v2 to store any format/space. */
      gegl_color_get_pixel (entry->color, babl_format ("R'G'B' u8"), rgb);

      g_string_append_printf (string, "%3d %3d %3d\t%s\n",
                              rgb[0], rgb[1], rgb[2], entry->name);
    }

  if (! g_output_stream_write_all (output, string->str, string->len,
                                   NULL, NULL, error))
    {
      g_string_free (string, TRUE);

      return FALSE;
    }

  g_string_free (string, TRUE);

  return TRUE;
}
