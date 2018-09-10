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

#define GEGL_ITERATOR2_API
#include <gegl.h>

#include "gimp-gegl-types.h"

#include "gegl/gimp-gegl-mask.h"


gboolean
gimp_gegl_mask_bounds (GeglBuffer *buffer,
                       gint        *x1,
                       gint        *y1,
                       gint        *x2,
                       gint        *y2)
{
  GeglBufferIterator *iter;
  GeglRectangle      *roi;
  gint                tx1, tx2, ty1, ty2;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), FALSE);
  g_return_val_if_fail (x1 != NULL, FALSE);
  g_return_val_if_fail (y1 != NULL, FALSE);
  g_return_val_if_fail (x2 != NULL, FALSE);
  g_return_val_if_fail (y2 != NULL, FALSE);

  /*  go through and calculate the bounds  */
  tx1 = gegl_buffer_get_width  (buffer);
  ty1 = gegl_buffer_get_height (buffer);
  tx2 = 0;
  ty2 = 0;

  iter = gegl_buffer_iterator_new (buffer, NULL, 0, babl_format ("Y float"),
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 1);
  roi = &iter->items[0].roi;

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *data  = iter->items[0].data;
      gfloat *data1 = data;
      gint    ex    = roi->x + roi->width;
      gint    ey    = roi->y + roi->height;
      gint    x, y;

      /*  only check the pixels if this tile is not fully within the
       *  currently computed bounds
       */
      if (roi->x < tx1 || ex > tx2 ||
          roi->y < ty1 || ey > ty2)
        {
          /* Check upper left and lower right corners to see if we can
           * avoid checking the rest of the pixels in this tile
           */
          if (data[0] && data[iter->length - 1])
            {
              /*  "ex/ey - 1" because the internal variables are the
               *  right/bottom pixel of the mask's contents, not one
               *  right/below it like the return values.
               */

              if (roi->x < tx1) tx1 = roi->x;
              if (ex > tx2)     tx2 = ex - 1;

              if (roi->y < ty1) ty1 = roi->y;
              if (ey > ty2)     ty2 = ey - 1;
            }
          else
            {
              for (y = roi->y; y < ey; y++, data1 += roi->width)
                {
                  for (x = roi->x, data = data1; x < ex; x++, data++)
                    {
                      if (*data)
                        {
                          gint minx = x;
                          gint maxx = x;

                          for (; x < ex; x++, data++)
                            if (*data)
                              maxx = x;

                          if (minx < tx1) tx1 = minx;
                          if (maxx > tx2) tx2 = maxx;

                          if (y < ty1) ty1 = y;
                          if (y > ty2) ty2 = y;
                        }
                    }
                }
            }
        }
    }

  tx2 = CLAMP (tx2 + 1, 0, gegl_buffer_get_width  (buffer));
  ty2 = CLAMP (ty2 + 1, 0, gegl_buffer_get_height (buffer));

  if (tx1 == gegl_buffer_get_width  (buffer) &&
      ty1 == gegl_buffer_get_height (buffer))
    {
      *x1 = 0;
      *y1 = 0;
      *x2 = gegl_buffer_get_width  (buffer);
      *y2 = gegl_buffer_get_height (buffer);

      return FALSE;
    }

  *x1 = tx1;
  *y1 = ty1;
  *x2 = tx2;
  *y2 = ty2;

  return TRUE;
}

gboolean
gimp_gegl_mask_is_empty (GeglBuffer *buffer)
{
  GeglBufferIterator *iter;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), FALSE);

  iter = gegl_buffer_iterator_new (buffer, NULL, 0, babl_format ("Y float"),
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *data = iter->items[0].data;
      gint    i;

      for (i = 0; i < iter->length; i++)
        {
          if (data[i])
            {
              gegl_buffer_iterator_stop (iter);

              return FALSE;
            }
        }
    }

  return TRUE;
}
