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

#include <glib-object.h>

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
  gint        x2, y2;
  PixelRegion maskPR;
  guchar      color;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  y2 = y + h;
  x2 = x + w;

  x  = CLAMP (x,  0, GIMP_ITEM (mask)->width);
  y  = CLAMP (y,  0, GIMP_ITEM (mask)->height);
  x2 = CLAMP (x2, 0, GIMP_ITEM (mask)->width);
  y2 = CLAMP (y2, 0, GIMP_ITEM (mask)->height);

  w = x2 - x;
  h = y2 - y;

  if (w <= 0 || h <= 0)
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

void
gimp_channel_combine_ellipse (GimpChannel    *mask,
			      GimpChannelOps  op,
			      gint            x,
			      gint            y,
			      gint            w,
			      gint            h,
			      gboolean        antialias)
{
  gint   i, j;
  gint   x0, x1, x2;
  gint   val, last;
  gfloat a_sqr, b_sqr, aob_sqr;
  gfloat w_sqr, h_sqr;
  gfloat y_sqr;
  gfloat t0, t1;
  gfloat r;
  gfloat cx, cy;
  gfloat rad;
  gfloat dist;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  if (!w || !h)
    return;

  a_sqr = (w * w / 4.0);
  b_sqr = (h * h / 4.0);
  aob_sqr = a_sqr / b_sqr;

  cx = x + w / 2.0;
  cy = y + h / 2.0;

  for (i = y; i < (y + h); i++)
    {
      if (i >= 0 && i < GIMP_ITEM (mask)->height)
	{
	  /*  Non-antialiased code  */
	  if (!antialias)
	    {
	      y_sqr = (i + 0.5 - cy) * (i + 0.5 - cy);
	      rad = sqrt (a_sqr - a_sqr * y_sqr / (double) b_sqr);
	      x1 = ROUND (cx - rad);
	      x2 = ROUND (cx + rad);

	      switch (op)
		{
		case GIMP_CHANNEL_OP_ADD:
		case GIMP_CHANNEL_OP_REPLACE:
		  gimp_channel_add_segment (mask, x1, i, (x2 - x1), 255);
		  break;

		case GIMP_CHANNEL_OP_SUBTRACT:
		  gimp_channel_sub_segment (mask, x1, i, (x2 - x1), 255);
		  break;

		default:
		  g_warning ("Only ADD, REPLACE, and SUBTRACT are valid for channel_combine!");
		  break;
		}
	    }
	  /*  antialiasing  */
	  else
	    {
	      x0 = x;
	      last = 0;
	      h_sqr = (i + 0.5 - cy) * (i + 0.5 - cy);
	      for (j = x; j < (x + w); j++)
		{
		  w_sqr = (j + 0.5 - cx) * (j + 0.5 - cx);

		  if (h_sqr != 0)
		    {
		      t0 = w_sqr / h_sqr;
		      t1 = a_sqr / (t0 + aob_sqr);
		      r = sqrt (t1 + t0 * t1);
		      rad = sqrt (w_sqr + h_sqr);
		      dist = rad - r;
		    }
		  else
		    dist = -1.0;

		  if (dist < -0.5)
		    val = 255;
		  else if (dist < 0.5)
		    val = (int) (255 * (1 - (dist + 0.5)));
		  else
		    val = 0;

		  if (last != val && last)
		    {
		      switch (op)
			{
			case GIMP_CHANNEL_OP_ADD:
			case GIMP_CHANNEL_OP_REPLACE:
			  gimp_channel_add_segment (mask, x0, i, j - x0, last);
			  break;

			case GIMP_CHANNEL_OP_SUBTRACT:
			  gimp_channel_sub_segment (mask, x0, i, j - x0, last);
			  break;

			default:
			  g_warning ("Only ADD, REPLACE, and SUBTRACT are valid for channel_combine!");
			  break;
			}
		    }

		  if (last != val)
		    {
		      x0 = j;
		      last = val;
		      /* because we are symetric accross the y axis we can
			 skip ahead a bit if we are inside the ellipse*/
		      if (val == 255 && j < cx)
			j = cx + (cx - j) - 1;
		    }
		}

	      if (last)
		{
                  switch (op)
                    {
                    case GIMP_CHANNEL_OP_ADD:
                    case GIMP_CHANNEL_OP_REPLACE:
                      gimp_channel_add_segment (mask, x0, i, j - x0, last);
                      break;

                    case GIMP_CHANNEL_OP_SUBTRACT:
                      gimp_channel_sub_segment (mask, x0, i, j - x0, last);
                      break;

                    default:
                      g_warning ("Only ADD, REPLACE, and SUBTRACT are valid for channel_combine!");
                      break;
                    }
		}
	    }

	}
    }

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
  gint        x1, y1, x2, y2;
  gint        w, h;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));
  g_return_if_fail (GIMP_IS_CHANNEL (add_on));

  x1 = CLAMP (off_x, 0, GIMP_ITEM (mask)->width);
  y1 = CLAMP (off_y, 0, GIMP_ITEM (mask)->height);
  x2 = CLAMP (off_x + GIMP_ITEM (add_on)->width, 0,
	      GIMP_ITEM (mask)->width);
  y2 = CLAMP (off_y + GIMP_ITEM (add_on)->height, 0,
	      GIMP_ITEM (mask)->height);

  w = (x2 - x1);
  h = (y2 - y1);

  pixel_region_init (&srcPR, GIMP_DRAWABLE (add_on)->tiles,
		     (x1 - off_x), (y1 - off_y), w, h, FALSE);
  pixel_region_init (&destPR, GIMP_DRAWABLE (mask)->tiles,
                     x1, y1, w, h, TRUE);

  switch (op)
    {
    case GIMP_CHANNEL_OP_ADD:
    case GIMP_CHANNEL_OP_REPLACE:
      pixel_regions_process_parallel ((p_func)
				      gimp_channel_combine_sub_region_add,
				      NULL, 2, &srcPR, &destPR);
      break;

    case GIMP_CHANNEL_OP_SUBTRACT:
      pixel_regions_process_parallel ((p_func)
				      gimp_channel_combine_sub_region_sub,
				      NULL, 2, &srcPR, &destPR);
      break;

    case GIMP_CHANNEL_OP_INTERSECT:
      pixel_regions_process_parallel ((p_func)
				      gimp_channel_combine_sub_region_intersect,
				      NULL, 2, &srcPR, &destPR);
      break;

    default:
      g_warning ("%s: unknown operation type", G_STRFUNC);
      break;
    }

  mask->bounds_known = FALSE;

  gimp_drawable_update (GIMP_DRAWABLE (mask), x1, y2, w, h);
}
