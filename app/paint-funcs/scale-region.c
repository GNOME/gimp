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

#include <string.h>

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "paint-funcs-types.h"

#include "base/tile.h"
#include "base/tile-manager.h"
#include "base/pixel-region.h"
#include "base/pixel-surround.h"

#include "paint-funcs.h"
#include "scale-region.h"

#include "gimp-log.h"


#define NUM_TILES(w,h) ((((w) + (TILE_WIDTH - 1)) / TILE_WIDTH) *  \
                        (((h) + (TILE_HEIGHT - 1)) / TILE_HEIGHT))


static void           scale_determine_levels   (PixelRegion           *srcPR,
                                                PixelRegion           *dstPR,
                                                gint                  *levelx,
                                                gint                  *levely);
static gint           scale_determine_progress (PixelRegion           *srcPR,
                                                PixelRegion           *dstPR,
                                                gint                   levelx,
                                                gint                   levely);

static void           scale_region_buffer      (PixelRegion           *srcPR,
                                                PixelRegion           *dstPR);
static void           scale_region_tile        (PixelRegion           *srcPR,
                                                PixelRegion           *dstPR,
                                                GimpInterpolationType  interpolation,
                                                GimpProgressFunc       progress_callback,
                                                gpointer               progress_data);
static void           scale                    (TileManager           *srcTM,
                                                TileManager           *dstTM,
                                                GimpInterpolationType  interpolation,
                                                GimpProgressFunc       progress_callback,
                                                gpointer               progress_data,
                                                gint                  *progress,
                                                gint                   max_progress,
                                                const gdouble          scalex,
                                                const gdouble          scaley);
static void           decimate_xy              (TileManager           *srcTM,
                                                TileManager           *dstTM,
                                                GimpInterpolationType  interpolation,
                                                GimpProgressFunc       progress_callback,
                                                gpointer               progress_data,
                                                gint                  *progress,
                                                gint                   max_progress);
static void           decimate_x               (TileManager           *srcTM,
                                                TileManager           *dstTM,
                                                GimpInterpolationType  interpolation,
                                                GimpProgressFunc       progress_callback,
                                                gpointer               progress_data,
                                                gint                  *progress,
                                                gint                   max_progress);
static void           decimate_y               (TileManager           *srcTM,
                                                TileManager           *dstTM,
                                                GimpInterpolationType  interpolation,
                                                GimpProgressFunc       progress_callback,
                                                gpointer               progress_data,
                                                gint                  *progress,
                                                gint                   max_progress);
static void           decimate_average_xy      (PixelSurround *surround,
                                                const gint     x0,
                                                const gint     y0,
                                                const gint     bytes,
                                                guchar        *pixel);
static void           decimate_average_y       (PixelSurround *surround,
                                                const gint     x0,
                                                const gint     y0,
                                                const gint     bytes,
                                                guchar        *pixel);
static void           decimate_average_x       (PixelSurround *surround,
                                                const gint     x0,
                                                const gint     y0,
                                                const gint     bytes,
                                                guchar        *pixel);
static void           interpolate_nearest      (TileManager   *srcTM,
                                                const gint     x0,
                                                const gint     y0,
                                                const gdouble  xfrac,
                                                const gdouble  yfrac,
                                                guchar        *pixel);
static void           interpolate_bilinear     (PixelSurround *surround,
                                                const gint     x0,
                                                const gint     y0,
                                                const gdouble  xfrac,
                                                const gdouble  yfrac,
                                                const gint     bytes,
                                                guchar        *pixel);
static void           interpolate_cubic        (PixelSurround *surround,
                                                const gint     x0,
                                                const gint     y0,
                                                const gdouble  xfrac,
                                                const gdouble  yfrac,
                                                const gint     bytes,
                                                guchar        *pixel);
static gfloat *       create_lanczos3_lookup   (void);
static void           interpolate_lanczos3     (PixelSurround *surround,
                                                const gint     x0,
                                                const gint     y0,
                                                const gdouble  xfrac,
                                                const gdouble  yfrac,
                                                const gint     bytes,
                                                guchar        *pixel,
                                                const gfloat  *kernel_lookup);
static void           interpolate_bilinear_pr  (PixelRegion   *srcPR,
                                                const gint     x0,
                                                const gint     y0,
                                                const gint     x1,
                                                const gint     y1,
                                                const gdouble  xfrac,
                                                const gdouble  yfrac,
                                                guchar        *pixel);
static inline gdouble cubic_spline_fit         (const gdouble  dx,
                                                const gdouble  x1,
                                                const gdouble  y1,
                                                const gdouble  x2,
                                                const gdouble  y2);
static inline gdouble weighted_sum             (const gdouble  dx,
                                                const gdouble  dy,
                                                const gint     s00,
                                                const gint     s10,
                                                const gint     s01,
                                                const gint     s11);
static inline gdouble sinc                     (const gdouble  x);
static inline gdouble lanczos3_mul_alpha       (const guchar  *pixels,
                                                const gdouble *x_kernel,
                                                const gdouble *y_kernel,
                                                const gint     stride,
                                                const gint     bytes,
                                                const gint     byte);
static inline gdouble lanczos3_mul             (const guchar  *pixels,
                                                const gdouble *x_kernel,
                                                const gdouble *y_kernel,
                                                const gint     stride,
                                                const gint     bytes,
                                                const gint     byte);



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
      g_return_if_fail (interpolation == GIMP_INTERPOLATION_LINEAR);
      g_return_if_fail (progress_callback == NULL);

      scale_region_buffer (srcPR, dstPR);
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
scale_determine_levels (PixelRegion *srcPR,
                        PixelRegion *dstPR,
                        gint        *levelx,
                        gint        *levely)
{
  gdouble scalex = (gdouble) dstPR->w / (gdouble) srcPR->w;
  gdouble scaley = (gdouble) dstPR->h / (gdouble) srcPR->h;
  gint    width  = srcPR->w;
  gint    height = srcPR->h;

  /* downscaling is done in multiple steps */

  while (scalex < 0.5 && width > 1)
    {
      scalex  *= 2;
      width    = (width + 1) / 2;
      *levelx += 1;
    }

  while (scaley < 0.5 && height > 1)
    {
      scaley  *= 2;
      height   = (height + 1) / 2;
      *levely += 1;
    }
}

/* This function calculates the number of tiles that are written in
 * one scale operation. This number is used as the max_progress
 * parameter in calls to GimpProgressFunc.
 */
static gint
scale_determine_progress (PixelRegion *srcPR,
                          PixelRegion *dstPR,
                          gint         levelx,
                          gint         levely)
{
  gint width  = srcPR->w;
  gint height = srcPR->h;
  gint tiles  = 0;

  /*  The logic here should be kept in sync with scale_region_tile().  */

  while (levelx > 0 && levely > 0)
    {
      width  = (width + 1) >> 1;
      height = (height + 1) >> 1;
      levelx--;
      levely--;

      tiles += NUM_TILES (width, height);
    }

  while (levelx > 0)
    {
      width  = (width + 1) >> 1;
      levelx--;

      tiles += NUM_TILES (width, height);
    }

  while (levely > 0)
    {
      height = (height + 1) >> 1;
      levely--;

      tiles += NUM_TILES (width, height);
    }

  tiles += NUM_TILES (dstPR->w, dstPR->h);

  return tiles;
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
  gint         max_progress = 0;
  gint         progress     = 0;
  gint         levelx       = 0;
  gint         levely       = 0;
  gdouble      scalex       = (gdouble) width / dstPR->w;
  gdouble      scaley       = (gdouble) height / dstPR->h;

  /* determine scaling levels */
  if (interpolation != GIMP_INTERPOLATION_NONE)
    {
      scale_determine_levels (srcPR, dstPR, &levelx, &levely);
    }

  max_progress = scale_determine_progress (srcPR, dstPR, levelx, levely);

  if (levelx == 0 && levely == 0)
    {
      scale (srcTM, dstTM, interpolation,
             progress_callback, progress_data, &progress, max_progress,
             scalex, scaley);
    }

  while (levelx > 0 && levely > 0)
    {
      width  = (width + 1) >> 1;
      height = (height + 1) >> 1;
      scalex *= .5;
      scaley *= .5;

      tmpTM = tile_manager_new (width, height, bytes);
      decimate_xy (srcTM, tmpTM, interpolation,
                   progress_callback, progress_data, &progress, max_progress);

      if (srcTM != srcPR->tiles)
        tile_manager_unref (srcTM);

      srcTM = tmpTM;
      levelx--;
      levely--;
    }

  while (levelx > 0)
    {
      width = (width + 1) >> 1;
      scalex *= .5;

      tmpTM = tile_manager_new (width, height, bytes);
      decimate_x (srcTM, tmpTM, interpolation,
                  progress_callback, progress_data, &progress, max_progress);

      if (srcTM != srcPR->tiles)
        tile_manager_unref (srcTM);

      srcTM = tmpTM;
      levelx--;
    }

  while (levely > 0)
    {
      height = (height + 1) >> 1;
      scaley *= .5;

      tmpTM = tile_manager_new (width, height, bytes);
      decimate_y (srcTM, tmpTM, interpolation,
                  progress_callback, progress_data, &progress, max_progress);

      if (srcTM != srcPR->tiles)
        tile_manager_unref (srcTM);

      srcTM = tmpTM;
      levely--;
    }

  if (tmpTM != NULL)
    {
      scale (tmpTM, dstTM, interpolation,
             progress_callback, progress_data, &progress, max_progress,
             scalex, scaley);
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
       gint                   max_progress,
       const gdouble          scalex,
       const gdouble          scaley)
{
  PixelRegion     region;
  PixelSurround  *surround   = NULL;
  const guint     src_width  = tile_manager_width  (srcTM);
  const guint     src_height = tile_manager_height (srcTM);
  const guint     bytes      = tile_manager_bpp    (dstTM);
  const guint     dst_width  = tile_manager_width  (dstTM);
  const guint     dst_height = tile_manager_height (dstTM);
  gpointer        pr;
  gfloat         *kernel_lookup = NULL;

  GIMP_LOG (SCALE, "scale: %dx%d -> %dx%d",
            src_width, src_height, dst_width, dst_height);

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

  switch (interpolation)
    {
    case GIMP_INTERPOLATION_NONE:
      break;

    case GIMP_INTERPOLATION_LINEAR:
      surround = pixel_surround_new (srcTM, 2, 2, PIXEL_SURROUND_SMEAR);
      break;

    case GIMP_INTERPOLATION_CUBIC:
      surround = pixel_surround_new (srcTM, 4, 4, PIXEL_SURROUND_SMEAR);
      break;

    case GIMP_INTERPOLATION_LANCZOS:
      surround = pixel_surround_new (srcTM, 6, 6, PIXEL_SURROUND_SMEAR);
      kernel_lookup = create_lanczos3_lookup ();
      break;
    }

  pixel_region_init (&region, dstTM, 0, 0, dst_width, dst_height, TRUE);

  for (pr = pixel_regions_register (1, &region);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const gint  x1  = region.x + region.w;
      const gint  y1  = region.y + region.h;
      guchar     *row = region.data;
      gint        y;

      for (y = region.y; y < y1; y++)
        {
          guchar  *pixel = row;
          gdouble  yfrac = (y + 0.5) * scaley - 0.5;
          gint     sy    = floor (yfrac);
          gint     x;

          yfrac = yfrac - sy;

          for (x = region.x; x < x1; x++)
            {
              gdouble xfrac = (x + 0.5) * scalex - 0.5;
              gint    sx    = floor (xfrac);

              xfrac = xfrac - sx;

              switch (interpolation)
                {
                case GIMP_INTERPOLATION_NONE:
                  interpolate_nearest (srcTM, sx, sy, xfrac, yfrac, pixel);
                  break;

                case GIMP_INTERPOLATION_LINEAR:
                  interpolate_bilinear (surround,
                                        sx, sy, xfrac, yfrac, bytes, pixel);
                  break;

                case GIMP_INTERPOLATION_CUBIC:
                  interpolate_cubic (surround,
                                     sx, sy, xfrac, yfrac, bytes, pixel);
                  break;

                case GIMP_INTERPOLATION_LANCZOS:
                  interpolate_lanczos3 (surround,
                                        sx, sy, xfrac, yfrac, bytes, pixel,
                                        kernel_lookup);
                  break;
                }

              pixel += region.bytes;
            }

          row += region.rowstride;
        }

      if (progress_callback)
        {
          (*progress)++;

          if (*progress % 8 == 0)
            progress_callback (0, max_progress, *progress, progress_data);
        }
    }

  if (kernel_lookup)
    g_free (kernel_lookup);

  if (surround)
    pixel_surround_destroy (surround);
}

static void
decimate_xy (TileManager           *srcTM,
             TileManager           *dstTM,
             GimpInterpolationType  interpolation,
             GimpProgressFunc       progress_callback,
             gpointer               progress_data,
             gint                  *progress,
             gint                   max_progress)
{
  PixelRegion     region;
  PixelSurround  *surround   = NULL;
  const guint     bytes      = tile_manager_bpp    (dstTM);
  const guint     dst_width  = tile_manager_width  (dstTM);
  const guint     dst_height = tile_manager_height (dstTM);
  gpointer        pr;

  GIMP_LOG (SCALE, "decimate_xy: %dx%d -> %dx%d\n",
            tile_manager_width (srcTM), tile_manager_height (srcTM),
            dst_width, dst_height);

  surround = pixel_surround_new (srcTM, 2, 2, PIXEL_SURROUND_SMEAR);

  pixel_region_init (&region, dstTM, 0, 0, dst_width, dst_height, TRUE);

  for (pr = pixel_regions_register (1, &region);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const gint  x1  = region.x + region.w;
      const gint  y1  = region.y + region.h;
      guchar     *row = region.data;
      gint        y;

      for (y = region.y; y < y1; y++)
        {
          const gint  sy    = y * 2;
          guchar     *pixel = row;
          gint        x;

          for (x = region.x; x < x1; x++)
            {
              decimate_average_xy (surround, x * 2, sy, bytes, pixel);

              pixel += region.bytes;
            }

          row += region.rowstride;
        }

      if (progress_callback)
        {
          (*progress)++;

          if (*progress % 16 == 0)
            progress_callback (0, max_progress, *progress, progress_data);
        }
    }

  pixel_surround_destroy (surround);
}

static void
decimate_x (TileManager           *srcTM,
            TileManager           *dstTM,
            GimpInterpolationType  interpolation,
            GimpProgressFunc       progress_callback,
            gpointer               progress_data,
            gint                  *progress,
            gint                   max_progress)
{
  PixelRegion     region;
  PixelSurround  *surround   = NULL;
  const guint     bytes      = tile_manager_bpp    (dstTM);
  const guint     dst_width  = tile_manager_width  (dstTM);
  const guint     dst_height = tile_manager_height (dstTM);
  gpointer        pr;

  GIMP_LOG (SCALE, "decimate_x: %dx%d -> %dx%d\n",
            tile_manager_width (srcTM), tile_manager_height (srcTM),
            dst_width, dst_height);

  surround = pixel_surround_new (srcTM, 2, 1, PIXEL_SURROUND_SMEAR);

  pixel_region_init (&region, dstTM, 0, 0, dst_width, dst_height, TRUE);

  for (pr = pixel_regions_register (1, &region);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const gint  x1  = region.x + region.w;
      const gint  y1  = region.y + region.h;
      guchar     *row = region.data;
      gint        y;

      for (y = region.y; y < y1; y++)
        {
          guchar *pixel = row;
          gint    x;

          for (x = region.x; x < x1; x++)
            {
              decimate_average_x (surround, x * 2, y, bytes, pixel);

              pixel += region.bytes;
            }

          row += region.rowstride;
        }

      if (progress_callback)
        {
          (*progress)++;

          if (*progress % 32 == 0)
            progress_callback (0, max_progress, *progress, progress_data);
        }
    }

  pixel_surround_destroy (surround);
}

static void
decimate_y (TileManager           *srcTM,
            TileManager           *dstTM,
            GimpInterpolationType  interpolation,
            GimpProgressFunc       progress_callback,
            gpointer               progress_data,
            gint                  *progress,
            gint                   max_progress)
{
  PixelRegion     region;
  PixelSurround  *surround   = NULL;
  const guint     bytes      = tile_manager_bpp    (dstTM);
  const guint     dst_width  = tile_manager_width  (dstTM);
  const guint     dst_height = tile_manager_height (dstTM);
  gpointer        pr;

  GIMP_LOG (SCALE, "decimate_y: %dx%d -> %dx%d\n",
            tile_manager_width (srcTM), tile_manager_height (srcTM),
            dst_width, dst_height);

  surround = pixel_surround_new (srcTM, 1, 2, PIXEL_SURROUND_SMEAR);

  pixel_region_init (&region, dstTM, 0, 0, dst_width, dst_height, TRUE);

  for (pr = pixel_regions_register (1, &region);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const gint  x1  = region.x + region.w;
      const gint  y1  = region.y + region.h;
      guchar     *row = region.data;
      gint        y;

      for (y = region.y; y < y1; y++)
        {
          const gint  sy    = y * 2;
          guchar     *pixel = row;
          gint        x;

          for (x = region.x; x < x1; x++)
            {
              decimate_average_y (surround, x, sy, bytes, pixel);

              pixel += region.bytes;
            }

          row += region.rowstride;
        }

      if (progress_callback)
        {
          (*progress)++;

          if (*progress % 32 == 0)
            progress_callback (0, max_progress, *progress, progress_data);
        }
    }

  pixel_surround_destroy (surround);
}

static void inline
pixel_average4 (const guchar *p1,
                const guchar *p2,
                const guchar *p3,
                const guchar *p4,
                guchar       *p,
                const gint    bytes)
{
  switch (bytes)
    {
    case 1:
      p[0] = (p1[0] + p2[0] + p3[0] + p4[0] + 2) >> 2;
      break;

    case 2:
      {
        guint a = p1[1] + p2[1] + p3[1] + p4[1];

        switch (a)
          {
          case 0:    /* all transparent */
            p[0] = p[1] = 0;
            break;

          case 1020: /* all opaque */
            p[0] = (p1[0]  + p2[0] + p3[0] + p4[0] + 2) >> 2;
            p[1] = 255;
            break;

          default:
            p[0] = ((p1[0] * p1[1] +
                     p2[0] * p2[1] +
                     p3[0] * p3[1] +
                     p4[0] * p4[1] + (a >> 1)) / a);
            p[1] = (a + 2) >> 2;
            break;
          }
      }
      break;

    case 3:
      p[0] = (p1[0] + p2[0] + p3[0] + p4[0] + 2) >> 2;
      p[1] = (p1[1] + p2[1] + p3[1] + p4[1] + 2) >> 2;
      p[2] = (p1[2] + p2[2] + p3[2] + p4[2] + 2) >> 2;
      break;

    case 4:
      {
        guint a = p1[3] + p2[3] + p3[3] + p4[3];

        switch (a)
          {
          case 0:    /* all transparent */
            p[0] = p[1] = p[2] = p[3] = 0;
            break;

          case 1020: /* all opaque */
            p[0] = (p1[0] + p2[0] + p3[0] + p4[0] + 2) >> 2;
            p[1] = (p1[1] + p2[1] + p3[1] + p4[1] + 2) >> 2;
            p[2] = (p1[2] + p2[2] + p3[2] + p4[2] + 2) >> 2;
            p[3] = 255;
            break;

          default:
            p[0] = ((p1[0] * p1[3] +
                     p2[0] * p2[3] +
                     p3[0] * p3[3] +
                     p4[0] * p4[3] + (a >> 1)) / a);
            p[1] = ((p1[1] * p1[3] +
                     p2[1] * p2[3] +
                     p3[1] * p3[3] +
                     p4[1] * p4[3] + (a >> 1)) / a);
            p[2] = ((p1[2] * p1[3] +
                     p2[2] * p2[3] +
                     p3[2] * p3[3] +
                     p4[2] * p4[3] + (a >> 1)) / a);
            p[3] = (a + 2) >> 2;
            break;
          }
      }
      break;
    }
}

static void inline
pixel_average2 (const guchar *p1,
                const guchar *p2,
                guchar       *p,
                const gint    bytes)
{
  switch (bytes)
    {
    case 1:
      p[0] = (p1[0] + p2[0] + 1) >> 1;
      break;

    case 2:
      {
        guint a = p1[1] + p2[1];

        switch (a)
          {
          case 0:    /* all transparent */
            p[0] = p[1] = 0;
            break;

          case 510: /* all opaque */
            p[0] = (p1[0]  + p2[0] + 1) >> 1;
            p[1] = 255;
            break;

          default:
            p[0] = ((p1[0] * p1[1] +
                     p2[0] * p2[1] + (a >> 1)) / a);
            p[1] = (a + 1) >> 1;
            break;
          }
      }
      break;

    case 3:
      p[0] = (p1[0] + p2[0] + 1) >> 1;
      p[1] = (p1[1] + p2[1] + 1) >> 1;
      p[2] = (p1[2] + p2[2] + 1) >> 1;
      break;

    case 4:
      {
        guint a = p1[3] + p2[3];

        switch (a)
          {
          case 0:    /* all transparent */
            p[0] = p[1] = p[2] = p[3] = 0;
            break;

          case 510: /* all opaque */
            p[0] = (p1[0] + p2[0] + 1) >> 1;
            p[1] = (p1[1] + p2[1] + 1) >> 1;
            p[2] = (p1[2] + p2[2] + 1) >> 1;
            p[3] = 255;
            break;

          default:
            p[0] = ((p1[0] * p1[3] +
                     p2[0] * p2[3] + (a >> 1)) / a);
            p[1] = ((p1[1] * p1[3] +
                     p2[1] * p2[3] + (a >> 1)) / a);
            p[2] = ((p1[2] * p1[3] +
                     p2[2] * p2[3] + (a >> 1)) / a);
            p[3] = (a + 1) >> 1;
            break;
          }
      }
      break;
    }
}

static void
decimate_average_xy (PixelSurround *surround,
                     const gint     x0,
                     const gint     y0,
                     const gint     bytes,
                     guchar        *pixel)
{
  gint          stride;
  const guchar *src = pixel_surround_lock (surround, x0, y0, &stride);

  pixel_average4 (src, src + bytes, src + stride, src + stride + bytes,
                  pixel, bytes);
}

static void
decimate_average_x (PixelSurround *surround,
                    const gint     x0,
                    const gint     y0,
                    const gint     bytes,
                    guchar        *pixel)
{
  gint          stride;
  const guchar *src = pixel_surround_lock (surround, x0, y0, &stride);

  pixel_average2 (src, src + bytes, pixel, bytes);
}

static void
decimate_average_y (PixelSurround *surround,
                    const gint     x0,
                    const gint     y0,
                    const gint     bytes,
                    guchar        *pixel)
{
  gint          stride;
  const guchar *src = pixel_surround_lock (surround, x0, y0, &stride);

  pixel_average2 (src, src + stride, pixel, bytes);
}

static inline gdouble
sinc (const gdouble x)
{
  gdouble y = x * G_PI;

  if (ABS (x) < LANCZOS_MIN)
    return 1.0;

  return sin (y) / y;
}

/*
 * allocate and fill lookup table of Lanczos windowed sinc function
 * use gfloat since errors due to granularity of array far exceed
 * data precision
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
interpolate_nearest (TileManager   *srcTM,
                     const gint     x0,
                     const gint     y0,
                     const gdouble  xfrac,
                     const gdouble  yfrac,
                     guchar        *pixel)
{
  const gint w = tile_manager_width (srcTM) - 1;
  const gint h = tile_manager_height (srcTM) - 1;
  const gint x = (xfrac <= 0.5) ? x0 : x0 + 1;
  const gint y = (yfrac <= 0.5) ? y0 : y0 + 1;

  tile_manager_read_pixel_data_1 (srcTM, CLAMP (x, 0, w), CLAMP (y, 0, h),
                                  pixel);
}

static inline gdouble
weighted_sum (const gdouble dx,
              const gdouble dy,
              const gint    s00,
              const gint    s10,
              const gint    s01,
              const gint    s11)
{
  return ((1 - dy) *
          ((1 - dx) * s00 + dx * s10) + dy * ((1 - dx) * s01 + dx * s11));
}

static void
interpolate_bilinear (PixelSurround *surround,
                      const gint     x0,
                      const gint     y0,
                      const gdouble  xfrac,
                      const gdouble  yfrac,
                      const gint     bytes,
                      guchar        *pixel)
{
  gint          stride;
  const guchar *src = pixel_surround_lock (surround, x0, y0, &stride);
  const guchar *p1  = src;
  const guchar *p2  = p1 + bytes;
  const guchar *p3  = src + stride;
  const guchar *p4  = p3 + bytes;
  gdouble       sum;
  gdouble       alphasum;
  gint          b;

  switch (bytes)
    {
    case 1:
      sum = RINT (weighted_sum (xfrac, yfrac, p1[0], p2[0], p3[0], p4[0]));

      pixel[0] = CLAMP (sum, 0, 255);
      break;

    case 2:
      alphasum = weighted_sum (xfrac, yfrac, p1[1], p2[1], p3[1], p4[1]);
      if (alphasum > 0)
        {
          sum = weighted_sum (xfrac, yfrac,
                              p1[0] * p1[1], p2[0] * p2[1],
                              p3[0] * p3[1], p4[0] * p4[1]);
          sum = RINT (sum / alphasum);
          alphasum = RINT (alphasum);

          pixel[0] = CLAMP (sum, 0, 255);
          pixel[1] = CLAMP (alphasum, 0, 255);
        }
      else
        {
          pixel[0] = pixel[1] = 0;
        }
      break;

    case 3:
      for (b = 0; b < 3; b++)
        {
          sum = RINT (weighted_sum (xfrac, yfrac, p1[b], p2[b], p3[b], p4[b]));

          pixel[b] = CLAMP (sum, 0, 255);
        }
      break;

    case 4:
      alphasum = weighted_sum (xfrac, yfrac, p1[3], p2[3], p3[3], p4[3]);
      if (alphasum > 0)
        {
          for (b = 0; b < 3; b++)
            {
              sum = weighted_sum (xfrac, yfrac,
                                  p1[b] * p1[3], p2[b] * p2[3],
                                  p3[b] * p3[3], p4[b] * p4[3]);
              sum = RINT (sum / alphasum);

              pixel[b] = CLAMP (sum, 0, 255);
            }

          alphasum = RINT (alphasum);
          pixel[3] = CLAMP (alphasum, 0, 255);
        }
      else
        {
          pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
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
cubic_spline_fit (const gdouble  dx,
                  const gdouble  pt0,
                  const gdouble  pt1,
                  const gdouble  pt2,
                  const gdouble  pt3)
{
  return (gdouble) ((( ( -pt0 + 3 * pt1 - 3 * pt2 + pt3 ) *   dx +
                       ( 2 * pt0 - 5 * pt1 + 4 * pt2 - pt3 ) ) * dx +
                     ( -pt0 + pt2 ) ) * dx + (pt1 + pt1) ) / 2.0;
}

static void
interpolate_cubic (PixelSurround *surround,
                   const gint     x0,
                   const gint     y0,
                   const gdouble  xfrac,
                   const gdouble  yfrac,
                   const gint     bytes,
                   guchar        *pixel)
{
  gint          stride;
  const guchar *src = pixel_surround_lock (surround, x0 - 1, y0 - 1, &stride);
  const guchar *s0  = src;
  const guchar *s1  = s0 + stride;
  const guchar *s2  = s1 + stride;
  const guchar *s3  = s2 + stride;
  gint          b;
  gdouble       p0, p1, p2, p3;
  gdouble       sum, alphasum;

  switch (bytes)
    {
    case 1:
      p0 = cubic_spline_fit (xfrac, s0[0], s0[1], s0[2], s0[3]);
      p1 = cubic_spline_fit (xfrac, s1[0], s1[1], s1[2], s1[3]);
      p2 = cubic_spline_fit (xfrac, s2[0], s2[1], s2[2], s2[3]);
      p3 = cubic_spline_fit (xfrac, s3[0], s3[1], s3[2], s3[3]);

      sum = RINT (cubic_spline_fit (yfrac, p0, p1, p2, p3));

      pixel[0]= CLAMP (sum, 0, 255);
      break;

    case 2:
      p0 = cubic_spline_fit (xfrac, s0[1], s0[3], s0[5], s0[7]);
      p1 = cubic_spline_fit (xfrac, s1[1], s1[3], s1[5], s1[7]);
      p2 = cubic_spline_fit (xfrac, s2[1], s2[3], s2[5], s2[7]);
      p3 = cubic_spline_fit (xfrac, s3[1], s3[3], s3[5], s3[7]);

      alphasum = cubic_spline_fit (yfrac, p0, p1, p2, p3);

      if (alphasum > 0)
        {
          p0 = cubic_spline_fit (xfrac,
                                 s0[0] * s0[1], s0[2] * s0[3],
                                 s0[4] * s0[5], s0[6] * s0[7]);
          p1 = cubic_spline_fit (xfrac,
                                 s1[0] * s1[1], s1[2] * s1[3],
                                 s1[4] * s1[5], s1[6] * s1[7]);
          p2 = cubic_spline_fit (xfrac,
                                 s2[0] * s2[1], s2[2] * s2[3],
                                 s2[4] * s2[5], s2[6] * s2[7]);
          p3 = cubic_spline_fit (xfrac,
                                 s3[0] * s3[1], s3[2] * s3[3],
                                 s3[4] * s3[5], s3[6] * s3[7]);

          sum = cubic_spline_fit (yfrac, p0, p1, p2, p3);
          sum = RINT (sum / alphasum);
          pixel[0] = CLAMP (sum, 0, 255);

          alphasum = RINT (alphasum);
          pixel[1] = CLAMP (alphasum, 0, 255);
        }
      else
        {
          pixel[0] = pixel[1] = 0;
        }
      break;

    case 3:
      for (b = 0; b < 3; b++)
        {
          p0 = cubic_spline_fit (xfrac, s0[b], s0[3 + b], s0[6 + b], s0[9 + b]);
          p1 = cubic_spline_fit (xfrac, s1[b], s1[3 + b], s1[6 + b], s1[9 + b]);
          p2 = cubic_spline_fit (xfrac, s2[b], s2[3 + b], s2[6 + b], s2[9 + b]);
          p3 = cubic_spline_fit (xfrac, s3[b], s3[3 + b], s3[6 + b], s3[9 + b]);

          sum = RINT (cubic_spline_fit (yfrac, p0, p1, p2, p3));

          pixel[b] = CLAMP (sum, 0, 255);
        }
      break;

    case 4:
      p0 = cubic_spline_fit (xfrac, s0[3], s0[7], s0[11], s0[15]);
      p1 = cubic_spline_fit (xfrac, s1[3], s1[7], s1[11], s1[15]);
      p2 = cubic_spline_fit (xfrac, s2[3], s2[7], s2[11], s2[15]);
      p3 = cubic_spline_fit (xfrac, s3[3], s3[7], s3[11], s3[15]);

      alphasum = cubic_spline_fit (yfrac, p0, p1, p2, p3);

      if (alphasum > 0)
        {
          for (b = 0; b < 3; b++)
            {
              p0 = cubic_spline_fit (xfrac,
                                     s0[0 + b] * s0[ 3], s0[ 4 + b] * s0[7],
                                     s0[8 + b] * s0[11], s0[12 + b] * s0[15]);
              p1 = cubic_spline_fit (xfrac,
                                     s1[0 + b] * s1[ 3], s1[ 4 + b] * s1[7],
                                     s1[8 + b] * s1[11], s1[12 + b] * s1[15]);
              p2 = cubic_spline_fit (xfrac,
                                     s2[0 + b] * s2[ 3], s2[ 4 + b] * s2[7],
                                     s2[8 + b] * s2[11], s2[12 + b] * s2[15]);
              p3 = cubic_spline_fit (xfrac,
                                     s3[0 + b] * s3[ 3], s3[ 4 + b] * s3[7],
                                     s3[8 + b] * s3[11], s3[12 + b] * s3[15]);

              sum = cubic_spline_fit (yfrac, p0, p1, p2, p3);
              sum = RINT (sum / alphasum);

              pixel[b] = CLAMP (sum, 0, 255);
            }

          alphasum = RINT (alphasum);
          pixel[3] = CLAMP (alphasum, 0, 255);
        }
      else
        {
          pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
        }
      break;
    }
}

static gdouble inline
lanczos3_mul_alpha (const guchar  *pixels,
                    const gdouble *x_kernel,
                    const gdouble *y_kernel,
                    const gint     stride,
                    const gint     bytes,
                    const gint     byte)
{
  const guchar *row   = pixels;
  const guchar  alpha = bytes - 1;
  gdouble       sum   = 0.0;
  gint          x, y;

  for (y = 0; y < 6; y++, row += stride)
    {
      const guchar *p      = row;
      gdouble       tmpsum = 0.0;

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
              const gint     stride,
              const gint     bytes,
              const gint     byte)
{
  const guchar *row = pixels;
  gdouble       sum = 0.0;
  gint          x, y;

  for (y = 0; y < 6; y++, row += stride)
    {
      const guchar *p = row;
      gdouble       tmpsum = 0.0;

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
interpolate_lanczos3 (PixelSurround *surround,
                      const gint     x0,
                      const gint     y0,
                      const gdouble  xfrac,
                      const gdouble  yfrac,
                      const gint     bytes,
                      guchar        *pixel,
                      const gfloat  *kernel_lookup)
{
  gint          stride;
  const guchar *src = pixel_surround_lock (surround, x0 - 2, y0 - 2, &stride);
  const gint    x_shift    = (gint) (xfrac * LANCZOS_SPP + 0.5);
  const gint    y_shift    = (gint) (yfrac * LANCZOS_SPP + 0.5);
  gint          b, i;
  gdouble       kx_sum, ky_sum;
  gdouble       x_kernel[6];
  gdouble       y_kernel[6];
  gdouble       sum, alphasum;

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

  switch (bytes)
    {
    case 1:
      sum = RINT (lanczos3_mul (src, x_kernel, y_kernel, stride, 1, 0));

      pixel[0] = CLAMP (sum, 0, 255);
      break;

    case 2:
      alphasum  = lanczos3_mul (src, x_kernel, y_kernel, stride, 2, 1);
      if (alphasum > 0)
        {
          sum = lanczos3_mul_alpha (src, x_kernel, y_kernel, stride, 2, 0);
          sum = RINT (sum / alphasum);
          pixel[0] = CLAMP (sum, 0, 255);

          alphasum = RINT (alphasum);
          pixel[1] = CLAMP (alphasum, 0, 255);
        }
      else
        {
          pixel[0] = pixel[1] = 0;
        }
      break;

    case 3:
      for (b = 0; b < 3; b++)
        {
          sum = RINT (lanczos3_mul (src, x_kernel, y_kernel, stride, 3, b));

          pixel[b] = CLAMP (sum, 0, 255);
        }
      break;

    case 4:
      alphasum = lanczos3_mul (src, x_kernel, y_kernel, stride, 4, 3);
      if (alphasum > 0)
        {
          for (b = 0; b < 3; b++)
            {
              sum = lanczos3_mul_alpha (src, x_kernel, y_kernel, stride, 4, b);
              sum = RINT (sum / alphasum);

              pixel[b] = CLAMP (sum, 0, 255);
            }

          alphasum = RINT (alphasum);
          pixel[3] = CLAMP (alphasum, 0, 255);
        }
      else
        {
          pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
        }
      break;
    }
}

static void
scale_region_buffer (PixelRegion *srcPR,
                     PixelRegion *dstPR)
{
  const gdouble   scalex     = (gdouble) dstPR->w / (gdouble) srcPR->w;
  const gdouble   scaley     = (gdouble) dstPR->h / (gdouble) srcPR->h;
  const gint      src_width  = srcPR->w;
  const gint      src_height = srcPR->h;
  const gint      bytes      = srcPR->bytes;
  const gint      dst_width  = dstPR->w;
  const gint      dst_height = dstPR->h;
  guchar         *pixel      = dstPR->data;
  gint            x, y;

  for (y = 0; y < dst_height; y++)
   {
     gdouble yfrac = (y / scaley);
     gint    sy0   = (gint) yfrac;
     gint    sy1   = sy0 + 1;

     sy1   = (sy1 < src_height - 1) ? sy1 : src_height - 1;
     yfrac =  yfrac - sy0;

      for (x = 0; x < dst_width; x++)
        {
          gdouble xfrac = (x / scalex);
          gint    sx0   = (gint) xfrac;
          gint    sx1   = sx0 + 1;

          sx1   = (sx1 < src_width - 1) ? sx1 : src_width - 1;
          xfrac =  xfrac - sx0;

          interpolate_bilinear_pr (srcPR,
                                   sx0, sy0, sx1, sy1, xfrac, yfrac,
                                   pixel);
          pixel += bytes;
        }
   }
}

static void
interpolate_bilinear_pr (PixelRegion    *srcPR,
                         const gint      x0,
                         const gint      y0,
                         const gint      x1,
                         const gint      y1,
                         const gdouble   xfrac,
                         const gdouble   yfrac,
                         guchar         *pixel)
{
  const gint  bytes = srcPR->bytes;
  const gint  width = srcPR->w;
  guchar     *p1    = srcPR->data + (y0 * width + x0) * bytes;
  guchar     *p2    = srcPR->data + (y0 * width + x1) * bytes;
  guchar     *p3    = srcPR->data + (y1 * width + x0) * bytes;
  guchar     *p4    = srcPR->data + (y1 * width + x1) * bytes;
  gint        b;
  gdouble     sum, alphasum;

  switch (bytes)
    {
    case 1:
      sum = weighted_sum (xfrac, yfrac, p1[0], p2[0], p3[0], p4[0]);

      pixel[0] = CLAMP (sum, 0, 255);
      break;

    case 2:
      alphasum = weighted_sum (xfrac, yfrac, p1[1], p2[1], p3[1], p4[1]);
      if (alphasum > 0)
        {
          sum  = weighted_sum (xfrac, yfrac,
                               p1[0] * p1[1], p2[0] * p2[1],
                               p3[0] * p3[1], p4[0] * p4[1]);
          sum /= alphasum;

          pixel[0] = CLAMP (sum, 0, 255);
          pixel[1] = CLAMP (alphasum, 0, 255);
        }
      else
        {
          pixel[0] = pixel[1] = 0;
        }
      break;

    case 3:
      for (b = 0; b < 3; b++)
        {
          sum  = weighted_sum (xfrac, yfrac, p1[b], p2[b], p3[b], p4[b]);

          pixel[b] = CLAMP (sum, 0, 255);
        }
      break;

    case 4:
      alphasum = weighted_sum (xfrac, yfrac, p1[3], p2[3], p3[3], p4[3]);
      if (alphasum > 0)
        {
          for (b = 0; b < 3; b++)
            {
              sum  = weighted_sum (xfrac, yfrac,
                                   p1[b] * p1[3], p2[b] * p2[3],
                                   p3[b] * p3[3], p4[b] * p4[3]);
              sum /= alphasum;

              pixel[b] = CLAMP (sum, 0, 255);
            }

          pixel[3] = CLAMP (alphasum, 0, 255);
        }
      else
        {
          pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
        }
      break;
    }
}
