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

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-utils.h"
#include "config/gimpcoreconfig.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp-parasites.h"
#include "gimp.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpgrid.h"
#include "gimpimage-colormap.h"
#include "gimpimage-grid.h"
#include "gimpimage-guides.h"
#include "gimpimage-projection.h"
#include "gimpimage-undo.h"
#include "gimpimage.h"
#include "gimpitemundo.h"
#include "gimplayer-floating-sel.h"
#include "gimplayer.h"
#include "gimplayermask.h"
#include "gimplist.h"
#include "gimpparasitelist.h"
#include "gimpundostack.h"

#include "paint/gimppaintcore.h"

#include "vectors/gimpvectors.h"

#include "gimp-intl.h"


/****************/
/*  Image Undo  */
/****************/

typedef struct _ImageUndo ImageUndo;

struct _ImageUndo
{
  TileManager *tiles;
  gint         x1, y1, x2, y2;
  gboolean     sparse;
};

static gboolean undo_pop_image  (GimpUndo            *undo,
                                 GimpUndoMode         undo_mode,
                                 GimpUndoAccumulator *accum);
static void     undo_free_image (GimpUndo            *undo,
                                 GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_image (GimpImage    *gimage,
                            const gchar  *undo_desc,
                            GimpDrawable *drawable,
                            gint          x1,
                            gint          y1,
                            gint          x2,
                            gint          y2)
{
  GimpUndo *new;
  GimpItem *item;
  gsize     size;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  item = GIMP_ITEM (drawable);

  x1 = CLAMP (x1, 0, gimp_item_width  (item));
  y1 = CLAMP (y1, 0, gimp_item_height (item));
  x2 = CLAMP (x2, 0, gimp_item_width  (item));
  y2 = CLAMP (y2, 0, gimp_item_height (item));

  size = sizeof (ImageUndo) + ((x2 - x1) * (y2 - y1) *
                               gimp_drawable_bytes (drawable));

  if ((new = gimp_image_undo_push_item (gimage, item,
                                        size, sizeof (ImageUndo),
                                        GIMP_UNDO_IMAGE, undo_desc,
                                        TRUE,
                                        undo_pop_image,
                                        undo_free_image)))
    {
      ImageUndo   *image_undo;
      TileManager *tiles;
      PixelRegion  srcPR, destPR;

      image_undo = new->data;

      /*  If we cannot create a new temp buf -- either because our
       *  parameters are degenerate or something else failed, simply
       *  return an unsuccessful push.
       */
      tiles = tile_manager_new ((x2 - x1), (y2 - y1),
				gimp_drawable_bytes (drawable));
      pixel_region_init (&srcPR, gimp_drawable_data (drawable),
			 x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, tiles,
			 0, 0, (x2 - x1), (y2 - y1), TRUE);
      copy_region (&srcPR, &destPR);

      /*  set the image undo structure  */
      image_undo->tiles  = tiles;
      image_undo->x1     = x1;
      image_undo->y1     = y1;
      image_undo->x2     = x2;
      image_undo->y2     = y2;
      image_undo->sparse = FALSE;

      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_image_undo_push_image_mod (GimpImage    *gimage,
                                const gchar  *undo_desc,
                                GimpDrawable *drawable,
                                gint          x1,
                                gint          y1,
                                gint          x2,
                                gint          y2,
                                TileManager  *tiles,
                                gboolean      sparse)
{
  GimpUndo *new;
  GimpItem *item;
  gsize     size;
  gint      dwidth, dheight;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (tiles != NULL, FALSE);

  item = GIMP_ITEM (drawable);

  dwidth  = gimp_item_width  (item);
  dheight = gimp_item_height (item);

  x1 = CLAMP (x1, 0, dwidth);
  y1 = CLAMP (y1, 0, dheight);
  x2 = CLAMP (x2, 0, dwidth);
  y2 = CLAMP (y2, 0, dheight);

  size = sizeof (ImageUndo) + tile_manager_get_memsize (tiles);

  if ((new = gimp_image_undo_push_item (gimage, item,
                                        size, sizeof (ImageUndo),
                                        GIMP_UNDO_IMAGE_MOD, undo_desc,
                                        TRUE,
                                        undo_pop_image,
                                        undo_free_image)))
    {
      ImageUndo *image_undo;

      image_undo = new->data;

      image_undo->tiles  = tile_manager_ref (tiles);
      image_undo->x1     = x1;
      image_undo->y1     = y1;
      image_undo->x2     = x2;
      image_undo->y2     = y2;
      image_undo->sparse = sparse;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_image (GimpUndo            *undo,
                GimpUndoMode         undo_mode,
                GimpUndoAccumulator *accum)
{
  ImageUndo    *image_undo;
  GimpDrawable *drawable;
  TileManager  *tiles;
  PixelRegion   PR1, PR2;
  gint          x, y;
  gint          w, h;

  image_undo = (ImageUndo *) undo->data;

  drawable = GIMP_DRAWABLE (GIMP_ITEM_UNDO (undo)->item);

  tiles = image_undo->tiles;

  /*  some useful values  */
  x = image_undo->x1;
  y = image_undo->y1;

  if (GIMP_IS_CHANNEL (drawable))
    gimp_drawable_invalidate_boundary (drawable);

  if (image_undo->sparse == FALSE)
    {
      w = tile_manager_width (tiles);
      h = tile_manager_height (tiles);

      pixel_region_init (&PR1, tiles,
			 0, 0, w, h, TRUE);
      pixel_region_init (&PR2, gimp_drawable_data (drawable),
			 x, y, w, h, TRUE);

      /*  swap the regions  */
      swap_region (&PR1, &PR2);
    }
  else
    {
      Tile *src_tile;
      Tile *dest_tile;
      gint  i, j;

      w = image_undo->x2 - image_undo->x1;
      h = image_undo->y2 - image_undo->y1;

      for (i = y; i < image_undo->y2; i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
	{
	  for (j = x; j < image_undo->x2; j += (TILE_WIDTH - (j % TILE_WIDTH)))
	    {
	      src_tile = tile_manager_get_tile (tiles, j, i, FALSE, FALSE);

	      if (tile_is_valid (src_tile) == TRUE)
		{
		  /* swap tiles, not pixels! */

		  src_tile = tile_manager_get_tile (tiles,
                                                    j, i, TRUE, FALSE /*TRUE*/);
		  dest_tile = tile_manager_get_tile (gimp_drawable_data (drawable), j, i, TRUE, FALSE /* TRUE */);

		  tile_manager_map_tile (tiles,
                                         j, i, dest_tile);
		  tile_manager_map_tile (gimp_drawable_data (drawable),
                                         j, i, src_tile);
#if 0
		  swap_pixels (tile_data_pointer (src_tile, 0, 0),
			       tile_data_pointer (dest_tile, 0, 0),
			       tile_size (src_tile));
#endif

		  tile_release (dest_tile, FALSE /* TRUE */);
		  tile_release (src_tile, FALSE /* TRUE */);
		}
	    }
	}
    }

  if (GIMP_IS_CHANNEL (drawable))
    GIMP_CHANNEL (drawable)->bounds_known = FALSE;

  gimp_drawable_update (drawable, x, y, w, h);

  return TRUE;
}

static void
undo_free_image (GimpUndo     *undo,
                 GimpUndoMode  undo_mode)
{
  ImageUndo *image_undo;

  image_undo = (ImageUndo *) undo->data;

  tile_manager_unref (image_undo->tiles);
  g_free (image_undo);
}


/*********************/
/*  Image Type Undo  */
/*********************/

typedef struct _ImageTypeUndo ImageTypeUndo;

struct _ImageTypeUndo
{
  GimpImageBaseType base_type;
};

static gboolean undo_pop_image_type  (GimpUndo            *undo,
                                      GimpUndoMode         undo_mode,
                                      GimpUndoAccumulator *accum);
static void     undo_free_image_type (GimpUndo            *undo,
                                      GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_image_type (GimpImage   *gimage,
                                 const gchar *undo_desc)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (ImageTypeUndo),
                                   sizeof (ImageTypeUndo),
                                   GIMP_UNDO_IMAGE_TYPE, undo_desc,
                                   TRUE,
                                   undo_pop_image_type,
                                   undo_free_image_type)))
    {
      ImageTypeUndo *itu;

      itu = new->data;

      itu->base_type = gimage->base_type;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_image_type (GimpUndo            *undo,
                     GimpUndoMode         undo_mode,
                     GimpUndoAccumulator *accum)
{
  ImageTypeUndo     *itu;
  GimpImageBaseType  tmp;

  itu = (ImageTypeUndo *) undo->data;

  tmp = itu->base_type;
  itu->base_type = undo->gimage->base_type;
  undo->gimage->base_type = tmp;

  gimp_image_projection_allocate (undo->gimage);
  gimp_image_colormap_changed (undo->gimage, -1);

  if (itu->base_type != undo->gimage->base_type)
    accum->mode_changed = TRUE;

  return TRUE;
}

static void
undo_free_image_type (GimpUndo     *undo,
                      GimpUndoMode  undo_mode)
{
  g_free (undo->data);
}


/*********************/
/*  Image Size Undo  */
/*********************/

typedef struct _ImageSizeUndo ImageSizeUndo;

struct _ImageSizeUndo
{
  gint width;
  gint height;
};

static gboolean undo_pop_image_size  (GimpUndo            *undo,
                                      GimpUndoMode         undo_mode,
                                      GimpUndoAccumulator *accum);
static void     undo_free_image_size (GimpUndo            *undo,
                                      GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_image_size (GimpImage   *gimage,
                                 const gchar *undo_desc)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (ImageSizeUndo),
                                   sizeof (ImageSizeUndo),
                                   GIMP_UNDO_IMAGE_SIZE, undo_desc,
                                   TRUE,
                                   undo_pop_image_size,
                                   undo_free_image_size)))
    {
      ImageSizeUndo *isu;

      isu = new->data;

      isu->width  = gimage->width;
      isu->height = gimage->height;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_image_size (GimpUndo            *undo,
                     GimpUndoMode         undo_mode,
                     GimpUndoAccumulator *accum)
{
  ImageSizeUndo *isu;
  gint           width;
  gint           height;

  isu = (ImageSizeUndo *) undo->data;

  width  = isu->width;
  height = isu->height;

  isu->width  = undo->gimage->width;
  isu->height = undo->gimage->height;

  undo->gimage->width  = width;
  undo->gimage->height = height;

  gimp_image_projection_allocate (undo->gimage);
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (gimp_image_get_mask (undo->gimage)));

  if (undo->gimage->width  != isu->width ||
      undo->gimage->height != isu->height)
    accum->size_changed = TRUE;

  return TRUE;
}

static void
undo_free_image_size (GimpUndo      *undo,
                      GimpUndoMode   undo_mode)
{
  g_free (undo->data);
}


/***************************/
/*  Image Resolution Undo  */
/***************************/

typedef struct _ResolutionUndo ResolutionUndo;

struct _ResolutionUndo
{
  gdouble  xres;
  gdouble  yres;
  GimpUnit unit;
};

static gboolean undo_pop_image_resolution  (GimpUndo            *undo,
                                            GimpUndoMode         undo_mode,
                                            GimpUndoAccumulator *accum);
static void     undo_free_image_resolution (GimpUndo            *undo,
                                            GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_image_resolution (GimpImage   *gimage,
                                       const gchar *undo_desc)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (ResolutionUndo),
                                   sizeof (ResolutionUndo),
                                   GIMP_UNDO_IMAGE_RESOLUTION, undo_desc,
                                   TRUE,
                                   undo_pop_image_resolution,
                                   undo_free_image_resolution)))
    {
      ResolutionUndo *ru;

      ru = new->data;

      ru->xres = gimage->xresolution;
      ru->yres = gimage->yresolution;
      ru->unit = gimage->unit;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_image_resolution (GimpUndo            *undo,
                           GimpUndoMode         undo_mode,
                           GimpUndoAccumulator *accum)
{
  ResolutionUndo *ru;

  ru = (ResolutionUndo *) undo->data;

  if (ABS (ru->xres - undo->gimage->xresolution) >= 1e-5 ||
      ABS (ru->yres - undo->gimage->yresolution) >= 1e-5)
    {
      gdouble xres;
      gdouble yres;

      xres = undo->gimage->xresolution;
      yres = undo->gimage->yresolution;

      undo->gimage->xresolution = ru->xres;
      undo->gimage->yresolution = ru->yres;

      ru->xres = xres;
      ru->yres = yres;

      accum->resolution_changed = TRUE;
    }

  if (ru->unit != undo->gimage->unit)
    {
      GimpUnit unit;

      unit = undo->gimage->unit;
      undo->gimage->unit = ru->unit;
      ru->unit = unit;

      accum->unit_changed = TRUE;
    }

  return TRUE;
}

static void
undo_free_image_resolution (GimpUndo      *undo,
                            GimpUndoMode   undo_mode)
{
  g_free (undo->data);
}


/****************/
/*  Qmask Undo  */
/****************/

typedef struct _QmaskUndo QmaskUndo;

struct _QmaskUndo
{
  gboolean qmask_state;
};

static gboolean undo_pop_image_qmask  (GimpUndo            *undo,
                                       GimpUndoMode         undo_mode,
                                       GimpUndoAccumulator *accum);
static void     undo_free_image_qmask (GimpUndo            *undo,
                                       GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_image_qmask (GimpImage   *gimage,
                                  const gchar *undo_desc)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (QmaskUndo),
                                   sizeof (QmaskUndo),
                                   GIMP_UNDO_IMAGE_QMASK, undo_desc,
                                   TRUE,
                                   undo_pop_image_qmask,
                                   undo_free_image_qmask)))
    {
      QmaskUndo *qu;

      qu = new->data;

      qu->qmask_state = gimage->qmask_state;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_image_qmask (GimpUndo            *undo,
                      GimpUndoMode         undo_mode,
                      GimpUndoAccumulator *accum)
{
  QmaskUndo *qu = undo->data;
  gboolean   tmp;

  tmp = undo->gimage->qmask_state;
  undo->gimage->qmask_state = qu->qmask_state;
  qu->qmask_state = tmp;

  accum->qmask_changed = TRUE;

  return TRUE;
}

static void
undo_free_image_qmask (GimpUndo     *undo,
                       GimpUndoMode  undo_mode)
{
  g_free (undo->data);
}


/****************/
/*  Grid Undo   */
/****************/

typedef struct _GridUndo GridUndo;

struct _GridUndo
{
  GimpGrid *grid;
};

static gboolean undo_pop_image_grid  (GimpUndo            *undo,
                                      GimpUndoMode         undo_mode,
                                      GimpUndoAccumulator *accum);
static void     undo_free_image_grid (GimpUndo            *undo,
                                      GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_image_grid (GimpImage   *gimage,
                                 const gchar *undo_desc,
                                 GimpGrid    *grid)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_GRID (grid), FALSE);

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (GridUndo),
                                   sizeof (GridUndo),
                                   GIMP_UNDO_IMAGE_GRID, undo_desc,
                                   TRUE,
                                   undo_pop_image_grid,
                                   undo_free_image_grid)))
    {
      GridUndo *gu = new->data;

      gu->grid = gimp_config_duplicate (GIMP_CONFIG (grid));

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_image_grid (GimpUndo            *undo,
                     GimpUndoMode         undo_mode,
                     GimpUndoAccumulator *accum)
{
  GridUndo *gu = undo->data;
  GimpGrid *grid;

  grid = gimp_config_duplicate (GIMP_CONFIG (undo->gimage->grid));

  gimp_image_set_grid (undo->gimage, gu->grid, FALSE);

  g_object_unref (gu->grid);
  gu->grid = grid;

  return TRUE;
}

static void
undo_free_image_grid (GimpUndo     *undo,
                      GimpUndoMode  undo_mode)
{
  GridUndo *gu = undo->data;

  if (gu->grid)
    g_object_unref (gu->grid);

  g_free (gu);
}


/****************/
/*  Guide Undo  */
/****************/

typedef struct _GuideUndo GuideUndo;

struct _GuideUndo
{
  GimpGuide           *guide;
  gint                 position;
  GimpOrientationType  orientation;
};

static gboolean undo_pop_image_guide  (GimpUndo            *undo,
                                       GimpUndoMode         undo_mode,
                                       GimpUndoAccumulator *accum);
static void     undo_free_image_guide (GimpUndo            *undo,
                                       GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_image_guide (GimpImage   *gimage,
                                  const gchar *undo_desc,
                                  GimpGuide   *guide)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (guide != NULL, FALSE);

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (GuideUndo),
                                   sizeof (GuideUndo),
                                   GIMP_UNDO_IMAGE_GUIDE, undo_desc,
                                   TRUE,
                                   undo_pop_image_guide,
                                   undo_free_image_guide)))
    {
      GuideUndo *gu;

      gu = new->data;

      gu->guide       = gimp_image_guide_ref (guide);
      gu->position    = guide->position;
      gu->orientation = guide->orientation;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_image_guide (GimpUndo            *undo,
                      GimpUndoMode         undo_mode,
                      GimpUndoAccumulator *accum)
{
  GuideUndo           *gu = undo->data;
  gint                 old_position;
  GimpOrientationType  old_orientation;

  old_position    = gu->guide->position;
  old_orientation = gu->guide->orientation;

  if (gu->guide->position == -1)
    {
      undo->gimage->guides = g_list_prepend (undo->gimage->guides, gu->guide);
      gu->guide->position = gu->position;
      gimp_image_guide_ref (gu->guide);
      gimp_image_update_guide (undo->gimage, gu->guide);
    }
  else if (gu->position == -1)
    {
      gimp_image_remove_guide (undo->gimage, gu->guide, FALSE);
    }
  else
    {
      gimp_image_update_guide (undo->gimage, gu->guide);
      gu->guide->position = gu->position;
      gimp_image_update_guide (undo->gimage, gu->guide);
    }

  gu->guide->orientation = gu->orientation;

  gu->position    = old_position;
  gu->orientation = old_orientation;

  return TRUE;
}

static void
undo_free_image_guide (GimpUndo     *undo,
                       GimpUndoMode  undo_mode)
{
  GuideUndo *gu = undo->data;

  gimp_image_guide_unref (gu->guide);
  g_free (gu);
}


/*******************/
/*  Colormap Undo  */
/*******************/

typedef struct _ColormapUndo ColormapUndo;

struct _ColormapUndo
{
  guchar cmap[GIMP_IMAGE_COLORMAP_SIZE];
  gint   num_colors;
};

static gboolean undo_pop_image_colormap  (GimpUndo            *undo,
                                          GimpUndoMode         undo_mode,
                                          GimpUndoAccumulator *accum);
static void     undo_free_image_colormap (GimpUndo            *undo,
                                          GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_image_colormap (GimpImage   *gimage,
                                     const gchar *undo_desc)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (gimp_image_get_colormap (gimage) != NULL, FALSE);

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (ColormapUndo),
                                   sizeof (ColormapUndo),
                                   GIMP_UNDO_IMAGE_COLORMAP, undo_desc,
                                   TRUE,
                                   undo_pop_image_colormap,
                                   undo_free_image_colormap)))
    {
      ColormapUndo *cu;

      cu = new->data;

      cu->num_colors = gimp_image_get_colormap_size (gimage);
      memcpy (cu->cmap, gimp_image_get_colormap (gimage), cu->num_colors * 3);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_image_colormap (GimpUndo            *undo,
                         GimpUndoMode         undo_mode,
                         GimpUndoAccumulator *accum)
{
  ColormapUndo *cu;
  guchar        cmap[GIMP_IMAGE_COLORMAP_SIZE];
  gint          num_colors;

  cu = (ColormapUndo *) undo->data;

  num_colors = gimp_image_get_colormap_size (undo->gimage);
  memcpy (cmap, gimp_image_get_colormap (undo->gimage), num_colors * 3);

  gimp_image_set_colormap (undo->gimage, cu->cmap, cu->num_colors, FALSE);

  memcpy (cu->cmap, cmap, sizeof (cmap));
  cu->num_colors = num_colors;

  return TRUE;
}

static void
undo_free_image_colormap (GimpUndo     *undo,
                          GimpUndoMode  undo_mode)
{
  g_free (undo->data);
}


/***************/
/*  Mask Undo  */
/***************/

typedef struct _MaskUndo MaskUndo;

struct _MaskUndo
{
  TileManager *tiles;  /*  the actual mask  */
  gint         x, y;   /*  offsets          */
};

static gboolean undo_pop_mask  (GimpUndo            *undo,
                                GimpUndoMode         undo_mode,
                                GimpUndoAccumulator *accum);
static void     undo_free_mask (GimpUndo            *undo,
                                GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_mask (GimpImage   *gimage,
                           const gchar *undo_desc,
                           GimpChannel *mask)
{
  TileManager *undo_tiles;
  gint         x1, y1, x2, y2;
  GimpUndo    *new;
  gsize        size;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (mask), FALSE);

  if (gimp_channel_bounds (mask, &x1, &y1, &x2, &y2))
    {
      PixelRegion srcPR, destPR;

      undo_tiles = tile_manager_new ((x2 - x1), (y2 - y1), 1);

      pixel_region_init (&srcPR, GIMP_DRAWABLE (mask)->tiles,
                         x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, undo_tiles,
                         0, 0, (x2 - x1), (y2 - y1), TRUE);

      copy_region (&srcPR, &destPR);
    }
  else
    undo_tiles = NULL;

  size = sizeof (MaskUndo);

  if (undo_tiles)
    size += tile_manager_get_memsize (undo_tiles);

  if ((new = gimp_image_undo_push_item (gimage, GIMP_ITEM (mask),
                                        size, sizeof (MaskUndo),
                                        GIMP_UNDO_MASK, undo_desc,
                                        FALSE,
                                        undo_pop_mask,
                                        undo_free_mask)))
    {
      MaskUndo *mu;

      mu = new->data;

      mu->tiles = undo_tiles;
      mu->x     = x1;
      mu->y     = y1;

      return TRUE;
    }

  if (undo_tiles)
    tile_manager_unref (undo_tiles);

  return FALSE;
}

static gboolean
undo_pop_mask (GimpUndo            *undo,
               GimpUndoMode         undo_mode,
               GimpUndoAccumulator *accum)
{
  MaskUndo    *mu;
  GimpChannel *channel;
  TileManager *new_tiles;
  PixelRegion  srcPR, destPR;
  gint         x1, y1, x2, y2;
  gint         width  = 0;
  gint         height = 0;
  guchar       empty  = 0;

  mu = (MaskUndo *) undo->data;

  channel = GIMP_CHANNEL (GIMP_ITEM_UNDO (undo)->item);

  if (gimp_channel_bounds (channel, &x1, &y1, &x2, &y2))
    {
      new_tiles = tile_manager_new ((x2 - x1), (y2 - y1), 1);

      pixel_region_init (&srcPR, GIMP_DRAWABLE (channel)->tiles,
                         x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, new_tiles,
			 0, 0, (x2 - x1), (y2 - y1), TRUE);

      copy_region (&srcPR, &destPR);

      pixel_region_init (&srcPR, GIMP_DRAWABLE (channel)->tiles,
			 x1, y1, (x2 - x1), (y2 - y1), TRUE);

      color_region (&srcPR, &empty);
    }
  else
    {
      new_tiles = NULL;
    }

  if (mu->tiles)
    {
      width  = tile_manager_width (mu->tiles);
      height = tile_manager_height (mu->tiles);

      pixel_region_init (&srcPR, mu->tiles,
			 0, 0, width, height, FALSE);
      pixel_region_init (&destPR, GIMP_DRAWABLE (channel)->tiles,
			 mu->x, mu->y, width, height, TRUE);

      copy_region (&srcPR, &destPR);

      tile_manager_unref (mu->tiles);
    }

  /* invalidate the current bounds and boundary of the mask */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (channel));

  if (mu->tiles)
    {
      channel->empty = FALSE;
      channel->x1    = mu->x;
      channel->y1    = mu->y;
      channel->x2    = mu->x + width;
      channel->y2    = mu->y + height;
    }
  else
    {
      channel->empty = TRUE;
      channel->x1    = 0;
      channel->y1    = 0;
      channel->x2    = GIMP_ITEM (channel)->width;
      channel->y2    = GIMP_ITEM (channel)->height;
    }

  /* we know the bounds */
  channel->bounds_known = TRUE;

  /*  set the new mask undo parameters  */
  mu->tiles = new_tiles;
  mu->x     = x1;
  mu->y     = y1;

  gimp_drawable_update (GIMP_DRAWABLE (channel),
                        0, 0,
                        GIMP_ITEM (channel)->width,
                        GIMP_ITEM (channel)->height);

  return TRUE;
}

static void
undo_free_mask (GimpUndo     *undo,
                GimpUndoMode  undo_mode)
{
  MaskUndo *mu;

  mu = (MaskUndo *) undo->data;

  if (mu->tiles)
    tile_manager_unref (mu->tiles);

  g_free (mu);
}


/**********************/
/*  Item Rename Undo  */
/**********************/

typedef struct _ItemRenameUndo ItemRenameUndo;

struct _ItemRenameUndo
{
  gchar *old_name;
};

static gboolean undo_pop_item_rename  (GimpUndo            *undo,
                                       GimpUndoMode         undo_mode,
                                       GimpUndoAccumulator *accum);
static void     undo_free_item_rename (GimpUndo            *undo,
                                       GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_item_rename (GimpImage   *gimage,
                                  const gchar *undo_desc,
                                  GimpItem    *item)
{
  GimpUndo    *new;
  gsize        size;
  const gchar *name;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  name = gimp_object_get_name (GIMP_OBJECT (item));

  size = sizeof (ItemRenameUndo) + strlen (name) + 1;

  if ((new = gimp_image_undo_push_item (gimage, item,
                                        size, sizeof (ItemRenameUndo),
                                        GIMP_UNDO_ITEM_RENAME, undo_desc,
                                        TRUE,
                                        undo_pop_item_rename,
                                        undo_free_item_rename)))
    {
      ItemRenameUndo *iru;

      iru = new->data;

      iru->old_name = g_strdup (name);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_item_rename (GimpUndo            *undo,
                      GimpUndoMode         undo_mode,
                      GimpUndoAccumulator *accum)
{
  ItemRenameUndo *iru;
  GimpItem       *item;
  gchar          *tmp;

  iru = (ItemRenameUndo *) undo->data;

  item = GIMP_ITEM_UNDO (undo)->item;

  tmp = g_strdup (gimp_object_get_name (GIMP_OBJECT (item)));
  gimp_object_set_name (GIMP_OBJECT (item), iru->old_name);
  g_free (iru->old_name);
  iru->old_name = tmp;

  return TRUE;
}

static void
undo_free_item_rename (GimpUndo     *undo,
                       GimpUndoMode  undo_mode)
{
  ItemRenameUndo *iru;

  iru = (ItemRenameUndo *) undo->data;

  g_free (iru->old_name);
  g_free (iru);
}


/****************************/
/*  Item displacement Undo  */
/****************************/

typedef struct _ItemDisplaceUndo ItemDisplaceUndo;

struct _ItemDisplaceUndo
{
  gint old_offset_x;
  gint old_offset_y;
};

static gboolean undo_pop_item_displace  (GimpUndo            *undo,
                                         GimpUndoMode         undo_mode,
                                         GimpUndoAccumulator *accum);
static void     undo_free_item_displace (GimpUndo            *undo,
                                         GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_item_displace (GimpImage   *gimage,
                                    const gchar *undo_desc,
                                    GimpItem    *item)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  if ((new = gimp_image_undo_push_item (gimage, item,
                                        sizeof (ItemDisplaceUndo),
                                        sizeof (ItemDisplaceUndo),
                                        GIMP_UNDO_ITEM_DISPLACE, undo_desc,
                                        TRUE,
                                        undo_pop_item_displace,
                                        undo_free_item_displace)))
    {
      ItemDisplaceUndo *idu;

      idu = new->data;

      gimp_item_offsets (item, &idu->old_offset_x, &idu->old_offset_y);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_item_displace (GimpUndo            *undo,
                        GimpUndoMode         undo_mode,
                        GimpUndoAccumulator *accum)
{
  ItemDisplaceUndo *idu;
  GimpItem         *item;
  gint              offset_x;
  gint              offset_y;

  idu = (ItemDisplaceUndo *) undo->data;

  item = GIMP_ITEM_UNDO (undo)->item;

  gimp_item_offsets (item, &offset_x, &offset_y);

  gimp_item_translate (item,
                       idu->old_offset_x - offset_x,
                       idu->old_offset_y - offset_y,
                       FALSE);

  idu->old_offset_x = offset_x;
  idu->old_offset_y = offset_y;

  return TRUE;
}

static void
undo_free_item_displace (GimpUndo     *undo,
                         GimpUndoMode  undo_mode)
{
  ItemDisplaceUndo *idu;

  idu = (ItemDisplaceUndo *) undo->data;

  g_free (idu);
}


/******************************/
/*  Item Visibility Undo  */
/******************************/

typedef struct _ItemVisibilityUndo ItemVisibilityUndo;

struct _ItemVisibilityUndo
{
  gboolean old_visible;
};

static gboolean undo_pop_item_visibility  (GimpUndo            *undo,
                                           GimpUndoMode         undo_mode,
                                           GimpUndoAccumulator *accum);
static void     undo_free_item_visibility (GimpUndo            *undo,
                                           GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_item_visibility (GimpImage   *gimage,
                                      const gchar *undo_desc,
                                      GimpItem    *item)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  if ((new = gimp_image_undo_push_item (gimage, item,
                                        sizeof (ItemVisibilityUndo),
                                        sizeof (ItemVisibilityUndo),
                                        GIMP_UNDO_ITEM_VISIBILITY, undo_desc,
                                        TRUE,
                                        undo_pop_item_visibility,
                                        undo_free_item_visibility)))
    {
      ItemVisibilityUndo *ivu;

      ivu = new->data;

      ivu->old_visible = gimp_item_get_visible (item);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_item_visibility (GimpUndo            *undo,
                          GimpUndoMode         undo_mode,
                          GimpUndoAccumulator *accum)
{
  ItemVisibilityUndo *ivu;
  GimpItem           *item;
  gboolean            visible;

  ivu = (ItemVisibilityUndo *) undo->data;

  item = GIMP_ITEM_UNDO (undo)->item;

  visible = gimp_item_get_visible (item);
  gimp_item_set_visible (item, ivu->old_visible, FALSE);
  ivu->old_visible = visible;

  return TRUE;
}

static void
undo_free_item_visibility (GimpUndo     *undo,
                           GimpUndoMode  undo_mode)
{
  g_free (undo->data);
}


/**********************/
/*  Item linked Undo  */
/**********************/

typedef struct _ItemLinkedUndo ItemLinkedUndo;

struct _ItemLinkedUndo
{
  gboolean old_linked;
};

static gboolean undo_pop_item_linked  (GimpUndo            *undo,
                                       GimpUndoMode         undo_mode,
                                       GimpUndoAccumulator *accum);
static void     undo_free_item_linked (GimpUndo            *undo,
                                       GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_item_linked (GimpImage   *gimage,
                                  const gchar *undo_desc,
                                  GimpItem    *item)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  if ((new = gimp_image_undo_push_item (gimage, item,
                                        sizeof (ItemLinkedUndo),
                                        sizeof (ItemLinkedUndo),
                                        GIMP_UNDO_ITEM_LINKED, undo_desc,
                                        TRUE,
                                        undo_pop_item_linked,
                                        undo_free_item_linked)))
    {
      ItemLinkedUndo *ilu;

      ilu = new->data;

      ilu->old_linked = gimp_item_get_linked (item);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_item_linked (GimpUndo            *undo,
                      GimpUndoMode         undo_mode,
                      GimpUndoAccumulator *accum)
{
  ItemLinkedUndo *ilu;
  GimpItem       *item;
  gboolean        linked;

  ilu = (ItemLinkedUndo *) undo->data;

  item = GIMP_ITEM_UNDO (undo)->item;

  linked = gimp_item_get_linked (item);
  gimp_item_set_linked (item, ilu->old_linked, FALSE);
  ilu->old_linked = linked;

  return TRUE;
}

static void
undo_free_item_linked (GimpUndo     *undo,
                       GimpUndoMode  undo_mode)
{
  g_free (undo->data);
}


/***************************/
/*  Layer Add/Remove Undo  */
/***************************/

typedef struct _LayerUndo LayerUndo;

struct _LayerUndo
{
  gint       prev_position;   /*  former position in list  */
  GimpLayer *prev_layer;      /*  previous active layer    */
};

static gboolean undo_push_layer (GimpImage           *gimage,
                                 const gchar         *undo_desc,
                                 GimpUndoType         type,
                                 GimpLayer           *layer,
                                 gint                 prev_position,
                                 GimpLayer           *prev_layer);
static gboolean undo_pop_layer  (GimpUndo            *undo,
                                 GimpUndoMode         undo_mode,
                                 GimpUndoAccumulator *accum);
static void     undo_free_layer (GimpUndo            *undo,
                                 GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_layer_add (GimpImage   *gimage,
                                const gchar *undo_desc,
                                GimpLayer   *layer,
                                gint         prev_position,
                                GimpLayer   *prev_layer)
{
  return undo_push_layer (gimage, undo_desc, GIMP_UNDO_LAYER_ADD,
                          layer, prev_position, prev_layer);
}

gboolean
gimp_image_undo_push_layer_remove (GimpImage   *gimage,
                                   const gchar *undo_desc,
                                   GimpLayer   *layer,
                                   gint         prev_position,
                                   GimpLayer   *prev_layer)
{
  return undo_push_layer (gimage, undo_desc, GIMP_UNDO_LAYER_REMOVE,
                          layer, prev_position, prev_layer);
}

static gboolean
undo_push_layer (GimpImage    *gimage,
                 const gchar  *undo_desc,
		 GimpUndoType  type,
                 GimpLayer    *layer,
                 gint          prev_position,
                 GimpLayer    *prev_layer)
{
  GimpUndo *new;
  gsize     size;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (prev_layer == NULL || GIMP_IS_LAYER (prev_layer),
                        FALSE);
  g_return_val_if_fail (type == GIMP_UNDO_LAYER_ADD ||
			type == GIMP_UNDO_LAYER_REMOVE, FALSE);

  size = sizeof (LayerUndo);

  if (type == GIMP_UNDO_LAYER_REMOVE)
    size += gimp_object_get_memsize (GIMP_OBJECT (layer), NULL);

  if ((new = gimp_image_undo_push_item (gimage, GIMP_ITEM (layer),
                                        size, sizeof (LayerUndo),
                                        type, undo_desc,
                                        TRUE,
                                        undo_pop_layer,
                                        undo_free_layer)))
    {
      LayerUndo *lu;

      lu = new->data;

      lu->prev_position = prev_position;
      lu->prev_layer    = prev_layer;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_layer (GimpUndo            *undo,
                GimpUndoMode         undo_mode,
                GimpUndoAccumulator *accum)
{
  LayerUndo *lu;
  GimpLayer *layer;

  lu = (LayerUndo *) undo->data;

  layer = GIMP_LAYER (GIMP_ITEM_UNDO (undo)->item);

  if ((undo_mode       == GIMP_UNDO_MODE_UNDO &&
       undo->undo_type == GIMP_UNDO_LAYER_ADD) ||
      (undo_mode       == GIMP_UNDO_MODE_REDO &&
       undo->undo_type == GIMP_UNDO_LAYER_REMOVE))
    {
      /*  remove layer  */

      g_print ("undo_pop_layer: taking ownership, size += %u\n",
               gimp_object_get_memsize (GIMP_OBJECT (layer), NULL));

      undo->size += gimp_object_get_memsize (GIMP_OBJECT (layer), NULL);

      /*  Make sure we're not caching any old selection info  */
      gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (layer));

      /*  record the current position  */
      lu->prev_position = gimp_image_get_layer_index (undo->gimage, layer);

      /*  remove the layer  */
      gimp_container_remove (undo->gimage->layers, GIMP_OBJECT (layer));
      undo->gimage->layer_stack = g_slist_remove (undo->gimage->layer_stack,
                                                  layer);

      /*  reset the gimage values  */
      if (gimp_layer_is_floating_sel (layer))
	{
	  undo->gimage->floating_sel = NULL;
	  /*  reset the old drawable  */
	  floating_sel_reset (layer);

	  gimp_image_floating_selection_changed (undo->gimage);
	}

      if (layer == gimp_image_get_active_layer (undo->gimage))
        {
          if (lu->prev_layer)
            {
              gimp_image_set_active_layer (undo->gimage, lu->prev_layer);
            }
          else if (undo->gimage->layer_stack)
            {
              gimp_image_set_active_layer (undo->gimage,
                                           undo->gimage->layer_stack->data);
            }
          else
            {
              undo->gimage->active_layer = NULL;
              gimp_image_active_layer_changed (undo->gimage);
            }
        }

      gimp_item_removed (GIMP_ITEM (layer));

      if (gimp_container_num_children (undo->gimage->layers) == 1 &&
          ! gimp_drawable_has_alpha (GIMP_LIST (undo->gimage->layers)->list->data))
        {
          accum->alpha_changed = TRUE;
        }
    }
  else
    {
      /*  restore layer  */

      g_print ("undo_pop_layer: dropping ownership, size -= %u\n",
               gimp_object_get_memsize (GIMP_OBJECT (layer), NULL));

      undo->size -= gimp_object_get_memsize (GIMP_OBJECT (layer), NULL);

      /*  record the active layer  */
      lu->prev_layer = gimp_image_get_active_layer (undo->gimage);

      /*  hide the current selection--for all views  */
      if (lu->prev_layer)
	gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (lu->prev_layer));

      /*  if this is a floating selection, set the fs pointer  */
      if (gimp_layer_is_floating_sel (layer))
	undo->gimage->floating_sel = layer;

      if (gimp_container_num_children (undo->gimage->layers) == 1 &&
          ! gimp_drawable_has_alpha (GIMP_LIST (undo->gimage->layers)->list->data))
        {
          accum->alpha_changed = TRUE;
        }

      /*  add the new layer  */
      gimp_container_insert (undo->gimage->layers,
			     GIMP_OBJECT (layer), lu->prev_position);
      gimp_image_set_active_layer (undo->gimage, layer);

      if (gimp_layer_is_floating_sel (layer))
	gimp_image_floating_selection_changed (undo->gimage);
    }

  return TRUE;
}

static void
undo_free_layer (GimpUndo     *undo,
                 GimpUndoMode  undo_mode)
{
  g_free (undo->data);
}


/********************/
/*  Layer Mod Undo  */
/********************/

typedef struct _LayerModUndo LayerModUndo;

struct _LayerModUndo
{
  TileManager   *tiles;
  GimpImageType  type;
  gint           offset_x;
  gint		 offset_y;
};

static gboolean undo_pop_layer_mod  (GimpUndo            *undo,
                                     GimpUndoMode         undo_mode,
                                     GimpUndoAccumulator *accum);
static void     undo_free_layer_mod (GimpUndo            *undo,
                                     GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_layer_mod (GimpImage   *gimage,
                                const gchar *undo_desc,
                                GimpLayer   *layer)
{
  GimpUndo    *new;
  TileManager *tiles;
  gsize        size;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  tiles = GIMP_DRAWABLE (layer)->tiles;

  size = sizeof (LayerModUndo) + tile_manager_get_memsize (tiles);

  if ((new = gimp_image_undo_push_item (gimage, GIMP_ITEM (layer),
                                        size, sizeof (LayerModUndo),
                                        GIMP_UNDO_LAYER_MOD, undo_desc,
                                        TRUE,
                                        undo_pop_layer_mod,
                                        undo_free_layer_mod)))
    {
      LayerModUndo *lmu;

      lmu = new->data;

      lmu->tiles    = tile_manager_ref (tiles);
      lmu->type     = GIMP_DRAWABLE (layer)->type;
      lmu->offset_x = GIMP_ITEM (layer)->offset_x;
      lmu->offset_y = GIMP_ITEM (layer)->offset_y;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_layer_mod (GimpUndo            *undo,
                    GimpUndoMode         undo_mode,
                    GimpUndoAccumulator *accum)
{
  LayerModUndo  *lmu;
  GimpLayer     *layer;
  GimpImageType  layer_type;
  gint           offset_x, offset_y;
  TileManager   *tiles;

  lmu = (LayerModUndo *) undo->data;

  layer = GIMP_LAYER (GIMP_ITEM_UNDO (undo)->item);

  /*  Issue the first update  */
  gimp_drawable_update (GIMP_DRAWABLE (layer),
                        0, 0,
                        GIMP_ITEM (layer)->width,
                        GIMP_ITEM (layer)->height);

  tiles      = lmu->tiles;
  layer_type = lmu->type;
  offset_x   = lmu->offset_x;
  offset_y   = lmu->offset_y;

  lmu->tiles    = GIMP_DRAWABLE (layer)->tiles;
  lmu->type     = GIMP_DRAWABLE (layer)->type;
  lmu->offset_x = GIMP_ITEM (layer)->offset_x;
  lmu->offset_y = GIMP_ITEM (layer)->offset_y;

  /*  Make sure we're not caching any old selection info  */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (layer));

  GIMP_DRAWABLE (layer)->tiles     = tiles;
  GIMP_DRAWABLE (layer)->bytes     = tile_manager_bpp (tiles);
  GIMP_DRAWABLE (layer)->type      = layer_type;
  GIMP_DRAWABLE (layer)->has_alpha = GIMP_IMAGE_TYPE_HAS_ALPHA (layer_type);

  GIMP_ITEM (layer)->width    = tile_manager_width (tiles);
  GIMP_ITEM (layer)->height   = tile_manager_height (tiles);
  GIMP_ITEM (layer)->offset_x = offset_x;
  GIMP_ITEM (layer)->offset_y = offset_y;

  if (layer->mask)
    {
      GIMP_ITEM (layer->mask)->offset_x = offset_x;
      GIMP_ITEM (layer->mask)->offset_y = offset_y;
    }

  if (GIMP_IMAGE_TYPE_HAS_ALPHA (GIMP_DRAWABLE (layer)->type) !=
      GIMP_IMAGE_TYPE_HAS_ALPHA (lmu->type))
    {
      gimp_drawable_alpha_changed (GIMP_DRAWABLE (layer));

      if (undo->gimage->layers->num_children == 1)
        gimp_image_alpha_changed (undo->gimage);
    }

  if (GIMP_ITEM (layer)->width  != tile_manager_width  (lmu->tiles) ||
      GIMP_ITEM (layer)->height != tile_manager_height (lmu->tiles))
    {
      gimp_viewable_size_changed (GIMP_VIEWABLE (layer));
    }

  /*  Issue the second update  */
  gimp_drawable_update (GIMP_DRAWABLE (layer),
			0, 0,
			GIMP_ITEM (layer)->width,
			GIMP_ITEM (layer)->height);

  return TRUE;
}

static void
undo_free_layer_mod (GimpUndo     *undo,
                     GimpUndoMode  undo_mode)
{
  LayerModUndo *lmu;

  lmu = (LayerModUndo *) undo->data;

  tile_manager_unref (lmu->tiles);
  g_free (lmu);
}


/********************************/
/*  Layer Mask Add/Remove Undo  */
/********************************/

typedef struct _LayerMaskUndo LayerMaskUndo;

struct _LayerMaskUndo
{
  GimpLayerMask *mask;
};

static gboolean undo_push_layer_mask (GimpImage           *gimage,
                                      const gchar         *undo_desc,
                                      GimpUndoType         type,
                                      GimpLayer           *layer,
                                      GimpLayerMask       *mask);
static gboolean undo_pop_layer_mask  (GimpUndo            *undo,
                                      GimpUndoMode         undo_mode,
                                      GimpUndoAccumulator *accum);
static void     undo_free_layer_mask (GimpUndo            *undo,
                                      GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_layer_mask_add (GimpImage     *gimage,
                                     const gchar   *undo_desc,
                                     GimpLayer     *layer,
                                     GimpLayerMask *mask)
{
  return undo_push_layer_mask (gimage, undo_desc, GIMP_UNDO_LAYER_MASK_ADD,
                               layer, mask);
}

gboolean
gimp_image_undo_push_layer_mask_remove (GimpImage     *gimage,
                                        const gchar   *undo_desc,
                                        GimpLayer     *layer,
                                        GimpLayerMask *mask)
{
  return undo_push_layer_mask (gimage, undo_desc, GIMP_UNDO_LAYER_MASK_REMOVE,
                               layer, mask);
}

static gboolean
undo_push_layer_mask (GimpImage     *gimage,
                      const gchar   *undo_desc,
		      GimpUndoType   type,
                      GimpLayer     *layer,
                      GimpLayerMask *mask)
{
  GimpUndo *new;
  gsize     size;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER_MASK (mask), FALSE);
  g_return_val_if_fail (type == GIMP_UNDO_LAYER_MASK_ADD ||
			type == GIMP_UNDO_LAYER_MASK_REMOVE, FALSE);

  size = sizeof (LayerMaskUndo);

  if (type == GIMP_UNDO_LAYER_MASK_REMOVE)
    size += gimp_object_get_memsize (GIMP_OBJECT (mask), NULL);

  if ((new = gimp_image_undo_push_item (gimage, GIMP_ITEM (layer),
                                        size,
                                        sizeof (LayerMaskUndo),
                                        type, undo_desc,
                                        TRUE,
                                        undo_pop_layer_mask,
                                        undo_free_layer_mask)))
    {
      LayerMaskUndo *lmu;

      lmu = new->data;

      lmu->mask = g_object_ref (mask);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_layer_mask (GimpUndo            *undo,
                     GimpUndoMode         undo_mode,
                     GimpUndoAccumulator *accum)
{
  LayerMaskUndo *lmu;
  GimpLayer     *layer;

  lmu = (LayerMaskUndo *) undo->data;

  layer = GIMP_LAYER (GIMP_ITEM_UNDO (undo)->item);

  if ((undo_mode       == GIMP_UNDO_MODE_UNDO &&
       undo->undo_type == GIMP_UNDO_LAYER_MASK_ADD) ||
      (undo_mode       == GIMP_UNDO_MODE_REDO &&
       undo->undo_type == GIMP_UNDO_LAYER_MASK_REMOVE))
    {
      /*  remove layer mask  */

      undo->size += gimp_object_get_memsize (GIMP_OBJECT (lmu->mask), NULL);

      gimp_layer_apply_mask (layer, GIMP_MASK_DISCARD, FALSE);
    }
  else
    {
      /*  restore layer  */

      undo->size -= gimp_object_get_memsize (GIMP_OBJECT (lmu->mask), NULL);

      gimp_layer_add_mask (layer, lmu->mask, FALSE);
    }

  return TRUE;
}

static void
undo_free_layer_mask (GimpUndo     *undo,
                      GimpUndoMode  undo_mode)
{
  LayerMaskUndo *lmu;

  lmu = (LayerMaskUndo *) undo->data;

  g_object_unref (lmu->mask);
  g_free (lmu);
}


/***************************/
/* Layer re-position Undo  */
/***************************/

typedef struct _LayerRepositionUndo LayerRepositionUndo;

struct _LayerRepositionUndo
{
  gint old_position;
};

static gboolean undo_pop_layer_reposition  (GimpUndo            *undo,
                                            GimpUndoMode         undo_mode,
                                            GimpUndoAccumulator *accum);
static void     undo_free_layer_reposition (GimpUndo            *undo,
                                            GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_layer_reposition (GimpImage   *gimage,
                                       const gchar *undo_desc,
                                       GimpLayer   *layer)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  if ((new = gimp_image_undo_push_item (gimage, GIMP_ITEM (layer),
                                        sizeof (LayerRepositionUndo),
                                        sizeof (LayerRepositionUndo),
                                        GIMP_UNDO_LAYER_REPOSITION, undo_desc,
                                        TRUE,
                                        undo_pop_layer_reposition,
                                        undo_free_layer_reposition)))
    {
      LayerRepositionUndo *lru;

      lru = new->data;

      lru->old_position = gimp_image_get_layer_index (gimage, layer);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_layer_reposition (GimpUndo            *undo,
                           GimpUndoMode         undo_mode,
                           GimpUndoAccumulator *accum)
{
  LayerRepositionUndo *lru;
  GimpLayer           *layer;
  gint                 pos;

  lru = (LayerRepositionUndo *) undo->data;

  layer = GIMP_LAYER (GIMP_ITEM_UNDO (undo)->item);

  /* what's the layer's current index? */
  pos = gimp_image_get_layer_index (undo->gimage, layer);
  gimp_image_position_layer (undo->gimage, layer, lru->old_position,
                             FALSE, NULL);
  lru->old_position = pos;

  return TRUE;
}

static void
undo_free_layer_reposition (GimpUndo     *undo,
                            GimpUndoMode  undo_mode)
{
  g_free (undo->data);
}


/***************************/
/*  Layer properties Undo  */
/***************************/

typedef struct _LayerPropertiesUndo LayerPropertiesUndo;

struct _LayerPropertiesUndo
{
  GimpLayerModeEffects old_mode;
  gdouble              old_opacity;
  gboolean             old_preserve_trans;
};

static gboolean undo_push_layer_properties (GimpImage           *gimage,
                                            GimpUndoType         undo_type,
                                            const gchar         *undo_desc,
                                            GimpLayer           *layer);
static gboolean undo_pop_layer_properties  (GimpUndo            *undo,
                                            GimpUndoMode         undo_mode,
                                            GimpUndoAccumulator *accum);
static void     undo_free_layer_properties (GimpUndo            *undo,
                                            GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_layer_mode (GimpImage   *gimage,
                                 const gchar *undo_desc,
                                 GimpLayer   *layer)
{
  return undo_push_layer_properties (gimage, GIMP_UNDO_LAYER_MODE,
                                     undo_desc, layer);
}

gboolean
gimp_image_undo_push_layer_opacity (GimpImage   *gimage,
                                    const gchar *undo_desc,
                                    GimpLayer   *layer)
{
  return undo_push_layer_properties (gimage, GIMP_UNDO_LAYER_OPACITY,
                                     undo_desc, layer);
}

gboolean
gimp_image_undo_push_layer_preserve_trans (GimpImage   *gimage,
                                           const gchar *undo_desc,
                                           GimpLayer   *layer)
{
  return undo_push_layer_properties (gimage, GIMP_UNDO_LAYER_PRESERVE_TRANS,
                                     undo_desc, layer);
}

static gboolean
undo_push_layer_properties (GimpImage    *gimage,
                            GimpUndoType  undo_type,
                            const gchar  *undo_desc,
                            GimpLayer    *layer)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  if ((new = gimp_image_undo_push_item (gimage, GIMP_ITEM (layer),
                                        sizeof (LayerPropertiesUndo),
                                        sizeof (LayerPropertiesUndo),
                                        undo_type, undo_desc,
                                        TRUE,
                                        undo_pop_layer_properties,
                                        undo_free_layer_properties)))
    {
      LayerPropertiesUndo *lpu;

      lpu = new->data;

      lpu->old_mode           = gimp_layer_get_mode (layer);
      lpu->old_opacity        = gimp_layer_get_opacity (layer);
      lpu->old_preserve_trans = gimp_layer_get_preserve_trans (layer);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_layer_properties (GimpUndo            *undo,
                           GimpUndoMode         undo_mode,
                           GimpUndoAccumulator *accum)
{
  LayerPropertiesUndo  *lpu;
  GimpLayer            *layer;

  lpu = (LayerPropertiesUndo *) undo->data;

  layer = GIMP_LAYER (GIMP_ITEM_UNDO (undo)->item);

  if (undo->undo_type == GIMP_UNDO_LAYER_MODE)
    {
      GimpLayerModeEffects mode;

      mode = gimp_layer_get_mode (layer);
      gimp_layer_set_mode (layer, lpu->old_mode, FALSE);
      lpu->old_mode = mode;
    }
  else if (undo->undo_type == GIMP_UNDO_LAYER_OPACITY)
    {
      gdouble opacity;

      opacity = gimp_layer_get_opacity (layer);
      gimp_layer_set_opacity (layer, lpu->old_opacity, FALSE);
      lpu->old_opacity = opacity;
    }
  else if (undo->undo_type == GIMP_UNDO_LAYER_PRESERVE_TRANS)
    {
      gboolean preserve_trans;

      preserve_trans = gimp_layer_get_preserve_trans (layer);
      gimp_layer_set_preserve_trans (layer, lpu->old_preserve_trans, FALSE);
      lpu->old_preserve_trans = preserve_trans;
    }

  return TRUE;
}

static void
undo_free_layer_properties (GimpUndo     *undo,
                            GimpUndoMode  undo_mode)
{
  g_free (undo->data);
}


/*****************************/
/*  Add/Remove Channel Undo  */
/*****************************/

typedef struct _ChannelUndo ChannelUndo;

struct _ChannelUndo
{
  gint         prev_position;   /*  former position in list     */
  GimpChannel *prev_channel;    /*  previous active channel     */
};

static gboolean undo_push_channel (GimpImage           *gimage,
                                   const gchar         *undo_desc,
                                   GimpUndoType         type,
                                   GimpChannel         *channel,
                                   gint                 prev_position,
                                   GimpChannel         *prev_channel);
static gboolean undo_pop_channel  (GimpUndo            *undo,
                                   GimpUndoMode         undo_mode,
                                   GimpUndoAccumulator *accum);
static void     undo_free_channel (GimpUndo            *undo,
                                   GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_channel_add (GimpImage   *gimage,
                                  const gchar *undo_desc,
                                  GimpChannel *channel,
                                  gint         prev_position,
                                  GimpChannel *prev_channel)
{
  return undo_push_channel (gimage, undo_desc, GIMP_UNDO_CHANNEL_ADD,
                            channel, prev_position, prev_channel);
}

gboolean
gimp_image_undo_push_channel_remove (GimpImage   *gimage,
                                     const gchar *undo_desc,
                                     GimpChannel *channel,
                                     gint         prev_position,
                                     GimpChannel *prev_channel)
{
  return undo_push_channel (gimage, undo_desc, GIMP_UNDO_CHANNEL_REMOVE,
                            channel, prev_position, prev_channel);
}

static gboolean
undo_push_channel (GimpImage    *gimage,
                   const gchar  *undo_desc,
		   GimpUndoType  type,
                   GimpChannel  *channel,
                   gint          prev_position,
                   GimpChannel  *prev_channel)
{
  GimpUndo *new;
  gsize     size;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);
  g_return_val_if_fail (prev_channel == NULL || GIMP_IS_CHANNEL (prev_channel),
                        FALSE);
  g_return_val_if_fail (type == GIMP_UNDO_CHANNEL_ADD ||
			type == GIMP_UNDO_CHANNEL_REMOVE, FALSE);

  size = sizeof (ChannelUndo);

  if (type == GIMP_UNDO_CHANNEL_REMOVE)
    size += gimp_object_get_memsize (GIMP_OBJECT (channel), NULL);

  if ((new = gimp_image_undo_push_item (gimage, GIMP_ITEM (channel),
                                        size,
                                        sizeof (ChannelUndo),
                                        type, undo_desc,
                                        TRUE,
                                        undo_pop_channel,
                                        undo_free_channel)))
    {
      ChannelUndo *cu;

      cu = new->data;

      cu->prev_position = prev_position;
      cu->prev_channel  = prev_channel;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_channel (GimpUndo            *undo,
                  GimpUndoMode         undo_mode,
                  GimpUndoAccumulator *accum)
{
  ChannelUndo *cu;
  GimpChannel *channel;

  cu = (ChannelUndo *) undo->data;

  channel = GIMP_CHANNEL (GIMP_ITEM_UNDO (undo)->item);

  if ((undo_mode       == GIMP_UNDO_MODE_UNDO &&
       undo->undo_type == GIMP_UNDO_CHANNEL_ADD) ||
      (undo_mode       == GIMP_UNDO_MODE_REDO &&
       undo->undo_type == GIMP_UNDO_CHANNEL_REMOVE))
    {
      /*  remove channel  */

      undo->size += gimp_object_get_memsize (GIMP_OBJECT (channel), NULL);

      /*  record the current position  */
      cu->prev_position = gimp_image_get_channel_index (undo->gimage,
                                                        channel);

      /*  remove the channel  */
      gimp_container_remove (undo->gimage->channels, GIMP_OBJECT (channel));

      gimp_item_removed (GIMP_ITEM (channel));

      if (channel == gimp_image_get_active_channel (undo->gimage))
        {
          if (cu->prev_channel)
            gimp_image_set_active_channel (undo->gimage, cu->prev_channel);
          else
            gimp_image_unset_active_channel (undo->gimage);
        }
    }
  else
    {
      /*  restore channel  */

      undo->size -= gimp_object_get_memsize (GIMP_OBJECT (channel), NULL);

      /*  record the active channel  */
      cu->prev_channel = gimp_image_get_active_channel (undo->gimage);

      /*  add the new channel  */
      gimp_container_insert (undo->gimage->channels,
			     GIMP_OBJECT (channel), cu->prev_position);
      gimp_image_set_active_channel (undo->gimage, channel);
    }

  return TRUE;
}

static void
undo_free_channel (GimpUndo     *undo,
                   GimpUndoMode  undo_mode)
{
  g_free (undo->data);
}


/**********************/
/*  Channel Mod Undo  */
/**********************/

typedef struct _ChannelModUndo ChannelModUndo;

struct _ChannelModUndo
{
  TileManager *tiles;
};

static gboolean undo_pop_channel_mod  (GimpUndo            *undo,
                                       GimpUndoMode         undo_mode,
                                       GimpUndoAccumulator *accum);
static void     undo_free_channel_mod (GimpUndo            *undo,
                                       GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_channel_mod (GimpImage   *gimage,
                                  const gchar *undo_desc,
                                  GimpChannel *channel)
{
  TileManager *tiles;
  GimpUndo    *new;
  gsize        size;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  tiles = GIMP_DRAWABLE (channel)->tiles;

  size = sizeof (ChannelModUndo) + tile_manager_get_memsize (tiles);

  if ((new = gimp_image_undo_push_item (gimage, GIMP_ITEM (channel),
                                        size,
                                        sizeof (ChannelModUndo),
                                        GIMP_UNDO_CHANNEL_MOD, undo_desc,
                                        TRUE,
                                        undo_pop_channel_mod,
                                        undo_free_channel_mod)))
    {
      ChannelModUndo *cmu;

      cmu = new->data;

      cmu->tiles = tile_manager_ref (tiles);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_channel_mod (GimpUndo            *undo,
                      GimpUndoMode         undo_mode,
                      GimpUndoAccumulator *accum)
{
  ChannelModUndo *cmu;
  GimpChannel    *channel;
  TileManager    *tiles;

  cmu = (ChannelModUndo *) undo->data;

  channel = GIMP_CHANNEL (GIMP_ITEM_UNDO (undo)->item);

  /* invalidate the current bounds and boundary of the mask */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (channel));

  /*  Issue the first update  */
  gimp_drawable_update (GIMP_DRAWABLE (channel),
                        0, 0,
                        GIMP_ITEM (channel)->width,
                        GIMP_ITEM (channel)->height);

  tiles = cmu->tiles;

  cmu->tiles = GIMP_DRAWABLE (channel)->tiles;

  GIMP_DRAWABLE (channel)->tiles = tiles;
  GIMP_ITEM (channel)->width     = tile_manager_width (tiles);
  GIMP_ITEM (channel)->height    = tile_manager_height (tiles);
  channel->bounds_known          = FALSE;

  if (GIMP_ITEM (channel)->width  != tile_manager_width  (cmu->tiles) ||
      GIMP_ITEM (channel)->height != tile_manager_height (cmu->tiles))
    {
      gimp_viewable_size_changed (GIMP_VIEWABLE (channel));
    }

  /*  Issue the second update  */
  gimp_drawable_update (GIMP_DRAWABLE (channel),
                        0, 0,
                        GIMP_ITEM (channel)->width,
                        GIMP_ITEM (channel)->height);

  return TRUE;
}

static void
undo_free_channel_mod (GimpUndo     *undo,
                       GimpUndoMode  undo_mode)
{
  ChannelModUndo *cmu;

  cmu = (ChannelModUndo *) undo->data;

  tile_manager_unref (cmu->tiles);
  g_free (cmu);
}


/******************************/
/*  Channel re-position Undo  */
/******************************/

typedef struct _ChannelRepositionUndo ChannelRepositionUndo;

struct _ChannelRepositionUndo
{
  gint old_position;
};

static gboolean undo_pop_channel_reposition  (GimpUndo            *undo,
                                              GimpUndoMode         undo_mode,
                                              GimpUndoAccumulator *accum);
static void     undo_free_channel_reposition (GimpUndo            *undo,
                                              GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_channel_reposition (GimpImage   *gimage,
                                         const gchar *undo_desc,
                                         GimpChannel *channel)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  if ((new = gimp_image_undo_push_item (gimage, GIMP_ITEM (channel),
                                        sizeof (ChannelRepositionUndo),
                                        sizeof (ChannelRepositionUndo),
                                        GIMP_UNDO_CHANNEL_REPOSITION, undo_desc,
                                        TRUE,
                                        undo_pop_channel_reposition,
                                        undo_free_channel_reposition)))
    {
      ChannelRepositionUndo *cru;

      cru = new->data;

      cru->old_position = gimp_image_get_channel_index (gimage, channel);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_channel_reposition (GimpUndo            *undo,
                             GimpUndoMode         undo_mode,
                             GimpUndoAccumulator *accum)
{
  ChannelRepositionUndo *cru;
  GimpChannel           *channel;
  gint                   pos;

  cru = (ChannelRepositionUndo *) undo->data;

  channel = GIMP_CHANNEL (GIMP_ITEM_UNDO (undo)->item);

  pos = gimp_image_get_channel_index (undo->gimage, channel);
  gimp_image_position_channel (undo->gimage, channel, cru->old_position,
                               FALSE, NULL);
  cru->old_position = pos;

  return TRUE;
}

static void
undo_free_channel_reposition (GimpUndo     *undo,
                              GimpUndoMode  undo_mode)
{
  g_free (undo->data);
}


/************************/
/*  Channel color Undo  */
/************************/

typedef struct _ChannelColorUndo ChannelColorUndo;

struct _ChannelColorUndo
{
  GimpRGB old_color;
};

static gboolean undo_pop_channel_color  (GimpUndo            *undo,
                                         GimpUndoMode         undo_mode,
                                         GimpUndoAccumulator *accum);
static void     undo_free_channel_color (GimpUndo            *undo,
                                         GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_channel_color (GimpImage   *gimage,
                                    const gchar *undo_desc,
                                    GimpChannel *channel)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  if ((new = gimp_image_undo_push_item (gimage, GIMP_ITEM (channel),
                                        sizeof (ChannelColorUndo),
                                        sizeof (ChannelColorUndo),
                                        GIMP_UNDO_CHANNEL_COLOR, undo_desc,
                                        TRUE,
                                        undo_pop_channel_color,
                                        undo_free_channel_color)))
    {
      ChannelColorUndo *ccu;

      ccu = new->data;

      gimp_channel_get_color (channel , &ccu->old_color);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_channel_color (GimpUndo            *undo,
                        GimpUndoMode         undo_mode,
                        GimpUndoAccumulator *accum)
{
  ChannelColorUndo *ccu;
  GimpChannel      *channel;
  GimpRGB           color;

  ccu = (ChannelColorUndo *) undo->data;

  channel = GIMP_CHANNEL (GIMP_ITEM_UNDO (undo)->item);

  gimp_channel_get_color (channel, &color);
  gimp_channel_set_color (channel, &ccu->old_color, FALSE);
  ccu->old_color = color;

  return TRUE;
}

static void
undo_free_channel_color (GimpUndo     *undo,
                         GimpUndoMode  undo_mode)
{
  g_free (undo->data);
}


/*****************************/
/*  Add/Remove Vectors Undo  */
/*****************************/

typedef struct _VectorsUndo VectorsUndo;

struct _VectorsUndo
{
  gint         prev_position;   /*  former position in list     */
  GimpVectors *prev_vectors;    /*  previous active vectors     */
};

static gboolean undo_push_vectors (GimpImage           *gimage,
                                   const gchar         *undo_desc,
                                   GimpUndoType         type,
                                   GimpVectors         *vectors,
                                   gint                 prev_position,
                                   GimpVectors         *prev_vectors);
static gboolean undo_pop_vectors  (GimpUndo            *undo,
                                   GimpUndoMode         undo_mode,
                                   GimpUndoAccumulator *accum);
static void     undo_free_vectors (GimpUndo            *undo,
                                   GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_vectors_add (GimpImage   *gimage,
                                  const gchar *undo_desc,
                                  GimpVectors *vectors,
                                  gint         prev_position,
                                  GimpVectors *prev_vectors)
{
  return undo_push_vectors (gimage, undo_desc, GIMP_UNDO_VECTORS_ADD,
                            vectors, prev_position, prev_vectors);
}

gboolean
gimp_image_undo_push_vectors_remove (GimpImage   *gimage,
                                     const gchar *undo_desc,
                                     GimpVectors *vectors,
                                     gint         prev_position,
                                     GimpVectors *prev_vectors)
{
  return undo_push_vectors (gimage, undo_desc, GIMP_UNDO_VECTORS_REMOVE,
                            vectors, prev_position, prev_vectors);
}

static gboolean
undo_push_vectors (GimpImage    *gimage,
                   const gchar  *undo_desc,
		   GimpUndoType  type,
                   GimpVectors  *vectors,
                   gint          prev_position,
                   GimpVectors  *prev_vectors)
{
  GimpUndo *new;
  gsize     size;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);
  g_return_val_if_fail (prev_vectors == NULL || GIMP_IS_VECTORS (prev_vectors),
                        FALSE);
  g_return_val_if_fail (type == GIMP_UNDO_VECTORS_ADD ||
			type == GIMP_UNDO_VECTORS_REMOVE, FALSE);

  size = sizeof (VectorsUndo);

  if (type == GIMP_UNDO_VECTORS_REMOVE)
    size += gimp_object_get_memsize (GIMP_OBJECT (vectors), NULL);

  if ((new = gimp_image_undo_push_item (gimage, GIMP_ITEM (vectors),
                                        size,
                                        sizeof (VectorsUndo),
                                        type, undo_desc,
                                        TRUE,
                                        undo_pop_vectors,
                                        undo_free_vectors)))
    {
      VectorsUndo *vu;

      vu = new->data;

      vu->prev_position = prev_position;
      vu->prev_vectors  = prev_vectors;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_vectors (GimpUndo            *undo,
                  GimpUndoMode         undo_mode,
                  GimpUndoAccumulator *accum)
{
  VectorsUndo *vu;
  GimpVectors *vectors;

  vu = (VectorsUndo *) undo->data;

  vectors = GIMP_VECTORS (GIMP_ITEM_UNDO (undo)->item);

  if ((undo_mode       == GIMP_UNDO_MODE_UNDO &&
       undo->undo_type == GIMP_UNDO_VECTORS_ADD) ||
      (undo_mode       == GIMP_UNDO_MODE_REDO &&
       undo->undo_type == GIMP_UNDO_VECTORS_REMOVE))
    {
      /*  remove vectors  */

      undo->size += gimp_object_get_memsize (GIMP_OBJECT (vectors), NULL);

      /*  record the current position  */
      vu->prev_position = gimp_image_get_vectors_index (undo->gimage,
                                                        vectors);

      /*  remove the vectors  */
      gimp_container_remove (undo->gimage->vectors, GIMP_OBJECT (vectors));

      gimp_item_removed (GIMP_ITEM (vectors));

      if (vectors == gimp_image_get_active_vectors (undo->gimage))
        {
          if (vu->prev_vectors)
            {
              gimp_image_set_active_vectors (undo->gimage, vu->prev_vectors);
            }
          else
            {
              undo->gimage->active_vectors = NULL;
              gimp_image_active_vectors_changed (undo->gimage);
            }
        }
    }
  else
    {
      /*  restore vectors  */

      undo->size -= gimp_object_get_memsize (GIMP_OBJECT (vectors), NULL);

      /*  record the active vectors  */
      vu->prev_vectors = gimp_image_get_active_vectors (undo->gimage);

      /*  add the new vectors  */
      gimp_container_insert (undo->gimage->vectors,
			     GIMP_OBJECT (vectors), vu->prev_position);
      gimp_image_set_active_vectors (undo->gimage, vectors);
    }

  return TRUE;
}

static void
undo_free_vectors (GimpUndo     *undo,
                   GimpUndoMode  undo_mode)
{
  g_free (undo->data);
}


/**********************/
/*  Vectors Mod Undo  */
/**********************/

typedef struct _VectorsModUndo VectorsModUndo;

struct _VectorsModUndo
{
  GimpVectors *undo_vectors;
};

static gboolean undo_pop_vectors_mod  (GimpUndo            *undo,
                                       GimpUndoMode         undo_mode,
                                       GimpUndoAccumulator *accum);
static void     undo_free_vectors_mod (GimpUndo            *undo,
                                       GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_vectors_mod (GimpImage   *gimage,
                                  const gchar *undo_desc,
                                  GimpVectors *vectors)
{
  GimpUndo *new;
  gsize     size;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);

  size = (sizeof (VectorsModUndo) +
          gimp_object_get_memsize (GIMP_OBJECT (vectors), NULL));

  if ((new = gimp_image_undo_push_item (gimage, GIMP_ITEM (vectors),
                                        size,
                                        sizeof (VectorsModUndo),
                                        GIMP_UNDO_VECTORS_MOD, undo_desc,
                                        TRUE,
                                        undo_pop_vectors_mod,
                                        undo_free_vectors_mod)))
    {
      VectorsModUndo *vmu;

      vmu = new->data;

      vmu->undo_vectors = GIMP_VECTORS (gimp_item_duplicate (GIMP_ITEM (vectors),
                                                             G_TYPE_FROM_INSTANCE (vectors),
                                                             FALSE));

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_vectors_mod (GimpUndo            *undo,
                      GimpUndoMode         undo_mode,
                      GimpUndoAccumulator *accum)
{
  VectorsModUndo *vmu;
  GimpVectors    *vectors;
  GimpVectors    *temp;

  vmu = (VectorsModUndo *) undo->data;

  vectors = GIMP_VECTORS (GIMP_ITEM_UNDO (undo)->item);

  temp = vmu->undo_vectors;

  vmu->undo_vectors =
    GIMP_VECTORS (gimp_item_duplicate (GIMP_ITEM (vectors),
                                       G_TYPE_FROM_INSTANCE (vectors),
                                       FALSE));

  gimp_vectors_freeze (vectors);

  gimp_vectors_copy_strokes (temp, vectors);

  GIMP_ITEM (vectors)->width    = GIMP_ITEM (temp)->width;
  GIMP_ITEM (vectors)->height   = GIMP_ITEM (temp)->height;
  GIMP_ITEM (vectors)->offset_x = GIMP_ITEM (temp)->offset_x;
  GIMP_ITEM (vectors)->offset_y = GIMP_ITEM (temp)->offset_y;

  g_object_unref (temp);

  gimp_vectors_thaw (vectors);

  return TRUE;
}

static void
undo_free_vectors_mod (GimpUndo     *undo,
                       GimpUndoMode  undo_mode)
{
  VectorsModUndo *vmu;

  vmu = (VectorsModUndo *) undo->data;

  g_object_unref (vmu->undo_vectors);
  g_free (vmu);
}


/******************************/
/*  Vectors re-position Undo  */
/******************************/

typedef struct _VectorsRepositionUndo VectorsRepositionUndo;

struct _VectorsRepositionUndo
{
  gint old_position;
};

static gboolean undo_pop_vectors_reposition  (GimpUndo            *undo,
                                              GimpUndoMode         undo_mode,
                                              GimpUndoAccumulator *accum);
static void     undo_free_vectors_reposition (GimpUndo            *undo,
                                              GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_vectors_reposition (GimpImage   *gimage,
                                         const gchar *undo_desc,
                                         GimpVectors *vectors)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);

  if ((new = gimp_image_undo_push_item (gimage, GIMP_ITEM (vectors),
                                        sizeof (VectorsRepositionUndo),
                                        sizeof (VectorsRepositionUndo),
                                        GIMP_UNDO_VECTORS_REPOSITION, undo_desc,
                                        TRUE,
                                        undo_pop_vectors_reposition,
                                        undo_free_vectors_reposition)))
    {
      VectorsRepositionUndo *vru;

      vru = new->data;

      vru->old_position = gimp_image_get_vectors_index (gimage, vectors);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_vectors_reposition (GimpUndo            *undo,
                             GimpUndoMode         undo_mode,
                             GimpUndoAccumulator *accum)
{
  VectorsRepositionUndo *vru;
  GimpVectors           *vectors;
  gint                   pos;

  vru = (VectorsRepositionUndo *) undo->data;

  vectors = GIMP_VECTORS (GIMP_ITEM_UNDO (undo)->item);

  /* what's the vectors's current index? */
  pos = gimp_image_get_vectors_index (undo->gimage, vectors);
  gimp_image_position_vectors (undo->gimage, vectors, vru->old_position,
                               FALSE, NULL);
  vru->old_position = pos;

  return TRUE;
}

static void
undo_free_vectors_reposition (GimpUndo     *undo,
                              GimpUndoMode  undo_mode)
{
  g_free (undo->data);
}


/**************************************/
/*  Floating Selection to Layer Undo  */
/**************************************/

typedef struct _FStoLayerUndo FStoLayerUndo;

struct _FStoLayerUndo
{
  GimpLayer    *floating_layer; /*  the floating layer        */
  GimpDrawable *drawable;       /*  drawable of floating sel  */
};

static gboolean undo_pop_fs_to_layer  (GimpUndo            *undo,
                                       GimpUndoMode         undo_mode,
                                       GimpUndoAccumulator *accum);
static void     undo_free_fs_to_layer (GimpUndo            *undo,
                                       GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_fs_to_layer (GimpImage    *gimage,
                                  const gchar  *undo_desc,
                                  GimpLayer    *floating_layer,
                                  GimpDrawable *drawable)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (floating_layer), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (FStoLayerUndo),
                                   sizeof (FStoLayerUndo),
                                   GIMP_UNDO_FS_TO_LAYER, undo_desc,
                                   TRUE,
                                   undo_pop_fs_to_layer,
                                   undo_free_fs_to_layer)))
    {
      FStoLayerUndo *fsu;

      fsu = new->data;

      fsu->floating_layer = floating_layer;
      fsu->drawable       = drawable;

      return TRUE;
    }

  tile_manager_unref (floating_layer->fs.backing_store);
  floating_layer->fs.backing_store = NULL;

  return FALSE;
}

static gboolean
undo_pop_fs_to_layer (GimpUndo            *undo,
                      GimpUndoMode         undo_mode,
                      GimpUndoAccumulator *accum)
{
  FStoLayerUndo *fsu;

  fsu = (FStoLayerUndo *) undo->data;

  switch (undo_mode)
    {
    case GIMP_UNDO_MODE_UNDO:
      /*  Update the preview for the floating sel  */
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (fsu->floating_layer));

      fsu->floating_layer->fs.drawable = fsu->drawable;
      gimp_image_set_active_layer (undo->gimage, fsu->floating_layer);
      undo->gimage->floating_sel = fsu->floating_layer;

      /*  restore the contents of the drawable  */
      floating_sel_store (fsu->floating_layer,
			  GIMP_ITEM (fsu->floating_layer)->offset_x,
			  GIMP_ITEM (fsu->floating_layer)->offset_y,
			  GIMP_ITEM (fsu->floating_layer)->width,
			  GIMP_ITEM (fsu->floating_layer)->height);
      fsu->floating_layer->fs.initial = TRUE;

      /*  clear the selection  */
      gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (fsu->floating_layer));

      /*  Update the preview for the gimage and underlying drawable  */
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (fsu->floating_layer));
      break;

    case GIMP_UNDO_MODE_REDO:
      /*  restore the contents of the drawable  */
      floating_sel_restore (fsu->floating_layer,
			    GIMP_ITEM (fsu->floating_layer)->offset_x,
			    GIMP_ITEM (fsu->floating_layer)->offset_y,
			    GIMP_ITEM (fsu->floating_layer)->width,
			    GIMP_ITEM (fsu->floating_layer)->height);

      /*  Update the preview for the gimage and underlying drawable  */
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (fsu->floating_layer));

      /*  clear the selection  */
      gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (fsu->floating_layer));

      /*  update the pointers  */
      fsu->floating_layer->fs.drawable = NULL;
      undo->gimage->floating_sel = NULL;

      /*  Update the fs drawable  */
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (fsu->floating_layer));
      break;
    }

  gimp_image_floating_selection_changed (undo->gimage);

  return TRUE;
}

static void
undo_free_fs_to_layer (GimpUndo     *undo,
                       GimpUndoMode  undo_mode)
{
  FStoLayerUndo *fsu;

  fsu = (FStoLayerUndo *) undo->data;

  if (undo_mode == GIMP_UNDO_MODE_UNDO)
    {
      tile_manager_unref (fsu->floating_layer->fs.backing_store);
      fsu->floating_layer->fs.backing_store = NULL;
    }

  g_free (fsu);
}


/***********************************/
/*  Floating Selection Rigor Undo  */
/***********************************/

static gboolean undo_pop_fs_rigor (GimpUndo            *undo,
                                   GimpUndoMode         undo_mode,
                                   GimpUndoAccumulator *accum);

gboolean
gimp_image_undo_push_fs_rigor (GimpImage    *gimage,
                               const gchar  *undo_desc,
                               GimpLayer    *floating_layer)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (floating_layer), FALSE);

  if ((new = gimp_image_undo_push_item (gimage, GIMP_ITEM (floating_layer),
                                        0, 0,
                                        GIMP_UNDO_FS_RIGOR, undo_desc,
                                        FALSE,
                                        undo_pop_fs_rigor,
                                        NULL)))
    {
      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_fs_rigor (GimpUndo            *undo,
                   GimpUndoMode         undo_mode,
                   GimpUndoAccumulator *accum)
{
  GimpLayer *floating_layer;

  floating_layer = GIMP_LAYER (GIMP_ITEM_UNDO (undo)->item);

  if (! gimp_layer_is_floating_sel (floating_layer))
    return FALSE;

  switch (undo_mode)
    {
    case GIMP_UNDO_MODE_UNDO:
      floating_sel_relax (floating_layer, FALSE);
      break;

    case GIMP_UNDO_MODE_REDO:
      floating_sel_rigor (floating_layer, FALSE);
      break;
    }

  return TRUE;
}


/***********************************/
/*  Floating Selection Relax Undo  */
/***********************************/

static gboolean undo_pop_fs_relax (GimpUndo            *undo,
                                   GimpUndoMode         undo_mode,
                                   GimpUndoAccumulator *accum);

gboolean
gimp_image_undo_push_fs_relax (GimpImage   *gimage,
                               const gchar *undo_desc,
                               GimpLayer   *floating_layer)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (floating_layer), FALSE);

  if ((new = gimp_image_undo_push_item (gimage, GIMP_ITEM (floating_layer),
                                        0, 0,
                                        GIMP_UNDO_FS_RELAX, undo_desc,
                                        FALSE,
                                        undo_pop_fs_relax,
                                        NULL)))
    {
      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_fs_relax (GimpUndo            *undo,
                   GimpUndoMode         undo_mode,
                   GimpUndoAccumulator *accum)
{
  GimpLayer *floating_layer;

  floating_layer = GIMP_LAYER (GIMP_ITEM_UNDO (undo)->item);

  if (! gimp_layer_is_floating_sel (floating_layer))
    return FALSE;

  switch (undo_mode)
    {
    case GIMP_UNDO_MODE_UNDO:
      floating_sel_rigor (floating_layer, FALSE);
      break;

    case GIMP_UNDO_MODE_REDO:
      floating_sel_relax (floating_layer, FALSE);
      break;
    }

  return TRUE;
}


/*******************/
/*  Parasite Undo  */
/*******************/

typedef struct _ParasiteUndo ParasiteUndo;

struct _ParasiteUndo
{
  GimpImage    *gimage;
  GimpItem     *item;
  GimpParasite *parasite;
  gchar        *name;
};

static gboolean undo_pop_parasite  (GimpUndo            *undo,
                                    GimpUndoMode         undo_mode,
                                    GimpUndoAccumulator *accum);
static void     undo_free_parasite (GimpUndo            *undo,
                                    GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_image_parasite (GimpImage   *gimage,
                                     const gchar *undo_desc,
                                     gpointer     parasite)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (ParasiteUndo),
                                   sizeof (ParasiteUndo),
                                   GIMP_UNDO_PARASITE_ATTACH, undo_desc,
                                   TRUE,
                                   undo_pop_parasite,
                                   undo_free_parasite)))
    {
      ParasiteUndo *pu;

      pu = new->data;

      pu->gimage   = gimage;
      pu->item     = NULL;
      pu->name     = g_strdup (gimp_parasite_name (parasite));
      pu->parasite = gimp_parasite_copy (gimp_image_parasite_find (gimage,
                                                                   pu->name));

      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_image_undo_push_image_parasite_remove (GimpImage   *gimage,
                                            const gchar *undo_desc,
                                            const gchar *name)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (ParasiteUndo),
                                   sizeof (ParasiteUndo),
                                   GIMP_UNDO_PARASITE_REMOVE, undo_desc,
                                   TRUE,
                                   undo_pop_parasite,
                                   undo_free_parasite)))
    {
      ParasiteUndo *pu;

      pu = new->data;

      pu->gimage   = gimage;
      pu->item     = NULL;
      pu->name     = g_strdup (name);
      pu->parasite = gimp_parasite_copy (gimp_image_parasite_find (gimage,
                                                                   pu->name));

      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_image_undo_push_item_parasite (GimpImage   *gimage,
                                    const gchar *undo_desc,
                                    GimpItem    *item,
                                    gpointer     parasite)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (ParasiteUndo),
                                   sizeof (ParasiteUndo),
                                   GIMP_UNDO_PARASITE_ATTACH, undo_desc,
                                   TRUE,
                                   undo_pop_parasite,
                                   undo_free_parasite)))
    {
      ParasiteUndo *pu;

      pu = new->data;

      pu->gimage   = NULL;
      pu->item     = item;
      pu->name     = g_strdup (gimp_parasite_name (parasite));
      pu->parasite = gimp_parasite_copy (gimp_item_parasite_find (item,
                                                                  pu->name));
      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_image_undo_push_item_parasite_remove (GimpImage   *gimage,
                                           const gchar *undo_desc,
                                           GimpItem    *item,
                                           const gchar *name)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (ParasiteUndo),
                                   sizeof (ParasiteUndo),
                                   GIMP_UNDO_PARASITE_REMOVE, undo_desc,
                                   TRUE,
                                   undo_pop_parasite,
                                   undo_free_parasite)))
    {
      ParasiteUndo *pu;

      pu = new->data;

      pu->gimage   = NULL;
      pu->item     = item;
      pu->name     = g_strdup (name);
      pu->parasite = gimp_parasite_copy (gimp_item_parasite_find (item,
                                                                  pu->name));
      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_parasite (GimpUndo            *undo,
                   GimpUndoMode         undo_mode,
                   GimpUndoAccumulator *accum)
{
  ParasiteUndo *pu;
  GimpParasite *tmp;

  pu = (ParasiteUndo *) undo->data;

  tmp = pu->parasite;

  if (pu->gimage)
    {
      pu->parasite = gimp_parasite_copy (gimp_image_parasite_find (undo->gimage,
                                                                   pu->name));
      if (tmp)
	gimp_parasite_list_add (pu->gimage->parasites, tmp);
      else
	gimp_parasite_list_remove (pu->gimage->parasites, pu->name);
    }
  else if (pu->item)
    {
      pu->parasite = gimp_parasite_copy (gimp_item_parasite_find (pu->item,
                                                                  pu->name));
      if (tmp)
	gimp_parasite_list_add (pu->item->parasites, tmp);
      else
	gimp_parasite_list_remove (pu->item->parasites, pu->name);
    }
  else
    {
      pu->parasite = gimp_parasite_copy (gimp_parasite_find (undo->gimage->gimp,
                                                             pu->name));
      if (tmp)
	gimp_parasite_attach (undo->gimage->gimp, tmp);
      else
	gimp_parasite_detach (undo->gimage->gimp, pu->name);
    }

  if (tmp)
    gimp_parasite_free (tmp);

  return TRUE;
}

static void
undo_free_parasite (GimpUndo     *undo,
                    GimpUndoMode  undo_mode)
{
  ParasiteUndo *pu;

  pu = (ParasiteUndo *) undo->data;

  if (pu->parasite)
    gimp_parasite_free (pu->parasite);
  if (pu->name)
    g_free (pu->name);

  g_free (pu);
}


/******************************************************************************/
/*  Something for which programmer is too lazy to write an undo function for  */
/******************************************************************************/

static gboolean undo_pop_cantundo (GimpUndo            *undo,
                                   GimpUndoMode         undo_mode,
                                   GimpUndoAccumulator *accum);

gboolean
gimp_image_undo_push_cantundo (GimpImage   *gimage,
                               const gchar *undo_desc)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  /* This is the sole purpose of this type of undo: the ability to
   * mark an image as having been mutated, without really providing
   * any adequate undo facility.
   */

  if ((new = gimp_image_undo_push (gimage,
                                   0, 0,
                                   GIMP_UNDO_CANT, undo_desc,
                                   TRUE,
                                   undo_pop_cantundo,
                                   NULL)))
    {
      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_cantundo (GimpUndo            *undo,
                   GimpUndoMode         undo_mode,
                   GimpUndoAccumulator *accum)
{
  switch (undo_mode)
    {
    case GIMP_UNDO_MODE_UNDO:
      g_message (_("Can't undo %s"), GIMP_OBJECT (undo)->name);
      break;

    case GIMP_UNDO_MODE_REDO:
      break;
    }

  return TRUE;
}
