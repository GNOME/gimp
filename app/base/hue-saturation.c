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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "base-types.h"

#include "hue-saturation.h"
#include "pixel-region.h"


void
hue_saturation_init (HueSaturation *hs)
{
  GimpHueRange partition;

  g_return_if_fail (hs != NULL);

  for (partition = GIMP_ALL_HUES; partition <= GIMP_MAGENTA_HUES; partition++)
    hue_saturation_partition_reset (hs, partition);
}

void
hue_saturation_partition_reset (HueSaturation *hs,
                                GimpHueRange   partition)
{
  g_return_if_fail (hs != NULL);

  hs->hue[partition]        = 0.0;
  hs->lightness[partition]  = 0.0;
  hs->saturation[partition] = 0.0;
}

void
hue_saturation_calculate_transfers (HueSaturation *hs)
{
  gint value;
  gint hue;
  gint i;

  g_return_if_fail (hs != NULL);

  /*  Calculate transfers  */
  for (hue = 0; hue < 6; hue++)
    for (i = 0; i < 256; i++)
      {
	value = (hs->hue[0] + hs->hue[hue + 1]) * 255.0 / 360.0;
	if ((i + value) < 0)
	  hs->hue_transfer[hue][i] = 255 + (i + value);
	else if ((i + value) > 255)
	  hs->hue_transfer[hue][i] = i + value - 255;
	else
	  hs->hue_transfer[hue][i] = i + value;

	/*  Lightness  */
	value = (hs->lightness[0] + hs->lightness[hue + 1]) * 127.0 / 100.0;
	value = CLAMP (value, -255, 255);

	if (value < 0)
	  hs->lightness_transfer[hue][i] = (guchar) ((i * (255 + value)) / 255);
	else
	  hs->lightness_transfer[hue][i] = (guchar) (i + ((255 - i) * value) / 255);

	/*  Saturation  */
	value = (hs->saturation[0] + hs->saturation[hue + 1]) * 255.0 / 100.0;
	value = CLAMP (value, -255, 255);

	/* This change affects the way saturation is computed. With the
	   old code (different code for value < 0), increasing the
	   saturation affected muted colors very much, and bright colors
	   less. With the new code, it affects muted colors and bright
	   colors more or less evenly. For enhancing the color in photos,
	   the new behavior is exactly what you want. It's hard for me
	   to imagine a case in which the old behavior is better.
	 */
	hs->saturation_transfer[hue][i] = CLAMP ((i * (255 + value)) / 255, 0, 255);
      }
}

void
hue_saturation (PixelRegion   *srcPR,
		PixelRegion   *destPR,
		HueSaturation *hs)
{
  guchar *src, *s;
  guchar *dest, *d;
  gint    alpha;
  gint    w, h;
  gint    r, g, b;
  gint    hue;

  /*  Set the transfer arrays  (for speed)  */
  h     = srcPR->h;
  src   = srcPR->data;
  dest  = destPR->data;
  alpha = (srcPR->bytes == 4) ? TRUE : FALSE;

  while (h--)
    {
      w = srcPR->w;
      s = src;
      d = dest;

      while (w--)
	{
	  r = s[RED_PIX];
	  g = s[GREEN_PIX];
	  b = s[BLUE_PIX];

	  gimp_rgb_to_hsl_int (&r, &g, &b);

          hue = (r + (128 / 6)) / 6;

	  if (r < 21)
	    hue = 0;
	  else if (r < 64)
	    hue = 1;
	  else if (r < 106)
	    hue = 2;
	  else if (r < 149)
	    hue = 3;
	  else if (r < 192)
	    hue = 4;
	  else if (r < 234)
	    hue = 5;
          else
            hue = 0;

	  r = hs->hue_transfer[hue][r];
	  g = hs->saturation_transfer[hue][g];
	  b = hs->lightness_transfer[hue][b];

	  gimp_hsl_to_rgb_int (&r, &g, &b);

	  d[RED_PIX]   = r;
	  d[GREEN_PIX] = g;
	  d[BLUE_PIX]  = b;

	  if (alpha)
	    d[ALPHA_PIX] = s[ALPHA_PIX];

	  s += srcPR->bytes;
	  d += destPR->bytes;
	}

      src  += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}
