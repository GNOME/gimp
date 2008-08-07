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

#include "paint-funcs-types.h"

#include "base/tile.h"
#include "base/tile-manager.h"
#include "base/pixel-region.h"

#include "paint-funcs.h"
#include "scale-region.h"


static void           scale_region_buffer     (PixelRegion           *srcPR,
                                               PixelRegion           *dstPR,
                                               GimpInterpolationType  interpolation,
                                               GimpProgressFunc       progress_callback,
                                               gpointer               progress_data);
static void           scale_region_tile       (PixelRegion           *srcPR,
                                               PixelRegion           *dstPR,
                                               GimpInterpolationType  interpolation,
                                               GimpProgressFunc       progress_callback,
                                               gpointer               progress_data);
static void           scale                   (TileManager           *srcTM,
                                               TileManager           *dstTM,
                                               GimpInterpolationType  interpolation,
                                               GimpProgressFunc       progress_callback,
                                               gpointer               progress_data,
                                               gint                  *progress,
                                               gint                   max_progress);
static void           scale_pr                (PixelRegion           *srcPR,
                                               PixelRegion           *dstPR,
                                               GimpInterpolationType  interpolation);
static void           interpolate_bilinear    (TileManager           *srcTM,
                                               gint                   x0,
                                               gint                   y0,
                                               gint                   x1,
                                               gint                   y1,
                                               gdouble                xfrac,
                                               gdouble                yfrac,
                                               guchar                *pixel,
                                               const gfloat          *kernel_lookup);
static void           interpolate_nearest     (TileManager           *srcTM,
                                               gint                   x0,
                                               gint                   y0,
                                               gint                   x1,
                                               gint                   y1,
                                               gdouble                xfrac,
                                               gdouble                yfrac,
                                               guchar                *pixel,
                                               const gfloat          *kernel_lookup);
static void           interpolate_cubic       (TileManager           *srcTM,
                                               gint                   x0,
                                               gint                   y0,
                                               gint                   x1,
                                               gint                   y1,
                                               gdouble                xfrac,
                                               gdouble                yfrac,
                                               guchar                *pixel,
                                               const gfloat          *kernel_lookup);
static void           decimate_gauss          (TileManager           *srcTM,
                                               gint                   x0,
                                               gint                   y0,
                                               gint                   x1,
                                               gint                   y1,
                                               gdouble                xfrac,
                                               gdouble                yfrac,
                                               guchar                *pixel,
                                               const gfloat          *kernel_lookup);
static void           decimate_average        (TileManager           *srcTM,
                                               gint                   x0,
                                               gint                   y0,
                                               gint                   x1,
                                               gint                   y1,
                                               gdouble                xfrac,
                                               gdouble                yfrac,
                                               guchar                *pixel,
                                               const gfloat          *kernel_lookup);
static gfloat *       create_lanczos3_lookup  (void);
static void           interpolate_lanczos3    (TileManager           *srcTM,
                                               gint                   x1,
                                               gint                   y1,
                                               gint                   x2,
                                               gint                   y2,
                                               gdouble                xfrac,
                                               gdouble                yfrac,
                                               guchar                *pixel,
                                               const gfloat          *kernel_lookup);
static void           decimate_average_pr     (PixelRegion           *srcPR,
                                               gint                   x0,
                                               gint                   y0,
                                               gint                   x1,
                                               gint                   y1,
                                               guchar                *pixel);
static void           interpolate_bilinear_pr (PixelRegion           *srcPR,
                                               gint                   x0,
                                               gint                   y0,
                                               gint                   x1,
                                               gint                   y1,
                                               gdouble                xfrac,
                                               gdouble                yfrac,
                                               guchar                *p);
static void           determine_scale         (PixelRegion           *srcPR,
                                               PixelRegion           *dstPR,
                                               gint                  *levelx,
                                               gint                  *levely,
                                               gint                  *max_progress);
static inline void    gaussan_lanczos2        (const guchar          *pixels,
                                               gint                   bytes,
                                               guchar                *pixel);
static inline void    decimate_lanczos2       (TileManager           *srcTM,
                                               gint                   x0,
                                               gint                   y0,
                                               gint                   x1,
                                               gint                   y1,
                                               gdouble                xfrac,
                                               gdouble                yfrac,
                                               guchar                *pixel,
                                               const gfloat          *kernel_lookup);
static inline void    pixel_average           (const guchar          *p1,
                                               const guchar          *p2,
                                               const guchar          *p3,
                                               const guchar          *p4,
                                               guchar                *p,
                                               const gint             bytes);
static inline void    gaussan_decimate        (const guchar          *pixels,
                                               const gint             bytes,
                                               guchar                *pixel);
static inline gdouble cubic_spline_fit        (gdouble                dx,
                                               gint                   pt0,
                                               gint                   pt1,
                                               gint                   pt2,
                                               gint                   pt3);
static inline gdouble weighted_sum            (gdouble                dx,
                                               gdouble                dy,
                                               gint                   s00,
                                               gint                   s10,
                                               gint                   s01,
                                               gint                   s11);
static inline gdouble sinc                    (gdouble                x);
static inline gdouble lanczos3_mul_alpha      (const guchar          *pixels,
                                               const gdouble         *x_kernel,
                                               const gdouble         *y_kernel,
                                               const gint             bytes,
                                               const gint             byte);
static inline gdouble lanczos3_mul            (const guchar          *pixels,
                                               const gdouble         *x_kernel,
                                               const gdouble         *y_kernel,
                                               const gint             bytes,
                                               const gint             byte);


static void
determine_scale (PixelRegion *srcPR,
                 PixelRegion *dstPR,
                 gint        *levelx,
                 gint        *levely,
                 gint        *max_progress)
{
  gdouble scalex = (gdouble) dstPR->w / (gdouble) srcPR->w;
  gdouble scaley = (gdouble) dstPR->h / (gdouble) srcPR->h;
  gint    width  = srcPR->w;
  gint    height = srcPR->h;

  *max_progress = ((height % TILE_HEIGHT) + 1) * ((width % TILE_WIDTH) + 1);

  /* determine scaling levels */
  while (scalex >= 2)
    {
      scalex  /= 2;
      width   *=2;
      *levelx -= 1;
      *max_progress += (((height % TILE_HEIGHT) + 1) *
                        ((width % TILE_WIDTH) + 1));
    }

  while (scaley >= 2)
    {
      scaley  /= 2;
      height  *= 2;
      *levely -= 1;
      *max_progress += (((height % TILE_HEIGHT) + 1) *
                        ((width % TILE_WIDTH) + 1));
    }

  while (scalex <= 0.5)
    {
      scalex  *= 2;
      width   /= 2;
      *levelx += 1;
      *max_progress += (((height % TILE_HEIGHT) + 1) *
                        ((width % TILE_WIDTH) + 1));
    }

  while (scaley <= 0.5)
    {
      scaley  *= 2;
      height  *= 2;
      *levely += 1;
      *max_progress += (((height % TILE_HEIGHT) + 1) *
                        ((width % TILE_WIDTH) + 1));
    }
}

static void
scale_region_buffer (PixelRegion           *srcPR,
                     PixelRegion           *dstPR,
                     GimpInterpolationType  interpolation,
                     GimpProgressFunc       progress_callback,
                     gpointer               progress_data)
{
  PixelRegion  tmpPR0;
  PixelRegion  tmpPR1;
  gint         width        = srcPR->w;
  gint         height       = srcPR->h;
  gint         bytes        = srcPR->bytes;
  gint         max_progress = 0;
  gint         levelx       = 0;
  gint         levely       = 0;

  /* determine scaling levels */
  determine_scale (srcPR, dstPR, &levelx, &levely, &max_progress);

  pixel_region_init_data (&tmpPR0,
                          g_memdup (srcPR->data, width * height * bytes),
                          bytes, width * bytes, 0, 0, width, height);

  while (levelx < 0 && levely < 0)
    {
      width  <<= 1;
      height <<= 1;

      pixel_region_init_data (&tmpPR1,
                              g_new (guchar, width * height * bytes),
                              bytes, width * bytes, 0, 0, width, height);

      scale_pr (&tmpPR0, &tmpPR1, interpolation);

      g_free (tmpPR0.data);
      pixel_region_init_data (&tmpPR0,
                              tmpPR1.data,
                              bytes, width * bytes, 0, 0, width, height);

      levelx++;
      levely++;
    }

  while (levelx < 0)
    {
      width <<= 1;

      pixel_region_init_data (&tmpPR1,
                              g_new (guchar, width * height * bytes),
                              bytes, width * bytes, 0, 0, width, height);

      scale_pr (&tmpPR0, &tmpPR1, interpolation);

      g_free (tmpPR0.data);
      pixel_region_init_data (&tmpPR0,
                              tmpPR1.data,
                              bytes, width * bytes, 0, 0, width, height);

      levelx++;
    }

  while (levely < 0)
    {
      height <<= 1;

      pixel_region_init_data (&tmpPR1,
                              g_new (guchar, width * height * bytes),
                              bytes, width * bytes, 0, 0, width, height);

      scale_pr (&tmpPR0, &tmpPR1, interpolation);

      g_free (tmpPR0.data);
      pixel_region_init_data (&tmpPR0,
                              tmpPR1.data,
                              bytes, width * bytes, 0, 0, width, height);
      levely++;
    }

  while (levelx > 0 && levely > 0)
    {
      width  >>= 1;
      height >>= 1;

      pixel_region_init_data (&tmpPR1,
                              g_new (guchar, width * height * bytes),
                              bytes, width * bytes, 0, 0, width, height);

      scale_pr (&tmpPR0, &tmpPR1, interpolation);

      g_free (tmpPR0.data);
      pixel_region_init_data (&tmpPR0,
                              tmpPR1.data,
                              bytes, width * bytes, 0, 0, width, height);

      levelx--;
      levely--;
    }

  while (levelx > 0)
    {
      width <<= 1;

      pixel_region_init_data (&tmpPR1,
                              g_new (guchar, width * height * bytes),
                              bytes, width * bytes, 0, 0, width, height);

      scale_pr (&tmpPR0, &tmpPR1, interpolation);

      g_free (tmpPR0.data);
      pixel_region_init_data (&tmpPR0,
                              tmpPR1.data,
                              bytes, width * bytes, 0, 0, width, height);

      levelx--;
    }

  while (levely > 0)
    {
      height <<= 1;

      pixel_region_init_data (&tmpPR1,
                              g_new (guchar, width * height * bytes),
                              bytes, width * bytes, 0, 0, width, height);

      scale_pr (&tmpPR0, &tmpPR1, interpolation);

      g_free (tmpPR0.data);
      pixel_region_init_data (&tmpPR0,
                              tmpPR1.data,
                              bytes, width * bytes, 0, 0, width, height);

      levely--;
    }

  scale_pr (&tmpPR0, dstPR, interpolation);

  g_free (tmpPR0.data);

  return;
}

static void
scale_region_tile (PixelRegion           *srcPR,
                   PixelRegion           *dstPR,
                   GimpInterpolationType  interpolation,
                   GimpProgressFunc       progress_callback,
                   gpointer               progress_data)
{
  TileManager *tmpTM        = NULL;
  TileManager *srcTM        = srcPR->tiles;
  TileManager *dstTM        = dstPR->tiles;
  gint         width        = srcPR->w;
  gint         height       = srcPR->h;
  gint         bytes        = srcPR->bytes;
  gint         max_progress = (((height % TILE_HEIGHT) + 1) *
                               ((width % TILE_WIDTH) + 1));
  gint         progress     = 0;
  gint         levelx       = 0;
  gint         levely       = 0;

  /* determine scaling levels */
  determine_scale (srcPR, dstPR, &levelx, &levely, &max_progress);

  if (levelx == 0 && levely == 0)
    {
       scale (srcTM, dstTM, interpolation,
              progress_callback,
              progress_data, &progress, max_progress);
    }

  while (levelx < 0 && levely < 0)
    {
      width  <<= 1;
      height <<= 1;

      tmpTM = tile_manager_new (width, height, bytes);
      scale (srcTM, tmpTM, interpolation,
             progress_callback, progress_data, &progress, max_progress);

      if (srcTM != srcPR->tiles)
        tile_manager_unref (srcTM);

      srcTM = tmpTM;
      levelx++;
      levely++;
    }

  while (levelx < 0)
    {
      width <<= 1;

      tmpTM = tile_manager_new (width, height, bytes);
      scale (srcTM, tmpTM, interpolation,
             progress_callback, progress_data, &progress, max_progress);

      if (srcTM != srcPR->tiles)
        tile_manager_unref (srcTM);

      srcTM = tmpTM;
      levelx++;
    }

  while (levely < 0)
    {
      height <<= 1;

      tmpTM = tile_manager_new (width, height, bytes);
      scale (srcTM, tmpTM, interpolation,
             progress_callback, progress_data, &progress, max_progress);

      if (srcTM != srcPR->tiles)
        tile_manager_unref (srcTM);

      srcTM = tmpTM;
      levely++;
    }

  while (levelx > 0 && levely > 0)
    {
      width  >>= 1;
      height >>= 1;

      tmpTM = tile_manager_new (width, height, bytes);
      scale (srcTM, tmpTM, interpolation,
             progress_callback, progress_data, &progress, max_progress);

      if (srcTM != srcPR->tiles)
        tile_manager_unref (srcTM);

      srcTM = tmpTM;
      levelx--;
      levely--;
    }

  while (levelx > 0)
    {
      width <<= 1;

      tmpTM = tile_manager_new (width, height, bytes);
      scale (srcTM, tmpTM, interpolation,
             progress_callback, progress_data, &progress, max_progress);

      if (srcTM != srcPR->tiles)
        tile_manager_unref (srcTM);

      srcTM = tmpTM;
      levelx--;
    }

  while (levely > 0)
    {
      height <<= 1;

      tmpTM = tile_manager_new (width, height, bytes);
      scale (srcTM, tmpTM, interpolation,
             progress_callback, progress_data, &progress, max_progress);

      if (srcTM != srcPR->tiles)
        tile_manager_unref (srcTM);

      srcTM = tmpTM;
      levely--;
    }

  if (tmpTM != NULL)
    {
      scale (tmpTM, dstTM, interpolation,
             progress_callback,
             progress_data, &progress, max_progress);
      tile_manager_unref (tmpTM);
    }

  if (progress_callback)
    progress_callback (0, max_progress, max_progress, progress_data);

  return;
}

static void
scale (TileManager           *srcTM,
       TileManager           *dstTM,
       GimpInterpolationType  interpolation,
       GimpProgressFunc       progress_callback,
       gpointer               progress_data,
       gint                  *progress,
       gint                   max_progress)
{
  guint              src_width    = tile_manager_width  (srcTM);
  guint              src_height   = tile_manager_height (srcTM);
  Tile              *dst_tile;
  guchar            *dst_data;
  guint              dst_width    = tile_manager_width  (dstTM);
  guint              dst_height   = tile_manager_height (dstTM);
  guint              dst_bpp      = tile_manager_bpp (dstTM);
  guint              dst_tilerows = tile_manager_tiles_per_row(dstTM);    /*  the number of tiles in each row      */
  guint              dst_tilecols = tile_manager_tiles_per_col(dstTM);    /*  the number of tiles in each columns  */
  guint              dst_ewidth;
  guint              dst_eheight;
  guint              dst_stride;
  gdouble            scalex       = (gdouble) dst_width  / (gdouble) src_width;
  gdouble            scaley       = (gdouble) dst_height / (gdouble) src_height;
  gdouble            xfrac;
  gdouble            yfrac;
  gint               x, y, x0, y0, x1, y1;
  gint               sx0, sy0, sx1, sy1;
  gint               col, row;
  guchar             pixel[4];
  gfloat            *kernel_lookup = NULL;

  /* fall back if not enough pixels available */
  if (interpolation != GIMP_INTERPOLATION_NONE)
    {
      if (src_width < 2 || src_height < 2 ||
          dst_width < 2 || dst_height < 2)
        {
          interpolation = GIMP_INTERPOLATION_NONE;
        }
      else if (src_width < 3 || src_height < 3 ||
               dst_width < 3 || dst_height < 3)
        {
          interpolation = GIMP_INTERPOLATION_LINEAR;
        }
    }

  /* if scale is 2^n */
  if (src_width == dst_width && src_height == dst_height)
    {
      for (row = 0; row < dst_tilerows; row++)
        {
          for (col = 0; col < dst_tilecols; col++)
            {
              dst_tile    = tile_manager_get_at (dstTM, col, row, TRUE, TRUE);
              dst_data    = tile_data_pointer (dst_tile, 0, 0);
              dst_bpp     = tile_bpp (dst_tile);
              dst_ewidth  = tile_ewidth (dst_tile);
              dst_eheight = tile_eheight (dst_tile);
              dst_stride  = dst_ewidth * dst_bpp;
              x0          = col * TILE_WIDTH;
              y0          = row * TILE_HEIGHT;
              x1          = x0 + dst_ewidth - 1;
              y1          = y0 + dst_eheight - 1;

              read_pixel_data (srcTM, x0, y0, x1, y1, dst_data, dst_stride);

              tile_release (dst_tile, TRUE);

              if (progress_callback)
                progress_callback (0, max_progress, ((*progress)++),
                                   progress_data);
            }
        }

      return;
    }

  if (interpolation == GIMP_INTERPOLATION_LANCZOS )
    kernel_lookup = create_lanczos3_lookup();

  for (row = 0; row < dst_tilerows; row++)
    {
      for (col = 0; col < dst_tilecols; col++)
        {
          dst_tile    = tile_manager_get_at (dstTM, col, row, FALSE, FALSE);
          dst_data    = tile_data_pointer (dst_tile, 0, 0);
          dst_bpp     = tile_bpp (dst_tile);
          dst_ewidth  = tile_ewidth (dst_tile);
          dst_eheight = tile_eheight (dst_tile);
          dst_stride  = dst_ewidth * dst_bpp;

          x0          = col * TILE_WIDTH;
          y0          = row * TILE_HEIGHT;
          x1          = x0 + dst_ewidth - 1;
          y1          = y0 + dst_eheight - 1;

          for (y = y0; y <= y1; y++)
            {
              yfrac = y / scaley;
              sy0   = (gint) yfrac;
              sy1   = sy0 + 1;
              sy1   = (sy1 >= src_height) ? src_height - 1 : sy1;
              yfrac = yfrac - sy0;

              for (x = x0; x <= x1; x++)
                {
                  xfrac = x / scalex;
                  sx0   = (gint) xfrac;
                  sx1   = sx0 + 1;
                  sx1   = (sx1 >= src_width) ? src_width - 1 : sx1;
                  xfrac = xfrac - sx0;

                  switch (interpolation)
                    {
                    case GIMP_INTERPOLATION_NONE:
                      interpolate_nearest (srcTM, sx0, sy0,
                                           sx1, sy1,
                                           xfrac, yfrac,
                                           pixel,
                                           kernel_lookup);
                      break;

                    case GIMP_INTERPOLATION_LINEAR:
                      if (scalex == 0.5 || scaley == 0.5)
                        decimate_average (srcTM, sx0, sy0,
                                          sx1, sy1,
                                          xfrac, yfrac,
                                          pixel,
                                          kernel_lookup);
                      else
                        interpolate_bilinear (srcTM, sx0, sy0,
                                              sx1, sy1,
                                              xfrac, yfrac,
                                              pixel,
                                              kernel_lookup);
                      break;

                    case GIMP_INTERPOLATION_CUBIC:
                      if (scalex == 0.5 || scaley == 0.5)
                        decimate_gauss (srcTM, sx0, sy0,
                                        sx1, sy1,
                                        xfrac, yfrac,
                                        pixel,
                                        kernel_lookup);
                      else
                        interpolate_cubic (srcTM, sx0, sy0,
                                           sx1, sy1,
                                           xfrac, yfrac,
                                           pixel,
                                           kernel_lookup);
                      break;

                    case GIMP_INTERPOLATION_LANCZOS:
                      if (scalex == 0.5 || scaley == 0.5)
                        decimate_lanczos2 (srcTM, sx0, sy0,
                                           sx1, sy1,
                                           xfrac, yfrac,
                                           pixel,
                                           kernel_lookup);
                      else
                        interpolate_lanczos3 (srcTM, sx0, sy0,
                                              sx1, sy1,
                                              xfrac, yfrac,
                                              pixel,
                                              kernel_lookup);
                      break;
                    }

                  write_pixel_data_1 (dstTM, x, y, pixel);
                }
            }

          if (progress_callback)
            progress_callback (0, max_progress, ((*progress)++), progress_data);
        }
    }

  if (interpolation == GIMP_INTERPOLATION_LANCZOS)
    g_free (kernel_lookup);
}

static void inline
pixel_average (const guchar *p1,
               const guchar *p2,
               const guchar *p3,
               const guchar *p4,
               guchar       *p,
               const gint    bytes)
{
  gdouble sum, alphasum;
  gdouble alpha;
  gint    b;

  for (b = 0; b < bytes; b++)
    p[b]=0;

  switch (bytes)
    {
    case 1:
      sum = ((p1[0] + p2[0] + p3[0] + p4[0]) / 4);

      p[0] = (guchar) CLAMP (sum, 0, 255);
      break;

    case 2:
      alphasum = p1[1] +  p2[1] +  p3[1] +  p4[1];

      if (alphasum > 0)
        {
          sum = p1[0] * p1[1] + p2[0] * p2[1] + p3[0] * p3[1] + p4[0] * p4[1];
          sum /= alphasum;

          alpha = alphasum / 4;

          p[0] = (guchar) CLAMP (sum, 0, 255);
          p[1] = (guchar) CLAMP (alpha, 0, 255);
        }
      break;

    case 3:
      for (b = 0; b<3; b++)
        {
          sum = ((p1[b] + p2[b] + p3[b] + p4[b]) / 4);
          p[b] = (guchar) CLAMP (sum, 0, 255);
        }
      break;

    case 4:
      alphasum = p1[3] +  p2[3] +  p3[3] +  p4[3];

      if (alphasum > 0)
        {
          for (b = 0; b<3; b++)
            {
              sum = p1[b] * p1[3] + p2[b] * p2[3] + p3[b] * p3[3] + p4[b] * p4[3];
              sum /= alphasum;

              p[b] = (guchar) CLAMP (sum, 0, 255);
            }

          alpha = alphasum / 4;

          p[3] = (guchar) CLAMP (alpha, 0, 255);
        }
      break;
    }
}

void
scale_region (PixelRegion           *srcPR,
              PixelRegion           *dstPR,
              GimpInterpolationType  interpolation,
              GimpProgressFunc       progress_callback,
              gpointer               progress_data)
{

  /* Copy and return if scale = 1.0 */
  if (srcPR->h == dstPR->h && srcPR->w == dstPR->w)
    {
      copy_region (srcPR, dstPR);
      return;
    }

  if (srcPR->tiles == NULL && srcPR->data != NULL)
    {
      scale_region_buffer (srcPR, dstPR, interpolation,
                           progress_callback, progress_data);
      return;
    }

  if (srcPR->tiles != NULL && srcPR->data == NULL)
    {
      scale_region_tile (srcPR, dstPR, interpolation,
                         progress_callback, progress_data);
      return;
    }

  g_assert_not_reached ();
}

static void
decimate_gauss (TileManager  *srcTM,
                gint          x0,
                gint          y0,
                gint          x1,
                gint          y1,
                gdouble       xfrac,
                gdouble       yfrac,
                guchar       *pixel,
                const gfloat *kernel_lookup)
{
  gint    src_bpp    = tile_manager_bpp  (srcTM);
  guint   src_width  = tile_manager_width  (srcTM);
  guint   src_height = tile_manager_height (srcTM);
  guchar  pixel1[4];
  guchar  pixel2[4];
  guchar  pixel3[4];
  guchar  pixel4[4];
  guchar  pixels[16 * 4];
  gint    x, y, i;
  guchar *p;

  for (y = y0 - 1, i = 0; y <= y0 + 2; y++)
    {
      for (x = x0 - 1; x <= x0 + 2; x++, i++)
        {
          x1 = ABS(x);
          y1 = ABS(y);
          x1 = (x1 < src_width)  ? x1 : 2 * src_width - x1 - 1;
          y1 = (y1 < src_height) ? y1 : 2 * src_height - y1 - 1;
          read_pixel_data_1 (srcTM, x1, y1, pixels + (i * src_bpp));
        }
    }

  p = pixels + (0 * src_bpp);
  gaussan_decimate (p, src_bpp, pixel1);
  p = pixels + (1 * src_bpp);
  gaussan_decimate (p, src_bpp, pixel2);
  p = pixels + (4 * src_bpp);
  gaussan_decimate (p, src_bpp, pixel3);
  p = pixels + (5 * src_bpp);
  gaussan_decimate (p, src_bpp, pixel4);

  pixel_average (pixel1, pixel2, pixel3, pixel4, pixel, src_bpp);

}

static inline void
gaussan_decimate (const guchar *pixels,
                  const gint    bytes,
                  guchar       *pixel)
{
  const guchar *p;
  gdouble       sum;
  gdouble       alphasum;
  gdouble       alpha;
  gint          b;

  for (b = 0; b < bytes; b++)
    pixel[b] = 0;

  p = pixels;

  switch (bytes)
    {
    case 1:
      sum  = p[0]     + p[1] * 2 + p[2];
      sum += p[4] * 2 + p[5] * 4 + p[6] * 2;
      sum += p[8]     + p[9] * 2 + p[10];
      sum /= 16;

      pixel[0] = (guchar) CLAMP (sum, 0, 255);
      break;

    case 2:
      alphasum  = p[1]     + p[3]  * 2 + p[5];
      alphasum += p[9] * 2 + p[11] * 4 + p[13] * 2;
      alphasum += p[17]    + p[19] * 2 + p[21];

      if (alphasum > 0)
        {
          sum  = p[0]  * p[1]     + p[2]  * p[3]  * 2 + p[4]  * p[5];
          sum += p[8]  * p[9] * 2 + p[10] * p[11] * 4 + p[12] * p[13] * 2;
          sum += p[16] * p[17]    + p[18] * p[19] * 2 + p[20] * p[21];
          sum /= alphasum;

          alpha = alphasum / 16;

          pixel[0] = (guchar) CLAMP (sum,   0, 255);
          pixel[1] = (guchar) CLAMP (alpha, 0, 255);
        }
      break;

    case 3:
      for (b = 0; b < 3; b++ )
        {
          sum  = p[b   ]     + p[3 + b] * 2 + p[6 + b];
          sum += p[12 + b] * 2 + p[15 + b] * 4 + p[18 + b] * 2;
          sum += p[24 + b]     + p[27 + b] * 2 + p[30 + b];
          sum /= 16;

          pixel[b] = (guchar) CLAMP (sum, 0, 255);
        }
      break;

    case 4:
      alphasum  = p[3]      + p[7]  * 2  + p[11];
      alphasum += p[19] * 2 + p[23] * 4  + p[27] * 2;
      alphasum += p[35]     + p[39] * 2  + p[43];

      if (alphasum > 0)
        {
          for (b = 0; b < 3; b++)
            {
              sum = p[   b] * p[3]      + p[4 + b] * p[7]  * 2 + p[8 + b] * p[11];
              sum += p[16 + b] * p[19] * 2 + p[20 + b] * p[23] * 4 + p[24 + b] * p[27] * 2;
              sum += p[32 + b] * p[35]     + p[36 + b] * p[39] * 2 + p[40 + b] * p[43];
              sum /= alphasum;

              pixel[b] = (guchar) CLAMP (sum, 0, 255);
            }

          alpha = alphasum / 16;

          pixel[3] = (guchar) CLAMP (alpha, 0, 255);
        }
      break;
    }
}

static inline void
decimate_lanczos2 (TileManager  *srcTM,
                   gint          x0,
                   gint          y0,
                   gint          x1,
                   gint          y1,
                   gdouble       xfrac,
                   gdouble       yfrac,
                   guchar       *pixel,
                   const gfloat *kernel_lookup)
{
  gint    src_bpp    = tile_manager_bpp  (srcTM);
  guint   src_width  = tile_manager_width  (srcTM);
  guint   src_height = tile_manager_height (srcTM);
  guchar  pixel1[4];
  guchar  pixel2[4];
  guchar  pixel3[4];
  guchar  pixel4[4];
  guchar  pixels[36 * 4];
  gint    x, y, i;
  guchar *p;

  for (y = y0 - 2, i = 0; y <= y0 + 3; y++)
    {
      for (x = x0 - 2; x <= x0 + 3; x++, i++)
        {
          x1 = ABS(x);
          y1 = ABS(y);
          x1 = (x1 < src_width)  ? x1 : 2 * src_width - x1 - 1;
          y1 = (y1 < src_height) ? y1 : 2 * src_height - y1 - 1;
          read_pixel_data_1 (srcTM, x1, y1, pixels + (i * src_bpp));
        }
    }

  p = pixels + (0 * src_bpp);
  gaussan_lanczos2 (p, src_bpp, pixel1);
  p = pixels + (1 * src_bpp);
  gaussan_lanczos2 (p, src_bpp, pixel2);
  p = pixels + (6 * src_bpp);
  gaussan_lanczos2 (p, src_bpp, pixel3);
  p = pixels + (7 * src_bpp);
  gaussan_lanczos2 (p, src_bpp, pixel4);

  pixel_average (pixel1, pixel2, pixel3, pixel4, pixel, src_bpp);

}

static inline void
gaussan_lanczos2 (const guchar *pixels,
                  gint          bytes,
                  guchar       *pixel)
{
  /*
   *   Filter source taken from document:
   *   www.worldserver.com/turk/computergraphics/ResamplingFilters.pdf
   *
   *   Filters for Common Resampling Tasks
   *
   *   Ken Turkowski, Apple computer
   *
   */
  const guchar *p;
  gdouble      sum;
  gdouble      alphasum;
  gdouble      alpha;
  gint         b;

  for (b = 0; b < bytes; b++)
    pixel[b] = 0;

  p = pixels;

  switch (bytes)
    {
    case 1:
      sum  = p[0] + p[1] * -9 + p[2] * -16 + p[3] * -9 + p[4];
      sum += p[6] * -9 + p[7] * 81 + p[8] * 144 + p[9] * 81 + p[10] * -9;
      sum += p[12] * -16 +
             p[13] * 144 + p[14] * 256 + p[15] * 144 + p[16] * -16;
      sum += p[18] * -9 + p[19] * 81 + p[20] * 144 + p[21] * 81 + p[22] * -9;
      sum += p[24] + p[25] * -9 + p[26] * -16 + p[27] * -9 + p[28];
      sum /= 1024;

      pixel[0] = (guchar) CLAMP (sum, 0, 255);
      break;

    case 2:
      alphasum =  p[1] + p[3] * -9 + p[5] * -16 + p[7] * -9 + p[9];
      alphasum += p[13] * -9 +
                  p[15] * 81 + p[17] * 144 + p[19] * 81 + p[21] * -9;
      alphasum += p[25] * -16 +
                  p[27] * 144 + p[29] * 256 + p[31] * 144 + p[33] * -16;
      alphasum += p[37] * -9 +
                  p[39] * 81 + p[41] * 144 + p[43] * 81 + p[45] * -9;
      alphasum += p[49] + p[51] * -9 + p[53] * -16 + p[55] * -9 + p[57];

      if (alphasum > 0)
        {
          sum =  p[0] * p[1] +
                 p[2] * p[3] * -9 +
                 p[4] * p[5] * -16 + p[6] * p[7] * -9 + p[8] * p[9];
          sum += p[12] * p[13] * -9 +
                 p[14] * p[15] * 81 +
                 p[16] * p[17] * 144 + p[18] * p[19] * 81 + p[20] * p[21] * -9;
          sum += p[24] * p[25] * -16 +
                 p[26] * p[27] * 144 +
                 p[28] * p[29] * 256 + p[30] * p[31] * 144 + p[32] * p[33] * -16;
          sum += p[36] * p[37] * -9 +
                 p[38] * p[39] * 81 +
                 p[40] * p[41] * 144 + p[42] * p[43] * 81 + p[44] * p[45] * -9;
          sum += p[48] * p[49] +
                 p[50] * p[51] * -9 +
                 p[52] * p[53] * -16 + p[54] * p[55] * -9 + p[56] * p[57];
          sum /= alphasum;

          alpha = alphasum / 1024;

          pixel[0] = (guchar) CLAMP (sum, 0, 255);
          pixel[1] = (guchar) CLAMP (alpha, 0, 255);
        }
      break;

    case 3:
      for (b = 0; b < 3; b++)
        {
          sum =  p[b] +
                 p[3 + b] * -9 + p[6 + b] * -16 + p[9 + b] * -9 + p[12 + b];
          sum += p[18 + b] * -9 +
                 p[21 + b] * 81 +
                 p[24 + b] * 144 + p[27 + b] * 81 + p[30 + b] * -9;
          sum += p[36 + b] * -16 +
                 p[39 + b] * 144 +
                 p[42 + b] * 256 + p[45 + b] * 144 + p[48 + b] * -16;
          sum += p[54 + b] * -9 +
                 p[57 + b] * 81 +
                 p[60 + b] * 144 + p[63 + b] * 81 + p[66 + b] * -9;
          sum += p[72 + b] +
                 p[75 + b] * -9 + p[78 + b] * -16 + p[81 + b] * -9 + p[84 + b];
          sum /= 1024;

          pixel[b] = (guchar) CLAMP (sum, 0, 255);
        }
      break;

    case 4:
      alphasum =  p[3] + p[7] * -9 + p[11] * -16 + p[15] * -9 + p[19];
      alphasum += p[27] * -9 +
                  p[31] * 81 + p[35] * 144 + p[39] * 81 + p[43] * -9;
      alphasum += p[51] * -16 +
                  p[55] * 144 + p[59] * 256 + p[63] * 144 + p[67] * -16;
      alphasum += p[75] * -9 +
                  p[79] * 81 + p[83] * 144 + p[87] * 81 + p[91] * -9;
      alphasum += p[99] + p[103] * -9 + p[107] * -16 + p[111] * -9 + p[115];

      if (alphasum > 0)
        {
          for (b = 0; b < 3; b++)
            {
              sum =  p[0 + b] * p[3] +
                     p[4 + b] * p[7] * -9 +
                     p[8 + b] * p[11] * -16 +
                     p[12 + b] * p[15] * -9 + p[16 + b] * p[19];
              sum += p[24 + b] * p[27] * -9 +
                     p[28 + b] * p[31] * 81 +
                     p[32 + b] * p[35] * 144 +
                     p[36 + b] * p[39] * 81 + p[40 + b] * p[43] * -9;
              sum += p[48 + b] * p[51] * -16 +
                     p[52 + b] * p[55] * 144 +
                     p[56 + b] * p[59] * 256 +
                     p[60 + b] * p[63] * 144 + p[64 + b] * p[67] * -16;
              sum += p[72 + b] * p[75] * -9 +
                     p[76 + b] * p[79] * 81 +
                     p[80 + b] * p[83] * 144 +
                     p[84 + b] * p[87] * 81 + p[88 + b] * p[91] * -9;
              sum += p[96 + b] * p[99] +
                     p[100 + b] * p[103] * -9 +
                    p[104 + b] * p[107] * -16 +
                    p[108 + b] * p[111] * -9 + p[112 + b] * p[115];
              sum /= alphasum;
              pixel[b] = (guchar) CLAMP (sum, 0, 255);
            }

          alpha = (gint) alphasum / 1024;
          pixel[3] = (guchar) CLAMP (alpha, 0, 255);
        }
      break;
    }
}

static void
decimate_average (TileManager  *srcTM,
                  gint          x0,
                  gint          y0,
                  gint          x1,
                  gint          y1,
                  gdouble       xfrac,
                  gdouble       yfrac,
                  guchar       *pixel,
                  const gfloat *kernel_lookup)
{
  guchar pixel1[4];
  guchar pixel2[4];
  guchar pixel3[4];
  guchar pixel4[4];

  read_pixel_data_1 (srcTM, x0, y0, pixel1);
  read_pixel_data_1 (srcTM, x1, y0, pixel2);
  read_pixel_data_1 (srcTM, x0, y1, pixel3);
  read_pixel_data_1 (srcTM, x1, y1, pixel4);

  pixel_average (pixel1, pixel2, pixel3, pixel4, pixel, tile_manager_bpp (srcTM));
}

static inline gdouble
sinc (gdouble x)
{
  gdouble y = x * G_PI;

  if (ABS (x) < LANCZOS_MIN)
    return 1.0;

  return sin (y) / y;
}

/*
 * allocate and fill lookup table of Lanczos windowed sinc function
 * use gfloat since errors due to granularity of array far exceed data precision
 */
gfloat *
create_lanczos_lookup (void)
{
  const gdouble dx = LANCZOS_WIDTH / (gdouble) (LANCZOS_SAMPLES - 1);

  gfloat  *lookup = g_new (gfloat, LANCZOS_SAMPLES);
  gdouble  x      = 0.0;
  gint     i;

  for (i = 0; i < LANCZOS_SAMPLES; i++)
    {
      lookup[i] = ((ABS (x) < LANCZOS_WIDTH) ?
                   (sinc (x) * sinc (x / LANCZOS_WIDTH)) : 0.0);
      x += dx;
    }

  return lookup;
}

static gfloat *
create_lanczos3_lookup (void)
{
  const gdouble dx = 3.0 / (gdouble) (LANCZOS_SAMPLES - 1);

  gfloat  *lookup = g_new (gfloat, LANCZOS_SAMPLES);
  gdouble  x      = 0.0;
  gint     i;

  for (i = 0; i < LANCZOS_SAMPLES; i++)
    {
      lookup[i] = ((ABS (x) < 3.0) ?
                   (sinc (x) * sinc (x / 3.0)) : 0.0);
      x += dx;
    }

  return lookup;
}

static void
interpolate_nearest (TileManager  *srcTM,
                     gint          x0,
                     gint          y0,
                     gint          x1,
                     gint          y1,
                     gdouble       xfrac,
                     gdouble       yfrac,
                     guchar       *pixel,
                     const gfloat *kernel_lookup)
{
  gint x = (xfrac <= 0.5) ? x0 : x1;
  gint y = (yfrac <= 0.5) ? y0 : y1;

  read_pixel_data_1 (srcTM, x, y, pixel);
}

static inline gdouble
weighted_sum (gdouble dx,
              gdouble dy,
              gint    s00,
              gint    s10,
              gint    s01,
              gint    s11)
{
  return ((1 - dy) *
          ((1 - dx) * s00 + dx * s10) + dy * ((1 - dx) * s01 + dx * s11));
}

static void
interpolate_bilinear (TileManager  *srcTM,
                      gint          x0,
                      gint          y0,
                      gint          x1,
                      gint          y1,
                      gdouble       xfrac,
                      gdouble       yfrac,
                      guchar       *p,
                      const gfloat *kernel_lookup)
{
  gint   src_bpp = tile_manager_bpp  (srcTM);
  guchar p1[4];
  guchar p2[4];
  guchar p3[4];
  guchar p4[4];

  gint   b;
  gdouble sum, alphasum;

  for (b=0; b < src_bpp; b++)
    p[b]=0;

  read_pixel_data_1 (srcTM, x0, y0, p1);
  read_pixel_data_1 (srcTM, x1, y0, p2);
  read_pixel_data_1 (srcTM, x0, y1, p3);
  read_pixel_data_1 (srcTM, x1, y1, p4);

  switch (src_bpp)
    {
    case 1:
      sum = weighted_sum (xfrac, yfrac, p1[0], p2[0], p3[0], p4[0]);
      p[0] = (guchar) CLAMP (sum, 0, 255);
      break;

    case 2:
      alphasum = weighted_sum (xfrac, yfrac, p1[1], p2[1], p3[1], p4[1]);
      if (alphasum > 0)
        {
          sum = weighted_sum (xfrac, yfrac, p1[0] * p1[1], p2[0] * p2[1],
                              p3[0] * p3[1], p4[0] * p4[1]);
          sum /= alphasum;

          p[0] = (guchar) CLAMP (sum, 0, 255);
          p[1] = (guchar) CLAMP (alphasum, 0, 255);
        }
      break;

    case 3:
      for (b = 0; b < 3; b++)
        {
          sum = weighted_sum (xfrac, yfrac, p1[b], p2[b], p3[b], p4[b]);
          p[b] = (guchar) CLAMP (sum, 0, 255);
        }
      break;

    case 4:
      alphasum = weighted_sum (xfrac, yfrac, p1[3], p2[3], p3[3], p4[3]);
      if (alphasum > 0)
        {
          for (b = 0; b<3; b++)
            {
              sum = weighted_sum (xfrac, yfrac, p1[b] * p1[3], p2[b] * p2[3],
                                  p3[b] * p3[3], p4[b] * p4[3]);
              sum /= alphasum;
              p[b] = (guchar) CLAMP (sum, 0, 255);
            }

          p[3] = (guchar) CLAMP (alphasum, 0, 255);
        }
      break;
    }
}

/* Catmull-Rom spline - not bad
  * basic intro http://www.mvps.org/directx/articles/catmull/
  * This formula will calculate an interpolated point between pt1 and pt2
  * dx=0 returns pt1; dx=1 returns pt2
  */

static inline gdouble
cubic_spline_fit (gdouble dx,
                  gint    pt0,
                  gint    pt1,
                  gint    pt2,
                  gint    pt3)
{

  return (gdouble) ((( ( -pt0 + 3 * pt1 - 3 * pt2 + pt3 ) *   dx +
                       ( 2 * pt0 - 5 * pt1 + 4 * pt2 - pt3 ) ) * dx +
                     ( -pt0 + pt2 ) ) * dx + (pt1 + pt1) ) / 2.0;
}

static void
interpolate_cubic (TileManager  *srcTM,
                   gint          x1,
                   gint          y1,
                   gint          x2,
                   gint          y2,
                   gdouble       xfrac,
                   gdouble       yfrac,
                   guchar       *p,
                   const gfloat *kernel_lookup)
{
  gint    src_bpp    = tile_manager_bpp  (srcTM);
  guint   src_width  = tile_manager_width  (srcTM);
  guint   src_height = tile_manager_height (srcTM);
  gint    b, i;
  gint    x, y;
  gint    x0;
  gint    y0;

  guchar  ps[16 * 4];
  gdouble p0, p1, p2, p3;

  gdouble sum, alphasum;

  for (b = 0; b < src_bpp; b++)
    p[b] = 0;

  for (y = y1 - 1, i = 0; y <= y1 + 2; y++)
    for (x = x1 - 1; x <= x1 + 2; x++, i++)
      {
        x0 = (x < src_width)  ? ABS(x) : 2 * src_width  - x - 1;
        y0 = (y < src_height) ? ABS(y) : 2 * src_height - y - 1;
        read_pixel_data_1 (srcTM, x0, y0, ps + (i * src_bpp));
      }

  switch (src_bpp)
    {
    case 1:
      p0 = cubic_spline_fit (xfrac, ps[0 ], ps[1 ], ps[2 ], ps[3 ]);
      p1 = cubic_spline_fit (xfrac, ps[4 ], ps[5 ], ps[6 ], ps[7 ]);
      p2 = cubic_spline_fit (xfrac, ps[8 ], ps[9 ], ps[10], ps[11]);
      p3 = cubic_spline_fit (xfrac, ps[12], ps[13], ps[14], ps[15]);

      sum = cubic_spline_fit (yfrac, p0, p1, p2, p3);

      p[0]= (guchar) CLAMP (sum, 0, 255);
      break;

    case 2:
      p0 = cubic_spline_fit (xfrac, ps[1 ], ps[3 ], ps[5 ], ps[7 ]);
      p1 = cubic_spline_fit (xfrac, ps[9 ], ps[11], ps[13], ps[15]);
      p2 = cubic_spline_fit (xfrac, ps[17], ps[19], ps[21], ps[23]);
      p3 = cubic_spline_fit (xfrac, ps[25], ps[27], ps[29], ps[31]);

      alphasum = cubic_spline_fit (yfrac, p0, p1, p2, p3);

      if (alphasum > 0)
        {
          p0 = cubic_spline_fit (xfrac, ps[0 ] * ps[1 ], ps[2 ] * ps[3 ],
                                 ps[4 ] * ps[5 ], ps[6 ] * ps[7 ]);
          p1 = cubic_spline_fit (xfrac, ps[8 ] * ps[9 ], ps[10] * ps[11],
                                 ps[12] * ps[13], ps[14] * ps[15]);
          p2 = cubic_spline_fit (xfrac, ps[16] * ps[17], ps[18] * ps[19],
                                 ps[20] * ps[21], ps[22] * ps[23]);
          p3 = cubic_spline_fit (xfrac, ps[24] * ps[25], ps[26] * ps[27],
                                 ps[28] * ps[29], ps[30] * ps[31]);

          sum  = cubic_spline_fit (yfrac, p0, p1, p2, p3);
          sum /= alphasum;

          p[0] = (guchar) CLAMP (sum, 0, 255);
          p[1] = (guchar) CLAMP (alphasum, 0, 255);
        }
      break;
    case 3:
      for (b = 0; b < 3; b++)
        {
          p0 = cubic_spline_fit (xfrac, ps[   b], ps[3 + b], ps[6 + b], ps[9 + b]);
          p1 = cubic_spline_fit (xfrac, ps[12 + b], ps[15 + b], ps[18 + b], ps[21 + b]);
          p2 = cubic_spline_fit (xfrac, ps[24 + b], ps[27 + b], ps[30 + b], ps[33 + b]);
          p3 = cubic_spline_fit (xfrac, ps[36 + b], ps[39 + b], ps[42 + b], ps[45 + b]);

          sum = cubic_spline_fit (yfrac, p0, p1, p2, p3);

          p[b] = (guchar) CLAMP (sum, 0, 255);
        }
      break;

    case 4:
      p0 = cubic_spline_fit (xfrac, ps[3],  ps[7],  ps[11], ps[15]);
      p1 = cubic_spline_fit (xfrac, ps[19], ps[23], ps[27], ps[31]);
      p2 = cubic_spline_fit (xfrac, ps[35], ps[39], ps[43], ps[47]);
      p3 = cubic_spline_fit (xfrac, ps[51], ps[55], ps[59], ps[63]);

      alphasum = cubic_spline_fit (yfrac, p0, p1, p2, p3);

      if (alphasum > 0)
        {
          for (b = 0; b < 3; b++)
            {
              p0 = cubic_spline_fit (xfrac, ps[0 + b] * ps[3],  ps[4 + b] * ps[7],
                                     ps[8 + b] * ps[11], ps[12 + b] * ps[15]);
              p1 = cubic_spline_fit (xfrac, ps[16 + b] * ps[19], ps[20 + b] * ps[23],
                                     ps[24 + b] * ps[27], ps[28 + b] * ps[31]);
              p2 = cubic_spline_fit (xfrac, ps[32 + b] * ps[35], ps[36 + b] * ps[39],
                                     ps[40 + b] * ps[43], ps[44 + b] * ps[47]);
              p3 = cubic_spline_fit (xfrac, ps[48 + b] * ps[51], ps[52 + b] * ps[55],
                                     ps[56 + b] * ps[59], ps[60 + b] * ps[63]);

              sum  = cubic_spline_fit (yfrac, p0, p1, p2, p3);
              sum /= alphasum;

              p[b] = (guchar) CLAMP (sum, 0, 255);
            }

          p[3] = (guchar) CLAMP (alphasum, 0, 255);
        }
      break;
    }
}

static gdouble inline
lanczos3_mul_alpha (const guchar  *pixels,
                    const gdouble *x_kernel,
                    const gdouble *y_kernel,
                    const gint     bytes,
                    const gint     byte)
{
  const guchar *p     = pixels;
  const guchar  alpha = bytes - 1;
  gdouble       sum   = 0.0;
  gint          x, y;

  for (y = 0; y < 6; y++)
    {
      gdouble tmpsum = 0.0;

      for (x = 0; x < 6; x++, p += bytes)
        {
          tmpsum += x_kernel[x] * p[byte] * p[alpha];
        }

      tmpsum *= y_kernel[y];
      sum    += tmpsum;
    }

  return sum;
}

static gdouble inline
lanczos3_mul (const guchar  *pixels,
              const gdouble *x_kernel,
              const gdouble *y_kernel,
              const gint     bytes,
              const gint     byte)
{
  const guchar *p   = pixels;
  gdouble       sum = 0.0;
  gint          x, y;

  for (y = 0; y < 6; y++)
    {
      gdouble tmpsum = 0.0;

      for (x = 0; x < 6; x++, p += bytes)
        {
          tmpsum += x_kernel[x] * p[byte];
        }

      tmpsum *= y_kernel[y];
      sum    += tmpsum;
    }

  return sum;
}

static void
interpolate_lanczos3 (TileManager  *srcTM,
                      gint          x1,
                      gint          y1,
                      gint          x2,
                      gint          y2,
                      gdouble       xfrac,
                      gdouble       yfrac,
                      guchar       *pixel,
                      const gfloat *kernel_lookup)
{
  gint    src_bpp    = tile_manager_bpp  (srcTM);
  guint   src_width  = tile_manager_width  (srcTM);
  guint   src_height = tile_manager_height (srcTM);
  gint    b, i;
  gint    x, y;
  gint    x0;
  gint    y0;
  gint    x_shift, y_shift;
  gdouble kx_sum, ky_sum;
  gdouble x_kernel[6], y_kernel[6];
  guchar  pixels[36 * 4];
  gdouble sum, alphasum;

  for (b = 0; b < src_bpp; b++)
    pixel[b] = 0;

  for (y = y1 - 2, i = 0; y <= y1 + 3; y++)
    {
      for (x = x1 - 2; x <= x1 + 3; x++, i++)
        {
          x0 = (x < src_width)  ? ABS(x) : 2 * src_width  - x - 1;
          y0 = (y < src_height) ? ABS(y) : 2 * src_height - y - 1;
          read_pixel_data_1 (srcTM, x0, y0, pixels + (i * src_bpp));
        }
    }

  x_shift = (gint) (xfrac * LANCZOS_SPP + 0.5);
  y_shift = (gint) (yfrac * LANCZOS_SPP + 0.5);
  kx_sum  = ky_sum = 0.0;

  for (i = 3; i >= -2; i--)
    {
      gint pos = i * LANCZOS_SPP;

      kx_sum += x_kernel[2 + i] = kernel_lookup[ABS (x_shift - pos)];
      ky_sum += y_kernel[2 + i] = kernel_lookup[ABS (y_shift - pos)];
    }

  /* normalise the kernel arrays */
  for (i = -2; i <= 3; i++)
    {
      x_kernel[2 + i] /= kx_sum;
      y_kernel[2 + i] /= ky_sum;
    }

  switch (src_bpp)
    {
    case 1:
      sum      = lanczos3_mul (pixels, x_kernel, y_kernel, 1, 0);
      pixel[0] = (guchar) CLAMP ((gint) sum, 0, 255);
      break;

    case 2:
      alphasum  = lanczos3_mul (pixels, x_kernel, y_kernel, 2, 1);
      if (alphasum > 0)
        {
          sum       = lanczos3_mul_alpha (pixels, x_kernel, y_kernel, 2, 0);
          sum      /= alphasum;
          pixel[0]  = (guchar) CLAMP (sum, 0, 255);
          pixel[1]  = (guchar) CLAMP (alphasum, 0, 255);
        }
      break;

    case 3:
      for (b = 0; b < 3; b++)
        {
          sum      = lanczos3_mul (pixels, x_kernel, y_kernel, 3, b);
          pixel[b] = (guchar) CLAMP (sum, 0, 255);
        }
      break;

    case 4:
      alphasum = lanczos3_mul (pixels, x_kernel, y_kernel, 4, 3);
      if (alphasum > 0)
        {
          for (b = 0; b < 3; b++)
            {
              sum      = lanczos3_mul_alpha (pixels, x_kernel, y_kernel, 4, b);
              sum     /= alphasum;
              pixel[b] = (guchar) CLAMP (sum, 0, 255);
            }

          pixel[3] = (guchar) CLAMP (alphasum, 0, 255);
        }
      break;
    }
}

static void
scale_pr (PixelRegion           *srcPR,
          PixelRegion           *dstPR,
          GimpInterpolationType  interpolation)
{
  gdouble scalex     = (gdouble) dstPR->w / (gdouble) srcPR->w;
  gdouble scaley     = (gdouble) dstPR->h / (gdouble) srcPR->h;
  gint    src_width  = srcPR->w;
  gint    src_height = srcPR->h;
  gint    bytes      = srcPR->bytes;
  guchar *dstPtr     = dstPR->data;
  gdouble xfrac, yfrac;
  gint    b, x, sx0, sx1, y, sy0, sy1;
  guchar  pixel[bytes];

  for (y = 0; y < dstPR->h; y++)
    {
      yfrac = (y / scaley);
      sy0   = (gint) yfrac;
      sy1   = sy0 + 1;
      sy1   = (sy1 < src_height) ? ABS(sy1) : 2 * src_height - sy1 - 1;

      yfrac =  yfrac - sy0;

      for (x = 0; x < dstPR->w; x++)
        {
          xfrac = (x / scalex);
          sx0   = (gint) xfrac;
          sx1   = sx0 + 1;
          sx1   = (sx1 < src_width) ? ABS(sx1) : 2 * src_width - sx1 - 1;
          xfrac =  xfrac - sx0;

          switch (interpolation)
            {
            case GIMP_INTERPOLATION_NONE:
            case GIMP_INTERPOLATION_LINEAR:
            case GIMP_INTERPOLATION_CUBIC:
            case GIMP_INTERPOLATION_LANCZOS:
              if (scalex == 0.5 || scaley == 0.5)
                {
                  decimate_average_pr (srcPR,
                                       sx0, sy0,
                                       sx1, sy1,
                                       pixel);
                }
              else
                {
                  interpolate_bilinear_pr (srcPR,
                                           sx0, sy0,
                                           sx1, sy1,
                                           xfrac, yfrac,
                                           pixel);
                }
              break;
            }

          for (b = 0; b < bytes; b++, dstPtr++)
            *dstPtr = pixel[b];
        }
    }
}

static void
decimate_average_pr (PixelRegion *srcPR,
                     gint         x0,
                     gint         y0,
                     gint         x1,
                     gint         y1,
                     guchar      *p)
{
  gint    bytes = srcPR->bytes;
  gint    width = srcPR->w;
  guchar *p1    = srcPR->data + (y0 * width + x0) * bytes;
  guchar *p2    = srcPR->data + (y0 * width + x1) * bytes;
  guchar *p3    = srcPR->data + (y1 * width + x0) * bytes;
  guchar *p4    = srcPR->data + (y1 * width + x1) * bytes;

  pixel_average (p1, p2, p3, p4, p, bytes);
}

static void
interpolate_bilinear_pr (PixelRegion *srcPR,
                         gint         x0,
                         gint         y0,
                         gint         x1,
                         gint         y1,
                         gdouble      xfrac,
                         gdouble      yfrac,
                         guchar      *p)
{
  gint    bytes = srcPR->bytes;
  gint    width = srcPR->w;
  guchar *p1    = srcPR->data + (y0 * width + x0) * bytes;
  guchar *p2    = srcPR->data + (y0 * width + x1) * bytes;
  guchar *p3    = srcPR->data + (y1 * width + x0) * bytes;
  guchar *p4    = srcPR->data + (y1 * width + x1) * bytes;

  gint    b;
  gdouble sum, alphasum;

  for (b = 0; b < bytes; b++)
    p[b] = 0;

  switch (bytes)
    {
    case 1:
      sum      = weighted_sum (xfrac, yfrac, p1[0], p2[0], p3[0], p4[0]);
      p[0]     = (guchar) CLAMP (sum, 0, 255);
      break;

    case 2:
      alphasum = weighted_sum (xfrac, yfrac, p1[1], p2[1], p3[1], p4[1]);
      if (alphasum > 0)
        {
          sum  = weighted_sum (xfrac, yfrac, p1[0] * p1[1], p2[0] * p2[1],
                               p3[0] * p3[1], p4[0] * p4[1]);
          sum /= alphasum;
          p[0] = (guchar) CLAMP (sum, 0, 255);
          p[1] = (guchar) CLAMP (alphasum, 0, 255);
        }
      break;

    case 3:
      for (b = 0; b < 3; b++)
        {
          sum  = weighted_sum (xfrac, yfrac, p1[b], p2[b], p3[b], p4[b]);
          p[b] = (guchar) CLAMP (sum, 0, 255);
        }
      break;

    case 4:
      alphasum = weighted_sum (xfrac, yfrac, p1[3], p2[3], p3[3], p4[3]);
      if (alphasum > 0)
        {
          for (b = 0; b < 3; b++)
            {
              sum  = weighted_sum (xfrac, yfrac, p1[b] * p1[3], p2[b] * p2[3],
                                   p3[b] * p3[3], p4[b] * p4[3]);
              sum /= alphasum;
              p[b] = (guchar) CLAMP (sum, 0, 255);
            }

          p[3] = (guchar) CLAMP (alphasum, 0, 255);
        }
      break;
    }
}
