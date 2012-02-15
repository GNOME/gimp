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

#include "color-balance.h"
#include "pixel-region.h"


/*  local function prototypes  */

static void   color_balance_transfer_init (void);


/*  private variables  */

static gboolean transfer_initialized = FALSE;

static gdouble  highlights[256] = { 0 };
static gdouble  midtones[256]   = { 0 };
static gdouble  shadows[256]    = { 0 };


/*  public functions  */

void
color_balance_init (ColorBalance *cb)
{
  GimpTransferMode range;

  g_return_if_fail (cb != NULL);

  for (range = GIMP_SHADOWS; range <= GIMP_HIGHLIGHTS; range++)
    {
      cb->cyan_red[range]      = 0.0;
      cb->magenta_green[range] = 0.0;
      cb->yellow_blue[range]   = 0.0;
    }

  cb->preserve_luminosity = TRUE;
}

void
color_balance_create_lookup_tables (ColorBalance *cb)
{
  gint     i;
  gint32   r_n, g_n, b_n;

  g_return_if_fail (cb != NULL);

  if (! transfer_initialized)
    {
      color_balance_transfer_init ();
      transfer_initialized = TRUE;
    }

  for (i = 0; i < 256; i++)
    {
      r_n = i;
      g_n = i;
      b_n = i;

      r_n += cb->cyan_red[GIMP_SHADOWS] * shadows[i];
      r_n += cb->cyan_red[GIMP_MIDTONES] * midtones[i];
      r_n += cb->cyan_red[GIMP_HIGHLIGHTS] * highlights[i];
      r_n = CLAMP0255 (r_n);

      g_n += cb->magenta_green[GIMP_SHADOWS] * shadows[i];
      g_n += cb->magenta_green[GIMP_MIDTONES] * midtones[i];
      g_n += cb->magenta_green[GIMP_HIGHLIGHTS] * highlights[i];
      g_n = CLAMP0255 (g_n);

      b_n += cb->yellow_blue[GIMP_SHADOWS] * shadows[i];
      b_n += cb->yellow_blue[GIMP_MIDTONES] * midtones[i];
      b_n += cb->yellow_blue[GIMP_HIGHLIGHTS] * highlights[i];
      b_n = CLAMP0255 (b_n);

      cb->r_lookup[i] = r_n;
      cb->g_lookup[i] = g_n;
      cb->b_lookup[i] = b_n;
    }
}

void
color_balance (ColorBalance *cb,
               PixelRegion  *srcPR,
               PixelRegion  *destPR)
{
  const guchar *src, *s;
  guchar       *dest, *d;
  gboolean      alpha;
  gint          r, g, b;
  gint          r_n, g_n, b_n;
  gint          w, h;

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
          r = s[RED];
          g = s[GREEN];
          b = s[BLUE];

          r_n = cb->r_lookup[r];
          g_n = cb->g_lookup[g];
          b_n = cb->b_lookup[b];

          if (cb->preserve_luminosity)
            {
              gimp_rgb_to_hsl_int (&r_n, &g_n, &b_n);
              b_n = gimp_rgb_to_l_int (r, g, b);
              gimp_hsl_to_rgb_int (&r_n, &g_n, &b_n);
            }

          d[RED]   = r_n;
          d[GREEN] = g_n;
          d[BLUE]  = b_n;

          if (alpha)
            d[ALPHA] = s[ALPHA];

          s += srcPR->bytes;
          d += destPR->bytes;
        }

      src  += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}


/*  private functions  */

static void
color_balance_transfer_init (void)
{
  gint i;

  for (i = 0; i < 256; i++)
    {
      static const gdouble a = 64, b = 85, scale = 1.785;
      gdouble low = CLAMP ((i - b) / -a + .5, 0, 1) * scale;
      gdouble mid = CLAMP ((i - b) /  a + .5, 0, 1) *
                    CLAMP ((i + b - 255) / -a + .5, 0, 1) * scale;

      shadows[i]          = low;
      midtones[i]         = mid;
      highlights[255 - i] = low;
    }
}
