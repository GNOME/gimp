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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "tools/tools-types.h"

#include "config/gimpcoreconfig.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimp-parasites.h"
#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-mask.h"
#include "core/gimpimage-projection.h"
#include "core/gimplayer.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimplayermask.h"
#include "core/gimplist.h"
#include "core/gimpparasitelist.h"

#include "core/gimpundo.h"
#include "core/gimpundostack.h"
#include "core/gimpimage-undo.h"

#include "paint/gimppaintcore.h"

#include "vectors/gimpvectors.h"

#include "tools/gimpbycolorselecttool.h"
#include "tools/gimppainttool.h"
#include "tools/gimptransformtool.h"
#include "tools/tool_manager.h"

#include "path_transform.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


/**********************/
/*  group Undo funcs  */

gboolean
undo_push_group_start (GimpImage    *gimage,
		       GimpUndoType  type)
{
  return gimp_image_undo_group_start (gimage, type, NULL);
}

gboolean
undo_push_group_end (GimpImage *gimage)
{
  return gimp_image_undo_group_end (gimage);
}


/****************/
/*  Image Undo  */
/****************/

typedef struct _ImageUndo ImageUndo;

struct _ImageUndo
{
  TileManager  *tiles;
  GimpDrawable *drawable;
  gint          x1, y1, x2, y2;
  gboolean      sparse;
};

static gboolean undo_pop_image  (GimpUndo            *undo,
                                 GimpImage           *gimage,
                                 GimpUndoMode         undo_mode,
                                 GimpUndoAccumulator *accum);
static void     undo_free_image (GimpUndo            *undo,
                                 GimpImage           *gimage,
                                 GimpUndoMode         undo_mode);

gboolean
undo_push_image (GimpImage    *gimage,
		 GimpDrawable *drawable,
		 gint          x1,
		 gint          y1,
		 gint          x2,
		 gint          y2)
{
  GimpUndo *new;
  gsize     size;

  x1 = CLAMP (x1, 0, gimp_drawable_width (drawable));
  y1 = CLAMP (y1, 0, gimp_drawable_height (drawable));
  x2 = CLAMP (x2, 0, gimp_drawable_width (drawable));
  y2 = CLAMP (y2, 0, gimp_drawable_height (drawable));

  size = sizeof (ImageUndo) + ((x2 - x1) * (y2 - y1) *
                               gimp_drawable_bytes (drawable));

  if ((new = gimp_image_undo_push (gimage,
                                   size, sizeof (ImageUndo),
                                   IMAGE_UNDO, NULL,
                                   TRUE,
                                   undo_pop_image,
                                   undo_free_image)))
    {
      ImageUndo   *image_undo;
      TileManager *tiles;
      PixelRegion  srcPR, destPR;

      image_undo = new->data;

      /*  If we cannot create a new temp buf--either because our parameters are
       *  degenerate or something else failed, simply return an unsuccessful push.
       */
      tiles = tile_manager_new ((x2 - x1), (y2 - y1),
				gimp_drawable_bytes (drawable));
      pixel_region_init (&srcPR, gimp_drawable_data (drawable),
			 x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, tiles,
			 0, 0, (x2 - x1), (y2 - y1), TRUE);
      copy_region (&srcPR, &destPR);

      /*  set the image undo structure  */
      image_undo->tiles    = tiles;
      image_undo->drawable = drawable;
      image_undo->x1       = x1;
      image_undo->y1       = y1;
      image_undo->x2       = x2;
      image_undo->y2       = y2;
      image_undo->sparse   = FALSE;

      return TRUE;
    }

  return FALSE;
}

gboolean
undo_push_image_mod (GimpImage    *gimage,
		     GimpDrawable *drawable,
		     gint          x1,
		     gint          y1,
		     gint          x2,
		     gint          y2,
		     TileManager  *tiles,
		     gboolean      sparse)
{
  GimpUndo *new;
  gsize     size;
  gint      dwidth, dheight;

  if (! tiles)
    return FALSE;

  dwidth  = gimp_drawable_width (drawable);
  dheight = gimp_drawable_height (drawable);

  x1 = CLAMP (x1, 0, dwidth);
  y1 = CLAMP (y1, 0, dheight);
  x2 = CLAMP (x2, 0, dwidth);
  y2 = CLAMP (y2, 0, dheight);

  size = sizeof (ImageUndo) + tile_manager_get_memsize (tiles);

  if ((new = gimp_image_undo_push (gimage,
                                   size, sizeof (ImageUndo),
                                   IMAGE_MOD_UNDO, NULL,
                                   TRUE,
                                   undo_pop_image,
                                   undo_free_image)))
    {
      ImageUndo *image_undo;

      image_undo = new->data;

      image_undo->tiles    = tiles;
      image_undo->drawable = drawable;
      image_undo->x1       = x1;
      image_undo->y1       = y1;
      image_undo->x2       = x2;
      image_undo->y2       = y2;
      image_undo->sparse   = sparse;

      return TRUE;
    }

  tile_manager_destroy (tiles);

  return FALSE;
}

static gboolean
undo_pop_image (GimpUndo            *undo,
                GimpImage           *gimage,
                GimpUndoMode         undo_mode,
                GimpUndoAccumulator *accum)
{
  ImageUndo   *image_undo;
  TileManager *tiles;
  PixelRegion  PR1, PR2;
  gint         x, y;
  gint         w, h;

  image_undo = (ImageUndo *) undo->data;

  tiles = image_undo->tiles;

  /*  some useful values  */
  x = image_undo->x1;
  y = image_undo->y1;

  if (image_undo->sparse == FALSE)
    {
      w = tile_manager_width (tiles);
      h = tile_manager_height (tiles);

      pixel_region_init (&PR1, tiles,
			 0, 0, w, h, TRUE);
      pixel_region_init (&PR2, gimp_drawable_data (image_undo->drawable),
			 x, y, w, h, TRUE);

      /*  swap the regions  */
      swap_region (&PR1, &PR2);
    }
  else
    {
      int i, j;
      Tile *src_tile;
      Tile *dest_tile;

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

		  src_tile = tile_manager_get_tile (tiles, j, i, TRUE, FALSE /* TRUE */);
		  dest_tile = tile_manager_get_tile (gimp_drawable_data (image_undo->drawable), j, i, TRUE, FALSE /* TRUE */);

		  tile_manager_map_tile (tiles, j, i, dest_tile);
		  tile_manager_map_tile (gimp_drawable_data (image_undo->drawable), j, i, src_tile);
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

  gimp_drawable_update (image_undo->drawable, x, y, w, h);

  return TRUE;
}

static void
undo_free_image (GimpUndo     *undo,
                 GimpImage    *gimage,
                 GimpUndoMode  undo_mode)
{
  ImageUndo *image_undo;

  image_undo = (ImageUndo *) undo->data;

  tile_manager_destroy (image_undo->tiles);
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
                                      GimpImage           *gimage,
                                      GimpUndoMode         undo_mode,
                                      GimpUndoAccumulator *accum);
static void     undo_free_image_type (GimpUndo            *undo,
                                      GimpImage           *gimage,
                                      GimpUndoMode         undo_mode);

gboolean
undo_push_image_type (GimpImage *gimage)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (ImageTypeUndo),
                                   sizeof (ImageTypeUndo),
                                   IMAGE_TYPE_UNDO, NULL,
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
                     GimpImage           *gimage,
                     GimpUndoMode         undo_mode,
                     GimpUndoAccumulator *accum)
{
  ImageTypeUndo     *itu;
  GimpImageBaseType  tmp;

  itu = (ImageTypeUndo *) undo->data;

  tmp = itu->base_type;
  itu->base_type = gimage->base_type;
  gimage->base_type = tmp;

  gimp_image_projection_allocate (gimage);

  gimp_image_colormap_changed (gimage, -1);

  if (itu->base_type != gimage->base_type)
    accum->mode_changed = TRUE;

  return TRUE;
}

static void
undo_free_image_type (GimpUndo     *undo,
                      GimpImage    *gimage,
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
                                      GimpImage           *gimage,
                                      GimpUndoMode         undo_mode,
                                      GimpUndoAccumulator *accum);
static void     undo_free_image_size (GimpUndo            *undo,
                                      GimpImage           *gimage,
                                      GimpUndoMode         undo_mode);

gboolean
undo_push_image_size (GimpImage *gimage)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (ImageSizeUndo),
                                   sizeof (ImageSizeUndo),
                                   IMAGE_SIZE_UNDO, NULL,
                                   TRUE,
                                   undo_pop_image_size,
                                   undo_free_image_size)))
    {
      ImageSizeUndo *isu;

      isu = new->data;

      new->data      = isu;

      isu->width  = gimage->width;
      isu->height = gimage->height;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_image_size (GimpUndo            *undo,
                     GimpImage           *gimage,
                     GimpUndoMode         undo_mode,
                     GimpUndoAccumulator *accum)
{
  ImageSizeUndo *isu;
  gint           width;
  gint           height;

  isu = (ImageSizeUndo *) undo->data;

  width  = isu->width;
  height = isu->height;

  isu->width  = gimage->width;
  isu->height = gimage->height;

  gimage->width  = width;
  gimage->height = height;

  gimp_image_projection_allocate (gimage);
  gimp_image_mask_invalidate (gimage);

  if (gimage->width  != isu->width ||
      gimage->height != isu->height)
    accum->size_changed = TRUE;

  return TRUE;
}

static void
undo_free_image_size (GimpUndo      *undo,
                      GimpImage     *gimage,
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
                                            GimpImage           *gimage,
                                            GimpUndoMode         undo_mode,
                                            GimpUndoAccumulator *accum);
static void     undo_free_image_resolution (GimpUndo            *undo,
                                            GimpImage           *gimage,
                                            GimpUndoMode         undo_mode);

gboolean
undo_push_image_resolution (GimpImage *gimage)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (ResolutionUndo),
                                   sizeof (ResolutionUndo),
                                   IMAGE_RESOLUTION_UNDO, NULL,
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
                           GimpImage           *gimage,
                           GimpUndoMode         undo_mode,
                           GimpUndoAccumulator *accum)
{
  ResolutionUndo *ru;

  ru = (ResolutionUndo *) undo->data;

  if (ABS (ru->xres - gimage->xresolution) >= 1e-5 ||
      ABS (ru->yres - gimage->yresolution) >= 1e-5)
    {
      gdouble xres;
      gdouble yres;

      xres = gimage->xresolution;
      yres = gimage->yresolution;

      gimage->xresolution = ru->xres;
      gimage->yresolution = ru->yres;

      ru->xres = xres;
      ru->yres = yres;

      accum->resolution_changed = TRUE;
    }

  if (ru->unit != gimage->unit)
    {
      GimpUnit unit;

      unit = gimage->unit;
      gimage->unit = ru->unit;
      ru->unit = unit;

      accum->unit_changed = TRUE;
    }

  return TRUE;
}

static void
undo_free_image_resolution (GimpUndo      *undo,
                            GimpImage     *gimage,
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
  GimpImage *gimage;
  gboolean   qmask;
};

static gboolean undo_pop_image_qmask  (GimpUndo            *undo,
                                       GimpImage           *gimage,
                                       GimpUndoMode         undo_mode,
                                       GimpUndoAccumulator *accum);
static void     undo_free_image_qmask (GimpUndo            *undo,
                                       GimpImage           *gimage,
                                       GimpUndoMode         undo_mode);

gboolean
undo_push_image_qmask (GimpImage *gimage)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (QmaskUndo),
                                   sizeof (QmaskUndo),
                                   IMAGE_QMASK_UNDO, NULL,
                                   TRUE,
                                   undo_pop_image_qmask,
                                   undo_free_image_qmask)))
    {
      QmaskUndo *qu;

      qu = new->data;

      qu->gimage = gimage;
      qu->qmask  = gimage->qmask_state;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_image_qmask (GimpUndo            *undo,
                      GimpImage           *gimage,
                      GimpUndoMode         undo_mode,
                      GimpUndoAccumulator *accum)
{
  QmaskUndo *data;
  gboolean   tmp;

  data = (QmaskUndo *) undo->data;
  
  tmp = gimage->qmask_state;
  gimage->qmask_state = data->qmask;
  data->qmask = tmp;

  accum->qmask_changed = TRUE;

  return TRUE;
}

static void
undo_free_image_qmask (GimpUndo      *undo,
                       GimpImage     *gimage,
                       GimpUndoMode   undo_mode)
{
  g_free (undo->data);
}


/****************/
/*  Guide Undo  */
/****************/

typedef struct _GuideUndo GuideUndo;

struct _GuideUndo
{
  GimpImage *gimage;
  GimpGuide *guide;
  GimpGuide  orig;
};

static gboolean undo_pop_image_guide  (GimpUndo            *undo,
                                       GimpImage           *gimage,
                                       GimpUndoMode         undo_mode,
                                       GimpUndoAccumulator *accum);
static void     undo_free_image_guide (GimpUndo            *undo,
                                       GimpImage           *gimage,
                                       GimpUndoMode         undo_mode);

gboolean
undo_push_image_guide (GimpImage *gimage,
                       GimpGuide *guide)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (GuideUndo),
                                   sizeof (GuideUndo),
                                   IMAGE_GUIDE_UNDO, NULL,
                                   TRUE,
                                   undo_pop_image_guide,
                                   undo_free_image_guide)))
    {
      GuideUndo *gu;

      gu = new->data;

      guide->ref_count++;

      gu->gimage = gimage;
      gu->guide  = guide;
      gu->orig   = *guide;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_image_guide (GimpUndo            *undo,
                      GimpImage           *gimage,
                      GimpUndoMode         undo_mode,
                      GimpUndoAccumulator *accum)
{
  GuideUndo *data;
  GimpGuide  tmp;
  gint       tmp_ref;

  data = (GuideUndo *) undo->data;

  gimp_image_update_guide (gimage, data->guide);

  tmp_ref = data->guide->ref_count;
  tmp = *(data->guide);
  *(data->guide) = data->orig;
  data->guide->ref_count = tmp_ref;
  data->orig = tmp;

  gimp_image_update_guide (gimage, data->guide);

  return TRUE;
}

static void
undo_free_image_guide (GimpUndo            *undo,
                       GimpImage           *gimage,
                       GimpUndoMode         undo_mode)
{
  GuideUndo *data;

  data = (GuideUndo *) undo->data;

  data->guide->ref_count--;
  if (data->guide->position < 0 && data->guide->ref_count <= 0)
    {
      gimp_image_remove_guide (gimage, data->guide);
      g_free (data->guide);
    }
  g_free (data);
}


/***************/
/*  Mask Undo  */
/***************/

typedef struct _MaskUndo MaskUndo;

struct _MaskUndo
{
  GimpChannel *channel;  /*  the channel this undo is for  */
  TileManager *tiles;    /*  the actual mask               */
  gint         x, y;     /*  offsets                       */
};

static gboolean undo_pop_mask  (GimpUndo            *undo,
                                GimpImage           *gimage,
                                GimpUndoMode         undo_mode,
                                GimpUndoAccumulator *accum);
static void     undo_free_mask (GimpUndo            *undo,
                                GimpImage           *gimage,
                                GimpUndoMode         undo_mode);

gboolean
undo_push_mask (GimpImage   *gimage,
                GimpChannel *mask)
{
  TileManager *undo_tiles;
  gint         x1, y1, x2, y2;
  GimpUndo    *new;
  gsize        size;

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

  if ((new = gimp_image_undo_push (gimage,
                                   size, sizeof (MaskUndo),
                                   MASK_UNDO, NULL,
                                   FALSE,
                                   undo_pop_mask,
                                   undo_free_mask)))
    {
      MaskUndo *mask_undo;

      mask_undo = new->data;

      mask_undo->channel = mask;
      mask_undo->tiles   = undo_tiles;
      mask_undo->x       = x1;
      mask_undo->y       = y1;

      return TRUE;
    }

  if (undo_tiles)
    tile_manager_destroy (undo_tiles);

  return FALSE;
}

static gboolean
undo_pop_mask (GimpUndo            *undo,
               GimpImage           *gimage,
               GimpUndoMode         undo_mode,
               GimpUndoAccumulator *accum)
{
  MaskUndo    *mask_undo;
  TileManager *new_tiles;
  PixelRegion  srcPR, destPR;
  gint         x1, y1, x2, y2;
  gint         width, height;
  guchar       empty = 0;

  width = height = 0;

  mask_undo = (MaskUndo *) undo->data;

  if (gimp_channel_bounds (mask_undo->channel, &x1, &y1, &x2, &y2))
    {
      new_tiles = tile_manager_new ((x2 - x1), (y2 - y1), 1);

      pixel_region_init (&srcPR, GIMP_DRAWABLE (mask_undo->channel)->tiles,
                         x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, new_tiles,
			 0, 0, (x2 - x1), (y2 - y1), TRUE);

      copy_region (&srcPR, &destPR);

      pixel_region_init (&srcPR, GIMP_DRAWABLE (mask_undo->channel)->tiles,
			 x1, y1, (x2 - x1), (y2 - y1), TRUE);

      color_region (&srcPR, &empty);
    }
  else
    new_tiles = NULL;

  if (mask_undo->tiles)
    {
      width  = tile_manager_width (mask_undo->tiles);
      height = tile_manager_height (mask_undo->tiles);

      pixel_region_init (&srcPR, mask_undo->tiles,
			 0, 0, width, height, FALSE);
      pixel_region_init (&destPR, GIMP_DRAWABLE (mask_undo->channel)->tiles,
			 mask_undo->x, mask_undo->y, width, height, TRUE);

      copy_region (&srcPR, &destPR);

      tile_manager_destroy (mask_undo->tiles);
    }

  if (mask_undo->channel == gimp_image_get_mask (gimage))
    {
      /* invalidate the current bounds and boundary of the mask */
      gimp_image_mask_invalidate (gimage);
    }
  else
    {
      mask_undo->channel->boundary_known = FALSE;
      GIMP_DRAWABLE (mask_undo->channel)->preview_valid = FALSE;
    }

  if (mask_undo->tiles)
    {
      mask_undo->channel->empty = FALSE;
      mask_undo->channel->x1    = mask_undo->x;
      mask_undo->channel->y1    = mask_undo->y;
      mask_undo->channel->x2    = mask_undo->x + width;
      mask_undo->channel->y2    = mask_undo->y + height;
    }
  else
    {
      mask_undo->channel->empty = TRUE;
      mask_undo->channel->x1    = 0;
      mask_undo->channel->y1    = 0;
      mask_undo->channel->x2    = GIMP_DRAWABLE (mask_undo->channel)->width;
      mask_undo->channel->y2    = GIMP_DRAWABLE (mask_undo->channel)->height;
    }

  /* we know the bounds */
  mask_undo->channel->bounds_known = TRUE;

  /*  set the new mask undo parameters  */
  mask_undo->tiles = new_tiles;
  mask_undo->x     = x1;
  mask_undo->y     = y1;

  if (mask_undo->channel == gimp_image_get_mask (gimage))
    {
      accum->mask_changed = TRUE;
    }
  else
    {
      gimp_drawable_update (GIMP_DRAWABLE (mask_undo->channel),
                            0, 0,
                            GIMP_DRAWABLE (mask_undo->channel)->width,
                            GIMP_DRAWABLE (mask_undo->channel)->height);
    }

  return TRUE;
}

static void
undo_free_mask (GimpUndo     *undo,
                GimpImage    *gimage,
                GimpUndoMode  undo_mode)
{
  MaskUndo *mask_undo;

  mask_undo = (MaskUndo *) undo->data;

  if (mask_undo->tiles)
    tile_manager_destroy (mask_undo->tiles);
  g_free (mask_undo);
}


/**********************/
/*  Item Rename Undo  */
/**********************/

typedef struct _ItemRenameUndo ItemRenameUndo;

struct _ItemRenameUndo
{
  GimpItem *item;
  gchar    *old_name;
};

static gboolean undo_pop_item_rename  (GimpUndo            *undo,
                                       GimpImage           *gimage,
                                       GimpUndoMode         undo_mode,
                                       GimpUndoAccumulator *accum);
static void     undo_free_item_rename (GimpUndo            *undo,
                                       GimpImage           *gimage,
                                       GimpUndoMode         undo_mode);

gboolean
undo_push_item_rename (GimpImage *gimage, 
                       GimpItem  *item)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (ItemRenameUndo),
                                   sizeof (ItemRenameUndo),
                                   ITEM_RENAME_UNDO, NULL,
                                   TRUE,
                                   undo_pop_item_rename,
                                   undo_free_item_rename)))
    {
      ItemRenameUndo *iru;

      iru = new->data;

      new->data      = iru;

      iru->item     = item;
      iru->old_name = g_strdup (gimp_object_get_name (GIMP_OBJECT (item)));

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_item_rename (GimpUndo            *undo,
                      GimpImage           *gimage,
                      GimpUndoMode         undo_mode,
                      GimpUndoAccumulator *accum)
{
  ItemRenameUndo *iru;
  gchar          *tmp;

  iru = (ItemRenameUndo *) undo->data;

  tmp = g_strdup (gimp_object_get_name (GIMP_OBJECT (iru->item)));
  gimp_object_set_name (GIMP_OBJECT (iru->item), iru->old_name);
  g_free (iru->old_name);
  iru->old_name = tmp;

  return TRUE;
}

static void
undo_free_item_rename (GimpUndo     *undo,
                       GimpImage    *gimage,
                       GimpUndoMode  undo_mode)
{
  ItemRenameUndo *iru;

  iru = (ItemRenameUndo *) undo->data;

  g_free (iru->old_name);
  g_free (iru);
}


/***************************/
/*  Layer Add/Remove Undo  */
/***************************/

typedef struct _LayerUndo LayerUndo;

struct _LayerUndo
{
  GimpLayer *layer;           /*  the actual layer         */
  gint       prev_position;   /*  former position in list  */
  GimpLayer *prev_layer;      /*  previous active layer    */
};

static gboolean undo_push_layer (GimpImage           *gimage,
                                 GimpUndoType         type,
                                 GimpLayer           *layer,
                                 gint                 prev_position,
                                 GimpLayer           *prev_layer);
static gboolean undo_pop_layer  (GimpUndo            *undo,
                                 GimpImage           *gimage,
                                 GimpUndoMode         undo_mode,
                                 GimpUndoAccumulator *accum);
static void     undo_free_layer (GimpUndo            *undo,
                                 GimpImage           *gimage,
                                 GimpUndoMode         undo_mode);

gboolean
undo_push_layer_add (GimpImage *gimage,
                     GimpLayer *layer,
                     gint       prev_position,
                     GimpLayer *prev_layer)
{
  return undo_push_layer (gimage, LAYER_ADD_UNDO,
                          layer, prev_position, prev_layer);
}

gboolean
undo_push_layer_remove (GimpImage *gimage,
                        GimpLayer *layer,
                        gint       prev_position,
                        GimpLayer *prev_layer)
{
  return undo_push_layer (gimage, LAYER_REMOVE_UNDO,
                          layer, prev_position, prev_layer);
}

static gboolean
undo_push_layer (GimpImage    *gimage,
		 GimpUndoType  type,
                 GimpLayer    *layer,
                 gint          prev_position,
                 GimpLayer    *prev_layer)
{
  GimpUndo *new;
  gsize     size;

  g_return_val_if_fail (type == LAYER_ADD_UNDO ||
			type == LAYER_REMOVE_UNDO,
			FALSE);

  size = sizeof (LayerUndo) + gimp_object_get_memsize (GIMP_OBJECT (layer));

  if ((new = gimp_image_undo_push (gimage,
                                   size, sizeof (LayerUndo),
                                   type, NULL,
                                   TRUE,
                                   undo_pop_layer,
                                   undo_free_layer)))
    {
      LayerUndo *lu;

      lu = new->data;

      lu->layer         = g_object_ref (layer);
      lu->prev_position = prev_position;
      lu->prev_layer    = prev_layer;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_layer (GimpUndo            *undo,
                GimpImage           *gimage,
                GimpUndoMode         undo_mode,
                GimpUndoAccumulator *accum)
{
  LayerUndo *lu;

  lu = (LayerUndo *) undo->data;

  if ((undo_mode == GIMP_UNDO_MODE_UNDO && undo->undo_type == LAYER_ADD_UNDO) ||
      (undo_mode == GIMP_UNDO_MODE_REDO && undo->undo_type == LAYER_REMOVE_UNDO))
    {
      /*  remove layer  */

      /*  record the current position  */
      lu->prev_position = gimp_image_get_layer_index (gimage, lu->layer);

      /*  if exists, set the previous layer  */
      if (lu->prev_layer)
	gimp_image_set_active_layer (gimage, lu->prev_layer);

      /*  remove the layer  */
      gimp_container_remove (gimage->layers, GIMP_OBJECT (lu->layer));
      gimage->layer_stack = g_slist_remove (gimage->layer_stack, lu->layer);

      /*  reset the gimage values  */
      if (gimp_layer_is_floating_sel (lu->layer))
	{
	  gimage->floating_sel = NULL;
	  /*  reset the old drawable  */
	  floating_sel_reset (lu->layer);

	  gimp_image_floating_selection_changed (gimage);
	}

      gimp_drawable_update (GIMP_DRAWABLE (lu->layer),
			    0, 0,
			    GIMP_DRAWABLE (lu->layer)->width,
			    GIMP_DRAWABLE (lu->layer)->height);

      if (gimp_container_num_children (gimage->layers) == 1 &&
          ! gimp_drawable_has_alpha (GIMP_LIST (gimage->layers)->list->data))
        {
          accum->alpha_changed = TRUE;
        }
    }
  else
    {
      /*  restore layer  */

      /*  record the active layer  */
      lu->prev_layer = gimp_image_get_active_layer (gimage);

      /*  hide the current selection--for all views  */
      if (gimp_image_get_active_layer (gimage))
	gimp_layer_invalidate_boundary (gimp_image_get_active_layer (gimage));

      /*  if this is a floating selection, set the fs pointer  */
      if (gimp_layer_is_floating_sel (lu->layer))
	gimage->floating_sel = lu->layer;

      if (gimp_container_num_children (gimage->layers) == 1 &&
          ! gimp_drawable_has_alpha (GIMP_LIST (gimage->layers)->list->data))
        {
          accum->alpha_changed = TRUE;
        }

      /*  add the new layer  */
      gimp_container_insert (gimage->layers, 
			     GIMP_OBJECT (lu->layer), lu->prev_position);
      gimp_image_set_active_layer (gimage, lu->layer);

      if (gimp_layer_is_floating_sel (lu->layer))
	gimp_image_floating_selection_changed (gimage);

      gimp_drawable_update (GIMP_DRAWABLE (lu->layer),
			    0, 0,
			    GIMP_DRAWABLE (lu->layer)->width,
			    GIMP_DRAWABLE (lu->layer)->height);
    }

  return TRUE;
}

static void
undo_free_layer (GimpUndo     *undo,
                 GimpImage    *gimage,
                 GimpUndoMode  undo_mode)
{
  LayerUndo *lu;

  lu = (LayerUndo *) undo->data;

  g_object_unref (lu->layer);
  g_free (lu);
}


/********************/
/*  Layer Mod Undo  */
/********************/

typedef struct _LayerModUndo LayerModUndo;

struct _LayerModUndo
{
  GimpLayer    *layer;
  TileManager  *tiles;
  GimpImageType type;
  gint          offset_x;
  gint		offset_y;
};

static gboolean undo_pop_layer_mod  (GimpUndo            *undo,
                                     GimpImage           *gimage,
                                     GimpUndoMode         undo_mode,
                                     GimpUndoAccumulator *accum);
static void     undo_free_layer_mod (GimpUndo            *undo,
                                     GimpImage           *gimage,
                                     GimpUndoMode         undo_mode);

gboolean
undo_push_layer_mod (GimpImage *gimage,
		     GimpLayer *layer)
{
  GimpUndo    *new;
  TileManager *tiles;
  gsize        size;

  tiles = GIMP_DRAWABLE (layer)->tiles;

  size = sizeof (LayerModUndo) + tile_manager_get_memsize (tiles);

  if ((new = gimp_image_undo_push (gimage,
                                   size, sizeof (LayerModUndo),
                                   LAYER_MOD_UNDO, NULL,
                                   TRUE,
                                   undo_pop_layer_mod,
                                   undo_free_layer_mod)))
    {
      LayerModUndo *lmu;

      lmu = new->data;

      lmu->layer     = layer;
      lmu->tiles     = tiles;
      lmu->type      = GIMP_DRAWABLE (layer)->type;
      lmu->offset_x  = GIMP_DRAWABLE (layer)->offset_x;
      lmu->offset_y  = GIMP_DRAWABLE (layer)->offset_y;

      return TRUE;
    }

  tile_manager_destroy (tiles);

  return FALSE;
}

static gboolean
undo_pop_layer_mod (GimpUndo            *undo,
                    GimpImage           *gimage,
                    GimpUndoMode         undo_mode,
                    GimpUndoAccumulator *accum)
{
  LayerModUndo  *lmu;
  GimpImageType  layer_type;
  gint           offset_x, offset_y;
  TileManager   *tiles;
  GimpLayer     *layer;

  lmu = (LayerModUndo *) undo->data;

  layer = lmu->layer;

  /*  Issue the first update  */
  gimp_image_update (gimage,
                     GIMP_DRAWABLE (layer)->offset_x,
                     GIMP_DRAWABLE (layer)->offset_y,
                     GIMP_DRAWABLE (layer)->width,
                     GIMP_DRAWABLE (layer)->height);

  tiles      = lmu->tiles;
  layer_type = lmu->type;
  offset_x   = lmu->offset_x;
  offset_y   = lmu->offset_y;

  lmu->tiles    = GIMP_DRAWABLE (layer)->tiles;
  lmu->type     = GIMP_DRAWABLE (layer)->type;
  lmu->offset_x = GIMP_DRAWABLE (layer)->offset_x;
  lmu->offset_y = GIMP_DRAWABLE (layer)->offset_y;

  GIMP_DRAWABLE (layer)->tiles     = tiles;
  GIMP_DRAWABLE (layer)->width     = tile_manager_width (tiles);
  GIMP_DRAWABLE (layer)->height    = tile_manager_height (tiles);
  GIMP_DRAWABLE (layer)->bytes     = tile_manager_bpp (tiles);
  GIMP_DRAWABLE (layer)->type      = layer_type;
  GIMP_DRAWABLE (layer)->has_alpha = GIMP_IMAGE_TYPE_HAS_ALPHA (layer_type);
  GIMP_DRAWABLE (layer)->offset_x  = offset_x;
  GIMP_DRAWABLE (layer)->offset_y  = offset_y;

  if (layer->mask)
    {
      GIMP_DRAWABLE (layer->mask)->offset_x = offset_x;
      GIMP_DRAWABLE (layer->mask)->offset_y = offset_y;
    }

  if (GIMP_IMAGE_TYPE_HAS_ALPHA (GIMP_DRAWABLE (layer)->type) !=
      GIMP_IMAGE_TYPE_HAS_ALPHA (lmu->type) &&
      GIMP_ITEM (layer)->gimage->layers->num_children == 1)
    {
      gimp_image_alpha_changed (GIMP_ITEM (layer)->gimage);
    }

  if (GIMP_DRAWABLE (layer)->width  != tile_manager_width (lmu->tiles) ||
      GIMP_DRAWABLE (layer)->height != tile_manager_height (lmu->tiles))
    {
      gimp_viewable_size_changed (GIMP_VIEWABLE (layer));
    }

  /*  Issue the second update  */
  gimp_drawable_update (GIMP_DRAWABLE (layer),
			0, 0, 
			GIMP_DRAWABLE (layer)->width,
			GIMP_DRAWABLE (layer)->height);

  return TRUE;
}

static void
undo_free_layer_mod (GimpUndo     *undo,
                     GimpImage    *gimage,
                     GimpUndoMode  undo_mode)
{
  LayerModUndo *lmu;

  lmu = (LayerModUndo *) undo->data;

  tile_manager_destroy (lmu->tiles);
  g_free (lmu);
}


/********************************/
/*  Layer Mask Add/Remove Undo  */
/********************************/

typedef struct _LayerMaskUndo LayerMaskUndo;

struct _LayerMaskUndo
{
  GimpLayer     *layer;    /*  the layer       */
  GimpLayerMask *mask;     /*  the layer mask  */
};

static gboolean undo_push_layer_mask (GimpImage           *gimage,
                                      GimpUndoType         type,
                                      GimpLayer           *layer,
                                      GimpLayerMask       *mask);
static gboolean undo_pop_layer_mask  (GimpUndo            *undo,
                                      GimpImage           *gimage,
                                      GimpUndoMode         undo_mode,
                                      GimpUndoAccumulator *accum);
static void     undo_free_layer_mask (GimpUndo            *undo,
                                      GimpImage           *gimage,
                                      GimpUndoMode         undo_mode);

gboolean
undo_push_layer_mask_add (GimpImage     *gimage,
                          GimpLayer     *layer,
                          GimpLayerMask *mask)
{
  return undo_push_layer_mask (gimage, LAYER_MASK_ADD_UNDO,
                               layer, mask);
}

gboolean
undo_push_layer_mask_remove (GimpImage     *gimage,
                             GimpLayer     *layer,
                             GimpLayerMask *mask)
{
  return undo_push_layer_mask (gimage, LAYER_MASK_REMOVE_UNDO,
                               layer, mask);
}

static gboolean
undo_push_layer_mask (GimpImage     *gimage,
		      GimpUndoType       type,
                      GimpLayer     *layer,
                      GimpLayerMask *mask)
{
  GimpUndo *new;
  gsize     size;

  g_return_val_if_fail (type == LAYER_MASK_ADD_UNDO ||
			type == LAYER_MASK_REMOVE_UNDO,
			FALSE);

  size = sizeof (LayerMaskUndo) + gimp_object_get_memsize (GIMP_OBJECT (mask));

  if ((new = gimp_image_undo_push (gimage,
                                   size,
                                   sizeof (LayerMaskUndo),
                                   type, NULL,
                                   TRUE,
                                   undo_pop_layer_mask,
                                   undo_free_layer_mask)))
    {
      LayerMaskUndo *lmu;

      lmu = new->data;

      lmu->layer = g_object_ref (mask);
      lmu->mask  = mask;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_layer_mask (GimpUndo            *undo,
                     GimpImage           *gimage,
                     GimpUndoMode         undo_mode,
                     GimpUndoAccumulator *accum)
{
  LayerMaskUndo *lmu;

  lmu = (LayerMaskUndo *) undo->data;

  if ((undo_mode == GIMP_UNDO_MODE_UNDO && undo->undo_type == LAYER_MASK_ADD_UNDO) ||
      (undo_mode == GIMP_UNDO_MODE_REDO && undo->undo_type == LAYER_MASK_REMOVE_UNDO))
    {
      /*  remove layer mask  */

      gimp_layer_apply_mask (lmu->layer, GIMP_MASK_DISCARD, FALSE);
    }
  else
    {
      /*  restore layer  */

      gimp_layer_add_mask (lmu->layer, lmu->mask, FALSE);
    }

  return TRUE;
}

static void
undo_free_layer_mask (GimpUndo     *undo,
                      GimpImage    *gimage,
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
  GimpLayer *layer;
  gint       old_position;
};

static gboolean undo_pop_layer_reposition  (GimpUndo            *undo,
                                            GimpImage           *gimage,
                                            GimpUndoMode         undo_mode,
                                            GimpUndoAccumulator *accum);
static void     undo_free_layer_reposition (GimpUndo            *undo,
                                            GimpImage           *gimage,
                                            GimpUndoMode         undo_mode);

gboolean
undo_push_layer_reposition (GimpImage *gimage, 
			    GimpLayer *layer)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (LayerRepositionUndo),
                                   sizeof (LayerRepositionUndo), 
                                   LAYER_REPOSITION_UNDO, NULL,
                                   TRUE,
                                   undo_pop_layer_reposition,
                                   undo_free_layer_reposition)))
    {
      LayerRepositionUndo *lru;

      lru = new->data;

      lru->layer        = layer;
      lru->old_position = gimp_image_get_layer_index (gimage, layer);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_layer_reposition (GimpUndo            *undo,
                           GimpImage           *gimage,
                           GimpUndoMode         undo_mode,
                           GimpUndoAccumulator *accum)
{
  LayerRepositionUndo *lru;
  gint                 pos;

  lru = (LayerRepositionUndo *) undo->data;

  /* what's the layer's current index? */
  pos = gimp_image_get_layer_index (gimage, lru->layer);
  gimp_image_position_layer (gimage, lru->layer, lru->old_position, FALSE);

  lru->old_position = pos;

  return TRUE;
}

static void
undo_free_layer_reposition (GimpUndo     *undo,
                            GimpImage    *gimage,
                            GimpUndoMode  undo_mode)
{
  g_free (undo->data);
}


/*****************************/
/*  Layer displacement Undo  */
/*****************************/

typedef struct _LayerDisplaceUndo LayerDisplaceUndo;

struct _LayerDisplaceUndo
{
  GimpLayer *layer;
  gint       offset_x;
  gint       offset_y;
  GSList    *path_undo;
};

static gboolean undo_pop_layer_displace  (GimpUndo            *undo,
                                          GimpImage           *gimage,
                                          GimpUndoMode         undo_mode,
                                          GimpUndoAccumulator *accum);
static void     undo_free_layer_displace (GimpUndo            *undo,
                                          GimpImage           *gimage,
                                          GimpUndoMode         undo_mode);

gboolean
undo_push_layer_displace (GimpImage *gimage,
			  GimpLayer *layer)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (LayerDisplaceUndo),
                                   sizeof (LayerDisplaceUndo),
                                   LAYER_DISPLACE_UNDO, NULL,
                                   TRUE,
                                   undo_pop_layer_displace,
                                   undo_free_layer_displace)))
    {
      LayerDisplaceUndo *ldu;

      ldu = new->data;

      ldu->layer     = g_object_ref (layer);
      ldu->offset_x  = GIMP_DRAWABLE (layer)->offset_x;
      ldu->offset_y  = GIMP_DRAWABLE (layer)->offset_y;
      ldu->path_undo = path_transform_start_undo (gimage);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_layer_displace (GimpUndo            *undo,
                         GimpImage           *gimage,
                         GimpUndoMode         undo_mode,
                         GimpUndoAccumulator *accum)
{
  LayerDisplaceUndo *ldu;
  GimpLayer         *layer;
  gint               old_offset_x;
  gint               old_offset_y;

  ldu = (LayerDisplaceUndo *) undo->data;

  layer = ldu->layer;

  old_offset_x = GIMP_DRAWABLE (layer)->offset_x;
  old_offset_y = GIMP_DRAWABLE (layer)->offset_y;
  gimp_drawable_update (GIMP_DRAWABLE (layer),
                        0, 0,
                        GIMP_DRAWABLE (layer)->width,
                        GIMP_DRAWABLE (layer)->height);

  GIMP_DRAWABLE (layer)->offset_x = ldu->offset_x;
  GIMP_DRAWABLE (layer)->offset_y = ldu->offset_y;
  gimp_drawable_update (GIMP_DRAWABLE (layer),
                        0, 0,
                        GIMP_DRAWABLE (layer)->width,
                        GIMP_DRAWABLE (layer)->height);

  if (layer->mask) 
    {
      GIMP_DRAWABLE (layer->mask)->offset_x = ldu->offset_x;
      GIMP_DRAWABLE (layer->mask)->offset_y = ldu->offset_y;
      gimp_drawable_update (GIMP_DRAWABLE (layer->mask),
                            0, 0,
                            GIMP_DRAWABLE (layer->mask)->width,
                            GIMP_DRAWABLE (layer->mask)->height);
    }

  /*  invalidate the selection boundary because of a layer modification  */
  gimp_layer_invalidate_boundary (layer);

  ldu->offset_x = old_offset_x;
  ldu->offset_y = old_offset_y;

  /* Now undo paths bits */
  if (ldu->path_undo)
    path_transform_do_undo (gimage, ldu->path_undo);

  return TRUE;
}

static void
undo_free_layer_displace (GimpUndo     *undo,
                          GimpImage    *gimage,
                          GimpUndoMode  undo_mode)
{
  LayerDisplaceUndo *ldu;

  ldu = (LayerDisplaceUndo *) undo->data;

  g_object_unref (ldu->layer);

  if (ldu->path_undo)
    path_transform_free_undo (ldu->path_undo);  

  g_free (ldu);
}


/*****************************/
/*  Add/Remove Channel Undo  */
/*****************************/

typedef struct _ChannelUndo ChannelUndo;

struct _ChannelUndo
{
  GimpChannel *channel;         /*  the actual channel          */
  gint         prev_position;   /*  former position in list     */
  GimpChannel *prev_channel;    /*  previous active channel     */
};

static gboolean undo_push_channel (GimpImage           *gimage,
                                   GimpUndoType         type,
                                   GimpChannel         *channel,
                                   gint                 prev_position,
                                   GimpChannel         *prev_channel);
static gboolean undo_pop_channel  (GimpUndo            *undo,
                                   GimpImage           *gimage,
                                   GimpUndoMode         undo_mode,
                                   GimpUndoAccumulator *accum);
static void     undo_free_channel (GimpUndo            *undo,
                                   GimpImage           *gimage,
                                   GimpUndoMode         undo_mode);

gboolean
undo_push_channel_add (GimpImage   *gimage,
                       GimpChannel *channel,
                       gint         prev_position,
                       GimpChannel *prev_channel)
{
  return undo_push_channel (gimage, CHANNEL_ADD_UNDO,
                            channel, prev_position, prev_channel);
}

gboolean
undo_push_channel_remove (GimpImage   *gimage,
                          GimpChannel *channel,
                          gint         prev_position,
                          GimpChannel *prev_channel)
{
  return undo_push_channel (gimage, CHANNEL_REMOVE_UNDO,
                            channel, prev_position, prev_channel);
}

static gboolean
undo_push_channel (GimpImage   *gimage,
		   GimpUndoType     type,
                   GimpChannel *channel,
                   gint         prev_position,
                   GimpChannel *prev_channel)
{
  GimpUndo *new;
  gsize     size;

  g_return_val_if_fail (type == CHANNEL_ADD_UNDO ||
			type == CHANNEL_REMOVE_UNDO,
			FALSE);

  size = sizeof (ChannelUndo) + gimp_object_get_memsize (GIMP_OBJECT (channel));

  if ((new = gimp_image_undo_push (gimage,
                                   size,
                                   sizeof (ChannelUndo),
                                   type, NULL,
                                   TRUE,
                                   undo_pop_channel,
                                   undo_free_channel)))
    {
      ChannelUndo *cu;

      cu = new->data;

      cu->channel       = g_object_ref (channel);
      cu->prev_position = prev_position;
      cu->prev_channel  = prev_channel;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_channel (GimpUndo            *undo,
                  GimpImage           *gimage,
                  GimpUndoMode         undo_mode,
                  GimpUndoAccumulator *accum)
{
  ChannelUndo *cu;

  cu = (ChannelUndo *) undo->data;

  if ((undo_mode == GIMP_UNDO_MODE_UNDO && undo->undo_type == CHANNEL_ADD_UNDO) ||
      (undo_mode == GIMP_UNDO_MODE_REDO && undo->undo_type == CHANNEL_REMOVE_UNDO))
    {
      /*  remove channel  */

      /*  record the current position  */
      cu->prev_position = gimp_image_get_channel_index (gimage, cu->channel);

      /*  remove the channel  */
      gimp_container_remove (gimage->channels, GIMP_OBJECT (cu->channel));

      /*  if exists, set the previous channel  */
      if (cu->prev_channel)
        gimp_image_set_active_channel (gimage, cu->prev_channel);

      /*  update the area  */
      gimp_drawable_update (GIMP_DRAWABLE (cu->channel),
			    0, 0, 
			    GIMP_DRAWABLE (cu->channel)->width,
			    GIMP_DRAWABLE (cu->channel)->height);
    }
  else
    {
      /*  restore channel  */

      /*  record the active channel  */
      cu->prev_channel = gimp_image_get_active_channel (gimage);

      /*  add the new channel  */
      gimp_container_insert (gimage->channels, 
			     GIMP_OBJECT (cu->channel),	cu->prev_position);
 
      /*  set the new channel  */
      gimp_image_set_active_channel (gimage, cu->channel);

      /*  update the area  */
      gimp_drawable_update (GIMP_DRAWABLE (cu->channel),
			    0, 0, 
			    GIMP_DRAWABLE (cu->channel)->width,
			    GIMP_DRAWABLE (cu->channel)->height);
    }

  return TRUE;
}

static void
undo_free_channel (GimpUndo     *undo,
                   GimpImage    *gimage,
                   GimpUndoMode  undo_mode)
{
  ChannelUndo *cu;

  cu = (ChannelUndo *) undo->data;

  g_object_unref (cu->channel);

  g_free (cu);
}


/**********************/
/*  Channel Mod Undo  */
/**********************/

typedef struct _ChannelModUndo ChannelModUndo;

struct _ChannelModUndo
{
  GimpChannel *channel;
  TileManager *tiles;
};

static gboolean undo_pop_channel_mod  (GimpUndo            *undo,
                                       GimpImage           *gimage,
                                       GimpUndoMode         undo_mode,
                                       GimpUndoAccumulator *accum);
static void     undo_free_channel_mod (GimpUndo            *undo,
                                       GimpImage           *gimage,
                                       GimpUndoMode         undo_mode);

gboolean
undo_push_channel_mod (GimpImage   *gimage,
		       GimpChannel *channel)
{
  TileManager *tiles;
  GimpUndo    *new;
  gsize        size;

  tiles = GIMP_DRAWABLE (channel)->tiles;

  size = sizeof (ChannelModUndo) + tile_manager_get_memsize (tiles);

  if ((new = gimp_image_undo_push (gimage,
                                   size,
                                   sizeof (ChannelModUndo),
                                   CHANNEL_MOD_UNDO, NULL,
                                   TRUE,
                                   undo_pop_channel_mod,
                                   undo_free_channel_mod)))
    {
      ChannelModUndo *cmu;

      cmu = new->data;

      cmu->channel = channel;
      cmu->tiles   = tiles;

      return TRUE;
    }

  tile_manager_destroy (tiles);

  return FALSE;
}

static gboolean
undo_pop_channel_mod (GimpUndo            *undo,
                      GimpImage           *gimage,
                      GimpUndoMode         undo_mode,
                      GimpUndoAccumulator *accum)
{
  ChannelModUndo *cmu;
  TileManager    *tiles;
  GimpChannel    *channel;

  cmu = (ChannelModUndo *) undo->data;

  channel = cmu->channel;

  /*  Issue the first update  */
  gimp_drawable_update (GIMP_DRAWABLE (channel),
			0, 0, 
			GIMP_DRAWABLE (channel)->width,
			GIMP_DRAWABLE (channel)->height);

  tiles = cmu->tiles;

  cmu->tiles = GIMP_DRAWABLE (channel)->tiles;

  GIMP_DRAWABLE (channel)->tiles       = tiles;
  GIMP_DRAWABLE (channel)->width       = tile_manager_width (tiles);
  GIMP_DRAWABLE (channel)->height      = tile_manager_height (tiles);
  GIMP_CHANNEL (channel)->bounds_known = FALSE; 

  if (GIMP_DRAWABLE (channel)->width  != tile_manager_width (cmu->tiles) ||
      GIMP_DRAWABLE (channel)->height != tile_manager_height (cmu->tiles))
    {
      gimp_viewable_size_changed (GIMP_VIEWABLE (channel));
    }

  /*  Issue the second update  */
  gimp_drawable_update (GIMP_DRAWABLE (channel),
			0, 0, 
			GIMP_DRAWABLE (channel)->width,
			GIMP_DRAWABLE (channel)->height);

  return TRUE;
}

static void
undo_free_channel_mod (GimpUndo     *undo,
                       GimpImage    *gimage,
                       GimpUndoMode  undo_mode)
{
  ChannelModUndo *cmu;

  cmu = (ChannelModUndo *) undo->data;

  tile_manager_destroy (cmu->tiles);

  g_free (cmu);
}


/******************************/
/*  Channel re-position Undo  */
/******************************/

typedef struct _ChannelRepositionUndo ChannelRepositionUndo;

struct _ChannelRepositionUndo
{
  GimpChannel *channel;
  gint         old_position;
};

static gboolean undo_pop_channel_reposition  (GimpUndo            *undo,
                                              GimpImage           *gimage,
                                              GimpUndoMode         undo_mode,
                                              GimpUndoAccumulator *accum);
static void     undo_free_channel_reposition (GimpUndo            *undo,
                                              GimpImage           *gimage,
                                              GimpUndoMode         undo_mode);

gboolean
undo_push_channel_reposition (GimpImage   *gimage, 
                              GimpChannel *channel)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (ChannelRepositionUndo),
                                   sizeof (ChannelRepositionUndo),
                                   CHANNEL_REPOSITION_UNDO, NULL,
                                   TRUE,
                                   undo_pop_channel_reposition,
                                   undo_free_channel_reposition)))
    {
      ChannelRepositionUndo *cru;

      cru = new->data;

      cru->channel      = channel;
      cru->old_position = gimp_image_get_channel_index (gimage, channel);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_channel_reposition (GimpUndo            *undo,
                             GimpImage           *gimage,
                             GimpUndoMode         undo_mode,
                             GimpUndoAccumulator *accum)
{
  ChannelRepositionUndo *cru;
  gint                   pos;

  cru = (ChannelRepositionUndo *) undo->data;

  /* what's the channel's current index? */
  pos = gimp_image_get_channel_index (gimage, cru->channel);
  gimp_image_position_channel (gimage, cru->channel, cru->old_position, FALSE);

  cru->old_position = pos;

  return TRUE;
}

static void
undo_free_channel_reposition (GimpUndo     *undo,
                              GimpImage    *gimage,
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
  GimpVectors *vectors;         /*  the actual vectors          */
  gint         prev_position;   /*  former position in list     */
  GimpVectors *prev_vectors;    /*  previous active vectors     */
};

static gboolean undo_push_vectors (GimpImage           *gimage,
                                   GimpUndoType         type,
                                   GimpVectors         *vectors,
                                   gint                 prev_position,
                                   GimpVectors         *prev_vectors);
static gboolean undo_pop_vectors  (GimpUndo            *undo,
                                   GimpImage           *gimage,
                                   GimpUndoMode         undo_mode,
                                   GimpUndoAccumulator *accum);
static void     undo_free_vectors (GimpUndo            *undo,
                                   GimpImage           *gimage,
                                   GimpUndoMode         undo_mode);

gboolean
undo_push_vectors_add (GimpImage   *gimage,
                       GimpVectors *vectors,
                       gint         prev_position,
                       GimpVectors *prev_vectors)
{
  return undo_push_vectors (gimage, VECTORS_ADD_UNDO,
                            vectors, prev_position, prev_vectors);
}

gboolean
undo_push_vectors_remove (GimpImage   *gimage,
                          GimpVectors *vectors,
                          gint         prev_position,
                          GimpVectors *prev_vectors)
{
  return undo_push_vectors (gimage, VECTORS_REMOVE_UNDO,
                            vectors, prev_position, prev_vectors);
}

static gboolean
undo_push_vectors (GimpImage   *gimage,
		   GimpUndoType     type,
                   GimpVectors *vectors,
                   gint         prev_position,
                   GimpVectors *prev_vectors)
{
  GimpUndo *new;
  gsize     size;

  g_return_val_if_fail (type == VECTORS_ADD_UNDO ||
			type == VECTORS_REMOVE_UNDO,
			FALSE);

  size = sizeof (VectorsUndo) + gimp_object_get_memsize (GIMP_OBJECT (vectors));

  if ((new = gimp_image_undo_push (gimage,
                                   size,
                                   sizeof (VectorsUndo),
                                   type, NULL,
                                   TRUE,
                                   undo_pop_vectors,
                                   undo_free_vectors)))
    {
      VectorsUndo *vu;

      vu = new->data;

      vu->vectors       = g_object_ref (vectors);
      vu->prev_position = prev_position;
      vu->prev_vectors  = prev_vectors;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_vectors (GimpUndo            *undo,
                  GimpImage           *gimage,
                  GimpUndoMode         undo_mode,
                  GimpUndoAccumulator *accum)
{
  VectorsUndo *vu;

  vu = (VectorsUndo *) undo->data;

  if ((undo_mode == GIMP_UNDO_MODE_UNDO && undo->undo_type == VECTORS_ADD_UNDO) ||
      (undo_mode == GIMP_UNDO_MODE_REDO && undo->undo_type == VECTORS_REMOVE_UNDO))
    {
      /*  remove vectors  */

      /*  record the current position  */
      vu->prev_position = gimp_image_get_vectors_index (gimage, vu->vectors);

      /*  remove the vectors  */
      gimp_container_remove (gimage->vectors, GIMP_OBJECT (vu->vectors));

      /*  if exists, set the previous vectors  */
      if (vu->prev_vectors)
        gimp_image_set_active_vectors (gimage, vu->prev_vectors);
    }
  else
    {
      /*  restore vectors  */

      /*  record the active vectors  */
      vu->prev_vectors = gimp_image_get_active_vectors (gimage);

      /*  add the new vectors  */
      gimp_container_insert (gimage->vectors, 
			     GIMP_OBJECT (vu->vectors),	vu->prev_position);
 
      /*  set the new vectors  */
      gimp_image_set_active_vectors (gimage, vu->vectors);
    }

  return TRUE;
}

static void
undo_free_vectors (GimpUndo     *undo,
                   GimpImage    *gimage,
                   GimpUndoMode  undo_mode)
{
  VectorsUndo *vu;

  vu = (VectorsUndo *) undo->data;

  g_object_unref (vu->vectors);

  g_free (vu);
}


/**********************/
/*  Vectors Mod Undo  */
/**********************/

typedef struct _VectorsModUndo VectorsModUndo;

struct _VectorsModUndo
{
  GimpVectors *vectors;
  GimpVectors *undo_vectors;
};

static gboolean undo_pop_vectors_mod  (GimpUndo            *undo,
                                       GimpImage           *gimage,
                                       GimpUndoMode         undo_mode,
                                       GimpUndoAccumulator *accum);
static void     undo_free_vectors_mod (GimpUndo            *undo,
                                       GimpImage           *gimage,
                                       GimpUndoMode         undo_mode);

gboolean
undo_push_vectors_mod (GimpImage   *gimage,
		       GimpVectors *vectors)
{
  GimpUndo *new;
  gsize     size;

  size = (sizeof (VectorsModUndo) +
          gimp_object_get_memsize (GIMP_OBJECT (vectors)));

  if ((new = gimp_image_undo_push (gimage,
                                   size,
                                   sizeof (VectorsModUndo),
                                   VECTORS_MOD_UNDO, NULL,
                                   TRUE,
                                   undo_pop_vectors_mod,
                                   undo_free_vectors_mod)))
    {
      VectorsModUndo *vmu;

      vmu = new->data;

      vmu->vectors      = vectors;
      vmu->undo_vectors = GIMP_VECTORS (gimp_item_duplicate (GIMP_ITEM (vectors),
                                                             G_TYPE_FROM_INSTANCE (vectors),
                                                             FALSE));

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_vectors_mod (GimpUndo            *undo,
                      GimpImage           *gimage,
                      GimpUndoMode         undo_mode,
                      GimpUndoAccumulator *accum)
{
  VectorsModUndo *vmu;
  GimpVectors    *temp;

  vmu = (VectorsModUndo *) undo->data;

  temp = vmu->undo_vectors;

  vmu->undo_vectors =
    GIMP_VECTORS (gimp_item_duplicate (GIMP_ITEM (vmu->vectors),
                                       G_TYPE_FROM_INSTANCE (vmu->vectors),
                                       FALSE));

  gimp_vectors_copy_strokes (temp, vmu->vectors);

  g_object_unref (temp);

  return TRUE;
}

static void
undo_free_vectors_mod (GimpUndo     *undo,
                       GimpImage    *gimage,
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
  GimpVectors *vectors;
  gint         old_position;
};

static gboolean undo_pop_vectors_reposition  (GimpUndo            *undo,
                                              GimpImage           *gimage,
                                              GimpUndoMode         undo_mode,
                                              GimpUndoAccumulator *accum);
static void     undo_free_vectors_reposition (GimpUndo            *undo,
                                              GimpImage           *gimage,
                                              GimpUndoMode         undo_mode);

gboolean
undo_push_vectors_reposition (GimpImage   *gimage, 
                              GimpVectors *vectors)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (VectorsRepositionUndo),
                                   sizeof (VectorsRepositionUndo),
                                   VECTORS_REPOSITION_UNDO, NULL,
                                   TRUE,
                                   undo_pop_vectors_reposition,
                                   undo_free_vectors_reposition)))
    {
      VectorsRepositionUndo *vru;

      vru = new->data;

      vru->vectors      = vectors;
      vru->old_position = gimp_image_get_vectors_index (gimage, vectors);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_vectors_reposition (GimpUndo            *undo,
                             GimpImage           *gimage,
                             GimpUndoMode         undo_mode,
                             GimpUndoAccumulator *accum)
{
  VectorsRepositionUndo *vru;
  gint                   pos;

  vru = (VectorsRepositionUndo *) undo->data;

  /* what's the vectors's current index? */
  pos = gimp_image_get_vectors_index (gimage, vru->vectors);
  gimp_image_position_vectors (gimage, vru->vectors, vru->old_position, FALSE);

  vru->old_position = pos;

  return TRUE;
}

static void
undo_free_vectors_reposition (GimpUndo     *undo,
                              GimpImage    *gimage,
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
                                       GimpImage           *gimage,
                                       GimpUndoMode         undo_mode,
                                       GimpUndoAccumulator *accum);
static void     undo_free_fs_to_layer (GimpUndo            *undo,
                                       GimpImage           *gimage,
                                       GimpUndoMode         undo_mode);

gboolean
undo_push_fs_to_layer (GimpImage    *gimage,
                       GimpLayer    *floating_layer,
                       GimpDrawable *drawable)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (FStoLayerUndo),
                                   sizeof (FStoLayerUndo),
                                   FS_TO_LAYER_UNDO, NULL,
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

  tile_manager_destroy (floating_layer->fs.backing_store);
  floating_layer->fs.backing_store = NULL;

  return FALSE;
}

static gboolean
undo_pop_fs_to_layer (GimpUndo            *undo,
                      GimpImage           *gimage,
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
      gimp_image_set_active_layer (gimage, fsu->floating_layer);
      gimage->floating_sel = fsu->floating_layer;

      /*  restore the contents of the drawable  */
      floating_sel_store (fsu->floating_layer, 
			  GIMP_DRAWABLE (fsu->floating_layer)->offset_x, 
			  GIMP_DRAWABLE (fsu->floating_layer)->offset_y,
			  GIMP_DRAWABLE (fsu->floating_layer)->width, 
			  GIMP_DRAWABLE (fsu->floating_layer)->height);
      fsu->floating_layer->fs.initial = TRUE;

      /*  clear the selection  */
      gimp_layer_invalidate_boundary (fsu->floating_layer);

      /*  Update the preview for the gimage and underlying drawable  */
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (fsu->floating_layer));
      break;

    case GIMP_UNDO_MODE_REDO:
      /*  restore the contents of the drawable  */
      floating_sel_restore (fsu->floating_layer, 
			    GIMP_DRAWABLE (fsu->floating_layer)->offset_x, 
			    GIMP_DRAWABLE (fsu->floating_layer)->offset_y,
			    GIMP_DRAWABLE (fsu->floating_layer)->width, 
			    GIMP_DRAWABLE (fsu->floating_layer)->height);

      /*  Update the preview for the gimage and underlying drawable  */
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (fsu->floating_layer));

      /*  clear the selection  */
      gimp_layer_invalidate_boundary (fsu->floating_layer);

      /*  update the pointers  */
      fsu->floating_layer->fs.drawable = NULL;
      gimage->floating_sel = NULL;

      /*  Update the fs drawable  */
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (fsu->floating_layer));
      break;
    }

  gimp_image_floating_selection_changed (gimage);

  return TRUE;
}

static void
undo_free_fs_to_layer (GimpUndo     *undo,
                       GimpImage    *gimage,
                       GimpUndoMode  undo_mode)
{
  FStoLayerUndo *fsu;

  fsu = (FStoLayerUndo *) undo->data;

  if (undo_mode == GIMP_UNDO_MODE_UNDO)
    {
      tile_manager_destroy (fsu->floating_layer->fs.backing_store);
      fsu->floating_layer->fs.backing_store = NULL;
    }

  g_free (fsu);
}


/***********************************/
/*  Floating Selection Rigor Undo  */
/***********************************/

static gboolean undo_pop_fs_rigor  (GimpUndo            *undo,
                                    GimpImage           *gimage,
                                    GimpUndoMode         undo_mode,
                                    GimpUndoAccumulator *accum);
static void     undo_free_fs_rigor (GimpUndo            *undo,
                                    GimpImage           *gimage,
                                    GimpUndoMode         undo_mode);

gboolean
undo_push_fs_rigor (GimpImage *gimage,
		    gint       layer_ID)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (gint32),
                                   sizeof (gint32),
                                   FS_RIGOR_UNDO, NULL,
                                   FALSE,
                                   undo_pop_fs_rigor,
                                   undo_free_fs_rigor)))
    {
      gint32 *id;

      id = new->data;

      *id = layer_ID;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_fs_rigor (GimpUndo            *undo,
                   GimpImage           *gimage,
                   GimpUndoMode         undo_mode,
                   GimpUndoAccumulator *accum)
{
  gint32     layer_ID;
  GimpLayer *floating_layer;

  layer_ID = *((gint32 *) undo->data);

  if ((floating_layer = (GimpLayer *) gimp_item_get_by_ID (gimage->gimp, layer_ID)) == NULL)
    return FALSE;

  if (! gimp_layer_is_floating_sel (floating_layer))
    return FALSE;

  switch (undo_mode)
    {
    case GIMP_UNDO_MODE_UNDO:
      /*  restore the contents of drawable the floating layer is attached to  */
      if (floating_layer->fs.initial == FALSE)
	floating_sel_restore (floating_layer,
			      GIMP_DRAWABLE (floating_layer)->offset_x, 
			      GIMP_DRAWABLE (floating_layer)->offset_y,
			      GIMP_DRAWABLE (floating_layer)->width, 
			      GIMP_DRAWABLE (floating_layer)->height);
      floating_layer->fs.initial = TRUE;
      break;

    case GIMP_UNDO_MODE_REDO:
      /*  store the affected area from the drawable in the backing store  */
      floating_sel_store (floating_layer,
			  GIMP_DRAWABLE (floating_layer)->offset_x, 
			  GIMP_DRAWABLE (floating_layer)->offset_y,
			  GIMP_DRAWABLE (floating_layer)->width, 
			  GIMP_DRAWABLE (floating_layer)->height);
      floating_layer->fs.initial = TRUE;
      break;
    }

  gimp_image_floating_selection_changed (gimage);

  return TRUE;
}

static void
undo_free_fs_rigor (GimpUndo     *undo,
                    GimpImage    *gimage,
                    GimpUndoMode  undo_mode)
{
  g_free (undo->data);
}


/***********************************/
/*  Floating Selection Relax Undo  */
/***********************************/

static gboolean undo_pop_fs_relax  (GimpUndo            *undo,
                                    GimpImage           *gimage,
                                    GimpUndoMode         undo_mode,
                                    GimpUndoAccumulator *accum);
static void     undo_free_fs_relax (GimpUndo            *undo,
                                    GimpImage           *gimage,
                                    GimpUndoMode         undo_mode);

gboolean
undo_push_fs_relax (GimpImage *gimage,
		    gint32     layer_ID)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (gint32),
                                   sizeof (gint32),
                                   FS_RELAX_UNDO, NULL,
                                   FALSE,
                                   undo_pop_fs_relax,
                                   undo_free_fs_relax)))
    {
      gint32 *id;

      id = new->data;

      *id = layer_ID;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_fs_relax (GimpUndo            *undo,
                   GimpImage           *gimage,
                   GimpUndoMode         undo_mode,
                   GimpUndoAccumulator *accum)
{
  gint32     layer_ID;
  GimpLayer *floating_layer;

  layer_ID = *((gint32 *) undo->data);

  if ((floating_layer = (GimpLayer *) gimp_item_get_by_ID (gimage->gimp, layer_ID)) == NULL)
    return FALSE;

  if (! gimp_layer_is_floating_sel (floating_layer))
    return FALSE;

  switch (undo_mode)
    {
    case GIMP_UNDO_MODE_UNDO:
      /*  store the affected area from the drawable in the backing store  */
      floating_sel_store (floating_layer,
			  GIMP_DRAWABLE (floating_layer)->offset_x, 
			  GIMP_DRAWABLE (floating_layer)->offset_y,
			  GIMP_DRAWABLE (floating_layer)->width, 
			  GIMP_DRAWABLE (floating_layer)->height);
      floating_layer->fs.initial = TRUE;
      break;

    case GIMP_UNDO_MODE_REDO:
      /*  restore the contents of drawable the floating layer is attached to  */
      if (floating_layer->fs.initial == FALSE)
	floating_sel_restore (floating_layer,
			      GIMP_DRAWABLE (floating_layer)->offset_x, 
			      GIMP_DRAWABLE (floating_layer)->offset_y,
			      GIMP_DRAWABLE (floating_layer)->width, 
			      GIMP_DRAWABLE (floating_layer)->height);
      floating_layer->fs.initial = TRUE;
      break;
    }

  gimp_image_floating_selection_changed (gimage);

  return TRUE;
}

static void
undo_free_fs_relax (GimpUndo     *undo,
                    GimpImage    *gimage,
                    GimpUndoMode  undo_mode)
{
  g_free (undo->data);
}


/********************/
/*  Transform Undo  */
/********************/

typedef struct _TransformUndo TransformUndo;

struct _TransformUndo
{
  gint         tool_ID;
  GType        tool_type;

  TransInfo    trans_info;
  TileManager *original;
  gpointer     path_undo;
};

static gboolean undo_pop_transform  (GimpUndo            *undo,
                                     GimpImage           *gimage,
                                     GimpUndoMode         undo_mode,
                                     GimpUndoAccumulator *accum);
static void     undo_free_transform (GimpUndo            *undo,
                                     GimpImage           *gimage,
                                     GimpUndoMode         undo_mode);

gboolean
undo_push_transform (GimpImage   *gimage,
                     gint         tool_ID,
                     GType        tool_type,
                     gdouble     *trans_info,
                     TileManager *original,
                     GSList      *path_undo)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (TransformUndo),
                                   sizeof (TransformUndo),
                                   TRANSFORM_UNDO, NULL,
                                   FALSE,
                                   undo_pop_transform,
                                   undo_free_transform)))
    {
      TransformUndo *tu;
      gint           i;

      tu = new->data;

      tu->tool_ID   = tool_ID;
      tu->tool_type = tool_type;

      for (i = 0; i < TRAN_INFO_SIZE; i++)
	tu->trans_info[i] = trans_info[i];

      tu->original  = original;
      tu->path_undo = path_undo;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_transform (GimpUndo            *undo,
                    GimpImage           *gimage,
                    GimpUndoMode         undo_mode,
                    GimpUndoAccumulator *accum)
{
  GimpTool *active_tool;

  active_tool = tool_manager_get_active (gimage->gimp);

  if (GIMP_IS_TRANSFORM_TOOL (active_tool))
    {
      GimpTransformTool *tt;
      TransformUndo     *tu;

      tt = GIMP_TRANSFORM_TOOL (active_tool);
      tu = (TransformUndo *) undo->data;

      path_transform_do_undo (gimage, tu->path_undo);

      /*  only pop if the active tool is the tool that pushed this undo  */
      if (tu->tool_ID == active_tool->ID)
        {
          TileManager *temp;
          gdouble      d;
          gint         i;

          /*  swap the transformation information arrays  */
          for (i = 0; i < TRAN_INFO_SIZE; i++)
            {
              d                 = tu->trans_info[i];
              tu->trans_info[i] = tt->trans_info[i];
              tt->trans_info[i] = d;
            }

          /*  swap the original buffer--the source buffer for repeated transforms
           */
          temp         = tu->original;
          tu->original = tt->original;
          tt->original = temp;

          /*  If we're re-implementing the first transform, reactivate tool  */
          if (undo_mode == GIMP_UNDO_MODE_REDO && tt->original)
            {
              gimp_tool_control_activate (active_tool->control);

              gimp_draw_tool_resume (GIMP_DRAW_TOOL (tt));
            }
        }
    }

  return TRUE;
}

static void
undo_free_transform (GimpUndo     *undo,
                     GimpImage    *gimage,
                     GimpUndoMode  undo_mode)
{
  TransformUndo * tu;

  tu = (TransformUndo *) undo->data;

  if (tu->original)
    tile_manager_destroy (tu->original);
  path_transform_free_undo (tu->path_undo);
  g_free (tu);
}


/****************/
/*  Paint Undo  */
/****************/

typedef struct _PaintUndo PaintUndo;

struct _PaintUndo
{
  gint        core_ID;
  GType       core_type;

  GimpCoords  last_coords;
};

static gboolean undo_pop_paint  (GimpUndo            *undo,
                                 GimpImage           *gimage,
                                 GimpUndoMode         undo_mode,
                                 GimpUndoAccumulator *accum);
static void     undo_free_paint (GimpUndo            *undo,
                                 GimpImage           *gimage,
                                 GimpUndoMode         undo_mode);

gboolean
undo_push_paint (GimpImage  *gimage,
                 gint        core_ID,
                 GType       core_type,
                 GimpCoords *last_coords)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (PaintUndo),
                                   sizeof (PaintUndo),
                                   PAINT_UNDO, NULL,
                                   FALSE,
                                   undo_pop_paint,
                                   undo_free_paint)))
    {
      PaintUndo *pu;

      pu = new->data;

      pu->core_ID     = core_ID;
      pu->core_type   = core_type;
      pu->last_coords = *last_coords;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_paint (GimpUndo            *undo,
                GimpImage           *gimage,
                GimpUndoMode         undo_mode,
                GimpUndoAccumulator *accum)
{
  GimpTool *active_tool;

  active_tool = tool_manager_get_active (gimage->gimp);

  if (GIMP_IS_PAINT_TOOL (active_tool))
    {
      GimpPaintTool *pt;
      PaintUndo     *pu;

      pt = GIMP_PAINT_TOOL (active_tool);
      pu = (PaintUndo *) undo->data;

      /*  only pop if the active paint core is the one that pushed this undo  */
      if (pu->core_ID == pt->core->ID)
        {
          GimpCoords tmp_coords;

          /*  swap the paint core information  */
          tmp_coords            = pt->core->last_coords;
          pt->core->last_coords = pu->last_coords;
          pu->last_coords       = tmp_coords;
        }
    }

  return TRUE;
}

static void
undo_free_paint (GimpUndo     *undo,
                 GimpImage    *gimage,
                 GimpUndoMode  undo_mode)
{
  PaintUndo *pu;

  pu = (PaintUndo *) undo->data;

  g_free (pu);
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
                                    GimpImage           *gimage,
                                    GimpUndoMode         undo_mode,
                                    GimpUndoAccumulator *accum);
static void     undo_free_parasite (GimpUndo            *undo,
                                    GimpImage           *gimage,
                                    GimpUndoMode         undo_mode);

gboolean
undo_push_image_parasite (GimpImage *gimage,
			  gpointer   parasite)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (ParasiteUndo),
                                   sizeof (ParasiteUndo),
                                   PARASITE_ATTACH_UNDO, NULL,
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
undo_push_image_parasite_remove (GimpImage   *gimage,
				 const gchar *name)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (ParasiteUndo),
                                   sizeof (ParasiteUndo),
                                   PARASITE_REMOVE_UNDO, NULL,
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
undo_push_item_parasite (GimpImage *gimage,
                         GimpItem  *item,
                         gpointer   parasite)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (ParasiteUndo),
                                   sizeof (ParasiteUndo),
                                   PARASITE_ATTACH_UNDO, NULL,
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
undo_push_item_parasite_remove (GimpImage   *gimage,
                                GimpItem    *item,
                                const gchar *name)
{
  GimpUndo *new;

  if ((new = gimp_image_undo_push (gimage,
                                   sizeof (ParasiteUndo),
                                   sizeof (ParasiteUndo),
                                   PARASITE_REMOVE_UNDO, NULL,
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
                   GimpImage           *gimage,
                   GimpUndoMode         undo_mode,
                   GimpUndoAccumulator *accum)
{
  ParasiteUndo *data;
  GimpParasite *tmp;

  data = (ParasiteUndo *) undo->data;

  tmp = data->parasite;
  
  if (data->gimage)
    {
      data->parasite =
	gimp_parasite_copy (gimp_image_parasite_find (gimage, data->name));

      if (tmp)
	gimp_parasite_list_add (data->gimage->parasites, tmp);
      else
	gimp_parasite_list_remove (data->gimage->parasites, data->name);
    }
  else if (data->item)
    {
      data->parasite =
	gimp_parasite_copy (gimp_item_parasite_find (data->item,
                                                     data->name));
      if (tmp)
	gimp_parasite_list_add (data->item->parasites, tmp);
      else
	gimp_parasite_list_remove (data->item->parasites, data->name);
    }
  else
    {
      data->parasite = gimp_parasite_copy (gimp_parasite_find (gimage->gimp,
							       data->name));
      if (tmp)
	gimp_parasite_attach (gimage->gimp, tmp);
      else
	gimp_parasite_detach (gimage->gimp, data->name);
    }

  if (tmp)
    gimp_parasite_free (tmp);

  return TRUE;
}


static void
undo_free_parasite (GimpUndo     *undo,
                    GimpImage    *gimage,
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
                                   GimpImage           *gimage,
                                   GimpUndoMode         undo_mode,
                                   GimpUndoAccumulator *accum);

gboolean
undo_push_cantundo (GimpImage   *gimage,
		    const gchar *action)
{
  GimpUndo *new;

  /* This is the sole purpose of this type of undo: the ability to
   * mark an image as having been mutated, without really providing
   * any adequate undo facility.
   */

  if ((new = gimp_image_undo_push (gimage,
                                   0, 0,
                                   CANT_UNDO, NULL,
                                   TRUE,
                                   undo_pop_cantundo,
                                   NULL)))
    {
      new->data = (gpointer) action;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_cantundo (GimpUndo            *undo,
                   GimpImage           *gimage,
                   GimpUndoMode         undo_mode,
                   GimpUndoAccumulator *accum)
{
  gchar *action = undo->data;

  switch (undo_mode)
    {
    case GIMP_UNDO_MODE_UNDO:
      g_message (_("Can't undo %s"), action);
      break;

    case GIMP_UNDO_MODE_REDO:
      break;
    }

  return TRUE;
}
