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

#include <string.h>

#include <gio/gio.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

extern "C"
{

#include "gimp-gegl-types.h"

#include "gimp-babl.h"
#include "gimp-gegl-loops.h"
#include "gimp-gegl-mask-combine.h"


#define EPSILON 1e-6

#define PIXELS_PER_THREAD \
  (/* each thread costs as much as */ 64.0 * 64.0 /* pixels */)


gboolean
gimp_gegl_mask_combine_rect (GeglBuffer     *mask,
                             GimpChannelOps  op,
                             gint            x,
                             gint            y,
                             gint            w,
                             gint            h)
{
  GeglRectangle rect;
  gfloat        value;

  g_return_val_if_fail (GEGL_IS_BUFFER (mask), FALSE);

  if (! gegl_rectangle_intersect (&rect,
                                  GEGL_RECTANGLE (x, y, w, h),
                                  gegl_buffer_get_abyss (mask)))
    {
      return FALSE;
    }

  switch (op)
    {
    case GIMP_CHANNEL_OP_REPLACE:
    case GIMP_CHANNEL_OP_ADD:
      value = 1.0f;
      break;

    case GIMP_CHANNEL_OP_SUBTRACT:
      value = 0.0f;
      break;

    case GIMP_CHANNEL_OP_INTERSECT:
      return TRUE;
    }

  gegl_buffer_set_color_from_pixel (mask, &rect, &value,
                                    babl_format ("Y float"));

  return TRUE;
}

gboolean
gimp_gegl_mask_combine_ellipse (GeglBuffer     *mask,
                                GimpChannelOps  op,
                                gint            x,
                                gint            y,
                                gint            w,
                                gint            h,
                                gboolean        antialias)
{
  return gimp_gegl_mask_combine_ellipse_rect (mask, op, x, y, w, h,
                                              w / 2.0, h / 2.0, antialias);
}

gboolean
gimp_gegl_mask_combine_ellipse_rect (GeglBuffer     *mask,
                                     GimpChannelOps  op,
                                     gint            x,
                                     gint            y,
                                     gint            w,
                                     gint            h,
                                     gdouble         rx,
                                     gdouble         ry,
                                     gboolean        antialias)
{
  GeglRectangle  rect;
  const Babl    *format;
  gint           bpp;
  gfloat         one_f = 1.0f;
  gpointer       one;
  gdouble        cx;
  gdouble        cy;
  gint           left;
  gint           right;
  gint           top;
  gint           bottom;

  g_return_val_if_fail (GEGL_IS_BUFFER (mask), FALSE);

  if (rx <= EPSILON || ry <= EPSILON)
    return gimp_gegl_mask_combine_rect (mask, op, x, y, w, h);

  left   = x;
  right  = x + w;
  top    = y;
  bottom = y + h;

  cx = (left + right)  / 2.0;
  cy = (top  + bottom) / 2.0;

  rx = MIN (rx, w / 2.0);
  ry = MIN (ry, h / 2.0);

  if (! gegl_rectangle_intersect (&rect,
                                  GEGL_RECTANGLE (x, y, w, h),
                                  gegl_buffer_get_abyss (mask)))
    {
      return FALSE;
    }

  format = gegl_buffer_get_format (mask);

  if (antialias)
    {
      format = gimp_babl_format_change_component_type (
        format, GIMP_COMPONENT_TYPE_FLOAT);
    }

  bpp = babl_format_get_bytes_per_pixel (format);
  one = g_alloca (bpp);

  babl_process (babl_fish ("Y float", format), &one_f, one, 1);

  /* coordinate-system transforms.  (x, y) coordinates are in the image
   * coordinate-system, and (u, v) coordinates are in a coordinate-system
   * aligned with the center of one of the elliptic corners, with the positive
   * directions pointing away from the rectangle.  when converting from (x, y)
   * to (u, v), we use the closest elliptic corner.
   */
  auto x_to_u = [=] (gdouble x)
  {
    if (x < cx)
      return (left + rx) - x;
    else
      return x - (right - rx);
  };

  auto y_to_v = [=] (gdouble y)
  {
    if (y < cy)
      return (top + ry) - y;
    else
      return y - (bottom - ry);
  };

  auto u_to_x_left = [=] (gdouble u)
  {
    return (left + rx) - u;
  };

  auto u_to_x_right = [=] (gdouble u)
  {
    return (right - rx) + u;
  };

  /* intersection of a horizontal line with the ellipse */
  auto v_to_u = [=] (gdouble v)
  {
    if (v > 0.0)
      return sqrt (MAX (SQR (rx) - SQR (rx * v / ry), 0.0));
    else
      return rx;
  };

  /* intersection of a vertical line with the ellipse */
  auto u_to_v = [=] (gdouble u)
  {
    if (u > 0.0)
      return sqrt (MAX (SQR (ry) - SQR (ry * u / rx), 0.0));
    else
      return ry;
  };

  /* signed, normalized distance of a point from the ellipse's circumference.
   * the sign of the result determines if the point is inside (positive) or
   * outside (negative) the ellipse.  the result is normalized to the cross-
   * section length of a pixel, in the direction of the closest point along the
   * ellipse.
   *
   * we use the following method to approximate the distance: pass horizontal
   * and vertical lines at the given point, P, and find their (positive) points
   * of intersection with the ellipse, A and B.  the segment AB is an
   * approximation of the corresponding elliptic arc (see bug #147836).  find
   * the closest point, C, to P, along the segment AB.  find the (positive)
   * point of intersection, Q, of the line PC and the ellipse.  Q is an
   * approximation for the closest point to P along the ellipse, and the
   * approximated distance is the distance from P to Q.
   */
  auto ellipse_distance = [=] (gdouble u,
                               gdouble v)
  {
    gdouble du;
    gdouble dv;
    gdouble t;
    gdouble a, b, c;
    gdouble d;

    u = MAX (u, 0.0);
    v = MAX (v, 0.0);

    du = v_to_u (v) - u;
    dv = u_to_v (u) - v;

    t = SQR (du) / (SQR (du) + SQR (dv));

    du *= 1.0 - t;
    dv *= t;

    v  *= rx / ry;
    dv *= rx / ry;

    a = SQR (du) + SQR (dv);
    b = u * du + v * dv;
    c = SQR (u) + SQR (v) - SQR (rx);

    if (a <= EPSILON)
      return 0.0;

    if (c < 0.0)
      t = (-b + sqrt (MAX (SQR (b) - a * c, 0.0))) / a;
    else
      t = (-b - sqrt (MAX (SQR (b) - a * c, 0.0))) / a;

    dv *= ry / rx;

    d = sqrt (SQR (du * t) + SQR (dv * t));

    if (c > 0.0)
      d = -d;

    d /= sqrt (SQR (MIN (du / dv, dv / du)) + 1.0);

    return d;
  };

  /* anti-aliased value of a pixel */
  auto pixel_value = [=] (gint x,
                          gint y)
  {
    gdouble u = x_to_u (x + 0.5);
    gdouble v = y_to_v (y + 0.5);
    gdouble d = ellipse_distance (u, v);

    /* use the distance of the pixel's center from the ellipse to approximate
     * the coverage
     */
    d = CLAMP (0.5 + d, 0.0, 1.0);

    /* we're at the horizontal boundary of an elliptic corner */
    if (u < 0.5)
      d = d * (0.5 + u) + (0.5 - u);

    /* we're at the vertical boundary of an elliptic corner */
    if (v < 0.5)
      d = d * (0.5 + v) + (0.5 - v);

    /* opposite horizontal corners intersect the pixel */
    if (x == (right - 1) - (x - left))
      d = 2.0 * d - 1.0;

    /* opposite vertical corners intersect the pixel */
    if (y == (bottom - 1) - (y - top))
      d = 2.0 * d - 1.0;

    return d;
  };

  auto ellipse_range = [=] (gdouble  y,
                            gdouble *x0,
                            gdouble *x1)
  {
    gdouble u = v_to_u (y_to_v (y));

    *x0 = u_to_x_left  (u);
    *x1 = u_to_x_right (u);
  };

  auto fill0 = [=] (gpointer dest,
                    gint     n)
  {
    switch (op)
      {
      case GIMP_CHANNEL_OP_REPLACE:
      case GIMP_CHANNEL_OP_INTERSECT:
        memset (dest, 0, bpp * n);
        break;

      case GIMP_CHANNEL_OP_ADD:
      case GIMP_CHANNEL_OP_SUBTRACT:
        break;
      }

    return (gpointer) ((guint8 *) dest + bpp * n);
  };

  auto fill1 = [=] (gpointer dest,
                    gint     n)
  {
    switch (op)
      {
      case GIMP_CHANNEL_OP_REPLACE:
      case GIMP_CHANNEL_OP_ADD:
        gegl_memset_pattern (dest, one, bpp, n);
        break;

      case GIMP_CHANNEL_OP_SUBTRACT:
        memset (dest, 0, bpp * n);
        break;

      case GIMP_CHANNEL_OP_INTERSECT:
        break;
      }

    return (gpointer) ((guint8 *) dest + bpp * n);
  };

  auto set = [=] (gpointer dest,
                  gfloat   value)
  {
    gfloat *p = (gfloat *) dest;

    switch (op)
      {
      case GIMP_CHANNEL_OP_REPLACE:
        *p = value;
        break;

      case GIMP_CHANNEL_OP_ADD:
        *p = MIN (*p + value, 1.0);
        break;

      case GIMP_CHANNEL_OP_SUBTRACT:
        *p = MAX (*p - value, 0.0);
        break;

      case GIMP_CHANNEL_OP_INTERSECT:
        *p = MIN (*p, value);
        break;
      }

    return (gpointer) (p + 1);
  };

  gegl_parallel_distribute_area (
    &rect, PIXELS_PER_THREAD,
    [=] (const GeglRectangle *area)
    {
      GeglBufferIterator *iter;

      iter = gegl_buffer_iterator_new (
        mask, area, 0, format,
        op == GIMP_CHANNEL_OP_REPLACE ? GEGL_ACCESS_WRITE :
                                        GEGL_ACCESS_READWRITE,
        GEGL_ABYSS_NONE, 1);

      while (gegl_buffer_iterator_next (iter))
        {
          const GeglRectangle *roi = &iter->items[0].roi;
          gpointer             d   = iter->items[0].data;
          gdouble              tx0, ty0;
          gdouble              tx1, ty1;
          gdouble              x0;
          gdouble              x1;
          gint                 y;

          /* tile bounds */
          tx0 = roi->x;
          ty0 = roi->y;

          tx1 = roi->x + roi->width;
          ty1 = roi->y + roi->height;

          if (! antialias)
            {
              tx0 += 0.5;
              ty0 += 0.5;

              tx1 -= 0.5;
              ty1 -= 0.5;
            }

          /* if the tile is fully inside/outside the ellipse, fill it with 1/0,
           * respectively, and skip the rest.
           */
          ellipse_range (ty0, &x0, &x1);

          if (tx0 >= x0 && tx1 <= x1)
            {
              ellipse_range (ty1, &x0, &x1);

              if (tx0 >= x0 && tx1 <= x1)
                {
                  fill1 (d, iter->length);

                  continue;
                }
            }
          else if (tx1 < x0 || tx0 > x1)
            {
              ellipse_range (ty1, &x0, &x1);

              if (tx1 < x0 || tx0 > x1)
                {
                  if ((ty0 - cy) * (ty1 - cy) >= 0.0)
                    {
                      fill0 (d, iter->length);

                      continue;
                    }
                }
            }

          for (y = roi->y; y < roi->y + roi->height; y++)
            {
              gint a, b;

              if (antialias)
                {
                  gdouble v  = y_to_v (y + 0.5);
                  gdouble u0 = v_to_u (v - 0.5);
                  gdouble u1 = v_to_u (v + 0.5);
                  gint    x;

                  a = floor (u_to_x_left (u0)) - roi->x;
                  a = CLAMP (a, 0, roi->width);

                  b = ceil  (u_to_x_left (u1)) - roi->x;
                  b = CLAMP (b, a, roi->width);

                  d = fill0 (d, a);

                  for (x = roi->x + a; x < roi->x + b; x++)
                    d = set (d, pixel_value (x, y));

                  a = floor (u_to_x_right (u1)) - roi->x;
                  a = CLAMP (a, b, roi->width);

                  d = fill1 (d, a - b);

                  b = ceil  (u_to_x_right (u0)) - roi->x;
                  b = CLAMP (b, a, roi->width);

                  for (x = roi->x + a; x < roi->x + b; x++)
                    d = set (d, pixel_value (x, y));

                  d = fill0 (d, roi->width - b);
                }
              else
                {
                  ellipse_range (y + 0.5, &x0, &x1);

                  a = ceil  (x0 - 0.5) - roi->x;
                  a = CLAMP (a, 0, roi->width);

                  b = floor (x1 + 0.5) - roi->x;
                  b = CLAMP (b, 0, roi->width);

                  d = fill0 (d, a);
                  d = fill1 (d, b - a);
                  d = fill0 (d, roi->width - b);
                }
            }
        }
    });

  return TRUE;
}

gboolean
gimp_gegl_mask_combine_buffer (GeglBuffer     *mask,
                               GeglBuffer     *add_on,
                               GimpChannelOps  op,
                               gint            off_x,
                               gint            off_y)
{
  GeglRectangle  mask_rect;
  GeglRectangle  add_on_rect;
  const Babl    *mask_format;
  const Babl    *add_on_format;

  g_return_val_if_fail (GEGL_IS_BUFFER (mask), FALSE);
  g_return_val_if_fail (GEGL_IS_BUFFER (add_on), FALSE);

  if (! gegl_rectangle_intersect (&mask_rect,
                                  GEGL_RECTANGLE (
                                    off_x + gegl_buffer_get_x (add_on),
                                    off_y + gegl_buffer_get_y (add_on),
                                    gegl_buffer_get_width  (add_on),
                                    gegl_buffer_get_height (add_on)),
                                  gegl_buffer_get_abyss (mask)))
    {
      return FALSE;
    }

  add_on_rect    = mask_rect;
  add_on_rect.x -= off_x;
  add_on_rect.y -= off_y;

  mask_format   = gegl_buffer_get_format (mask);
  add_on_format = gegl_buffer_get_format (add_on);

  if (op == GIMP_CHANNEL_OP_REPLACE                                          &&
      (gimp_babl_is_bounded (gimp_babl_format_get_precision (add_on_format)) ||
       gimp_babl_is_bounded (gimp_babl_format_get_precision (mask_format))))
    {
      /*  See below: this additional hack is only needed for the
       *  gimp-channel-combine-masks procedure, it's the only place that
       *  allows to combine arbitrary channels with each other.
       */
      gegl_buffer_set_format (
        add_on,
        gimp_babl_format_change_trc (
          add_on_format, gimp_babl_format_get_trc (mask_format)));

      gimp_gegl_buffer_copy (add_on, &add_on_rect, GEGL_ABYSS_NONE,
                             mask,   &mask_rect);

      gegl_buffer_set_format (add_on, NULL);

      return TRUE;
    }

  /*  This is a hack: all selections/layer masks/channels are always
   *  linear except for channels in 8-bit images. We don't want these
   *  "Y' u8" to be converted to "Y float" because that would cause a
   *  gamma canversion and give unexpected results for
   *  "add/subtract/etc channel from selection". Instead, use all
   *  channel values "as-is", which makes no differce except in the
   *  8-bit case where we need it.
   *
   *  See https://bugzilla.gnome.org/show_bug.cgi?id=791519
   */
  mask_format = gimp_babl_format_change_component_type (
    mask_format, GIMP_COMPONENT_TYPE_FLOAT);

  add_on_format = gimp_babl_format_change_component_type (
    add_on_format, GIMP_COMPONENT_TYPE_FLOAT);

  gegl_parallel_distribute_area (
    &mask_rect, PIXELS_PER_THREAD,
    [=] (const GeglRectangle *mask_area)
    {
      GeglBufferIterator *iter;
      GeglRectangle       add_on_area;

      add_on_area    = *mask_area;
      add_on_area.x -= off_x;
      add_on_area.y -= off_y;

      iter = gegl_buffer_iterator_new (mask, mask_area, 0,
                                       mask_format,
                                       op == GIMP_CHANNEL_OP_REPLACE ?
                                         GEGL_ACCESS_WRITE :
                                         GEGL_ACCESS_READWRITE,
                                       GEGL_ABYSS_NONE, 2);

      gegl_buffer_iterator_add (iter, add_on, &add_on_area, 0,
                                add_on_format,
                                GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

      auto process = [=] (auto value)
      {
        while (gegl_buffer_iterator_next (iter))
          {
            gfloat       *mask_data   = (gfloat       *) iter->items[0].data;
            const gfloat *add_on_data = (const gfloat *) iter->items[1].data;
            gint          count       = iter->length;

            while (count--)
              {
                const gfloat val = value (mask_data, add_on_data);

                *mask_data = CLAMP (val, 0.0f, 1.0f);

                add_on_data++;
                mask_data++;
              }
          }
      };

      switch (op)
        {
        case GIMP_CHANNEL_OP_REPLACE:
          process ([] (const gfloat *mask,
                       const gfloat *add_on)
                   {
                     return *add_on;
                   });
          break;

        case GIMP_CHANNEL_OP_ADD:
          process ([] (const gfloat *mask,
                       const gfloat *add_on)
                   {
                     return *mask + *add_on;
                   });
          break;

        case GIMP_CHANNEL_OP_SUBTRACT:
          process ([] (const gfloat *mask,
                       const gfloat *add_on)
                   {
                     return *mask - *add_on;
                   });
          break;

        case GIMP_CHANNEL_OP_INTERSECT:
          process ([] (const gfloat *mask,
                       const gfloat *add_on)
                   {
                     return MIN (*mask, *add_on);
                   });
          break;
        }
    });

  return TRUE;
}

} /* extern "C" */
