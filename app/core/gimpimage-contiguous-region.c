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

#include <stdlib.h>

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/pixel-processor.h"
#include "base/pixel-region.h"
#include "base/tile.h"
#include "base/tile-manager.h"

#include "gimpchannel.h"
#include "gimpimage.h"
#include "gimpimage-contiguous-region.h"
#include "gimppickable.h"


typedef struct
{
  GimpImage           *image;
  GimpImageType        type;
  gboolean             sample_merged;
  gboolean             antialias;
  gint                 threshold;
  gboolean             select_transparent;
  GimpSelectCriterion  select_criterion;
  gboolean             has_alpha;
  guchar               color[MAX_CHANNELS];
} ContinuousRegionData;


/*  local function prototypes  */

static void contiguous_region_by_color    (ContinuousRegionData *cont,
                                           PixelRegion          *imagePR,
                                           PixelRegion          *maskPR);

static gint pixel_difference              (const guchar        *col1,
                                           const guchar        *col2,
                                           gboolean             antialias,
                                           gint                 threshold,
                                           gint                 bytes,
                                           gboolean             has_alpha,
                                           gboolean             select_transparent,
                                           GimpSelectCriterion  select_criterion);
static void ref_tiles                     (TileManager         *src,
                                           TileManager         *mask,
                                           Tile               **s_tile,
                                           Tile               **m_tile,
                                           gint                 x,
                                           gint                 y,
                                           guchar             **s,
                                           guchar             **m);
static gint find_contiguous_segment       (GimpImage           *image,
                                           const guchar        *col,
                                           PixelRegion         *src,
                                           PixelRegion         *mask,
                                           gint                 width,
                                           gint                 bytes,
                                           GimpImageType        src_type,
                                           gboolean             has_alpha,
                                           gboolean             select_transparent,
                                           GimpSelectCriterion  select_criterion,
                                           gboolean             antialias,
                                           gint                 threshold,
                                           gint                 initial,
                                           gint                *start,
                                           gint                *end);
static void find_contiguous_region_helper (GimpImage           *image,
                                           PixelRegion         *mask,
                                           PixelRegion         *src,
                                           GimpImageType        src_type,
                                           gboolean             has_alpha,
                                           gboolean             select_transparent,
                                           GimpSelectCriterion  select_criterion,
                                           gboolean             antialias,
                                           gint                 threshold,
                                           gint                 x,
                                           gint                 y,
                                           const guchar        *col);


/*  public functions  */

GimpChannel *
gimp_image_contiguous_region_by_seed (GimpImage           *image,
                                      GimpDrawable        *drawable,
                                      gboolean             sample_merged,
                                      gboolean             antialias,
                                      gint                 threshold,
                                      gboolean             select_transparent,
                                      GimpSelectCriterion  select_criterion,
                                      gint                 x,
                                      gint                 y)
{
  PixelRegion    srcPR, maskPR;
  GimpPickable  *pickable;
  TileManager   *tiles;
  GimpChannel   *mask;
  GimpImageType  src_type;
  gboolean       has_alpha;
  gint           bytes;
  Tile          *tile;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  if (sample_merged)
    pickable = GIMP_PICKABLE (image->projection);
  else
    pickable = GIMP_PICKABLE (drawable);

  gimp_pickable_flush (pickable);

  src_type  = gimp_pickable_get_image_type (pickable);
  has_alpha = GIMP_IMAGE_TYPE_HAS_ALPHA (src_type);
  bytes     = GIMP_IMAGE_TYPE_BYTES (src_type);

  tiles = gimp_pickable_get_tiles (pickable);
  pixel_region_init (&srcPR, tiles,
                     0, 0,
                     tile_manager_width (tiles),
                     tile_manager_height (tiles),
                     FALSE);

  mask = gimp_channel_new_mask (image, srcPR.w, srcPR.h);
  pixel_region_init (&maskPR, gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                     0, 0,
                     gimp_item_width  (GIMP_ITEM (mask)),
                     gimp_item_height (GIMP_ITEM (mask)),
                     TRUE);

  tile = tile_manager_get_tile (srcPR.tiles, x, y, TRUE, FALSE);
  if (tile)
    {
      const guchar *start;
      guchar        start_col[MAX_CHANNELS];

      start = tile_data_pointer (tile, x, y);

      if (has_alpha)
        {
          if (select_transparent)
            {
              /*  don't select transparent regions if the start pixel isn't
               *  fully transparent
               */
              if (start[bytes - 1] > 0)
                select_transparent = FALSE;
            }
        }
      else
        {
          select_transparent = FALSE;
        }

      if (GIMP_IMAGE_TYPE_IS_INDEXED (src_type))
        {
          gimp_image_get_color (image, src_type, start, start_col);
        }
      else
        {
          gint i;

          for (i = 0; i < bytes; i++)
            start_col[i] = start[i];
        }

      find_contiguous_region_helper (image, &maskPR, &srcPR,
                                     src_type, has_alpha,
                                     select_transparent, select_criterion,
                                     antialias, threshold,
                                     x, y, start_col);

      tile_release (tile, FALSE);
    }

  return mask;
}

GimpChannel *
gimp_image_contiguous_region_by_color (GimpImage            *image,
                                       GimpDrawable         *drawable,
                                       gboolean              sample_merged,
                                       gboolean              antialias,
                                       gint                  threshold,
                                       gboolean              select_transparent,
                                       GimpSelectCriterion  select_criterion,
                                       const GimpRGB        *color)
{
  /*  Scan over the image's active layer, finding pixels within the
   *  specified threshold from the given R, G, & B values.  If
   *  antialiasing is on, use the same antialiasing scheme as in
   *  fuzzy_select.  Modify the image's mask to reflect the
   *  additional selection
   */
  GimpPickable *pickable;
  TileManager  *tiles;
  GimpChannel  *mask;
  PixelRegion   imagePR, maskPR;
  gint          width, height;

  ContinuousRegionData  cont;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (color != NULL, NULL);

  gimp_rgba_get_uchar (color,
                       cont.color + 0,
                       cont.color + 1,
                       cont.color + 2,
                       cont.color + 3);

  if (sample_merged)
    pickable = GIMP_PICKABLE (image->projection);
  else
    pickable = GIMP_PICKABLE (drawable);

  gimp_pickable_flush (pickable);

  cont.type      = gimp_pickable_get_image_type (pickable);
  cont.has_alpha = GIMP_IMAGE_TYPE_HAS_ALPHA (cont.type);

  tiles  = gimp_pickable_get_tiles (pickable);
  width  = tile_manager_width (tiles);
  height = tile_manager_height (tiles);

  pixel_region_init (&imagePR, tiles, 0, 0, width, height, FALSE);

  if (cont.has_alpha)
    {
      if (select_transparent)
        {
          /*  don't select transparancy if "color" isn't fully transparent
           */
          if (cont.color[3] > 0)
            select_transparent = FALSE;
        }
    }
  else
    {
      select_transparent = FALSE;
    }

  cont.image              = image;
  cont.antialias          = antialias;
  cont.threshold          = threshold;
  cont.select_transparent = select_transparent;
  cont.select_criterion   = select_criterion;

  mask = gimp_channel_new_mask (image, width, height);

  pixel_region_init (&maskPR, gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                     0, 0, width, height,
                     TRUE);

  pixel_regions_process_parallel ((PixelProcessorFunc)
                                  contiguous_region_by_color, &cont,
                                  2, &imagePR, &maskPR);

  return mask;
}


/*  private functions  */

static void
contiguous_region_by_color (ContinuousRegionData *cont,
                            PixelRegion          *imagePR,
                            PixelRegion          *maskPR)
{
  const guchar *image = imagePR->data;
  guchar       *mask  = maskPR->data;
  gint          x, y;

  for (y = 0; y < imagePR->h; y++)
    {
      const guchar *i = image;
      guchar       *m = mask;

      for (x = 0; x < imagePR->w; x++)
        {
          guchar  rgb[MAX_CHANNELS];

          /*  Get the rgb values for the color  */
          gimp_image_get_color (cont->image, cont->type, i, rgb);

          /*  Find how closely the colors match  */
          *m++ = pixel_difference (cont->color, rgb,
                                   cont->antialias,
                                   cont->threshold,
                                   cont->has_alpha ? 4 : 3,
                                   cont->has_alpha,
                                   cont->select_transparent,
                                   cont->select_criterion);

          i += imagePR->bytes;
        }

      image += imagePR->rowstride;
      mask += maskPR->rowstride;
    }
}

static gint
pixel_difference (const guchar        *col1,
                  const guchar        *col2,
                  gboolean             antialias,
                  gint                 threshold,
                  gint                 bytes,
                  gboolean             has_alpha,
                  gboolean             select_transparent,
                  GimpSelectCriterion  select_criterion)
{
  gint max = 0;

  /*  if there is an alpha channel, never select transparent regions  */
  if (! select_transparent && has_alpha && col2[bytes - 1] == 0)
    return 0;

  if (select_transparent && has_alpha)
    {
      max = abs (col1[bytes - 1] - col2[bytes - 1]);
    }
  else
    {
      gint diff;
      gint b;
      gint av0, av1, av2;
      gint bv0, bv1, bv2;

      if (has_alpha)
        bytes--;

      switch (select_criterion)
        {
        case GIMP_SELECT_CRITERION_COMPOSITE:
          for (b = 0; b < bytes; b++)
            {
              diff = abs (col1[b] - col2[b]);
              if (diff > max)
                max = diff;
            }
          break;

        case GIMP_SELECT_CRITERION_R:
          max = abs (col1[0] - col2[0]);
          break;

        case GIMP_SELECT_CRITERION_G:
          max = abs (col1[1] - col2[1]);
          break;

        case GIMP_SELECT_CRITERION_B:
          max = abs (col1[2] - col2[2]);
          break;

        case GIMP_SELECT_CRITERION_H:
          av0 = (gint)col1[0];
          av1 = (gint)col1[1];
          av2 = (gint)col1[2];
          bv0 = (gint)col2[0];
          bv1 = (gint)col2[1];
          bv2 = (gint)col2[2];
          gimp_rgb_to_hsv_int (&av0, &av1, &av2);
          gimp_rgb_to_hsv_int (&bv0, &bv1, &bv2);
          max = abs (av0 - bv0);
          break;

        case GIMP_SELECT_CRITERION_S:
          av0 = (gint)col1[0];
          av1 = (gint)col1[1];
          av2 = (gint)col1[2];
          bv0 = (gint)col2[0];
          bv1 = (gint)col2[1];
          bv2 = (gint)col2[2];
          gimp_rgb_to_hsv_int (&av0, &av1, &av2);
          gimp_rgb_to_hsv_int (&bv0, &bv1, &bv2);
          max = abs (av1 - bv1);
          break;

        case GIMP_SELECT_CRITERION_V:
          av0 = (gint)col1[0];
          av1 = (gint)col1[1];
          av2 = (gint)col1[2];
          bv0 = (gint)col2[0];
          bv1 = (gint)col2[1];
          bv2 = (gint)col2[2];
          gimp_rgb_to_hsv_int (&av0, &av1, &av2);
          gimp_rgb_to_hsv_int (&bv0, &bv1, &bv2);
          max = abs (av2 - bv2);
          break;
        }
    }

  if (antialias && threshold > 0)
    {
      gfloat aa = 1.5 - ((gfloat) max / threshold);

      if (aa <= 0.0)
        return 0;
      else if (aa < 0.5)
        return (guchar) (aa * 512);
      else
        return 255;
    }
  else
    {
      if (max > threshold)
        return 0;
      else
        return 255;
    }
}

static void
ref_tiles (TileManager  *src,
           TileManager  *mask,
           Tile        **s_tile,
           Tile        **m_tile,
           gint          x,
           gint          y,
           guchar      **s,
           guchar      **m)
{
  if (*s_tile != NULL)
    tile_release (*s_tile, FALSE);
  if (*m_tile != NULL)
    tile_release (*m_tile, TRUE);

  *s_tile = tile_manager_get_tile (src, x, y, TRUE, FALSE);
  *m_tile = tile_manager_get_tile (mask, x, y, TRUE, TRUE);

  *s = tile_data_pointer (*s_tile, x, y);
  *m = tile_data_pointer (*m_tile, x, y);
}

static int
find_contiguous_segment (GimpImage           *image,
                         const guchar        *col,
                         PixelRegion         *src,
                         PixelRegion         *mask,
                         gint                 width,
                         gint                 bytes,
                         GimpImageType        src_type,
                         gboolean             has_alpha,
                         gboolean             select_transparent,
                         GimpSelectCriterion  select_criterion,
                         gboolean             antialias,
                         gint                 threshold,
                         gint                 initial,
                         gint                *start,
                         gint                *end)
{
  guchar *s;
  guchar *m;
  guchar  s_color[MAX_CHANNELS];
  guchar  diff;
  gint    col_bytes = bytes;
  Tile   *s_tile    = NULL;
  Tile   *m_tile    = NULL;

  ref_tiles (src->tiles, mask->tiles,
             &s_tile, &m_tile, src->x, src->y, &s, &m);

  if (GIMP_IMAGE_TYPE_IS_INDEXED (src_type))
    {
      col_bytes = has_alpha ? 4 : 3;

      gimp_image_get_color (image, src_type, s, s_color);

      diff = pixel_difference (col, s_color, antialias, threshold,
                               col_bytes, has_alpha, select_transparent,
                               select_criterion);
     }
  else
    {
      diff = pixel_difference (col, s, antialias, threshold,
                               col_bytes, has_alpha, select_transparent,
                               select_criterion);
    }

  /* check the starting pixel */
  if (! diff)
    {
      tile_release (s_tile, FALSE);
      tile_release (m_tile, TRUE);
      return FALSE;
    }

  *m-- = diff;
  s -= bytes;
  *start = initial - 1;

  while (*start >= 0 && diff)
    {
      if (! ((*start + 1) % TILE_WIDTH))
        ref_tiles (src->tiles, mask->tiles,
                   &s_tile, &m_tile, *start, src->y, &s, &m);

      if (GIMP_IMAGE_TYPE_IS_INDEXED (src_type))
        {
          gimp_image_get_color (image, src_type, s, s_color);

          diff = pixel_difference (col, s_color, antialias, threshold,
                                   col_bytes, has_alpha, select_transparent,
                                   select_criterion);
        }
      else
        {
          diff = pixel_difference (col, s, antialias, threshold,
                                   col_bytes, has_alpha, select_transparent,
                                   select_criterion);
        }

      if ((*m-- = diff))
        {
          s -= bytes;
          (*start)--;
        }
    }

  diff = 1;
  *end = initial + 1;

  if (*end % TILE_WIDTH && *end < width)
    ref_tiles (src->tiles, mask->tiles,
               &s_tile, &m_tile, *end, src->y, &s, &m);

  while (*end < width && diff)
    {
      if (! (*end % TILE_WIDTH))
        ref_tiles (src->tiles, mask->tiles,
                   &s_tile, &m_tile, *end, src->y, &s, &m);

      if (GIMP_IMAGE_TYPE_IS_INDEXED (src_type))
        {
          gimp_image_get_color (image, src_type, s, s_color);

          diff = pixel_difference (col, s_color, antialias, threshold,
                                   col_bytes, has_alpha, select_transparent,
                                   select_criterion);
        }
      else
        {
          diff = pixel_difference (col, s, antialias, threshold,
                                   col_bytes, has_alpha, select_transparent,
                                   select_criterion);
        }

      if ((*m++ = diff))
        {
          s += bytes;
          (*end)++;
        }
    }

  tile_release (s_tile, FALSE);
  tile_release (m_tile, TRUE);

  return TRUE;
}

static void
find_contiguous_region_helper (GimpImage           *image,
                               PixelRegion         *mask,
                               PixelRegion         *src,
                               GimpImageType        src_type,
                               gboolean             has_alpha,
                               gboolean             select_transparent,
                               GimpSelectCriterion  select_criterion,
                               gboolean             antialias,
                               gint                 threshold,
                               gint                 x,
                               gint                 y,
                               const guchar        *col)
{
  gint    start, end;
  gint    new_start, new_end;
  gint    val;
  Tile   *tile;
  GQueue *coord_stack;

  coord_stack = g_queue_new();

  /* To avoid excessive memory allocation (y, start, end) tuples are
   * stored in interleaved format:
   *
   * [y1] [start1] [end1] [y2] [start2] [end2]
   */
  g_queue_push_tail (coord_stack, GINT_TO_POINTER (y));
  g_queue_push_tail (coord_stack, GINT_TO_POINTER (x - 1));
  g_queue_push_tail (coord_stack, GINT_TO_POINTER (x + 1));

  do
    {
      y     = GPOINTER_TO_INT (g_queue_pop_head (coord_stack));
      start = GPOINTER_TO_INT (g_queue_pop_head (coord_stack));
      end   = GPOINTER_TO_INT (g_queue_pop_head (coord_stack));

      for (x = start + 1; x < end; x++)
        {
          tile = tile_manager_get_tile (mask->tiles, x, y, TRUE, FALSE);
          val = *(const guchar *) (tile_data_pointer (tile, x, y));
          tile_release (tile, FALSE);
          if (val != 0)
            continue;

          src->x = x;
          src->y = y;

          if (! find_contiguous_segment (image, col, src, mask, src->w,
                                         src->bytes, src_type, has_alpha,
                                         select_transparent, select_criterion,
                                         antialias, threshold, x,
                                         &new_start, &new_end))
            continue;

          if (y + 1 < src->h)
            {
              g_queue_push_tail (coord_stack, GINT_TO_POINTER (y + 1));
              g_queue_push_tail (coord_stack, GINT_TO_POINTER (new_start));
              g_queue_push_tail (coord_stack, GINT_TO_POINTER (new_end));
            }

          if (y - 1 >= 0)
            {
              g_queue_push_tail (coord_stack, GINT_TO_POINTER (y - 1));
              g_queue_push_tail (coord_stack, GINT_TO_POINTER (new_start));
              g_queue_push_tail (coord_stack, GINT_TO_POINTER (new_end));
            }
        }
    }
  while (!g_queue_is_empty (coord_stack));

  g_queue_free (coord_stack);
}
