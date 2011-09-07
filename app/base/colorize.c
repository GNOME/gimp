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

#include <cairo.h>
#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "base-types.h"

#include "colorize.h"
#include "pixel-region.h"


void
colorize_init (Colorize *colorize)
{
  gint i;

  g_return_if_fail (colorize != NULL);

  colorize->hue        = 180.0;
  colorize->saturation =  50.0;
  colorize->lightness  =   0.0;

  for (i = 0; i < 256; i ++)
    {
      colorize->lum_red_lookup[i]   = i * GIMP_RGB_LUMINANCE_RED;
      colorize->lum_green_lookup[i] = i * GIMP_RGB_LUMINANCE_GREEN;
      colorize->lum_blue_lookup[i]  = i * GIMP_RGB_LUMINANCE_BLUE;
    }
}

void
colorize_calculate (Colorize *colorize)
{
  GimpHSL hsl;
  GimpRGB rgb;
  gint    i;

  g_return_if_fail (colorize != NULL);

  hsl.h = colorize->hue        / 360.0;
  hsl.s = colorize->saturation / 100.0;

  /*  Calculate transfers  */
  for (i = 0; i < 256; i ++)
    {
      hsl.l = (gdouble) i / 255.0;

      gimp_hsl_to_rgb (&hsl, &rgb);

      /*  this used to read i * rgb.r,g,b in GIMP 2.4, but this produced
       *  darkened results, multiplying with 255 is correct and preserves
       *  the lightness unless modified with the slider.
       */
      colorize->final_red_lookup[i]   = 255.0 * rgb.r;
      colorize->final_green_lookup[i] = 255.0 * rgb.g;
      colorize->final_blue_lookup[i]  = 255.0 * rgb.b;
    }
}

void
colorize (Colorize    *colorize,
          PixelRegion *srcPR,
          PixelRegion *destPR)
{
  const guchar *src, *s;
  guchar       *dest, *d;
  gboolean      alpha;
  gint          w, h;
  gint          lum;

  /*  Set the transfer arrays  (for speed)  */
  h     = srcPR->h;
  src   = srcPR->data;
  dest  = destPR->data;
  alpha = pixel_region_has_alpha (srcPR);

  while (h--)
    {
      w = srcPR->w;
      s = src;
      d = dest;

      while (w--)
        {
          lum = (colorize->lum_red_lookup[s[RED]] +
                 colorize->lum_green_lookup[s[GREEN]] +
                 colorize->lum_blue_lookup[s[BLUE]]); /* luminosity */

          if (colorize->lightness > 0)
            {
              lum = (gdouble) lum * (100.0 - colorize->lightness) / 100.0;

              lum += 255 - (100.0 - colorize->lightness) * 255.0 / 100.0;
            }
          else if (colorize->lightness < 0)
            {
              lum = (gdouble) lum * (colorize->lightness + 100.0) / 100.0;
            }

          d[RED]   = colorize->final_red_lookup[lum];
          d[GREEN] = colorize->final_green_lookup[lum];
          d[BLUE]  = colorize->final_blue_lookup[lum];

          if (alpha)
            d[ALPHA] = s[ALPHA];

          s += srcPR->bytes;
          d += destPR->bytes;
        }

      src  += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}
