/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplut.c: Copyright (C) 1999 Jay Cox <jaycox@gimp.org>
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

#include "base-types.h"

#include "gimplut.h"
#include "pixel-region.h"


GimpLut *
gimp_lut_new (void)
{
  GimpLut *lut;

  lut = g_new (GimpLut, 1);

  lut->luts      = NULL;
  lut->nchannels = 0;

  return lut;
}

void
gimp_lut_free (GimpLut *lut)
{
  gint i;

  for (i = 0; i < lut->nchannels; i++)
    g_free (lut->luts[i]);

  g_free (lut->luts);
  g_free (lut);
}

void
gimp_lut_setup (GimpLut     *lut,
                GimpLutFunc  func,
                void        *user_data,
                gint         nchannels)
{
  guint   i, v;
  gdouble val;

  if (lut->luts)
    {
      for (i = 0; i < lut->nchannels; i++)
        g_free (lut->luts[i]);

      g_free (lut->luts);
    }

  lut->nchannels = nchannels;
  lut->luts      = g_new (guchar *, lut->nchannels);

  for (i = 0; i < lut->nchannels; i++)
    {
      lut->luts[i] = g_new (guchar, 256);

      for (v = 0; v < 256; v++)
        {
          /* to add gamma correction use func(v ^ g) ^ 1/g instead. */
          val = 255.0 * func (user_data, lut->nchannels, i, v/255.0) + 0.5;

          lut->luts[i][v] = CLAMP (val, 0, 255);
        }
    }
}

/*  see comment in gimplut.h  */
void
gimp_lut_setup_exact (GimpLut     *lut,
                      GimpLutFunc  func,
                      void        *user_data,
                      gint         nchannels)
{
  gimp_lut_setup (lut, func, user_data, nchannels);
}

void
gimp_lut_process (GimpLut     *lut,
                  PixelRegion *srcPR,
                  PixelRegion *destPR)
{
  const guchar *src;
  guchar       *dest;
  guchar       *lut0 = NULL, *lut1 = NULL, *lut2 = NULL, *lut3 = NULL;
  guint         h, width, src_r_i, dest_r_i;

  if (lut->nchannels > 0)
    lut0 = lut->luts[0];
  if (lut->nchannels > 1)
    lut1 = lut->luts[1];
  if (lut->nchannels > 2)
    lut2 = lut->luts[2];
  if (lut->nchannels > 3)
    lut3 = lut->luts[3];

  h        = srcPR->h;
  src      = srcPR->data;
  dest     = destPR->data;
  width    = srcPR->w;
  src_r_i  = srcPR->rowstride  - (srcPR->bytes  * srcPR->w);
  dest_r_i = destPR->rowstride - (destPR->bytes * srcPR->w);

  if (src_r_i == 0 && dest_r_i == 0)
    {
      width *= h;
      h = 1;
    }

  while (h--)
    {
      switch (lut->nchannels)
        {
        case 1:
          while (width--)
            {
              *dest = lut0[*src];
              src++;
              dest++;
            }
          break;

        case 2:
          while (width--)
            {
              dest[0] = lut0[src[0]];
              dest[1] = lut1[src[1]];
              src  += 2;
              dest += 2;
            }
          break;

        case 3:
          while (width--)
            {
              dest[0] = lut0[src[0]];
              dest[1] = lut1[src[1]];
              dest[2] = lut2[src[2]];
              src  += 3;
              dest += 3;
            }
          break;

        case 4:
          while (width--)
            {
              dest[0] = lut0[src[0]];
              dest[1] = lut1[src[1]];
              dest[2] = lut2[src[2]];
              dest[3] = lut3[src[3]];
              src  += 4;
              dest += 4;
            }
          break;

        default:
          g_warning ("gimplut: Error: nchannels = %d\n", lut->nchannels);
          break;
        }

      width = srcPR->w;
      src  += src_r_i;
      dest += dest_r_i;
    }
}

void
gimp_lut_process_value (GimpLut     *lut,
                        PixelRegion *srcPR,
                        PixelRegion *destPR)
{
  const guchar *src;
  guchar       *dest;
  guchar       *lut0 = NULL, *lut1 = NULL, *lut2 = NULL, *lut3 = NULL;
  guint         h, width, src_r_i, dest_r_i;

  if (lut->nchannels > 0)
    lut0 = lut->luts[0];
  if (lut->nchannels > 1)
    lut1 = lut->luts[1];
  if (lut->nchannels > 2)
    lut2 = lut->luts[2];
  if (lut->nchannels > 3)
    lut3 = lut->luts[3];

  h        = srcPR->h;
  src      = srcPR->data;
  dest     = destPR->data;
  width    = srcPR->w;
  src_r_i  = srcPR->rowstride  - (srcPR->bytes  * srcPR->w);
  dest_r_i = destPR->rowstride - (destPR->bytes * srcPR->w);

  if (src_r_i == 0 && dest_r_i == 0)
    {
      width *= h;
      h = 1;
    }

  while (h--)
    {
      switch (lut->nchannels)
        {
        case 1:
          while (width--)
            {
              *dest = lut0[*src];
              src++;
              dest++;
            }
          break;

        case 2:
          while (width--)
            {
              dest[0] = lut0[src[0]];
              dest[1] = lut1[src[1]];
              src  += 2;
              dest += 2;
            }
          break;

        case 3:
          while (width--)
            {
              gint r, g, b;
              gint value, value2, min;
              gint delta;

              r = src[0];
              g = src[1];
              b = src[2];

              if (r > g)
                {
                  value = MAX (r, b);
                  min = MIN (g, b);
                }
              else
                {
                  value = MAX (g, b);
                  min = MIN (r, b);
                }

              delta = value - min;
              if ((value == 0) || (delta == 0))
                {
                  r = lut0[value];
                  g = lut0[value];
                  b = lut0[value];
                }
              else
                {
                  value2 = value / 2;

                  if (r == value)
                    {
                      r = lut0[r];
                      b = ((r * b) + value2) / value;
                      g = ((r * g) + value2) / value;
                    }
                  else if (g == value)
                    {
                      g = lut0[g];
                      r = ((g * r) + value2) / value;
                      b = ((g * b) + value2) / value;
                    }
                  else
                    {
                      b = lut0[b];
                      g = ((b * g) + value2) / value;
                      r = ((b * r) + value2) / value;
                    }
                }

              dest[0] = r;
              dest[1] = g;
              dest[2] = b;

              src  += 3;
              dest += 3;
            }
          break;

        case 4:
          while (width--)
            {
              gint r, g, b;
              gint value, value2, min;
              gint delta;

              r = src[0];
              g = src[1];
              b = src[2];

              if (r > g)
                {
                  value = MAX (r, b);
                  min = MIN (g, b);
                }
              else
                {
                  value = MAX (g, b);
                  min = MIN (r, b);
                }

              delta = value - min;
              if ((value == 0) || (delta == 0))
                {
                  r = lut0[value];
                  g = lut0[value];
                  b = lut0[value];
                }
              else
                {
                  value2 = value / 2;

                  if (r == value)
                    {
                      r = lut0[r];
                      b = ((r * b) + value2) / value;
                      g = ((r * g) + value2) / value;
                    }
                  else if (g == value)
                    {
                      g = lut0[g];
                      r = ((g * r) + value2) / value;
                      b = ((g * b) + value2) / value;
                    }
                  else
                    {
                      b = lut0[b];
                      g = ((b * g) + value2) / value;
                      r = ((b * r) + value2) / value;
                    }
                }

              dest[0] = r;
              dest[1] = g;
              dest[2] = b;

              dest[3] = lut3[src[3]];

              src  += 4;
              dest += 4;
            }
          break;

        default:
          g_warning ("gimplut: Error: nchannels = %d\n", lut->nchannels);
          break;
        }

      width = srcPR->w;
      src  += src_r_i;
      dest += dest_r_i;
    }
}

void
gimp_lut_process_inline (GimpLut     *lut,
                         PixelRegion *srcPR)
{
  guint   h, width, src_r_i;
  guchar *src;
  guchar *lut0 = NULL, *lut1 = NULL, *lut2 = NULL, *lut3 = NULL;

  if (lut->nchannels > 0)
    lut0 = lut->luts[0];
  if (lut->nchannels > 1)
    lut1 = lut->luts[1];
  if (lut->nchannels > 2)
    lut2 = lut->luts[2];
  if (lut->nchannels > 3)
    lut3 = lut->luts[3];

  h       = srcPR->h;
  src     = srcPR->data;
  width   = srcPR->w;
  src_r_i =  srcPR->rowstride  - (srcPR->bytes  * srcPR->w);

  if (src_r_i == 0)
    {
      width *= h;
      h = 1;
    }

  while (h--)
    {
      switch (lut->nchannels)
        {
        case 1:
          while (width--)
            {
              *src = lut0[*src];
              src++;
            }
          break;

        case 2:
          while (width--)
            {
              src[0] = lut0[src[0]];
              src[1] = lut1[src[1]];
              src  += 2;
            }
          break;

        case 3:
          while (width--)
            {
              src[0] = lut0[src[0]];
              src[1] = lut1[src[1]];
              src[2] = lut2[src[2]];
              src  += 3;
            }
          break;

        case 4:
          while (width--)
            {
              src[0] = lut0[src[0]];
              src[1] = lut1[src[1]];
              src[2] = lut2[src[2]];
              src[3] = lut3[src[3]];
              src  += 4;
            }
          break;

        default:
          g_warning ("gimplut: Error: nchannels = %d\n", lut->nchannels);
          break;
        }

      width = srcPR->w;
      src  += src_r_i;
    }
}
