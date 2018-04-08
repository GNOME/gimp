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

#include <string.h>

#include <gio/gio.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "gimp-gegl-types.h"

#include "gimp-babl.h"
#include "gimp-gegl-mask-combine.h"


gboolean
gimp_gegl_mask_combine_rect (GeglBuffer     *mask,
                             GimpChannelOps  op,
                             gint            x,
                             gint            y,
                             gint            w,
                             gint            h)
{
  GeglColor *color;

  g_return_val_if_fail (GEGL_IS_BUFFER (mask), FALSE);

  if (! gimp_rectangle_intersect (x, y, w, h,
                                  0, 0,
                                  gegl_buffer_get_width  (mask),
                                  gegl_buffer_get_height (mask),
                                  &x, &y, &w, &h))
    return FALSE;

  if (op == GIMP_CHANNEL_OP_ADD || op == GIMP_CHANNEL_OP_REPLACE)
    color = gegl_color_new ("#fff");
  else
    color = gegl_color_new ("#000");

  gegl_buffer_set_color (mask, GEGL_RECTANGLE (x, y, w, h), color);
  g_object_unref (color);

  return TRUE;
}

/**
 * gimp_gegl_mask_combine_ellipse:
 * @mask:      the channel with which to combine the ellipse
 * @op:        whether to replace, add to, or subtract from the current
 *             contents
 * @x:         x coordinate of upper left corner of ellipse
 * @y:         y coordinate of upper left corner of ellipse
 * @w:         width of ellipse bounding box
 * @h:         height of ellipse bounding box
 * @antialias: if %TRUE, antialias the ellipse
 *
 * Mainly used for elliptical selections.  If @op is
 * %GIMP_CHANNEL_OP_REPLACE or %GIMP_CHANNEL_OP_ADD, sets pixels
 * within the ellipse to 255.  If @op is %GIMP_CHANNEL_OP_SUBTRACT,
 * sets pixels within to zero.  If @antialias is %TRUE, pixels that
 * impinge on the edge of the ellipse are set to intermediate values,
 * depending on how much they overlap.
 **/
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

static void
gimp_gegl_mask_combine_span (gfloat         *data,
                             GimpChannelOps  op,
                             gint            x1,
                             gint            x2,
                             gfloat          value)
{
  if (x2 <= x1)
    return;

  switch (op)
    {
    case GIMP_CHANNEL_OP_ADD:
    case GIMP_CHANNEL_OP_REPLACE:
      if (value == 1.0)
        {
          while (x1 < x2)
            data[x1++] = 1.0;
        }
      else
        {
          while (x1 < x2)
            {
              const gfloat val = data[x1] + value;
              data[x1++] = val > 1.0 ? 1.0 : val;
            }
        }
      break;

    case GIMP_CHANNEL_OP_SUBTRACT:
      if (value == 1.0)
        {
          while (x1 < x2)
            data[x1++] = 0.0;
        }
      else
        {
          while (x1 < x2)
            {
              const gfloat val = data[x1] - value;
              data[x1++] = val > 0.0 ? val : 0.0;
            }
        }
      break;

    case GIMP_CHANNEL_OP_INTERSECT:
      /* Should not happen */
      break;
    }
}

/**
 * gimp_gegl_mask_combine_ellipse_rect:
 * @mask:      the channel with which to combine the elliptic rect
 * @op:        whether to replace, add to, or subtract from the current
 *             contents
 * @x:         x coordinate of upper left corner of bounding rect
 * @y:         y coordinate of upper left corner of bounding rect
 * @w:         width of bounding rect
 * @h:         height of bounding rect
 * @a:         elliptic a-constant applied to corners
 * @b:         elliptic b-constant applied to corners
 * @antialias: if %TRUE, antialias the elliptic corners
 *
 * Used for rounded cornered rectangles and ellipses.  If @op is
 * %GIMP_CHANNEL_OP_REPLACE or %GIMP_CHANNEL_OP_ADD, sets pixels
 * within the ellipse to 255.  If @op is %GIMP_CHANNEL_OP_SUBTRACT,
 * sets pixels within to zero.  If @antialias is %TRUE, pixels that
 * impinge on the edge of the ellipse are set to intermediate values,
 * depending on how much they overlap.
 **/
gboolean
gimp_gegl_mask_combine_ellipse_rect (GeglBuffer     *mask,
                                     GimpChannelOps  op,
                                     gint            x,
                                     gint            y,
                                     gint            w,
                                     gint            h,
                                     gdouble         a,
                                     gdouble         b,
                                     gboolean        antialias)
{
  GeglBufferIterator *iter;
  GeglRectangle      *roi;
  gdouble             a_sqr;
  gdouble             b_sqr;
  gdouble             ellipse_center_x;
  gint                x0, y0;
  gint                width, height;

  g_return_val_if_fail (GEGL_IS_BUFFER (mask), FALSE);
  g_return_val_if_fail (a >= 0.0 && b >= 0.0, FALSE);
  g_return_val_if_fail (op != GIMP_CHANNEL_OP_INTERSECT, FALSE);

  /* Make sure the elliptic corners fit into the rect */
  a = MIN (a, w / 2.0);
  b = MIN (b, h / 2.0);

  a_sqr = SQR (a);
  b_sqr = SQR (b);

  if (! gimp_rectangle_intersect (x, y, w, h,
                                  0, 0,
                                  gegl_buffer_get_width  (mask),
                                  gegl_buffer_get_height (mask),
                                  &x0, &y0, &width, &height))
    return FALSE;

  ellipse_center_x = x + a;

  iter = gegl_buffer_iterator_new (mask,
                                   GEGL_RECTANGLE (x0, y0, width, height), 0,
                                   babl_format ("Y float"),
                                   GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);
  roi = &iter->roi[0];

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *data = iter->data[0];
      gint    py;

      for (py = roi->y;
           py < roi->y + roi->height;
           py++, data += roi->width)
        {
          const gint px = roi->x;
          gdouble    ellipse_center_y;

          if (py >= y + b && py < y + h - b)
            {
              /*  we are on a row without rounded corners  */
              gimp_gegl_mask_combine_span (data, op, 0, roi->width, 1.0);
              continue;
            }

          /* Match the ellipse center y with our current y */
          if (py < y + b)
            {
              ellipse_center_y = y + b;
            }
          else
            {
              ellipse_center_y = y + h - b;
            }

          /* For a non-antialiased ellipse, use the normal equation
           * for an ellipse with an arbitrary center
           * (ellipse_center_x, ellipse_center_y).
           */
          if (! antialias)
            {
              gdouble half_ellipse_width_at_y;
              gint    x_start;
              gint    x_end;

              half_ellipse_width_at_y =
                sqrt (a_sqr -
                      a_sqr * SQR (py + 0.5f - ellipse_center_y) / b_sqr);

              x_start = ROUND (ellipse_center_x - half_ellipse_width_at_y);
              x_end   = ROUND (ellipse_center_x + w - 2 * a +
                               half_ellipse_width_at_y);

              gimp_gegl_mask_combine_span (data, op,
                                           MAX (x_start - px, 0),
                                           MIN (x_end   - px, roi->width), 1.0);
            }
          else  /* use antialiasing */
            {
              /* algorithm changed 7-18-04, because the previous one
               * did not work well for eccentric ellipses.  The new
               * algorithm measures the distance to the ellipse in the
               * X and Y directions, and uses trigonometry to
               * approximate the distance to the ellipse as the
               * distance to the hypotenuse of a right triangle whose
               * legs are the X and Y distances.  (WES)
               */
              const gfloat yi       = ABS (py + 0.5 - ellipse_center_y);
              gfloat       last_val = -1;
              gint         x_start  = px;
              gint         cur_x;

              for (cur_x = px; cur_x < (px + roi->width); cur_x++)
                {
                  gfloat  xj;
                  gfloat  xdist;
                  gfloat  ydist;
                  gfloat  r;
                  gfloat  dist;
                  gfloat  val;

                  if (cur_x < x + w / 2)
                    {
                      ellipse_center_x = x + a;
                    }
                  else
                    {
                      ellipse_center_x = x + w - a;
                    }

                  xj = ABS (cur_x + 0.5 - ellipse_center_x);

                  if (yi < b)
                    xdist = xj - a * sqrt (1 - SQR (yi) / b_sqr);
                  else
                    xdist = 1000.0;  /* anything large will work */

                  if (xj < a)
                    ydist = yi - b * sqrt (1 - SQR (xj) / a_sqr);
                  else
                    ydist = 1000.0;  /* anything large will work */

                  r = hypot (xdist, ydist);

                  if (r < 0.001)
                    dist = 0.0;
                  else
                    dist = xdist * ydist / r; /* trig formula for distance to
                                               * hypotenuse
                                               */

                  if (xdist < 0.0)
                    dist *= -1;

                  if (dist < -0.5)
                    val = 1.0;
                  else if (dist < 0.5)
                    val = (1.0 - (dist + 0.5));
                  else
                    val = 0.0;

                  if (last_val != val)
                    {
                      if (last_val != -1)
                        gimp_gegl_mask_combine_span (data, op,
                                                     MAX (x_start - px, 0),
                                                     MIN (cur_x   - px, roi->width),
                                                     last_val);

                      x_start = cur_x;
                      last_val = val;
                    }

                  /*  skip ahead if we are on the straight segment
                   *  between rounded corners
                   */
                  if (cur_x >= x + a && cur_x < x + w - a)
                    {
                      gimp_gegl_mask_combine_span (data, op,
                                                   MAX (x_start - px, 0),
                                                   MIN (cur_x   - px, roi->width),
                                                   last_val);

                      x_start = cur_x;
                      cur_x = x + w - a;
                      last_val = val = 1.0;
                    }

                  /* Time to change center? */
                  if (cur_x >= x + w / 2)
                    {
                      ellipse_center_x = x + w - a;
                    }
                }

              gimp_gegl_mask_combine_span (data, op,
                                           MAX (x_start - px, 0),
                                           MIN (cur_x   - px, roi->width),
                                           last_val);
            }
        }
    }

  return TRUE;
}

gboolean
gimp_gegl_mask_combine_buffer (GeglBuffer     *mask,
                               GeglBuffer     *add_on,
                               GimpChannelOps  op,
                               gint            off_x,
                               gint            off_y)
{
  GeglBufferIterator *iter;
  GeglRectangle       rect;
  const Babl         *add_on_format;
  gint                x, y, w, h;

  g_return_val_if_fail (GEGL_IS_BUFFER (mask), FALSE);
  g_return_val_if_fail (GEGL_IS_BUFFER (add_on), FALSE);

  if (! gimp_rectangle_intersect (off_x, off_y,
                                  gegl_buffer_get_width  (add_on),
                                  gegl_buffer_get_height (add_on),
                                  0, 0,
                                  gegl_buffer_get_width  (mask),
                                  gegl_buffer_get_height (mask),
                                  &x, &y, &w, &h))
    return FALSE;

  rect.x      = x;
  rect.y      = y;
  rect.width  = w;
  rect.height = h;

  iter = gegl_buffer_iterator_new (mask, &rect, 0,
                                   babl_format ("Y float"),
                                   GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

  rect.x -= off_x;
  rect.y -= off_y;

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
  if (gimp_babl_format_get_linear (gegl_buffer_get_format (add_on)))
    add_on_format = babl_format ("Y float");
  else
    add_on_format = babl_format ("Y' float");

  gegl_buffer_iterator_add (iter, add_on, &rect, 0,
                            add_on_format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  switch (op)
    {
    case GIMP_CHANNEL_OP_ADD:
    case GIMP_CHANNEL_OP_REPLACE:
      while (gegl_buffer_iterator_next (iter))
        {
          gfloat       *mask_data   = iter->data[0];
          const gfloat *add_on_data = iter->data[1];
          gint          count       = iter->length;

          while (count--)
            {
              const gfloat val = *mask_data + *add_on_data;

              *mask_data = CLAMP (val, 0.0, 1.0);

              add_on_data++;
              mask_data++;
            }
        }
      break;

    case GIMP_CHANNEL_OP_SUBTRACT:
      while (gegl_buffer_iterator_next (iter))
        {
          gfloat       *mask_data   = iter->data[0];
          const gfloat *add_on_data = iter->data[1];
          gint          count       = iter->length;

          while (count--)
            {
              if (*add_on_data > *mask_data)
                *mask_data = 0.0;
              else
                *mask_data -= *add_on_data;

              add_on_data++;
              mask_data++;
            }
        }
      break;

    case GIMP_CHANNEL_OP_INTERSECT:
      while (gegl_buffer_iterator_next (iter))
        {
          gfloat       *mask_data   = iter->data[0];
          const gfloat *add_on_data = iter->data[1];
          gint          count       = iter->length;

          while (count--)
            {
              *mask_data = MIN (*mask_data, *add_on_data);

              add_on_data++;
              mask_data++;
            }
        }
      break;

    default:
      g_warning ("%s: unknown operation type", G_STRFUNC);
      break;
    }

  return TRUE;
}
