/* The GIMP -- an image manipulation program
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
                              gint          width,
                              gint          height)
{
  GimpVectors *vectors;
  GimpItem    *item;
  GArray      *coords;
  GimpCoords   point;
  GimpStroke  *cur_stroke = NULL;
  gdouble      xscale, yscale, xres, yres;
  gint         x, y;
  guchar      *data;
  gboolean     closed;

  TempBuf     *temp_buf;
  guchar       white[1] = { 255 };
  gint         i;

  vectors = GIMP_VECTORS (viewable);
  item = GIMP_ITEM (viewable);

  xscale = ((gdouble) width) / gimp_image_get_width (item->gimage);
  yscale = ((gdouble) height) / gimp_image_get_height (item->gimage);
  
  temp_buf = temp_buf_new (width, height, 1, 0, 0, white);
  data = temp_buf_data (temp_buf);

  while ((cur_stroke = gimp_vectors_stroke_get_next (vectors, cur_stroke)))
    {
      coords = gimp_stroke_interpolate (cur_stroke, 1.0, &closed);

      for (i=0; i < coords->len; i++)
        {
          point = g_array_index (coords, GimpCoords, i);

          x = (gint) (point.x * xscale + 0.5);
          y = (gint) (point.y * yscale + 0.5);

          if (x >= 0 && y >= 0 && x < width && y < height)
            data[y * width + x] = 0;
        }

      g_array_free (coords, TRUE);
    }

  return temp_buf;
}
