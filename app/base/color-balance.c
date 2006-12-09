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

#include "color-balance.h"
#include "pixel-region.h"


/*  local function prototypes  */

static void   color_balance_transfer_init (void);


/*  private variables  */

static gboolean transfer_initialized = FALSE;

/*  for lightening  */
static gdouble  highlights_add[256] = { 0 };
static gdouble  midtones_add[256]   = { 0 };
static gdouble  shadows_add[256]    = { 0 };

/*  for darkening  */
static gdouble  highlights_sub[256] = { 0 };
static gdouble  midtones_sub[256]   = { 0 };
static gdouble  shadows_sub[256]    = { 0 };


/*  public functions  */

void
color_balance_init (ColorBalance *cb)
{
  GimpTransferMode range;

  g_return_if_fail (cb != NULL);

  for (range = GIMP_SHADOWS; range <= GIMP_HIGHLIGHTS; range++)
    color_balance_range_reset (cb, range);

  cb->preserve_luminosity = TRUE;
}

void
color_balance_range_reset (ColorBalance     *cb,
                           GimpTransferMode  range)
{
  g_return_if_fail (cb != NULL);

  cb->cyan_red[range]      = 0.0;
  cb->magenta_green[range] = 0.0;
  cb->yellow_blue[range]   = 0.0;
}

void
color_balance_create_lookup_tables (ColorBalance *cb)
{
  gdouble *cyan_red_transfer[3];
  gdouble *magenta_green_transfer[3];
  gdouble *yellow_blue_transfer[3];
  gint     i;
  gint32   r_n, g_n, b_n;

  g_return_if_fail (cb != NULL);

  if (! transfer_initialized)
    {
      color_balance_transfer_init ();
      transfer_initialized = TRUE;
    }

  /*  Set the transfer arrays  (for speed)  */
  cyan_red_transfer[GIMP_SHADOWS] =
    (cb->cyan_red[GIMP_SHADOWS] > 0) ? shadows_add : shadows_sub;
  cyan_red_transfer[GIMP_MIDTONES] =
    (cb->cyan_red[GIMP_MIDTONES] > 0) ? midtones_add : midtones_sub;
  cyan_red_transfer[GIMP_HIGHLIGHTS] =
    (cb->cyan_red[GIMP_HIGHLIGHTS] > 0) ? highlights_add : highlights_sub;

  magenta_green_transfer[GIMP_SHADOWS] =
    (cb->magenta_green[GIMP_SHADOWS] > 0) ? shadows_add : shadows_sub;
  magenta_green_transfer[GIMP_MIDTONES] =
    (cb->magenta_green[GIMP_MIDTONES] > 0) ? midtones_add : midtones_sub;
  magenta_green_transfer[GIMP_HIGHLIGHTS] =
    (cb->magenta_green[GIMP_HIGHLIGHTS] > 0) ? highlights_add : highlights_sub;
  yellow_blue_transfer[GIMP_SHADOWS] =
    (cb->yellow_blue[GIMP_SHADOWS] > 0) ? shadows_add : shadows_sub;
  yellow_blue_transfer[GIMP_MIDTONES] =
    (cb->yellow_blue[GIMP_MIDTONES] > 0) ? midtones_add : midtones_sub;
  yellow_blue_transfer[GIMP_HIGHLIGHTS] =
    (cb->yellow_blue[GIMP_HIGHLIGHTS] > 0) ? highlights_add : highlights_sub;

  for (i = 0; i < 256; i++)
    {
      r_n = i;
      g_n = i;
      b_n = i;

      r_n += cb->cyan_red[GIMP_SHADOWS] * cyan_red_transfer[GIMP_SHADOWS][r_n];
      r_n = CLAMP0255 (r_n);
      r_n += cb->cyan_red[GIMP_MIDTONES] * cyan_red_transfer[GIMP_MIDTONES][r_n];
      r_n = CLAMP0255 (r_n);
      r_n += cb->cyan_red[GIMP_HIGHLIGHTS] * cyan_red_transfer[GIMP_HIGHLIGHTS][r_n];
      r_n = CLAMP0255 (r_n);

      g_n += cb->magenta_green[GIMP_SHADOWS] * magenta_green_transfer[GIMP_SHADOWS][g_n];
      g_n = CLAMP0255 (g_n);
      g_n += cb->magenta_green[GIMP_MIDTONES] * magenta_green_transfer[GIMP_MIDTONES][g_n];
      g_n = CLAMP0255 (g_n);
      g_n += cb->magenta_green[GIMP_HIGHLIGHTS] * magenta_green_transfer[GIMP_HIGHLIGHTS][g_n];
      g_n = CLAMP0255 (g_n);

      b_n += cb->yellow_blue[GIMP_SHADOWS] * yellow_blue_transfer[GIMP_SHADOWS][b_n];
      b_n = CLAMP0255 (b_n);
      b_n += cb->yellow_blue[GIMP_MIDTONES] * yellow_blue_transfer[GIMP_MIDTONES][b_n];
      b_n = CLAMP0255 (b_n);
      b_n += cb->yellow_blue[GIMP_HIGHLIGHTS] * yellow_blue_transfer[GIMP_HIGHLIGHTS][b_n];
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

          r_n = cb->r_lookup[r];
          g_n = cb->g_lookup[g];
          b_n = cb->b_lookup[b];

          if (cb->preserve_luminosity)
            {
              gimp_rgb_to_hsl_int (&r_n, &g_n, &b_n);
              b_n = gimp_rgb_to_l_int (r, g, b);
              gimp_hsl_to_rgb_int (&r_n, &g_n, &b_n);
            }

          d[RED_PIX]   = r_n;
          d[GREEN_PIX] = g_n;
           d[BLUE_PIX]  = b_n;

          if (alpha)
            d[ALPHA_PIX] = s[ALPHA_PIX];

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
      gdouble low = (1.075 - 1 / ((gdouble) i / 16.0 + 1));
      gdouble mid = 0.667 * (1 - SQR (((gdouble) i - 127.0) / 127.0));

      shadows_add[i]          = low;
      shadows_sub[255 - i]    = low;

      midtones_add[i]         = mid;
      midtones_sub[i]         = mid;

      highlights_add[255 - i] = low;
      highlights_sub[i]       = low;
    }
}
