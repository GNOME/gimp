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

#include "libgimpcolor/gimpcolor.h"

#include "base-types.h"

#include "desaturate.h"
#include "pixel-region.h"


static void  desaturate_region_lightness  (PixelRegion    *srcPR,
                                           PixelRegion    *destPR,
                                           const gboolean  has_alpha);
static void  desaturate_region_luminosity (PixelRegion    *srcPR,
                                           PixelRegion    *destPR,
                                           const gboolean  has_alpha);
static void  desaturate_region_average    (PixelRegion    *srcPR,
                                           PixelRegion    *destPR,
                                           const gboolean  has_alpha);


void
desaturate_region (GimpDesaturateMode *mode,
                   PixelRegion        *srcPR,
                   PixelRegion        *destPR)
{
  g_return_if_fail (mode != NULL);
  g_return_if_fail (srcPR->bytes == destPR->bytes);
  g_return_if_fail (srcPR->bytes == 3 || srcPR->bytes == 4);

  switch (*mode)
    {
    case GIMP_DESATURATE_LIGHTNESS:
      desaturate_region_lightness (srcPR, destPR, srcPR->bytes == 4);
      break;

    case GIMP_DESATURATE_LUMINOSITY:
      desaturate_region_luminosity (srcPR, destPR, srcPR->bytes == 4);
      break;

    case GIMP_DESATURATE_AVERAGE:
      desaturate_region_average (srcPR, destPR, srcPR->bytes == 4);
      break;
    }
}

static void
desaturate_region_lightness (PixelRegion    *srcPR,
                             PixelRegion    *destPR,
                             const gboolean  has_alpha)
{
  const guchar *src  = srcPR->data;
  guchar       *dest = destPR->data;
  gint          h    = srcPR->h;

  while (h--)
    {
      const guchar *s = src;
      guchar       *d = dest;
      gint          j;

      for (j = 0; j < srcPR->w; j++)
        {
          gint min, max;
          gint lightness;

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

          d += destPR->bytes;
          s += srcPR->bytes;
        }

      src += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}

static void
desaturate_region_luminosity (PixelRegion    *srcPR,
                              PixelRegion    *destPR,
                              const gboolean  has_alpha)
{
  const guchar *src  = srcPR->data;
  guchar       *dest = destPR->data;
  gint          h    = srcPR->h;

  while (h--)
    {
      const guchar *s = src;
      guchar       *d = dest;
      gint          j;

      for (j = 0; j < srcPR->w; j++)
        {
          gint luminosity = GIMP_RGB_LUMINANCE (s[RED_PIX],
                                                s[GREEN_PIX],
                                                s[BLUE_PIX]) + 0.5;

          d[RED_PIX]   = luminosity;
          d[GREEN_PIX] = luminosity;
          d[BLUE_PIX]  = luminosity;

          if (has_alpha)
            d[ALPHA_PIX] = s[ALPHA_PIX];

          d += destPR->bytes;
          s += srcPR->bytes;
        }

      src += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}

static void
desaturate_region_average (PixelRegion    *srcPR,
                           PixelRegion    *destPR,
                           const gboolean  has_alpha)
{
  const guchar *src  = srcPR->data;
  guchar       *dest = destPR->data;
  gint          h    = srcPR->h;

  while (h--)
    {
      const guchar *s = src;
      guchar       *d = dest;
      gint          j;

      for (j = 0; j < srcPR->w; j++)
        {
          gint average = (s[RED_PIX] + s[GREEN_PIX] + s[BLUE_PIX] + 1) / 3;

          d[RED_PIX]   = average;
          d[GREEN_PIX] = average;
          d[BLUE_PIX]  = average;

          if (has_alpha)
            d[ALPHA_PIX] = s[ALPHA_PIX];

          d += destPR->bytes;
          s += srcPR->bytes;
        }

      src += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}
