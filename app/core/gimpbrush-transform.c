/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrush-transform.c
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

#include <glib-object.h>

#include "core-types.h"

#include "libgimpmath/gimpmath.h"

#include "gimpbrush.h"
#include "gimpbrush-transform.h"

#include "base/temp-buf.h"


/*  local function prototypes  */

static void  gimp_brush_transform_matrix       (GimpBrush         *brush,
                                                gdouble            scale_x,
                                                gdouble            scale_y,
                                                gdouble            angle,
                                                GimpMatrix3       *matrix);
static void  gimp_brush_transform_bounding_box (GimpBrush         *brush,
                                                const GimpMatrix3 *matrix,
                                                gint              *x,
                                                gint              *y,
                                                gint              *width,
                                                gint              *height);


/*  public functions  */

void
gimp_brush_real_transform_size (GimpBrush *brush,
                                gdouble    scale_x,
                                gdouble    scale_y,
                                gdouble    angle,
                                gint      *width,
                                gint      *height)
{
  GimpMatrix3 matrix;
  gint        x, y;

  gimp_brush_transform_matrix (brush, scale_x, scale_y, angle, &matrix);
  gimp_brush_transform_bounding_box (brush, &matrix, &x, &y, width, height);
}

TempBuf *
gimp_brush_real_transform_mask (GimpBrush *brush,
                                gdouble    scale_x,
                                gdouble    scale_y,
                                gdouble    angle)
{
  TempBuf      *result;
  guchar       *dest;
  const guchar *src;
  GimpMatrix3   matrix;
  gint          src_width;
  gint          src_height;
  gint          dest_width;
  gint          dest_height;
  gint          x, y;

  gimp_brush_transform_matrix (brush, scale_x, scale_y, angle, &matrix);

  if (gimp_matrix3_is_identity (&matrix))
    return temp_buf_copy (brush->mask, NULL);

  src_width  = brush->mask->width;
  src_height = brush->mask->height;

  gimp_brush_transform_bounding_box (brush, &matrix,
                                     &x, &y, &dest_width, &dest_height);
  gimp_matrix3_translate (&matrix, -x, -y);
  gimp_matrix3_invert (&matrix);

  result = temp_buf_new (dest_width, dest_height, 1, 0, 0, NULL);

  dest = temp_buf_get_data (result);
  src  = temp_buf_get_data (brush->mask);

  for (y = 0; y < dest_height; y++)
    {
      for (x = 0; x < dest_width; x++)
        {
          gdouble dx, dy;
          gint    ix, iy;

          gimp_matrix3_transform_point (&matrix, x, y, &dx, &dy);

          ix = ROUND (dx);
          iy = ROUND (dy);

          if (ix > 0 && ix < src_width &&
              iy > 0 && iy < src_height)
            {
              *dest = src[iy * src_width + ix];
            }
          else
            {
              *dest = 0;
            }

          dest++;
        }
    }

  return result;
}

TempBuf *
gimp_brush_real_transform_pixmap (GimpBrush *brush,
                                  gdouble    scale_x,
                                  gdouble    scale_y,
                                  gdouble    angle)
{
  TempBuf      *result;
  guchar       *dest;
  const guchar *src;
  GimpMatrix3   matrix;
  gint          src_width;
  gint          src_height;
  gint          dest_width;
  gint          dest_height;
  gint          x, y;

  gimp_brush_transform_matrix (brush, scale_x, scale_y, angle, &matrix);

  if (gimp_matrix3_is_identity (&matrix))
    return temp_buf_copy (brush->pixmap, NULL);

  src_width  = brush->pixmap->width;
  src_height = brush->pixmap->height;

  gimp_brush_transform_bounding_box (brush, &matrix,
                                     &x, &y, &dest_width, &dest_height);
  gimp_matrix3_translate (&matrix, -x, -y);
  gimp_matrix3_invert (&matrix);

  result = temp_buf_new (dest_width, dest_height, 3, 0, 0, NULL);

  dest = temp_buf_get_data (result);
  src  = temp_buf_get_data (brush->pixmap);

  for (y = 0; y < dest_height; y++)
    {
      for (x = 0; x < dest_width; x++)
        {
          gdouble dx, dy;
          gint    ix, iy;

          gimp_matrix3_transform_point (&matrix, x, y, &dx, &dy);

          ix = ROUND (dx);
          iy = ROUND (dy);

          if (ix > 0 && ix < src_width &&
              iy > 0 && iy < src_height)
            {
              const guchar *s = src + 3 * (iy * src_width + ix);

              dest[0] = s[0];
              dest[1] = s[1];
              dest[2] = s[2];
            }
          else
            {
              dest[0] = 0;
              dest[1] = 0;
              dest[2] = 0;
            }

          dest += 3;
        }
    }

  return result;
}


/*  private functions  */

static void
gimp_brush_transform_matrix (GimpBrush   *brush,
                             gdouble      scale_x,
                             gdouble      scale_y,
                             gdouble      angle,
                             GimpMatrix3 *matrix)
{
  const gdouble center_x = brush->mask->width  / 2;
  const gdouble center_y = brush->mask->height / 2;

  gimp_matrix3_identity (matrix);
  gimp_matrix3_translate (matrix, - center_x, - center_x);
  gimp_matrix3_rotate (matrix, -2 * G_PI * angle);
  gimp_matrix3_translate (matrix, center_x, center_y);
  gimp_matrix3_scale (matrix, scale_x, scale_y);
}

static void
gimp_brush_transform_bounding_box (GimpBrush         *brush,
                                   const GimpMatrix3 *matrix,
                                   gint              *x,
                                   gint              *y,
                                   gint              *width,
                                   gint              *height)
{
  const gdouble  w = brush->mask->width;
  const gdouble  h = brush->mask->height;
  gdouble        x1, x2, x3, x4;
  gdouble        y1, y2, y3, y4;

  gimp_matrix3_transform_point (matrix, 0, 0, &x1, &y1);
  gimp_matrix3_transform_point (matrix, w, 0, &x2, &y2);
  gimp_matrix3_transform_point (matrix, 0, h, &x3, &y3);
  gimp_matrix3_transform_point (matrix, w, h, &x4, &y4);

  *x = floor (MIN (MIN (x1, x2), MIN (x3, x4)));
  *y = floor (MIN (MIN (y1, y2), MIN (y3, y4)));

  *width  = (gint) (ceil  (MAX (MAX (x1, x2), MAX (x3, x4))) - *x);
  *height = (gint) (ceil  (MAX (MAX (y1, y2), MAX (y3, y4))) - *y);
}
