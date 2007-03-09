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

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "paint-funcs/paint-funcs.h"

#include "base/pixel-processor.h"
#include "base/pixel-region.h"

#include "gimpchannel.h"
#include "gimpchannel-combine.h"


void
gimp_channel_add_segment (GimpChannel *mask,
                          gint         x,
                          gint         y,
                          gint         width,
                          gint         value)
{
  PixelRegion  maskPR;
  guchar      *data;
  gint         val;
  gint         x2;
  gpointer     pr;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  /*  check horizontal extents...  */
  x2 = x + width;
  x2 = CLAMP (x2, 0, GIMP_ITEM (mask)->width);
  x  = CLAMP (x,  0, GIMP_ITEM (mask)->width);
  width = x2 - x;
  if (!width)
    return;

  if (y < 0 || y >= GIMP_ITEM (mask)->height)
    return;

  pixel_region_init (&maskPR, GIMP_DRAWABLE (mask)->tiles,
                     x, y, width, 1, TRUE);

  for (pr = pixel_regions_register (1, &maskPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      data = maskPR.data;
      width = maskPR.w;
      while (width--)
        {
          val = *data + value;
          if (val > 255)
            val = 255;
          *data++ = val;
        }
    }
}

void
gimp_channel_sub_segment (GimpChannel *mask,
                          gint         x,
                          gint         y,
                          gint         width,
                          gint         value)
{
  PixelRegion  maskPR;
  guchar      *data;
  gint         val;
  gint         x2;
  gpointer     pr;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  /*  check horizontal extents...  */
  x2 = x + width;
  x2 = CLAMP (x2, 0, GIMP_ITEM (mask)->width);
  x =  CLAMP (x,  0, GIMP_ITEM (mask)->width);
  width = x2 - x;

  if (! width)
    return;

  if (y < 0 || y > GIMP_ITEM (mask)->height)
    return;

  pixel_region_init (&maskPR, GIMP_DRAWABLE (mask)->tiles,
                     x, y, width, 1, TRUE);

  for (pr = pixel_regions_register (1, &maskPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      data = maskPR.data;
      width = maskPR.w;
      while (width--)
        {
          val = *data - value;
          if (val < 0)
            val = 0;
          *data++ = val;
        }
    }
}

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
                                  GIMP_ITEM (mask)->width,
                                  GIMP_ITEM (mask)->height,
                                  &x, &y, &w, &h))
    return;

  pixel_region_init (&maskPR, GIMP_DRAWABLE (mask)->tiles,
                     x, y, w, h, TRUE);

  if (op == GIMP_CHANNEL_OP_ADD || op == GIMP_CHANNEL_OP_REPLACE)
    color = OPAQUE_OPACITY;
  else
    color = TRANSPARENT_OPACITY;

  color_region (&maskPR, &color);

  /*  Determine new boundary  */
  if (mask->bounds_known && (op == GIMP_CHANNEL_OP_ADD) && !mask->empty)
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
    mask->bounds_known = FALSE;

  mask->x1 = CLAMP (mask->x1, 0, GIMP_ITEM (mask)->width);
  mask->y1 = CLAMP (mask->y1, 0, GIMP_ITEM (mask)->height);
  mask->x2 = CLAMP (mask->x2, 0, GIMP_ITEM (mask)->width);
  mask->y2 = CLAMP (mask->y2, 0, GIMP_ITEM (mask)->height);

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

static inline void
gimp_channel_combine_segment (GimpChannel    *mask,
                              GimpChannelOps  op,
                              gint            start,
                              gint            row,
                              gint            width,
                              gint            value)
{
  switch (op)
    {
    case GIMP_CHANNEL_OP_ADD:
    case GIMP_CHANNEL_OP_REPLACE:
      gimp_channel_add_segment (mask, start, row, width, value);
      break;

    case GIMP_CHANNEL_OP_SUBTRACT:
      gimp_channel_sub_segment (mask, start, row, width, value);
      break;

    case GIMP_CHANNEL_OP_INTERSECT:
      /* Should not happend */
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
  gint    cur_y;

  gdouble a_sqr;
  gdouble b_sqr;

  gdouble straight_width;
  gdouble straight_height;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));
  g_return_if_fail (a >= 0.0 && b >= 0.0);

  if (! gimp_rectangle_intersect (x, y, w, h,
                                  0, 0,
                                  GIMP_ITEM (mask)->width,
                                  GIMP_ITEM (mask)->height,
                                  NULL, NULL, NULL, NULL))
  {
    return;
  }

  /*  Allow us to use gimp_channel_combine_segment without breaking
   *  previous logic
   */
  if (op == GIMP_CHANNEL_OP_INTERSECT)
    return;

  /* Make sure the elliptic corners fit into the rect */
  a = MIN (a, w / 2.0);
  b = MIN (b, h / 2.0);

  a_sqr = SQR (a);
  b_sqr = SQR (b);

  straight_width  = w - 2 * a;
  straight_height = h - 2 * b;

  for (cur_y = y; cur_y < (y + h); cur_y++)
    {
      gdouble x_start;
      gdouble x_end;
      gdouble ellipse_center_x;
      gdouble ellipse_center_y;
      gdouble half_ellipse_width_at_y;

      /* If this row is not part of the mask, continue with the next row */
      if (cur_y < 0 || cur_y >= GIMP_ITEM (mask)->height)
        {
          continue;
        }

      /* If we are on a row not affected by rounded corners, simply combine the
       * whole row.
       */
      if (cur_y >= y + b && cur_y < y + b + straight_height)
        {
          x_start = x;
          x_end  = x + w;

          gimp_channel_combine_segment (mask, op, x_start,
                                        cur_y, x_end - x_start, 255);
          continue;
        }

      /* Match the ellipse center y with our current y */
      if (cur_y < y + b)
        {
          ellipse_center_y = y + b;
        }
      else
        {
          ellipse_center_y = y + b + straight_height;
        }

      /* For a non-antialiased ellipse, use the normal equation for an ellipse
       * with an arbitrary center (ellipse_center_x, ellipse_center_y).
       */
      if (!antialias)
        {
          ellipse_center_x = x + a;

          half_ellipse_width_at_y =
            sqrt (a_sqr - a_sqr * SQR (cur_y + 0.5f - ellipse_center_y) / b_sqr);

          x_start = ROUND (ellipse_center_x - half_ellipse_width_at_y);
          x_end   = ROUND (ellipse_center_x + straight_width +
                           half_ellipse_width_at_y);

          gimp_channel_combine_segment (mask, op, x_start,
                                        cur_y, x_end - x_start, 255);
        }
      else  /* use antialiasing */
        {
          /* algorithm changed 7-18-04, because the previous one did not
           * work well for eccentric ellipses.  The new algorithm
           * measures the distance to the ellipse in the X and Y directions,
           * and uses trigonometry to approximate the distance to the
           * ellipse as the distance to the hypotenuse of a right triangle
           * whose legs are the X and Y distances.  (WES)
           */

          gint   val;
          gint   last_val;
          gint   x_start;
          gint   cur_x;
          gfloat xj, yi;
          gfloat xdist, ydist;
          gfloat r;
          gfloat dist;

          x_start = x;
          yi = ABS (cur_y + 0.5 - ellipse_center_y);

          last_val = 0;

          ellipse_center_x = x + a;

          for (cur_x = x; cur_x < (x + w); cur_x++)
            {
              xj =  ABS (cur_x + 0.5 - ellipse_center_x);

              if (yi < b)
                xdist = xj - a * sqrt (1 - yi * yi / b_sqr);
              else
                xdist = 100.0;  /* anything large will work */

              if (xj < a)
                ydist = yi - b * sqrt (1 - xj * xj / a_sqr);
              else
                ydist = 100.0;  /* anything large will work */

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

              gimp_channel_combine_segment (mask, op,
                                            x_start, cur_y,
                                            cur_x - x_start,
                                            last_val);

              if (last_val != val)
                {
                  x_start = cur_x;
                  last_val = val;

                  /*  because we are symetric accross the y axis we can
                   *  skip ahead a bit if we are inside. Do this if we
                   *  have reached a value of 255 OR if we have passed
                   *  the center of the leftmost ellipse.
                   */
                  if ((val == 255 || cur_x >= x + a) && cur_x < x + w / 2)
                    {
                      last_val = val = 255;
                      cur_x = (ellipse_center_x +
                               (ellipse_center_x - cur_x) - 1 +
                               floor (straight_width));
                    }
                }

              /* Time to change center? */
              if (cur_x >= x + w / 2)
                {
                  ellipse_center_x = x + a + straight_width;
                }

            }

          gimp_channel_combine_segment (mask, op, x_start,
                                        cur_y, cur_x - x_start, last_val);
        }
    }

  /*  determine new boundary  */
  if (mask->bounds_known && (op == GIMP_CHANNEL_OP_ADD) && !mask->empty)
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

  mask->x1 = CLAMP (mask->x1, 0, GIMP_ITEM (mask)->width);
  mask->y1 = CLAMP (mask->y1, 0, GIMP_ITEM (mask)->height);
  mask->x2 = CLAMP (mask->x2, 0, GIMP_ITEM (mask)->width);
  mask->y2 = CLAMP (mask->y2, 0, GIMP_ITEM (mask)->height);

  gimp_drawable_update (GIMP_DRAWABLE (mask), x, y, w, h);
}

static void
gimp_channel_combine_sub_region_add (gpointer     unused,
                                     PixelRegion *srcPR,
                                     PixelRegion *destPR)
{
  guchar *src, *dest;
  gint    x, y, val;

  src  = srcPR->data;
  dest = destPR->data;

  for (y = 0; y < srcPR->h; y++)
    {
      for (x = 0; x < srcPR->w; x++)
        {
          val = dest[x] + src[x];
          if (val > 255)
            dest[x] = 255;
          else
            dest[x] = val;
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
  guchar *src, *dest;
  gint    x, y;

  src  = srcPR->data;
  dest = destPR->data;

  for (y = 0; y < srcPR->h; y++)
    {
      for (x = 0; x < srcPR->w; x++)
        {
          if (src[x] > dest[x])
            dest[x] = 0;
          else
            dest[x]-= src[x];
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
  guchar *src, *dest;
  gint    x, y;

  src  = srcPR->data;
  dest = destPR->data;

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
                                  GIMP_ITEM (add_on)->width,
                                  GIMP_ITEM (add_on)->height,
                                  0, 0,
                                  GIMP_ITEM (mask)->width,
                                  GIMP_ITEM (mask)->height,
                                  &x, &y, &w, &h))
    return;

  pixel_region_init (&srcPR, GIMP_DRAWABLE (add_on)->tiles,
                     x - off_x, y - off_y, w, h, FALSE);
  pixel_region_init (&destPR, GIMP_DRAWABLE (mask)->tiles,
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
