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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
gimp_palette_save (GimpData  *data,
                   GError   **error)
{
  GimpPalette   *palette = GIMP_PALETTE (data);
  GOutputStream *output;
  GString       *string;
  GList         *list;
  gsize          bytes_written;
  GError        *my_error = NULL;

  output = G_OUTPUT_STREAM (g_file_replace (gimp_data_get_file (data),
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, &my_error));
  if (! output)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for writing: %s"),
                   gimp_file_get_utf8_name (gimp_data_get_file (data)),
                   my_error->message);
      g_clear_error (&my_error);
      return FALSE;
    }

  string = g_string_new ("GIMP Palette\n");

  g_string_append_printf (string,
                          "Name: %s\n"
                          "Columns: %d\n#\n",
                          gimp_object_get_name (palette),
                          CLAMP (gimp_palette_get_columns (palette), 0, 256));

  for (list = gimp_palette_get_colors (palette);
       list;
       list = g_list_next (list))
    {
      GimpPaletteEntry *entry = list->data;
      guchar            r, g, b;

      gimp_rgb_get_uchar (&entry->color, &r, &g, &b);

      g_string_append_printf (string, "%3d %3d %3d\t%s\n",
                              r, g, b, entry->name);
    }

  if (! g_output_stream_write_all (output, string->str, string->len,
                                   &bytes_written, NULL, &my_error) ||
      bytes_written != string->len)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_WRITE,
                   _("Writing palette file '%s' failed: %s"),
                   gimp_file_get_utf8_name (gimp_data_get_file (data)),
                   my_error->message);
      g_clear_error (&my_error);
      g_string_free (string, TRUE);
      g_object_unref (output);
      return FALSE;
    }

  g_string_free (string, TRUE);
  g_object_unref (output);

  return TRUE;
}
