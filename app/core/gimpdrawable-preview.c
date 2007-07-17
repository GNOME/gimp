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

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager-preview.h"

#include "paint-funcs/scale-funcs.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpdrawable-preview.h"
#include "gimplayer.h"
#include "gimppreviewcache.h"


/*  local function prototypes  */

static TempBuf * gimp_drawable_preview_private (GimpDrawable *drawable,
                                                gint          width,
                                                gint          height);
static TempBuf * gimp_drawable_indexed_preview (GimpDrawable *drawable,
                                                const guchar *cmap,
                                                gint          src_x,
                                                gint          src_y,
                                                gint          src_width,
                                                gint          src_height,
                                                gint          dest_width,
                                                gint          dest_height);

static void      subsample_indexed_region      (PixelRegion  *srcPR,
                                                PixelRegion  *destPR,
                                                const guchar *cmap,
                                                gint          subsample);


/*  public functions  */

TempBuf *
gimp_drawable_get_preview (GimpViewable *viewable,
                           GimpContext  *context,
                           gint          width,
                           gint          height)
{
  GimpDrawable *drawable;
  GimpImage    *image;

  drawable = GIMP_DRAWABLE (viewable);
  image   = gimp_item_get_image (GIMP_ITEM (drawable));

  if (! image->gimp->config->layer_previews)
    return NULL;

  /* Ok prime the cache with a large preview if the cache is invalid */
  if (! drawable->preview_valid                  &&
      width  <= PREVIEW_CACHE_PRIME_WIDTH        &&
      height <= PREVIEW_CACHE_PRIME_HEIGHT       &&
      image                                     &&
      image->width  > PREVIEW_CACHE_PRIME_WIDTH &&
      image->height > PREVIEW_CACHE_PRIME_HEIGHT)
    {
      TempBuf *tb = gimp_drawable_preview_private (drawable,
                                                   PREVIEW_CACHE_PRIME_WIDTH,
                                                   PREVIEW_CACHE_PRIME_HEIGHT);

      /* Save the 2nd call */
      if (width  == PREVIEW_CACHE_PRIME_WIDTH &&
          height == PREVIEW_CACHE_PRIME_HEIGHT)
        return tb;
    }

  /* Second call - should NOT visit the tile cache...*/
  return gimp_drawable_preview_private (drawable, width, height);
}

gint
gimp_drawable_preview_bytes (GimpDrawable *drawable)
{
  GimpImageBaseType base_type;
  gint              bytes = 0;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), 0);

  base_type = GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (drawable));

  switch (base_type)
    {
    case GIMP_RGB:
    case GIMP_GRAY:
      bytes = gimp_drawable_bytes (drawable);
      break;

    case GIMP_INDEXED:
      bytes = gimp_drawable_has_alpha (drawable) ? 4 : 3;
      break;
    }

  return bytes;
}

TempBuf *
gimp_drawable_get_sub_preview (GimpDrawable *drawable,
                               gint          src_x,
                               gint          src_y,
                               gint          src_width,
                               gint          src_height,
                               gint          dest_width,
                               gint          dest_height)
{
  GimpItem    *item;
  GimpImage   *image;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (src_x >= 0, NULL);
  g_return_val_if_fail (src_y >= 0, NULL);
  g_return_val_if_fail (src_width  > 0, NULL);
  g_return_val_if_fail (src_height > 0, NULL);
  g_return_val_if_fail (dest_width  > 0, NULL);
  g_return_val_if_fail (dest_height > 0, NULL);

  item = GIMP_ITEM (drawable);

  g_return_val_if_fail ((src_x + src_width)  <= gimp_item_width  (item), NULL);
  g_return_val_if_fail ((src_y + src_height) <= gimp_item_height (item), NULL);

  image = gimp_item_get_image (item);

  if (! image->gimp->config->layer_previews)
    return NULL;

  if (GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (drawable)) == GIMP_INDEXED)
    return gimp_drawable_indexed_preview (drawable,
                                          gimp_image_get_colormap (image),
                                          src_x, src_y, src_width, src_height,
                                          dest_width, dest_height);

  return tile_manager_get_sub_preview (gimp_drawable_get_tiles (drawable),
                                       src_x, src_y, src_width, src_height,
                                       dest_width, dest_height);
}


/*  private functions  */

static TempBuf *
gimp_drawable_preview_private (GimpDrawable *drawable,
                               gint          width,
                               gint          height)
{
  TempBuf *ret_buf;

  if (! drawable->preview_valid ||
      ! (ret_buf = gimp_preview_cache_get (&drawable->preview_cache,
                                           width, height)))
    {
      GimpItem *item = GIMP_ITEM (drawable);

      ret_buf = gimp_drawable_get_sub_preview (drawable,
                                               0, 0,
                                               gimp_item_width (item),
                                               gimp_item_height (item),
                                               width,
                                               height);

      if (! drawable->preview_valid)
        gimp_preview_cache_invalidate (&drawable->preview_cache);

      drawable->preview_valid = TRUE;

      gimp_preview_cache_add (&drawable->preview_cache, ret_buf);
    }

  return ret_buf;
}

static TempBuf *
gimp_drawable_indexed_preview (GimpDrawable *drawable,
                               const guchar *cmap,
                               gint          src_x,
                               gint          src_y,
                               gint          src_width,
                               gint          src_height,
                               gint          dest_width,
                               gint          dest_height)
{
  TempBuf     *preview_buf;
  PixelRegion  srcPR;
  PixelRegion  destPR;
  gint         bytes     = gimp_drawable_preview_bytes (drawable);
  gint         subsample = 1;

  /*  calculate 'acceptable' subsample  */
  while ((dest_width  * (subsample + 1) * 2 < src_width) &&
         (dest_height * (subsample + 1) * 2 < src_width))
    subsample += 1;

  pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                     src_x, src_y, src_width, src_height,
                     FALSE);

  preview_buf = temp_buf_new (dest_width, dest_height, bytes, 0, 0, NULL);

  pixel_region_init_temp_buf (&destPR, preview_buf,
                              0, 0, dest_width, dest_height);

  subsample_indexed_region (&srcPR, &destPR, cmap, subsample);

  return preview_buf;
}

static void
subsample_indexed_region (PixelRegion  *srcPR,
                          PixelRegion  *destPR,
                          const guchar *cmap,
                          gint          subsample)
{
#define EPSILON 0.000001
  guchar  *src,  *s;
  guchar  *dest, *d;
  gdouble *row,  *r;
  gint     destwidth;
  gint     src_row, src_col;
  gint     bytes, b;
  gint     width, height;
  gint     orig_width, orig_height;
  gdouble  x_rat, y_rat;
  gdouble  x_cum, y_cum;
  gdouble  x_last, y_last;
  gdouble *x_frac, y_frac, tot_frac;
  gint     i, j;
  gint     frac;
  gboolean advance_dest;
  gboolean has_alpha;

  g_return_if_fail (cmap != NULL);

  orig_width  = srcPR->w / subsample;
  orig_height = srcPR->h / subsample;
  width       = destPR->w;
  height      = destPR->h;

  /*  Some calculations...  */
  bytes     = destPR->bytes;
  destwidth = destPR->rowstride;

  has_alpha = pixel_region_has_alpha (srcPR);

  /*  the data pointers...  */
  src  = g_new (guchar, orig_width * bytes);
  dest = destPR->data;

  /*  find the ratios of old x to new x and old y to new y  */
  x_rat = (gdouble) orig_width  / (gdouble) width;
  y_rat = (gdouble) orig_height / (gdouble) height;

  /*  allocate an array to help with the calculations  */
  row    = g_new0 (gdouble, width * bytes);
  x_frac = g_new (gdouble, width + orig_width);

  /*  initialize the pre-calculated pixel fraction array  */
  src_col = 0;
  x_cum   = (gdouble) src_col;
  x_last  = x_cum;

  for (i = 0; i < width + orig_width; i++)
    {
      if (x_cum + x_rat <= (src_col + 1 + EPSILON))
        {
          x_cum += x_rat;
          x_frac[i] = x_cum - x_last;
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
  y_cum   = (gdouble) src_row;
  y_last  = y_cum;

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
      x_cum   = (gdouble) src_col;

      /* determine the fraction of the src pixel we are using for y */
      if (y_cum + y_rat <= (src_row + 1 + EPSILON))
        {
          y_cum += y_rat;
          y_frac = y_cum - y_last;
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
          if (x_cum + x_rat <= (src_col + 1 + EPSILON))
            {
              r += bytes;
              x_cum += x_rat;
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
          tot_frac = 1.0 / (x_rat * y_rat);

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
