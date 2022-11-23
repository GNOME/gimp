/* LIGMA - The GNU Image Manipulation Program
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "vectors-types.h"

#include "libligmamath/ligmamath.h"

#include "core/ligmaimage.h"
#include "core/ligmatempbuf.h"

#include "ligmastroke.h"
#include "ligmavectors.h"
#include "ligmavectors-preview.h"


/*  public functions  */

LigmaTempBuf *
ligma_vectors_get_new_preview (LigmaViewable *viewable,
                              LigmaContext  *context,
                              gint          width,
                              gint          height)
{
  LigmaVectors *vectors;
  LigmaItem    *item;
  LigmaStroke  *cur_stroke;
  gdouble      xscale, yscale;
  guchar      *data;
  LigmaTempBuf *temp_buf;

  vectors = LIGMA_VECTORS (viewable);
  item    = LIGMA_ITEM (viewable);

  xscale = ((gdouble) width)  / ligma_image_get_width  (ligma_item_get_image (item));
  yscale = ((gdouble) height) / ligma_image_get_height (ligma_item_get_image (item));

  temp_buf = ligma_temp_buf_new (width, height, babl_format ("Y' u8"));
  data = ligma_temp_buf_get_data (temp_buf);
  memset (data, 255, width * height);

  for (cur_stroke = ligma_vectors_stroke_get_next (vectors, NULL);
       cur_stroke;
       cur_stroke = ligma_vectors_stroke_get_next (vectors, cur_stroke))
    {
      GArray   *coords;
      gboolean  closed;
      gint      i;

      coords = ligma_stroke_interpolate (cur_stroke, 0.5, &closed);

      if (coords)
        {
          for (i = 0; i < coords->len; i++)
            {
              LigmaCoords point;
              gint       x, y;

              point = g_array_index (coords, LigmaCoords, i);

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
