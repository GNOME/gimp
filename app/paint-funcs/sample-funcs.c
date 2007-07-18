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

#include <string.h>

#include <glib-object.h>

#include "paint-funcs-types.h"

#include "base/pixel-region.h"

#include "sample-funcs.h"


#define EPSILON  (0.0001)  /* arbitary small number for avoiding zero */


void
subsample_region (PixelRegion *srcPR,
                  PixelRegion *destPR,
                  gint         subsample)
{
  const gint     width       = destPR->w;
  const gint     height      = destPR->h;
  const gint     orig_width  = srcPR->w / subsample;
  const gint     orig_height = srcPR->h / subsample;
  const gdouble  x_ratio     = (gdouble) orig_width / (gdouble) width;
  const gdouble  y_ratio     = (gdouble) orig_height / (gdouble) height;
  const gint     bytes       = destPR->bytes;
  const gint     destwidth   = destPR->rowstride;
  const gboolean has_alpha   = pixel_region_has_alpha (srcPR);
  guchar        *src,  *s;
  guchar        *dest, *d;
  gdouble       *row,  *r;
  gint           src_row, src_col;
  gdouble        x_sum, y_sum;
  gdouble        x_last, y_last;
  gdouble       *x_frac, y_frac;
  gdouble        tot_frac;
  gint           i, j;
  gint           b;
  gint           frac;
  gboolean       advance_dest;

#if 0
  g_printerr ("subsample_region: (%d x %d) -> (%d x %d)\n",
              orig_width, orig_height, width, height);
#endif

  /*  the data pointers...  */
  src  = g_new (guchar, orig_width * bytes);
  dest = destPR->data;

  /*  allocate an array to help with the calculations  */
  row    = g_new (gdouble, width * bytes);
  x_frac = g_new (gdouble, width + orig_width);

  /*  initialize the pre-calculated pixel fraction array  */
  src_col = 0;
  x_sum = (gdouble) src_col;
  x_last = x_sum;

  for (i = 0; i < width + orig_width; i++)
    {
      if (x_sum + x_ratio <= (src_col + 1 + EPSILON))
        {
          x_sum += x_ratio;
          x_frac[i] = x_sum - x_last;
        }
      else
        {
          src_col ++;
          x_frac[i] = src_col - x_last;
        }

      x_last += x_frac[i];
    }

  /*  clear the "row" array  */
  memset (row, 0, sizeof (gdouble) * width * bytes);

  /*  counters...  */
  src_row = 0;
  y_sum = (gdouble) src_row;
  y_last = y_sum;

  pixel_region_get_row (srcPR,
                        srcPR->x, srcPR->y + src_row * subsample,
                        orig_width * subsample,
                        src, subsample);

  /*  Scale the selected region  */
  for (i = 0; i < height; )
    {
      src_col = 0;
      x_sum = (gdouble) src_col;

      /* determine the fraction of the src pixel we are using for y */
      if (y_sum + y_ratio <= (src_row + 1 + EPSILON))
        {
          y_sum += y_ratio;
          y_frac = y_sum - y_last;
          advance_dest = TRUE;
        }
      else
        {
          src_row ++;
          y_frac = src_row - y_last;
          advance_dest = FALSE;
        }

      y_last += y_frac;

      s = src;
      r = row;

      frac = 0;
      j = width;

      while (j)
        {
          tot_frac = x_frac[frac++] * y_frac;

          if (has_alpha)
            {
              /* premultiply */

              gdouble local_frac = tot_frac * (gdouble) s[bytes - 1] / 255.0;

              for (b = 0; b < (bytes - 1); b++)
                r[b] += s[b] * local_frac;

              r[bytes - 1] += local_frac;
            }
          else
            {
              for (b = 0; b < bytes; b++)
                r[b] += s[b] * tot_frac;
            }

          /*  increment the destination  */
          if (x_sum + x_ratio <= (src_col + 1 + EPSILON))
            {
              r += bytes;
              x_sum += x_ratio;
              j--;
            }

          /* increment the source */
          else
            {
              s += bytes;
              src_col++;
            }
        }

      if (advance_dest)
        {
          tot_frac = 1.0 / (x_ratio * y_ratio);

          /*  copy "row" to "dest"  */
          d = dest;
          r = row;

          j = width;
          while (j--)
            {
              if (has_alpha)
                {
                  /* unpremultiply */

                  gdouble alpha = r[bytes - 1];

                  if (alpha > EPSILON)
                    {
                      for (b = 0; b < (bytes - 1); b++)
                        d[b] = (guchar) ((r[b] / alpha) + 0.5);

                      d[bytes - 1] = (guchar) ((alpha * tot_frac * 255.0) + 0.5);
                    }
                  else
                    {
                      for (b = 0; b < bytes; b++)
                        d[b] = 0;
                    }
                }
              else
                {
                  for (b = 0; b < bytes; b++)
                    d[b] = (guchar) ((r[b] * tot_frac) + 0.5);
                }

              r += bytes;
              d += bytes;
            }

          dest += destwidth;

          /*  clear the "row" array  */
          memset (row, 0, sizeof (gdouble) * destwidth);

          i++;
        }
      else
        {
          pixel_region_get_row (srcPR,
                                srcPR->x,
                                srcPR->y + src_row * subsample,
                                orig_width * subsample,
                                src, subsample);
        }
    }

  /*  free up temporary arrays  */
  g_free (row);
  g_free (x_frac);
  g_free (src);
}


void
subsample_indexed_region (PixelRegion  *srcPR,
                          PixelRegion  *destPR,
                          const guchar *cmap,
                          gint          subsample)
{
  const gint     width       = destPR->w;
  const gint     height      = destPR->h;
  const gint     orig_width  = srcPR->w / subsample;
  const gint     orig_height = srcPR->h / subsample;
  const gdouble  x_ratio     = (gdouble) orig_width / (gdouble) width;
  const gdouble  y_ratio     = (gdouble) orig_height / (gdouble) height;
  const gint     bytes       = destPR->bytes;
  const gint     destwidth   = destPR->rowstride;
  const gboolean has_alpha   = pixel_region_has_alpha (srcPR);
  guchar        *src,  *s;
  guchar        *dest, *d;
  gdouble       *row,  *r;
  gint           src_row, src_col;
  gdouble        x_sum, y_sum;
  gdouble        x_last, y_last;
  gdouble       *x_frac, y_frac;
  gdouble        tot_frac;
  gint           i, j;
  gint           b;
  gint           frac;
  gboolean       advance_dest;

  g_return_if_fail (cmap != NULL);

  /*  the data pointers...  */
  src  = g_new (guchar, orig_width * bytes);
  dest = destPR->data;

  /*  allocate an array to help with the calculations  */
  row    = g_new0 (gdouble, width * bytes);
  x_frac = g_new (gdouble, width + orig_width);

  /*  initialize the pre-calculated pixel fraction array  */
  src_col = 0;
  x_sum   = (gdouble) src_col;
  x_last  = x_sum;

  for (i = 0; i < width + orig_width; i++)
    {
      if (x_sum + x_ratio <= (src_col + 1 + EPSILON))
        {
          x_sum += x_ratio;
          x_frac[i] = x_sum - x_last;
        }
      else
        {
          src_col++;
          x_frac[i] = src_col - x_last;
        }

      x_last += x_frac[i];
    }

  /*  counters...  */
  src_row = 0;
  y_sum   = (gdouble) src_row;
  y_last  = y_sum;

  pixel_region_get_row (srcPR,
                        srcPR->x,
                        srcPR->y + src_row * subsample,
                        orig_width * subsample,
                        src,
                        subsample);

  /*  Scale the selected region  */
  for (i = 0; i < height; )
    {
      src_col = 0;
      x_sum   = (gdouble) src_col;

      /* determine the fraction of the src pixel we are using for y */
      if (y_sum + y_ratio <= (src_row + 1 + EPSILON))
        {
          y_sum += y_ratio;
          y_frac = y_sum - y_last;
          advance_dest = TRUE;
        }
      else
        {
          src_row++;
          y_frac = src_row - y_last;
          advance_dest = FALSE;
        }

      y_last += y_frac;

      s = src;
      r = row;

      frac = 0;

      j  = width;
      while (j)
        {
          gint index = *s * 3;

          tot_frac = x_frac[frac++] * y_frac;

          /*  transform the color to RGB  */
          if (has_alpha)
            {
              if (s[ALPHA_I_PIX] & 0x80)
                {
                  r[RED_PIX]   += cmap[index++] * tot_frac;
                  r[GREEN_PIX] += cmap[index++] * tot_frac;
                  r[BLUE_PIX]  += cmap[index++] * tot_frac;
                  r[ALPHA_PIX] += tot_frac;
                }
              /* else the pixel contributes nothing and needs not to be added
               */
            }
          else
            {
              r[RED_PIX]   += cmap[index++] * tot_frac;
              r[GREEN_PIX] += cmap[index++] * tot_frac;
              r[BLUE_PIX]  += cmap[index++] * tot_frac;
            }

          /*  increment the destination  */
          if (x_sum + x_ratio <= (src_col + 1 + EPSILON))
            {
              r += bytes;
              x_sum += x_ratio;
              j--;
            }
          /* increment the source */
          else
            {
              s += srcPR->bytes;
              src_col++;
            }
        }

      if (advance_dest)
        {
          tot_frac = 1.0 / (x_ratio * y_ratio);

          /*  copy "row" to "dest"  */
          d = dest;
          r = row;

          j = width;
          while (j--)
            {
              if (has_alpha)
                {
                  /* unpremultiply */

                  gdouble alpha = r[bytes - 1];

                  if (alpha > EPSILON)
                    {
                      for (b = 0; b < (bytes - 1); b++)
                        d[b] = (guchar) ((r[b] / alpha) + 0.5);

                      d[bytes - 1] = (guchar) ((alpha * tot_frac * 255.0) + 0.5);
                    }
                  else
                    {
                      for (b = 0; b < bytes; b++)
                        d[b] = 0;
                    }
                }
              else
                {
                  for (b = 0; b < bytes; b++)
                    d[b] = (guchar) ((r[b] * tot_frac) + 0.5);
                }

              r += bytes;
              d += bytes;
            }

          dest += destwidth;

          /*  clear the "row" array  */
          memset (row, 0, sizeof (gdouble) * destwidth);

          i++;
        }
      else
        {
          pixel_region_get_row (srcPR,
                                srcPR->x,
                                srcPR->y + src_row * subsample,
                                orig_width * subsample,
                                src,
                                subsample);
        }
    }

  /*  free up temporary arrays  */
  g_free (row);
  g_free (x_frac);
  g_free (src);
}
