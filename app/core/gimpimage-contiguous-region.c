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

#include <stdlib.h>

#include <cairo.h>
#include <gegl.h>

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


/*  local function prototypes  */

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
static gboolean find_contiguous_segment   (GimpImage           *image,
                                           const guchar        *col,
                                           PixelRegion         *src,
                                           PixelRegion         *mask,
                                           gint                 width,
                                           gint                 bytes,
                                           const Babl          *fish,
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
                                           const Babl          *fish,
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
  const Babl    *src_format;
  const Babl    *fish = NULL;
  gboolean       has_alpha;
  gint           bytes;
  Tile          *tile;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  if (sample_merged)
    pickable = GIMP_PICKABLE (gimp_image_get_projection (image));
  else
    pickable = GIMP_PICKABLE (drawable);

  gimp_pickable_flush (pickable);

  src_format = gimp_pickable_get_format (pickable);
  has_alpha  = babl_format_has_alpha (src_format);
  bytes      = babl_format_get_bytes_per_pixel (src_format);

  tiles = gimp_pickable_get_tiles (pickable);
  pixel_region_init (&srcPR, tiles,
                     0, 0,
                     tile_manager_width (tiles),
                     tile_manager_height (tiles),
                     FALSE);

  mask = gimp_channel_new_mask (image, srcPR.w, srcPR.h);
  pixel_region_init (&maskPR, gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                     0, 0,
                     gimp_item_get_width  (GIMP_ITEM (mask)),
                     gimp_item_get_height (GIMP_ITEM (mask)),
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

      if (babl_format_is_palette (src_format))
        {
          fish = babl_fish (src_format, babl_format ("R'G'B'A u8"));

          babl_process (fish, start, start_col, 1);
        }
      else
        {
          gint i;

          for (i = 0; i < bytes; i++)
            start_col[i] = start[i];
        }

      find_contiguous_region_helper (image, &maskPR, &srcPR,
                                     fish, has_alpha,
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
  GeglBufferIterator *iter;
  GimpPickable       *pickable;
  GimpChannel        *mask;
  GeglBuffer         *src_buffer;
  GeglBuffer         *mask_buffer;
  gint                width, height;
  gboolean            has_alpha;
  guchar              col[MAX_CHANNELS];

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (color != NULL, NULL);

  gimp_rgba_get_uchar (color, &col[0], &col[1], &col[2], &col[3]);

  if (sample_merged)
    pickable = GIMP_PICKABLE (gimp_image_get_projection (image));
  else
    pickable = GIMP_PICKABLE (drawable);

  gimp_pickable_flush (pickable);

  has_alpha = babl_format_has_alpha (gimp_pickable_get_format (pickable));

  src_buffer = gimp_pickable_get_buffer (pickable);
  width  = gegl_buffer_get_width (src_buffer);
  height = gegl_buffer_get_height (src_buffer);

  iter = gegl_buffer_iterator_new (src_buffer,
                                   NULL, 0, babl_format ("R'G'B'A u8"),
                                   GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  if (has_alpha)
    {
      if (select_transparent)
        {
          /*  don't select transparancy if "color" isn't fully transparent
           */
          if (col[3] > 0)
            select_transparent = FALSE;
        }
    }
  else
    {
      select_transparent = FALSE;
    }

  mask = gimp_channel_new_mask (image, width, height);

  mask_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));

  gegl_buffer_iterator_add (iter, mask_buffer,
                            NULL, 0, babl_format ("Y' u8"),
                            GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const guchar *src  = iter->data[0];
      guchar       *dest = iter->data[1];

      while (iter->length--)
        {
          /*  Find how closely the colors match  */
          *dest = pixel_difference (col, src,
                                    antialias,
                                    threshold,
                                    has_alpha ? 4 : 3,
                                    has_alpha,
                                    select_transparent,
                                    select_criterion);

          src  += 4;
          dest += 1;
        }
    }

  return mask;
}


/*  private functions  */

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
          av0 = (gint) col1[0];
          av1 = (gint) col1[1];
          av2 = (gint) col1[2];
          bv0 = (gint) col2[0];
          bv1 = (gint) col2[1];
          bv2 = (gint) col2[2];
          gimp_rgb_to_hsv_int (&av0, &av1, &av2);
          gimp_rgb_to_hsv_int (&bv0, &bv1, &bv2);
          /* wrap around candidates for the actual distance */
          {
            gint dist1 = abs (av0 - bv0);
            gint dist2 = abs (av0 - 360 - bv0);
            gint dist3 = abs (av0 - bv0 + 360);
            max = MIN (dist1, dist2);
            if (max > dist3)
              max = dist3;
          }
          break;

        case GIMP_SELECT_CRITERION_S:
          av0 = (gint) col1[0];
          av1 = (gint) col1[1];
          av2 = (gint) col1[2];
          bv0 = (gint) col2[0];
          bv1 = (gint) col2[1];
          bv2 = (gint) col2[2];
          gimp_rgb_to_hsv_int (&av0, &av1, &av2);
          gimp_rgb_to_hsv_int (&bv0, &bv1, &bv2);
          max = abs (av1 - bv1);
          break;

        case GIMP_SELECT_CRITERION_V:
          av0 = (gint) col1[0];
          av1 = (gint) col1[1];
          av2 = (gint) col1[2];
          bv0 = (gint) col2[0];
          bv1 = (gint) col2[1];
          bv2 = (gint) col2[2];
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

static gboolean
find_contiguous_segment (GimpImage           *image,
                         const guchar        *col,
                         PixelRegion         *src,
                         PixelRegion         *mask,
                         gint                 width,
                         gint                 bytes,
                         const Babl          *fish,
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

  if (fish)
    {
      col_bytes = has_alpha ? 4 : 3;

      babl_process (fish, s, s_color, 1);

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

      if (fish)
        {
          babl_process (fish, s, s_color, 1);

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

      if (fish)
        {
          babl_process (fish, s, s_color, 1);

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
                               const Babl          *fish,
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

  coord_stack = g_queue_new ();

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
          val = *(const guchar *) tile_data_pointer (tile, x, y);
          tile_release (tile, FALSE);
          if (val != 0)
            continue;

          src->x = x;
          src->y = y;

          if (! find_contiguous_segment (image, col, src, mask, src->w,
                                         src->bytes, fish, has_alpha,
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
  while (! g_queue_is_empty (coord_stack));

  g_queue_free (coord_stack);
}
