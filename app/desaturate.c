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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "appenv.h"
#include "drawable.h"
#include "desaturate.h"
#include "interface.h"
#include "paint_funcs.h"
#include "gimage.h"

#include "libgimp/gimpintl.h"


void
image_desaturate (gimage_ptr)
     void *gimage_ptr;
{
  GImage *gimage;
  GimpDrawable *drawable;

  gimage = (GImage *) gimage_ptr;
  drawable = gimage_active_drawable (gimage);

  if (! drawable_color (drawable))
    {
      g_message (_("Desaturate operates only on RGB color drawables."));
      return;
    }
  desaturate (drawable);
}


/*  Desaturater  */

void
desaturate (GimpDrawable *drawable)
{
  PixelRegion srcPR, destPR;
  unsigned char *src, *s;
  unsigned char *dest, *d;
  int h, j;
  int lightness, min, max;
  int has_alpha;
  void *pr;
  int x1, y1, x2, y2;

  if (!drawable) 
    return;

  has_alpha = drawable_has_alpha (drawable);
  drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);
  pixel_region_init (&srcPR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixel_region_init (&destPR, drawable_shadow (drawable), x1, y1, (x2 - x1), (y2 - y1), TRUE);

  for (pr = pixel_regions_register (2, &srcPR, &destPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      src = srcPR.data;
      dest = destPR.data;
      h = srcPR.h;

      while (h--)
	{
	  s = src;
	  d = dest;

	  for (j = 0; j < srcPR.w; j++)
	    {
	      max = MAXIMUM (s[RED_PIX], s[GREEN_PIX]);
	      max = MAXIMUM (max, s[BLUE_PIX]);
	      min = MINIMUM (s[RED_PIX], s[GREEN_PIX]);
	      min = MINIMUM (min, s[BLUE_PIX]);

	      lightness = (max + min) / 2;

	      d[RED_PIX] = lightness;
	      d[GREEN_PIX] = lightness;
	      d[BLUE_PIX] = lightness;

	      if (has_alpha)
		d[ALPHA_PIX] = s[ALPHA_PIX];

	      d += destPR.bytes;
	      s += srcPR.bytes;
	    }

	  src += srcPR.rowstride;
	  dest += destPR.rowstride;
	}
    }

  drawable_merge_shadow (drawable, TRUE);
  drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
}
