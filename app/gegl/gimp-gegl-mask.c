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
  GeglBufferIterator  *iter;
  const GeglRectangle *extent;
  const GeglRectangle *roi;
  const Babl          *format;
  gint                 bpp;
  gint                 tx1, tx2, ty1, ty2;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), FALSE);
  g_return_val_if_fail (x1 != NULL, FALSE);
  g_return_val_if_fail (y1 != NULL, FALSE);
  g_return_val_if_fail (x2 != NULL, FALSE);
  g_return_val_if_fail (y2 != NULL, FALSE);

  extent = gegl_buffer_get_extent (buffer);

  /*  go through and calculate the bounds  */
  tx1 = extent->x + extent->width;
  ty1 = extent->y + extent->height;
  tx2 = extent->x;
  ty2 = extent->y;

  format = gegl_buffer_get_format (buffer);
  bpp    = babl_format_get_bytes_per_pixel (format);

  iter = gegl_buffer_iterator_new (buffer, NULL, 0, format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 1);
  roi = &iter->items[0].roi;

  while (gegl_buffer_iterator_next (iter))
    {
      const guint8 *data_u8 = iter->items[0].data;
      gint          ex      = roi->x + roi->width;
      gint          ey      = roi->y + roi->height;

      /*  only check the pixels if this tile is not fully within the
       *  currently computed bounds
       */
      if (roi->x < tx1 || ex > tx2 ||
          roi->y < ty1 || ey > ty2)
        {
          /* Check upper left and lower right corners to see if we can
           * avoid checking the rest of the pixels in this tile
           */
          if (! gegl_memeq_zero (data_u8,                            bpp) &&
              ! gegl_memeq_zero (data_u8 + (iter->length - 1) * bpp, bpp))
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
              #define FIND_BOUNDS(bpp, type)                                 \
                G_STMT_START                                                 \
                  {                                                          \
                    const type *data;                                        \
                    gint        y;                                           \
                                                                             \
                    if ((guintptr) data_u8 % bpp)                            \
                      goto generic;                                          \
                                                                             \
                    data = (const type *) data_u8;                           \
                                                                             \
                    for (y = roi->y; y < ey; y++)                            \
                      {                                                      \
                        gint x1;                                             \
                                                                             \
                        for (x1 = 0; x1 < roi->width; x1++)                  \
                          {                                                  \
                            if (data[x1])                                    \
                              {                                              \
                                gint x2;                                     \
                                gint x2_end = MAX (x1, tx2 - roi->x);        \
                                                                             \
                                for (x2 = roi->width - 1; x2 > x2_end; x2--) \
                                  {                                          \
                                    if (data[x2])                            \
                                      break;                                 \
                                  }                                          \
                                                                             \
                                x1 += roi->x;                                \
                                x2 += roi->x;                                \
                                                                             \
                                if (x1 < tx1) tx1 = x1;                      \
                                if (x2 > tx2) tx2 = x2;                      \
                                                                             \
                                if (y < ty1) ty1 = y;                        \
                                if (y > ty2) ty2 = y;                        \
                                                                             \
                                break;                                       \
                              }                                              \
                          }                                                  \
                                                                             \
                        data += roi->width;                                  \
                      }                                                      \
                  }                                                          \
                G_STMT_END

              switch (bpp)
                {
                case 1:
                  FIND_BOUNDS (1, guint8);
                  break;

                case 2:
                  FIND_BOUNDS (2, guint16);
                  break;

                case 4:
                  FIND_BOUNDS (4, guint32);
                  break;

                case 8:
                  FIND_BOUNDS (8, guint64);
                  break;

                default:
                generic:
                  {
                    const guint8 *data = data_u8;
                    gint          y;

                    for (y = roi->y; y < ey; y++)
                      {
                        gint x1;

                        for (x1 = 0; x1 < roi->width; x1++)
                          {
                            if (! gegl_memeq_zero (data + x1 * bpp, bpp))
                              {
                                gint x2;
                                gint x2_end = MAX (x1, tx2 - roi->x);

                                for (x2 = roi->width - 1; x2 > x2_end; x2--)
                                  {
                                    if (! gegl_memeq_zero (data + x2 * bpp,
                                                           bpp))
                                      {
                                        break;
                                      }
                                  }

                                x1 += roi->x;
                                x2 += roi->x;

                                if (x1 < tx1) tx1 = x1;
                                if (x2 > tx2) tx2 = x2;

                                if (y < ty1) ty1 = y;
                                if (y > ty2) ty2 = y;
                              }
                          }

                        data += roi->width * bpp;
                      }
                  }
                  break;
                }

              #undef FIND_BOUNDS
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
  const Babl         *format;
  gint                bpp;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), FALSE);

  format = gegl_buffer_get_format (buffer);
  bpp    = babl_format_get_bytes_per_pixel (format);

  iter = gegl_buffer_iterator_new (buffer, NULL, 0, format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      if (! gegl_memeq_zero (iter->items[0].data, bpp * iter->length))
        {
          gegl_buffer_iterator_stop (iter);

          return FALSE;
        }
    }

  return TRUE;
}
