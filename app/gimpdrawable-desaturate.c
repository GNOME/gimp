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

#include <gtk/gtk.h>

#include "apptypes.h"

#include "appenv.h"
#include "drawable.h"
#include "desaturate.h"
#include "gimage.h"
#include "paint_funcs.h"
#include "pixel_region.h"

#include "libgimp/gimpintl.h"


void
image_desaturate (GimpImage *gimage)
{
  GimpDrawable *drawable;

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
  PixelRegion  srcPR, destPR;
  guchar      *src, *s;
  guchar      *dest, *d;
  gint         h, j;
  gint         lightness, min, max;
  gint         has_alpha;
  gpointer     pr;
  gint         x1, y1, x2, y2;

  if (!drawable) 
    return;

  has_alpha = gimp_drawable_has_alpha (drawable);
  gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  pixel_region_init (&srcPR, gimp_drawable_data (drawable),
		     x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixel_region_init (&destPR, gimp_drawable_shadow (drawable),
		     x1, y1, (x2 - x1), (y2 - y1), TRUE);

  for (pr = pixel_regions_register (2, &srcPR, &destPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
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
	      max = MAX (s[RED_PIX], s[GREEN_PIX]);
	      max = MAX (max, s[BLUE_PIX]);
	      min = MIN (s[RED_PIX], s[GREEN_PIX]);
	      min = MIN (min, s[BLUE_PIX]);

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
