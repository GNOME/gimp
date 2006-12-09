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

#include "core-types.h"

#include "base/pixel-processor.h"
#include "base/pixel-region.h"

#include "gimpdrawable.h"
#include "gimpdrawable-desaturate.h"
#include "gimpimage.h"

#include "gimp-intl.h"


static void  desaturate_region_lightness  (gpointer     data,
                                           PixelRegion *srcPR,
                                           PixelRegion *destPR);

static void  desaturate_region_luminosity (gpointer     data,
                                           PixelRegion *srcPR,
                                           PixelRegion *destPR);

static void  desaturate_region_average    (gpointer     data,
                                           PixelRegion *srcPR,
                                           PixelRegion *destPR);


void
gimp_drawable_desaturate (GimpDrawable       *drawable,
                          GimpDesaturateMode  mode)
{
  PixelRegion         srcPR, destPR;
  PixelProcessorFunc  function;
  gint                x, y;
  gint                width, height;
  gboolean            has_alpha;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_drawable_is_rgb (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  if (! gimp_drawable_mask_intersect (drawable, &x, &y, &width, &height))
    return;

  switch (mode)
    {
    case GIMP_DESATURATE_LIGHTNESS:
      function = (PixelProcessorFunc) desaturate_region_lightness;
      break;

      break;
    case GIMP_DESATURATE_LUMINOSITY:
      function = (PixelProcessorFunc) desaturate_region_luminosity;
      break;

    case GIMP_DESATURATE_AVERAGE:
      function = (PixelProcessorFunc) desaturate_region_average;
      break;

    default:
      g_return_if_reached ();
      return;
    }

  has_alpha = gimp_drawable_has_alpha (drawable);

  pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                     x, y, width, height, FALSE);
  pixel_region_init (&destPR, gimp_drawable_get_shadow_tiles (drawable),
                     x, y, width, height, TRUE);

  pixel_regions_process_parallel (function, GINT_TO_POINTER (has_alpha),
                                  2, &srcPR, &destPR);

  gimp_drawable_merge_shadow (drawable, TRUE, _("Desaturate"));

  gimp_drawable_update (drawable, x, y, width, height);
}

static void
desaturate_region_lightness (gpointer     data,
                             PixelRegion *srcPR,
                             PixelRegion *destPR)
{
  const guchar *src       = srcPR->data;
  guchar       *dest      = destPR->data;
  gint          h         = srcPR->h;
  gboolean      has_alpha = GPOINTER_TO_INT (data);

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
desaturate_region_luminosity (gpointer     data,
                              PixelRegion *srcPR,
                              PixelRegion *destPR)
{
  const guchar *src       = srcPR->data;
  guchar       *dest      = destPR->data;
  gint          h         = srcPR->h;
  gboolean      has_alpha = GPOINTER_TO_INT (data);

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
desaturate_region_average (gpointer     data,
                           PixelRegion *srcPR,
                           PixelRegion *destPR)
{
  const guchar *src       = srcPR->data;
  guchar       *dest      = destPR->data;
  gint          h         = srcPR->h;
  gboolean      has_alpha = GPOINTER_TO_INT (data);

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
