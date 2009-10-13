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

#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "paint-funcs/paint-funcs.h"

#include "base/pixel-processor.h"
#include "base/pixel-region.h"

#include "gimpchannel.h"
#include "gimpchannel-combine.h"


void
gimp_channel_combine_rect (GimpChannel    *mask,
                           GimpChannelOps  op,
                           gint            x,
                           gint            y,
                           gint            w,
                           gint            h)
{
  PixelRegion maskPR;
  guchar      color;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  if (! gimp_rectangle_intersect (x, y, w, h,
                                  0, 0,
                                  gimp_item_get_width  (GIMP_ITEM (mask)),
                                  gimp_item_get_height (GIMP_ITEM (mask)),
                                  &x, &y, &w, &h))
    return;

  pixel_region_init (&maskPR, gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                     x, y, w, h, TRUE);

  if (op == GIMP_CHANNEL_OP_ADD || op == GIMP_CHANNEL_OP_REPLACE)
    color = OPAQUE_OPACITY;
  else
    color = TRANSPARENT_OPACITY;

  color_region (&maskPR, &color);

  /*  Determine new boundary  */
  if (mask->bounds_known && (op == GIMP_CHANNEL_OP_ADD) && ! mask->empty)
    {
      if (x < mask->x1)
        mask->x1 = x;
      if (y < mask->y1)
        mask->y1 = y;
      if ((x + w) > mask->x2)
        mask->x2 = (x + w);
      if ((y + h) > mask->y2)
        mask->y2 = (y + h);
    }
  else if (op == GIMP_CHANNEL_OP_REPLACE || mask->empty)
    {
      mask->empty = FALSE;
      mask->x1    = x;
      mask->y1    = y;
      mask->x2    = x + w;
      mask->y2    = y + h;
    }
  else
    {
      mask->bounds_known = FALSE;
    }

  mask->x1 = CLAMP (mask->x1, 0, gimp_item_get_width  (GIMP_ITEM (mask)));
  mask->y1 = CLAMP (mask->y1, 0, gimp_item_get_height (GIMP_ITEM (mask)));
  mask->x2 = CLAMP (mask->x2, 0, gimp_item_get_width  (GIMP_ITEM (mask)));
  mask->y2 = CLAMP (mask->y2, 0, gimp_item_get_height (GIMP_ITEM (mask)));

  gimp_drawable_update (GIMP_DRAWABLE (mask), x, y, w, h);
}

/**
 * gimp_channel_combine_ellipse:
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
void
gimp_channel_combine_ellipse (GimpChannel    *mask,
                              GimpChannelOps  op,
                              gint            x,
                              gint            y,
                              gint            w,
                              gint            h,
                              gboolean        antialias)
{
  gimp_channel_combine_ellipse_rect (mask, op, x, y, w, h,
                                     w / 2.0, h / 2.0, antialias);
}

static void
gimp_channel_combine_span (guchar         *data,
                           GimpChannelOps  op,
                           gint            x1,
                           gint            x2,
                           gint            value)
{
  if (x2 <= x1)
    return;

  switch (op)
    {
    case GIMP_CHANNEL_OP_ADD:
    case GIMP_CHANNEL_OP_REPLACE:
      if (value == 255)
        {
          memset (data + x1, 255, x2 - x1);
        }
      else
        {
          while (x1 < x2)
            {
              const guint val = data[x1] + value;
              data[x1++] = val > 255 ? 255 : val;
            }
        }
      break;

    case GIMP_CHANNEL_OP_SUBTRACT:
      if (value == 255)
        {
          memset (data + x1, 0, x2 - x1);
        }
      else
        {
          while (x1 < x2)
            {
              const gint val = data[x1] - value;
              data[x1++] = val > 0 ? val : 0;
            }
        }
      break;

    case GIMP_CHANNEL_OP_INTERSECT:
      /* Should not happen */
      break;
    }
}

/**
 * gimp_channel_combine_ellipse_rect:
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
void
gimp_channel_combine_ellipse_rect (GimpChannel    *mask,
                                   GimpChannelOps  op,
                                   gint            x,
                                   gint            y,
                                   gint            w,
                                   gint            h,
                                   gdouble         a,
                                   gdouble         b,
                                   gboolean        antialias)
{
  PixelRegion  maskPR;
  gdouble      a_sqr;
  gdouble      b_sqr;
  gdouble      ellipse_center_x;
  gint         x0, y0;
  gint         width, height;
  gpointer     pr;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));
  g_return_if_fail (a >= 0.0 && b >= 0.0);
  g_return_if_fail (op != GIMP_CHANNEL_OP_INTERSECT);

  /* Make sure the elliptic corners fit into the rect */
  a = MIN (a, w / 2.0);
  b = MIN (b, h / 2.0);

  a_sqr = SQR (a);
  b_sqr = SQR (b);

  if (! gimp_rectangle_intersect (x, y, w, h,
                                  0, 0,
                                  gimp_item_get_width  (GIMP_ITEM (mask)),
                                  gimp_item_get_height (GIMP_ITEM (mask)),
                                  &x0, &y0, &width, &height))
    return;

  ellipse_center_x = x + a;

  pixel_region_init (&maskPR, gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                     x0, y0, width, height, TRUE);

  for (pr = pixel_regions_register (1, &maskPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      guchar *data = maskPR.data;
      gint    py;

      for (py = maskPR.y;
           py < maskPR.y + maskPR.h;
           py++, data += maskPR.rowstride)
        {
          const gint px = maskPR.x;
          gdouble    ellipse_center_y;

          if (py >= y + b && py < y + h - b)
            {
              /*  we are on a row without rounded corners  */
              gimp_channel_combine_span (data, op, 0, maskPR.w, 255);
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
              gdouble     half_ellipse_width_at_y;
              gint        x_start;
              gint        x_end;

              half_ellipse_width_at_y =
                sqrt (a_sqr -
                      a_sqr * SQR (py + 0.5f - ellipse_center_y) / b_sqr);

              x_start = ROUND (ellipse_center_x - half_ellipse_width_at_y);
              x_end   = ROUND (ellipse_center_x + w - 2 * a +
                               half_ellipse_width_at_y);

              gimp_channel_combine_span (data, op,
                                         MAX (x_start - px, 0),
                                         MIN (x_end   - px, maskPR.w), 255);
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
              gint         last_val = -1;
              gint         x_start  = px;
              gint         cur_x;

              for (cur_x = px; cur_x < (px + maskPR.w); cur_x++)
                {
                  gfloat  xj;
                  gfloat  xdist;
                  gfloat  ydist;
                  gfloat  r;
                  gfloat  dist;
                  gint    val;

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
                    dist = 0.;
                  else
                    dist = xdist * ydist / r; /* trig formula for distance to
                                               * hypotenuse
                                               */

                  if (xdist < 0.0)
                    dist *= -1;

                  if (dist < -0.5)
                    val = 255;
                  else if (dist < 0.5)
                    val = (gint) (255 * (1 - (dist + 0.5)));
                  else
                    val = 0;

                  if (last_val != val)
                    {
                      if (last_val != -1)
                        gimp_channel_combine_span (data, op,
                                                   MAX (x_start - px, 0),
                                                   MIN (cur_x   - px, maskPR.w),
                                                   last_val);

                      x_start = cur_x;
                      last_val = val;
                    }

                  /*  skip ahead if we are on the straight segment
                   *  between rounded corners
                   */
                  if (cur_x >= x + a && cur_x < x + w - a)
                    {
                      gimp_channel_combine_span (data, op,
                                                 MAX (x_start - px, 0),
                                                 MIN (cur_x   - px, maskPR.w),
                                                 last_val);

                      x_start = cur_x;
                      cur_x = x + w - a;
                      last_val = val = 255;
                    }

                  /* Time to change center? */
                  if (cur_x >= x + w / 2)
                    {
                      ellipse_center_x = x + w - a;
                    }
                }

              gimp_channel_combine_span (data, op,
                                         MAX (x_start - px, 0),
                                         MIN (cur_x   - px, maskPR.w),
                                         last_val);
            }
        }
    }

  /*  use the intersected values for the boundary calculation  */
  x = x0;
  y = y0;
  w = width;
  h = height;

  /*  determine new boundary  */
  if (mask->bounds_known && (op == GIMP_CHANNEL_OP_ADD) && ! mask->empty)
    {
      if (x < mask->x1)
        mask->x1 = x;
      if (y < mask->y1)
        mask->y1 = y;
      if ((x + w) > mask->x2)
        mask->x2 = (x + w);
      if ((y + h) > mask->y2)
        mask->y2 = (y + h);
    }
  else if (op == GIMP_CHANNEL_OP_REPLACE || mask->empty)
    {
      mask->empty = FALSE;
      mask->x1    = x;
      mask->y1    = y;
      mask->x2    = x + w;
      mask->y2    = y + h;
    }
  else
    {
      mask->bounds_known = FALSE;
    }

  gimp_drawable_update (GIMP_DRAWABLE (mask), x, y, w, h);
}

static void
gimp_channel_combine_sub_region_add (gpointer     unused,
                                     PixelRegion *srcPR,
                                     PixelRegion *destPR)
{
  const guchar *src  = srcPR->data;
  guchar       *dest = destPR->data;
  gint          x, y;

  for (y = 0; y < srcPR->h; y++)
    {
      for (x = 0; x < srcPR->w; x++)
        {
          const guint val = dest[x] + src[x];

          dest[x] = val > 255 ? 255 : val;
        }

      src  += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}

static void
gimp_channel_combine_sub_region_sub (gpointer     unused,
                                     PixelRegion *srcPR,
                                     PixelRegion *destPR)
{
  const guchar *src  = srcPR->data;
  guchar       *dest = destPR->data;
  gint          x, y;

  for (y = 0; y < srcPR->h; y++)
    {
      for (x = 0; x < srcPR->w; x++)
        {
          if (src[x] > dest[x])
            dest[x] = 0;
          else
            dest[x] -= src[x];
        }

      src  += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}

static void
gimp_channel_combine_sub_region_intersect (gpointer     unused,
                                           PixelRegion *srcPR,
                                           PixelRegion *destPR)
{
  const guchar *src  = srcPR->data;
  guchar       *dest = destPR->data;
  gint          x, y;

  for (y = 0; y < srcPR->h; y++)
    {
      for (x = 0; x < srcPR->w; x++)
        {
          dest[x] = MIN (dest[x], src[x]);
        }

      src  += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}

void
gimp_channel_combine_mask (GimpChannel    *mask,
                           GimpChannel    *add_on,
                           GimpChannelOps  op,
                           gint            off_x,
                           gint            off_y)
{
  PixelRegion srcPR, destPR;
  gint        x, y, w, h;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));
  g_return_if_fail (GIMP_IS_CHANNEL (add_on));

  if (! gimp_rectangle_intersect (off_x, off_y,
                                  gimp_item_get_width  (GIMP_ITEM (add_on)),
                                  gimp_item_get_height (GIMP_ITEM (add_on)),
                                  0, 0,
                                  gimp_item_get_width  (GIMP_ITEM (mask)),
                                  gimp_item_get_height (GIMP_ITEM (mask)),
                                  &x, &y, &w, &h))
    return;

  pixel_region_init (&srcPR, gimp_drawable_get_tiles (GIMP_DRAWABLE (add_on)),
                     x - off_x, y - off_y, w, h, FALSE);
  pixel_region_init (&destPR, gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                     x, y, w, h, TRUE);

  switch (op)
    {
    case GIMP_CHANNEL_OP_ADD:
    case GIMP_CHANNEL_OP_REPLACE:
      pixel_regions_process_parallel ((PixelProcessorFunc)
                                      gimp_channel_combine_sub_region_add,
                                      NULL, 2, &srcPR, &destPR);
      break;

    case GIMP_CHANNEL_OP_SUBTRACT:
      pixel_regions_process_parallel ((PixelProcessorFunc)
                                      gimp_channel_combine_sub_region_sub,
                                      NULL, 2, &srcPR, &destPR);
      break;

    case GIMP_CHANNEL_OP_INTERSECT:
      pixel_regions_process_parallel ((PixelProcessorFunc)
                                      gimp_channel_combine_sub_region_intersect,
                                      NULL, 2, &srcPR, &destPR);
      break;

    default:
      g_warning ("%s: unknown operation type", G_STRFUNC);
      break;
    }

  mask->bounds_known = FALSE;

  gimp_drawable_update (GIMP_DRAWABLE (mask), x, y, w, h);
}
