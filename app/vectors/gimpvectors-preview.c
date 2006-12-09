/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "vectors-types.h"

#include "base/temp-buf.h"

#include "core/gimpimage.h"

#include "gimpstroke.h"
#include "gimpvectors.h"
#include "gimpvectors-preview.h"


/*  public functions  */

TempBuf *
gimp_vectors_get_new_preview (GimpViewable *viewable,
                              GimpContext  *context,
                              gint          width,
                              gint          height)
{
  GimpVectors *vectors;
  GimpItem    *item;
  GimpStroke  *cur_stroke;
  gdouble      xscale, yscale;
  guchar      *data;
  TempBuf     *temp_buf;
  guchar       white[1] = { 255 };

  vectors = GIMP_VECTORS (viewable);
  item    = GIMP_ITEM (viewable);

  xscale = ((gdouble) width)  / gimp_image_get_width  (item->image);
  yscale = ((gdouble) height) / gimp_image_get_height (item->image);

  temp_buf = temp_buf_new (width, height, 1, 0, 0, white);
  data = temp_buf_data (temp_buf);

  for (cur_stroke = gimp_vectors_stroke_get_next (vectors, NULL);
       cur_stroke;
       cur_stroke = gimp_vectors_stroke_get_next (vectors, cur_stroke))
    {
      GArray   *coords;
      gboolean  closed;
      gint      i;

      coords = gimp_stroke_interpolate (cur_stroke, 0.5, &closed);

      if (coords)
        {
          for (i = 0; i < coords->len; i++)
            {
              GimpCoords point;
              gint       x, y;

              point = g_array_index (coords, GimpCoords, i);

              x = ROUND (point.x * xscale);
              y = ROUND (point.y * yscale);

              if (x >= 0 && y >= 0 && x < width && y < height)
                data[y * width + x] = 0;
            }

          g_array_free (coords, TRUE);
        }
    }

  return temp_buf;
}
