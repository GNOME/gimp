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

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "core-types.h"

#include "gimpbrush.h"
#include "gimpbrush-header.h"
#include "gimpbrush-save.h"
#include "gimptempbuf.h"


gboolean
gimp_brush_save (GimpData       *data,
                 GOutputStream  *output,
                 GError        **error)
{
  GimpBrush       *brush  = GIMP_BRUSH (data);
  GimpTempBuf     *mask   = gimp_brush_get_mask (brush);
  GimpTempBuf     *pixmap = gimp_brush_get_pixmap (brush);
  GimpBrushHeader  header;
  const gchar     *name;
  gint             width;
  gint             height;

  name   = gimp_object_get_name (brush);
  width  = gimp_temp_buf_get_width  (mask);
  height = gimp_temp_buf_get_height (mask);

  header.header_size  = g_htonl (sizeof (GimpBrushHeader) +
                                 strlen (name) + 1);
  header.version      = g_htonl (2);
  header.width        = g_htonl (width);
  header.height       = g_htonl (height);
  header.bytes        = g_htonl (pixmap ? 4 : 1);
  header.magic_number = g_htonl (GIMP_BRUSH_MAGIC);
  header.spacing      = g_htonl (gimp_brush_get_spacing (brush));

  if (! g_output_stream_write_all (output, &header, sizeof (header),
                                   NULL, NULL, error))
    {
      return FALSE;
    }

  if (! g_output_stream_write_all (output, name, strlen (name) + 1,
                                   NULL, NULL, error))
    {
      return FALSE;
    }

  if (pixmap)
    {
      gsize   size = width * height * 4;
      guchar *data = g_malloc (size);
      guchar *p    = gimp_temp_buf_get_data (pixmap);
      guchar *m    = gimp_temp_buf_get_data (mask);
      guchar *d    = data;
      gint    i;

      for (i = 0; i < width * height; i++)
        {
          *d++ = *p++;
          *d++ = *p++;
          *d++ = *p++;
          *d++ = *m++;
        }

      if (! g_output_stream_write_all (output, data, size,
                                       NULL, NULL, error))
        {
          g_free (data);

          return FALSE;
        }

      g_free (data);
    }
  else
    {
      if (! g_output_stream_write_all (output,
                                       gimp_temp_buf_get_data (mask),
                                       gimp_temp_buf_get_data_size (mask),
                                       NULL, NULL, error))
        {
          return FALSE;
        }
    }

  return TRUE;
}
