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

#include "base/pixel-region.h"

#include "gimpdrawable.h"
#include "gimpdrawable-desaturate.h"
#include "gimpimage.h"

#include "gimp-intl.h"


void
gimp_drawable_desaturate (GimpDrawable *drawable)
{
  PixelRegion  srcPR, destPR;
  guchar      *src, *s;
  guchar      *dest, *d;
  gint         h, j;
  gint         lightness, min, max;
  gboolean     has_alpha;
  gpointer     pr;
  gint         x, y, width, height;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_drawable_is_rgb (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  has_alpha = gimp_drawable_has_alpha (drawable);

  if (! gimp_drawable_mask_intersect (drawable, &x, &y, &width, &height))
    return;

  pixel_region_init (&srcPR, gimp_drawable_data (drawable),
		     x, y, width, height, FALSE);
  pixel_region_init (&destPR, gimp_drawable_shadow (drawable),
		     x, y, width, height, TRUE);

  for (pr = pixel_regions_register (2, &srcPR, &destPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      src  = srcPR.data;
      dest = destPR.data;
      h    = srcPR.h;

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

	      d[RED_PIX]   = lightness;
	      d[GREEN_PIX] = lightness;
	      d[BLUE_PIX]  = lightness;

	      if (has_alpha)
		d[ALPHA_PIX] = s[ALPHA_PIX];

	      d += destPR.bytes;
	      s += srcPR.bytes;
	    }

	  src += srcPR.rowstride;
	  dest += destPR.rowstride;
	}
    }

  gimp_drawable_merge_shadow (drawable, TRUE, _("Desaturate"));

  gimp_drawable_update (drawable, x, y, width, height);
}
