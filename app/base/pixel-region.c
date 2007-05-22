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

#include <stdarg.h>
#include <string.h>

#include <glib-object.h>

#include "base-types.h"

#include "pixel-region.h"
#include "temp-buf.h"
#include "tile-manager.h"
#include "tile.h"


/*********************/
/*  Local Functions  */

static gint                  get_portion_width       (PixelRegionIterator *PRI);
static gint                  get_portion_height      (PixelRegionIterator *PRI);
static PixelRegionIterator * pixel_regions_configure (PixelRegionIterator *PRI);
static void                  pixel_region_configure  (PixelRegionHolder   *PRH,
                                                      PixelRegionIterator *PRI);


/**************************/
/*  Function definitions  */

void
pixel_region_init (PixelRegion *PR,
                   TileManager *tiles,
                   gint         x,
                   gint         y,
                   gint         w,
                   gint         h,
                   gboolean     dirty)
{
  PR->data          = NULL;
  PR->tiles         = tiles;
  PR->curtile       = NULL;
  PR->offx          = 0;
  PR->offy          = 0;
  PR->bytes         = tile_manager_bpp (tiles);
  PR->rowstride     = PR->bytes * TILE_WIDTH;
  PR->x             = x;
  PR->y             = y;
  PR->w             = w;
  PR->h             = h;
  PR->dirty         = dirty;
  PR->process_count = 0;
}

void
pixel_region_init_temp_buf (PixelRegion *PR,
                            TempBuf     *temp_buf,
                            gint         x,
                            gint         y,
                            gint         w,
                            gint         h)
{
  PR->data          = temp_buf_data (temp_buf);
  PR->tiles         = NULL;
  PR->curtile       = NULL;
  PR->offx          = 0;
  PR->offy          = 0;
  PR->bytes         = temp_buf->bytes;
  PR->rowstride     = temp_buf->width * temp_buf->bytes;
  PR->x             = x;
  PR->y             = y;
  PR->w             = w;
  PR->h             = h;
  PR->dirty         = FALSE;
  PR->process_count = 0;
}

void
pixel_region_init_data (PixelRegion *PR,
                        guchar      *data,
                        gint         bytes,
                        gint         rowstride,
                        gint         x,
                        gint         y,
                        gint         w,
                        gint         h)
{
  PR->data          = data;
  PR->tiles         = NULL;
  PR->curtile       = NULL;
  PR->offx          = 0;
  PR->offy          = 0;
  PR->bytes         = bytes;
  PR->rowstride     = rowstride;
  PR->x             = x;
  PR->y             = y;
  PR->w             = w;
  PR->h             = h;
  PR->dirty         = FALSE;
  PR->process_count = 0;
}

void
pixel_region_resize (PixelRegion *PR,
                     gint         x,
                     gint         y,
                     gint         w,
                     gint         h)
{
  /*  If the data is non-null, data is contiguous--need to advance  */
  if (PR->data != NULL)
    PR->data += ((y - PR->y) * PR->rowstride +
                 (x - PR->x) * PR->bytes);

  /*  update sizes for both contiguous and tiled regions  */
  PR->x = x;
  PR->y = y;
  PR->w = w;
  PR->h = h;
}

void
pixel_region_get_row (PixelRegion *PR,
                      gint         x,
                      gint         y,
                      gint         w,
                      guchar      *data,
                      gint         subsample)
{
  Tile   *tile;
  guchar *tile_data;
  gint    inc;
  gint    end;
  gint    boundary;
  gint    b;
  gint    npixels;
  gint    bpp;

  end = x + w;

  bpp = PR->bytes;
  inc = subsample * bpp;

  if (subsample == 1)
    {
      if (PR->tiles)
        read_pixel_data (PR->tiles, x, y, end - 1, y, data, PR->bytes);
      else
        memcpy (data, PR->data + x * bpp + y * PR->rowstride, w * bpp);
    }
  else
    {
      while (x < end)
        {
          tile = tile_manager_get_tile (PR->tiles, x, y, TRUE, FALSE);
          tile_data = tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT);
          npixels = tile_ewidth (tile) - (x % TILE_WIDTH);

          if ((x + npixels) > end) /* make sure we don't write past the end */
            npixels = end - x;

          boundary = x + npixels;
          for ( ; x < boundary; x += subsample)
            {
              for (b = 0; b < bpp; b++)
                *data++ = tile_data[b];

              tile_data += inc;
            }
          tile_release (tile, FALSE);
        }
    }
}

void
pixel_region_set_row (PixelRegion  *PR,
                      gint          x,
                      gint          y,
                      gint          w,
                      const guchar *data)
{
  if (PR->tiles)
    {
      gint end = x + w;

      write_pixel_data (PR->tiles, x, y, end - 1, y, data, PR->bytes);
    }
  else
    {
      memcpy (PR->data + x * PR->bytes + y * PR->rowstride, data, w * PR->bytes);
    }
}


void
pixel_region_get_col (PixelRegion *PR,
                      gint         x,
                      gint         y,
                      gint         h,
                      guchar      *data,
                      gint         subsample)
{
  Tile   *tile;
  guchar *tile_data;
  gint    bpp;
  gint    inc;
  gint    end;
  gint    boundary;
  gint    b;

  end = y + h;
  bpp = PR->bytes;

  while (y < end)
    {
      tile = tile_manager_get_tile (PR->tiles, x, y, TRUE, FALSE);
      tile_data = tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT);
      boundary = y + (tile_eheight(tile) - (y % TILE_HEIGHT));

      if (boundary > end) /* make sure we don't write past the end */
        boundary = end;

      inc = subsample * bpp * tile_ewidth (tile);

      for ( ; y < boundary; y += subsample)
        {
          for (b = 0; b < bpp; b++)
            *data++ = tile_data[b];

          tile_data += inc;
        }

      tile_release (tile, FALSE);
    }
}


void
pixel_region_set_col (PixelRegion  *PR,
                      gint          x,
                      gint          y,
                      gint          h,
                      const guchar *data)
{
  gint end = y + h;
  gint bpp = PR->bytes;

  write_pixel_data (PR->tiles, x, y, x, end-1, data, bpp);
}

gboolean
pixel_region_has_alpha (PixelRegion *PR)
{
  if (PR->bytes == 2 || PR->bytes == 4)
    return TRUE;
  else
    return FALSE;
}

PixelRegionIterator *
pixel_regions_register (gint num_regions,
                        ...)
{
  PixelRegionIterator *PRI;
  gboolean             found;
  va_list              ap;

  if (num_regions < 1)
    return NULL;

  PRI = g_slice_new0 (PixelRegionIterator);
  PRI->dirty_tiles = 1;

  va_start (ap, num_regions);

  found = FALSE;

  while (num_regions --)
    {
      PixelRegionHolder *PRH;
      PixelRegion       *PR;

      PR = va_arg (ap, PixelRegion *);

      PRH = g_slice_new0 (PixelRegionHolder);
      PRH->PR = PR;

      if (PR != NULL)
        {
          /*  If there is a defined value for data, make sure tiles is NULL  */
          if (PR->data)
            PR->tiles = NULL;

          PRH->original_data     = PR->data;
          PRH->startx            = PR->x;
          PRH->starty            = PR->y;
          PRH->PR->process_count = 0;

          if (! found)
            {
              found = TRUE;

              PRI->region_width  = PR->w;
              PRI->region_height = PR->h;
            }
        }

      /*  Add the pixel region holder to the list  */
      PRI->pixel_regions = g_slist_prepend (PRI->pixel_regions, PRH);
    }

  va_end (ap);

  return pixel_regions_configure (PRI);
}


PixelRegionIterator *
pixel_regions_process (PixelRegionIterator *PRI)
{
  GSList *list;

  PRI->process_count++;

  /*  Unref all referenced tiles and increment the offsets  */

  for (list = PRI->pixel_regions; list; list = g_slist_next (list))
    {
      PixelRegionHolder *PRH = list->data;

      if ((PRH->PR != NULL) && (PRH->PR->process_count != PRI->process_count))
        {
          /*  This eliminates the possibility of incrementing the
           *  same region twice
           */
          PRH->PR->process_count++;

          /*  Unref the last referenced tile if the underlying region
              is a tile manager  */
          if (PRH->PR->tiles)
            {
              /* only set the dirty flag if PRH->dirty_tiles == TRUE */
              tile_release (PRH->PR->curtile,
                            PRH->PR->dirty && PRI->dirty_tiles);
              PRH->PR->curtile = NULL;
            }

          PRH->PR->x += PRI->portion_width;

          if ((PRH->PR->x - PRH->startx) >= PRI->region_width)
            {
              PRH->PR->x  = PRH->startx;
              PRH->PR->y += PRI->portion_height;
            }
        }
    }

  return pixel_regions_configure (PRI);
}

void
pixel_regions_process_stop (PixelRegionIterator *PRI)
{
  GSList *list;

  PRI->process_count++;

  /*  Unref all referenced tiles and increment the offsets  */

  for (list = PRI->pixel_regions; list; list = g_slist_next (list))
    {
      PixelRegionHolder *PRH = list->data;

      if ((PRH->PR != NULL) && (PRH->PR->process_count != PRI->process_count))
        {
          /*  This eliminates the possibility of incrementing the
           *  same region twice
           */
          PRH->PR->process_count++;

          /*  Unref the last referenced tile if the underlying region
              is a tile manager  */
          if (PRH->PR->tiles)
            {
              tile_release (PRH->PR->curtile, PRH->PR->dirty);
              PRH->PR->curtile = NULL;
            }
        }
    }

  if (PRI->pixel_regions)
    {
      for (list = PRI->pixel_regions; list; list = g_slist_next (list))
        g_slice_free (PixelRegionHolder, list->data);

      g_slist_free (PRI->pixel_regions);

      g_slice_free (PixelRegionIterator, PRI);
    }
}


/*********************************/
/*  Static Function Definitions  */

static gint
get_portion_height (PixelRegionIterator *PRI)
{
  GSList *list;
  gint    min_height = G_MAXINT;
  gint    height;

  /*  Find the minimum height to the next vertical tile
   *  (in the case of a tile manager) or to the end of the
   *  pixel region (in the case of no tile manager)
   */

  for (list = PRI->pixel_regions; list; list = g_slist_next (list))
    {
      PixelRegionHolder *PRH = list->data;

      if (PRH->PR)
        {
          /*  Check if we're past the point of no return  */
          if ((PRH->PR->y - PRH->starty) >= PRI->region_height)
            return 0;

          if (PRH->PR->tiles)
            {
              height = TILE_HEIGHT - (PRH->PR->y % TILE_HEIGHT);
              height = CLAMP (height,
                              0,
                              (PRI->region_height - (PRH->PR->y - PRH->starty)));
            }
          else
            {
              height = (PRI->region_height - (PRH->PR->y - PRH->starty));
            }

          if (height < min_height)
            min_height = height;
        }
    }

  return min_height;
}


static gint
get_portion_width (PixelRegionIterator *PRI)
{
  GSList *list;
  gint    min_width = G_MAXINT;
  gint    width;

  /*  Find the minimum width to the next vertical tile
   *  (in the case of a tile manager) or to the end of
   *  the pixel region (in the case of no tile manager)
   */

  for (list = PRI->pixel_regions; list; list = g_slist_next (list))
    {
      PixelRegionHolder *PRH = list->data;

      if (PRH->PR)
        {
          /*  Check if we're past the point of no return  */
          if ((PRH->PR->x - PRH->startx) >= PRI->region_width)
            return 0;

          if (PRH->PR->tiles)
            {
              width = TILE_WIDTH - (PRH->PR->x % TILE_WIDTH);
              width = CLAMP (width,
                             0,
                             (PRI->region_width - (PRH->PR->x - PRH->startx)));
            }
          else
            {
              width = (PRI->region_width - (PRH->PR->x - PRH->startx));
            }

          if (width < min_width)
            min_width = width;
        }
    }

  return min_width;
}


static PixelRegionIterator *
pixel_regions_configure (PixelRegionIterator *PRI)
{
  GSList *list;

  /*  Determine the portion width and height  */
  PRI->portion_width  = get_portion_width (PRI);
  PRI->portion_height = get_portion_height (PRI);

  if (PRI->portion_width  == 0 || PRI->portion_height == 0)
    {
      /*  free the pixel regions list  */
      if (PRI->pixel_regions)
        {
          for (list = PRI->pixel_regions; list; list = g_slist_next (list))
            g_slice_free (PixelRegionHolder, list->data);

          g_slist_free (PRI->pixel_regions);
          g_slice_free (PixelRegionIterator, PRI);
        }

      return NULL;
    }

  PRI->process_count++;

  for (list = PRI->pixel_regions; list; list = g_slist_next (list))
    {
      PixelRegionHolder *PRH = list->data;

      if ((PRH->PR != NULL) && (PRH->PR->process_count != PRI->process_count))
        {
          PRH->PR->process_count++;
          pixel_region_configure (PRH, PRI);
        }
    }

  return PRI;
}


static void
pixel_region_configure (PixelRegionHolder   *PRH,
                        PixelRegionIterator *PRI)
{
  /*  Configure the rowstride and data pointer for the pixel region
   *  based on the current offsets into the region and whether the
   *  region is represented by a tile manager or not
   */
  if (PRH->PR->tiles)
    {
      PRH->PR->curtile = tile_manager_get_tile (PRH->PR->tiles,
                                                PRH->PR->x,
                                                PRH->PR->y,
                                                TRUE,
                                                PRH->PR->dirty);

      PRH->PR->offx = PRH->PR->x % TILE_WIDTH;
      PRH->PR->offy = PRH->PR->y % TILE_HEIGHT;

      PRH->PR->rowstride = tile_ewidth (PRH->PR->curtile) * PRH->PR->bytes;
      PRH->PR->data = tile_data_pointer (PRH->PR->curtile,
                                         PRH->PR->offx,
                                         PRH->PR->offy);
    }
  else
    {
      PRH->PR->data = (PRH->original_data +
                       PRH->PR->y * PRH->PR->rowstride +
                       PRH->PR->x * PRH->PR->bytes);
    }

  PRH->PR->w = PRI->portion_width;
  PRH->PR->h = PRI->portion_height;
}
