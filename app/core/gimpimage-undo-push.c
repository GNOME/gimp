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

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "base/tile-manager.h"

#include "gimp.h"
#include "gimp-parasites.h"
#include "gimpchannelundo.h"
#include "gimpdrawableundo.h"
#include "gimpgrid.h"
#include "gimpguide.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpimage-grid.h"
#include "gimpimage-guides.h"
#include "gimpimage-sample-points.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimpimageundo.h"
#include "gimpitempropundo.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplayermask.h"
#include "gimplayerpropundo.h"
#include "gimplist.h"
#include "gimpparasitelist.h"
#include "gimpselection.h"

#include "text/gimptextlayer.h"
#include "text/gimptextundo.h"

#include "vectors/gimpvectors.h"

#include "gimp-intl.h"


/*********************/
/*  Image Type Undo  */
/*********************/

gboolean
gimp_image_undo_push_image_type (GimpImage   *image,
                                 const gchar *undo_desc)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_IMAGE_UNDO,
                                   0, 0,
                                   GIMP_UNDO_IMAGE_TYPE, undo_desc,
                                   GIMP_DIRTY_IMAGE,
                                   NULL, NULL,
                                   NULL)))
    {
      return TRUE;
    }

  return FALSE;
}


/*********************/
/*  Image Size Undo  */
/*********************/

gboolean
gimp_image_undo_push_image_size (GimpImage   *image,
                                 const gchar *undo_desc)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_IMAGE_UNDO,
                                   0, 0,
                                   GIMP_UNDO_IMAGE_SIZE, undo_desc,
                                   GIMP_DIRTY_IMAGE | GIMP_DIRTY_IMAGE_SIZE,
                                   NULL, NULL,
                                   NULL)))
    {
      return TRUE;
    }

  return FALSE;
}


/***************************/
/*  Image Resolution Undo  */
/***************************/

gboolean
gimp_image_undo_push_image_resolution (GimpImage   *image,
                                       const gchar *undo_desc)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_IMAGE_UNDO,
                                   0, 0,
                                   GIMP_UNDO_IMAGE_RESOLUTION, undo_desc,
                                   GIMP_DIRTY_IMAGE,
                                   NULL, NULL,
                                   NULL)))
    {
      return TRUE;
    }

  return FALSE;
}


/****************/
/*  Guide Undo  */
/****************/

typedef struct _GuideUndo GuideUndo;

struct _GuideUndo
{
  GimpGuide           *guide;
  GimpOrientationType  orientation;
  gint                 position;
};

static gboolean undo_pop_image_guide  (GimpUndo            *undo,
                                       GimpUndoMode         undo_mode,
                                       GimpUndoAccumulator *accum);
static void     undo_free_image_guide (GimpUndo            *undo,
                                       GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_image_guide (GimpImage   *image,
                                  const gchar *undo_desc,
                                  GimpGuide   *guide)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_GUIDE (guide), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_UNDO,
                                   sizeof (GuideUndo),
                                   sizeof (GuideUndo),
                                   GIMP_UNDO_IMAGE_GUIDE, undo_desc,
                                   GIMP_DIRTY_IMAGE_META,
                                   undo_pop_image_guide,
                                   undo_free_image_guide,
                                   NULL)))
    {
      GuideUndo *gu = new->data;

      gu->guide       = g_object_ref (guide);
      gu->orientation = gimp_guide_get_orientation (guide);
      gu->position    = gimp_guide_get_position (guide);

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
  GimpOrientationType  old_orientation;
  gint                 old_position;

  old_orientation = gimp_guide_get_orientation (gu->guide);
  old_position    = gimp_guide_get_position (gu->guide);

  /*  add and move guides manually (nor using the gimp_image_guide
   *  API), because we might be in the middle of an image resizing
   *  undo group and the guide's position might be temporarily out of
   *  image.
   */

  if (old_position == -1)
    {
      undo->image->guides = g_list_prepend (undo->image->guides, gu->guide);
      gimp_guide_set_position (gu->guide, gu->position);
      g_object_ref (gu->guide);
      gimp_image_update_guide (undo->image, gu->guide);
    }
  else if (gu->position == -1)
    {
      gimp_image_remove_guide (undo->image, gu->guide, FALSE);
    }
  else
    {
      gimp_image_update_guide (undo->image, gu->guide);
      gimp_guide_set_position (gu->guide, gu->position);
      gimp_image_update_guide (undo->image, gu->guide);
    }

  gimp_guide_set_orientation (gu->guide, gu->orientation);

  gu->position    = old_position;
  gu->orientation = old_orientation;

  return TRUE;
}

static void
undo_free_image_guide (GimpUndo     *undo,
                       GimpUndoMode  undo_mode)
{
  GuideUndo *gu = undo->data;

  g_object_unref (gu->guide);
  g_free (gu);
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
gimp_image_undo_push_image_grid (GimpImage   *image,
                                 const gchar *undo_desc,
                                 GimpGrid    *grid)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_GRID (grid), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_UNDO,
                                   sizeof (GridUndo),
                                   sizeof (GridUndo),
                                   GIMP_UNDO_IMAGE_GRID, undo_desc,
                                   GIMP_DIRTY_IMAGE_META,
                                   undo_pop_image_grid,
                                   undo_free_image_grid,
                                   NULL)))
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

  grid = gimp_config_duplicate (GIMP_CONFIG (undo->image->grid));

  gimp_image_set_grid (undo->image, gu->grid, FALSE);

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


/**********************/
/*  Sampe Point Undo  */
/**********************/

typedef struct _SamplePointUndo SamplePointUndo;

struct _SamplePointUndo
{
  GimpSamplePoint *sample_point;
  gint             x;
  gint             y;
};

static gboolean undo_pop_image_sample_point  (GimpUndo            *undo,
                                              GimpUndoMode         undo_mode,
                                              GimpUndoAccumulator *accum);
static void     undo_free_image_sample_point (GimpUndo            *undo,
                                              GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_image_sample_point (GimpImage       *image,
                                         const gchar     *undo_desc,
                                         GimpSamplePoint *sample_point)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (sample_point != NULL, FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_UNDO,
                                   sizeof (SamplePointUndo),
                                   sizeof (SamplePointUndo),
                                   GIMP_UNDO_IMAGE_SAMPLE_POINT, undo_desc,
                                   GIMP_DIRTY_IMAGE_META,
                                   undo_pop_image_sample_point,
                                   undo_free_image_sample_point,
                                   NULL)))
    {
      SamplePointUndo *spu = new->data;

      spu->sample_point = gimp_image_sample_point_ref (sample_point);
      spu->x            = sample_point->x;
      spu->y            = sample_point->y;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_image_sample_point (GimpUndo            *undo,
                             GimpUndoMode         undo_mode,
                             GimpUndoAccumulator *accum)
{
  SamplePointUndo *spu = undo->data;
  gint             old_x;
  gint             old_y;

  old_x = spu->sample_point->x;
  old_y = spu->sample_point->y;

  /*  add and move sample points manually (nor using the
   *  gimp_image_sample_point API), because we might be in the middle
   *  of an image resizing undo group and the sample point's position
   *  might be temporarily out of image.
   */

  if (spu->sample_point->x == -1)
    {
      undo->image->sample_points = g_list_append (undo->image->sample_points,
                                                   spu->sample_point);

      spu->sample_point->x = spu->x;
      spu->sample_point->y = spu->y;
      gimp_image_sample_point_ref (spu->sample_point);

      gimp_image_sample_point_added (undo->image, spu->sample_point);
      gimp_image_update_sample_point (undo->image, spu->sample_point);
    }
  else if (spu->x == -1)
    {
      gimp_image_remove_sample_point (undo->image, spu->sample_point, FALSE);
    }
  else
    {
      gimp_image_update_sample_point (undo->image, spu->sample_point);
      spu->sample_point->x = spu->x;
      spu->sample_point->y = spu->y;
      gimp_image_update_sample_point (undo->image, spu->sample_point);
    }

  spu->x = old_x;
  spu->y = old_y;

  return TRUE;
}

static void
undo_free_image_sample_point (GimpUndo     *undo,
                              GimpUndoMode  undo_mode)
{
  SamplePointUndo *gu = undo->data;

  gimp_image_sample_point_unref (gu->sample_point);
  g_free (gu);
}


/*******************/
/*  Colormap Undo  */
/*******************/

typedef struct _ColormapUndo ColormapUndo;

struct _ColormapUndo
{
  gint    num_colors;
  guchar *cmap;
};

static gboolean undo_pop_image_colormap  (GimpUndo            *undo,
                                          GimpUndoMode         undo_mode,
                                          GimpUndoAccumulator *accum);
static void     undo_free_image_colormap (GimpUndo            *undo,
                                          GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_image_colormap (GimpImage   *image,
                                     const gchar *undo_desc)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_UNDO,
                                   sizeof (ColormapUndo),
                                   sizeof (ColormapUndo),
                                   GIMP_UNDO_IMAGE_COLORMAP, undo_desc,
                                   GIMP_DIRTY_IMAGE,
                                   undo_pop_image_colormap,
                                   undo_free_image_colormap,
                                   NULL)))
    {
      ColormapUndo *cu = new->data;

      cu->num_colors = gimp_image_get_colormap_size (image);
      cu->cmap       = g_memdup (gimp_image_get_colormap (image),
                                 cu->num_colors * 3);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_image_colormap (GimpUndo            *undo,
                         GimpUndoMode         undo_mode,
                         GimpUndoAccumulator *accum)
{
  ColormapUndo *cu = undo->data;
  guchar       *cmap;
  gint          num_colors;

  num_colors = gimp_image_get_colormap_size (undo->image);
  cmap       = g_memdup (gimp_image_get_colormap (undo->image),
                         num_colors * 3);

  gimp_image_set_colormap (undo->image, cu->cmap, cu->num_colors, FALSE);

  if (cu->cmap)
    g_free (cu->cmap);

  cu->num_colors = num_colors;
  cu->cmap       = cmap;

  return TRUE;
}

static void
undo_free_image_colormap (GimpUndo     *undo,
                          GimpUndoMode  undo_mode)
{
  ColormapUndo *cu = undo->data;

  if (cu->cmap)
    g_free (cu->cmap);

  g_free (cu);
}


/*******************/
/*  Drawable Undo  */
/*******************/

gboolean
gimp_image_undo_push_drawable (GimpImage    *image,
                               const gchar  *undo_desc,
                               GimpDrawable *drawable,
                               TileManager  *tiles,
                               gboolean      sparse,
                               gint          x,
                               gint          y,
                               gint          width,
                               gint          height)
{
  GimpItem *item;
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (tiles != NULL, FALSE);
  g_return_val_if_fail (sparse == TRUE ||
                        tile_manager_width (tiles) == width, FALSE);
  g_return_val_if_fail (sparse == TRUE ||
                        tile_manager_height (tiles) == height, FALSE);

  item = GIMP_ITEM (drawable);

  g_return_val_if_fail (gimp_item_is_attached (item), FALSE);
  g_return_val_if_fail (sparse == FALSE ||
                        tile_manager_width (tiles) == gimp_item_width (item),
                        FALSE);
  g_return_val_if_fail (sparse == FALSE ||
                        tile_manager_height (tiles) == gimp_item_height (item),
                        FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_DRAWABLE_UNDO,
                                   0, 0,
                                   GIMP_UNDO_DRAWABLE, undo_desc,
                                   GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE,
                                   NULL, NULL,
                                   "item",   item,
                                   "tiles",  tiles,
                                   "sparse", sparse,
                                   "x",      x,
                                   "y",      y,
                                   "width",  width,
                                   "height", height,
                                   NULL)))
    {
      return TRUE;
    }

  return FALSE;
}


/***********************/
/*  Drawable Mod Undo  */
/***********************/

typedef struct _DrawableModUndo DrawableModUndo;

struct _DrawableModUndo
{
  TileManager   *tiles;
  GimpImageType  type;
  gint           offset_x;
  gint           offset_y;
};

static gboolean undo_pop_drawable_mod  (GimpUndo            *undo,
                                        GimpUndoMode         undo_mode,
                                        GimpUndoAccumulator *accum);
static void     undo_free_drawable_mod (GimpUndo            *undo,
                                        GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_drawable_mod (GimpImage    *image,
                                   const gchar  *undo_desc,
                                   GimpDrawable *drawable)
{
  GimpUndo *new;
  gint64    size;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);

  size = sizeof (DrawableModUndo) + tile_manager_get_memsize (drawable->tiles,
                                                              FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_ITEM_UNDO,
                                   size, sizeof (DrawableModUndo),
                                   GIMP_UNDO_DRAWABLE_MOD, undo_desc,
                                   GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE,
                                   undo_pop_drawable_mod,
                                   undo_free_drawable_mod,
                                   "item", drawable,
                                   NULL)))
    {
      DrawableModUndo *drawable_undo = new->data;

      drawable_undo->tiles    = tile_manager_ref (drawable->tiles);
      drawable_undo->type     = drawable->type;
      drawable_undo->offset_x = GIMP_ITEM (drawable)->offset_x;
      drawable_undo->offset_y = GIMP_ITEM (drawable)->offset_y;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_drawable_mod (GimpUndo            *undo,
                       GimpUndoMode         undo_mode,
                       GimpUndoAccumulator *accum)
{
  DrawableModUndo *drawable_undo = undo->data;
  GimpDrawable    *drawable      = GIMP_DRAWABLE (GIMP_ITEM_UNDO (undo)->item);
  TileManager     *tiles;
  GimpImageType    drawable_type;
  gint             offset_x, offset_y;

  undo->size -= tile_manager_get_memsize (drawable_undo->tiles, FALSE);

  tiles         = drawable_undo->tiles;
  drawable_type = drawable_undo->type;
  offset_x      = drawable_undo->offset_x;
  offset_y      = drawable_undo->offset_y;

  drawable_undo->tiles    = tile_manager_ref (drawable->tiles);
  drawable_undo->type     = drawable->type;
  drawable_undo->offset_x = GIMP_ITEM (drawable)->offset_x;
  drawable_undo->offset_y = GIMP_ITEM (drawable)->offset_y;

  gimp_drawable_set_tiles_full (drawable, FALSE, NULL,
                                tiles, drawable_type, offset_x, offset_y);
  tile_manager_unref (tiles);

  undo->size += tile_manager_get_memsize (drawable_undo->tiles, FALSE);

  return TRUE;
}

static void
undo_free_drawable_mod (GimpUndo     *undo,
                        GimpUndoMode  undo_mode)
{
  DrawableModUndo *drawable_undo = undo->data;

  tile_manager_unref (drawable_undo->tiles);
  g_free (drawable_undo);
}


/***************/
/*  Mask Undo  */
/***************/

gboolean
gimp_image_undo_push_mask (GimpImage   *image,
                           const gchar *undo_desc,
                           GimpChannel *mask)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (mask), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (mask)), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_CHANNEL_UNDO,
                                   0, 0,
                                   GIMP_UNDO_MASK, undo_desc,
                                   GIMP_IS_SELECTION (mask) ?
                                   GIMP_DIRTY_SELECTION :
                                   GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE,
                                   NULL, NULL,
                                   "item", mask,
                                   NULL)))
    {
      return TRUE;
    }

  return FALSE;
}


/**********************/
/*  Item Rename Undo  */
/**********************/

gboolean
gimp_image_undo_push_item_rename (GimpImage   *image,
                                  const gchar *undo_desc,
                                  GimpItem    *item)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (item), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_ITEM_PROP_UNDO,
                                   0, 0,
                                   GIMP_UNDO_ITEM_RENAME, undo_desc,
                                   GIMP_DIRTY_ITEM_META,
                                   NULL, NULL,
                                   "item", item,
                                   NULL)))
    {
      return TRUE;
    }

  return FALSE;
}


/****************************/
/*  Item displacement Undo  */
/****************************/

gboolean
gimp_image_undo_push_item_displace (GimpImage   *image,
                                    const gchar *undo_desc,
                                    GimpItem    *item)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (item), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_ITEM_PROP_UNDO,
                                   0, 0,
                                   GIMP_UNDO_ITEM_DISPLACE, undo_desc,
                                   GIMP_IS_DRAWABLE (item) ?
                                   GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE :
                                   GIMP_DIRTY_ITEM | GIMP_DIRTY_VECTORS,
                                   NULL, NULL,
                                   "item", item,
                                   NULL)))
    {
      return TRUE;
    }

  return FALSE;
}


/******************************/
/*  Item Visibility Undo  */
/******************************/

gboolean
gimp_image_undo_push_item_visibility (GimpImage   *image,
                                      const gchar *undo_desc,
                                      GimpItem    *item)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (item), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_ITEM_PROP_UNDO,
                                   0, 0,
                                   GIMP_UNDO_ITEM_VISIBILITY, undo_desc,
                                   GIMP_DIRTY_ITEM_META,
                                   NULL, NULL,
                                   "item", item,
                                   NULL)))
    {
      return TRUE;
    }

  return FALSE;
}


/**********************/
/*  Item linked Undo  */
/**********************/

gboolean
gimp_image_undo_push_item_linked (GimpImage   *image,
                                  const gchar *undo_desc,
                                  GimpItem    *item)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (item), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_ITEM_PROP_UNDO,
                                   0, 0,
                                   GIMP_UNDO_ITEM_LINKED, undo_desc,
                                   GIMP_DIRTY_ITEM_META,
                                   NULL, NULL,
                                   "item", item,
                                   NULL)))
    {
      return TRUE;
    }

  return FALSE;
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

static gboolean undo_push_layer (GimpImage           *image,
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
gimp_image_undo_push_layer_add (GimpImage   *image,
                                const gchar *undo_desc,
                                GimpLayer   *layer,
                                gint         prev_position,
                                GimpLayer   *prev_layer)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (! gimp_item_is_attached (GIMP_ITEM (layer)), FALSE);
  g_return_val_if_fail (prev_layer == NULL || GIMP_IS_LAYER (prev_layer),
                        FALSE);

  return undo_push_layer (image, undo_desc, GIMP_UNDO_LAYER_ADD,
                          layer, prev_position, prev_layer);
}

gboolean
gimp_image_undo_push_layer_remove (GimpImage   *image,
                                   const gchar *undo_desc,
                                   GimpLayer   *layer,
                                   gint         prev_position,
                                   GimpLayer   *prev_layer)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), FALSE);
  g_return_val_if_fail (prev_layer == NULL || GIMP_IS_LAYER (prev_layer),
                        FALSE);

  return undo_push_layer (image, undo_desc, GIMP_UNDO_LAYER_REMOVE,
                          layer, prev_position, prev_layer);
}

static gboolean
undo_push_layer (GimpImage    *image,
                 const gchar  *undo_desc,
                 GimpUndoType  type,
                 GimpLayer    *layer,
                 gint          prev_position,
                 GimpLayer    *prev_layer)
{
  GimpUndo *new;
  gint64    size;

  size = sizeof (LayerUndo);

  if (type == GIMP_UNDO_LAYER_REMOVE)
    size += gimp_object_get_memsize (GIMP_OBJECT (layer), NULL);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_ITEM_UNDO,
                                   size, sizeof (LayerUndo),
                                   type, undo_desc,
                                   GIMP_DIRTY_IMAGE_STRUCTURE,
                                   undo_pop_layer,
                                   undo_free_layer,
                                   "item", layer,
                                   NULL)))
    {
      LayerUndo *lu = new->data;

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
  LayerUndo *lu    = undo->data;
  GimpLayer *layer = GIMP_LAYER (GIMP_ITEM_UNDO (undo)->item);
  gboolean   old_has_alpha;

  old_has_alpha = gimp_image_has_alpha (undo->image);

  if ((undo_mode       == GIMP_UNDO_MODE_UNDO &&
       undo->undo_type == GIMP_UNDO_LAYER_ADD) ||
      (undo_mode       == GIMP_UNDO_MODE_REDO &&
       undo->undo_type == GIMP_UNDO_LAYER_REMOVE))
    {
      /*  remove layer  */

      undo->size += gimp_object_get_memsize (GIMP_OBJECT (layer), NULL);

      /*  record the current position  */
      lu->prev_position = gimp_image_get_layer_index (undo->image, layer);

      gimp_container_remove (undo->image->layers, GIMP_OBJECT (layer));
      undo->image->layer_stack = g_slist_remove (undo->image->layer_stack,
                                                  layer);

      if (gimp_layer_is_floating_sel (layer))
        {
          /*  invalidate the boundary *before* setting the
           *  floating_sel pointer to NULL because the selection's
           *  outline is affected by the floating_sel and won't be
           *  completely cleared otherwise (bug #160247).
           */
          gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (layer));

          undo->image->floating_sel = NULL;

          /*  activate the underlying drawable  */
          floating_sel_activate_drawable (layer);

          gimp_image_floating_selection_changed (undo->image);
        }
      else if (layer == gimp_image_get_active_layer (undo->image))
        {
          if (lu->prev_layer)
            {
              gimp_image_set_active_layer (undo->image, lu->prev_layer);
            }
          else if (undo->image->layer_stack)
            {
              gimp_image_set_active_layer (undo->image,
                                           undo->image->layer_stack->data);
            }
          else
            {
              gimp_image_set_active_layer (undo->image, NULL);
            }
        }

      gimp_item_removed (GIMP_ITEM (layer));
    }
  else
    {
      /*  restore layer  */

      undo->size -= gimp_object_get_memsize (GIMP_OBJECT (layer), NULL);

      /*  record the active layer  */
      lu->prev_layer = gimp_image_get_active_layer (undo->image);

      /*  if this is a floating selection, set the fs pointer  */
      if (gimp_layer_is_floating_sel (layer))
        undo->image->floating_sel = layer;

      gimp_container_insert (undo->image->layers,
                             GIMP_OBJECT (layer), lu->prev_position);
      gimp_image_set_active_layer (undo->image, layer);

      if (gimp_layer_is_floating_sel (layer))
        gimp_image_floating_selection_changed (undo->image);

      GIMP_ITEM (layer)->removed = FALSE;

      if (layer->mask)
        GIMP_ITEM (layer->mask)->removed = FALSE;
    }

  if (old_has_alpha != gimp_image_has_alpha (undo->image))
    accum->alpha_changed = TRUE;

  return TRUE;
}

static void
undo_free_layer (GimpUndo     *undo,
                 GimpUndoMode  undo_mode)
{
  g_free (undo->data);
}


/***************************/
/* Layer re-position Undo  */
/***************************/

gboolean
gimp_image_undo_push_layer_reposition (GimpImage   *image,
                                       const gchar *undo_desc,
                                       GimpLayer   *layer)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_LAYER_PROP_UNDO,
                                   0, 0,
                                   GIMP_UNDO_LAYER_REPOSITION, undo_desc,
                                   GIMP_DIRTY_IMAGE_STRUCTURE,
                                   NULL, NULL,
                                   "item", layer,
                                   NULL)))
    {
      return TRUE;
    }

  return FALSE;
}


/*********************/
/*  Layer Mode Undo  */
/*********************/

gboolean
gimp_image_undo_push_layer_mode (GimpImage   *image,
                                 const gchar *undo_desc,
                                 GimpLayer   *layer)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_LAYER_PROP_UNDO,
                                   0, 0,
                                   GIMP_UNDO_LAYER_MODE, undo_desc,
                                   GIMP_DIRTY_ITEM_META,
                                   NULL, NULL,
                                   "item", layer,
                                   NULL)))
    {
      return TRUE;
    }

  return FALSE;
}


/************************/
/*  Layer Opacity Undo  */
/************************/

gboolean
gimp_image_undo_push_layer_opacity (GimpImage   *image,
                                    const gchar *undo_desc,
                                    GimpLayer   *layer)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_LAYER_PROP_UNDO,
                                   0, 0,
                                   GIMP_UNDO_LAYER_OPACITY, undo_desc,
                                   GIMP_DIRTY_ITEM_META,
                                   NULL, NULL,
                                   "item", layer,
                                   NULL)))
    {
      return TRUE;
    }

  return FALSE;
}


/***************************/
/*  Layer Lock Alpha Undo  */
/***************************/

gboolean
gimp_image_undo_push_layer_lock_alpha (GimpImage   *image,
                                       const gchar *undo_desc,
                                       GimpLayer   *layer)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_LAYER_PROP_UNDO,
                                   0, 0,
                                   GIMP_UNDO_LAYER_LOCK_ALPHA, undo_desc,
                                   GIMP_DIRTY_ITEM_META,
                                   NULL, NULL,
                                   "item", layer,
                                   NULL)))
    {
      return TRUE;
    }

  return FALSE;
}


/*********************/
/*  Text Layer Undo  */
/*********************/

gboolean
gimp_image_undo_push_text_layer (GimpImage        *image,
                                 const gchar      *undo_desc,
                                 GimpTextLayer    *layer,
                                 const GParamSpec *pspec)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_TEXT_LAYER (layer), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_TEXT_UNDO,
                                   0, 0,
                                   GIMP_UNDO_TEXT_LAYER, undo_desc,
                                   GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE,
                                   NULL, NULL,
                                   "item",  layer,
                                   "param", pspec,
                                   NULL)))
    {
      return TRUE;
    }

  return FALSE;
}


/******************************/
/*  Text Layer Modified Undo  */
/******************************/

typedef struct _TextLayerModifiedUndo TextLayerModifiedUndo;

struct _TextLayerModifiedUndo
{
  gboolean old_modified;
};

static gboolean undo_pop_text_layer_modified  (GimpUndo            *undo,
                                               GimpUndoMode         undo_mode,
                                               GimpUndoAccumulator *accum);
static void     undo_free_text_layer_modified (GimpUndo            *undo,
                                               GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_text_layer_modified (GimpImage     *image,
                                          const gchar   *undo_desc,
                                          GimpTextLayer *layer)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_TEXT_LAYER (layer), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_ITEM_UNDO,
                                   sizeof (TextLayerModifiedUndo),
                                   sizeof (TextLayerModifiedUndo),
                                   GIMP_UNDO_TEXT_LAYER_MODIFIED, undo_desc,
                                   GIMP_DIRTY_ITEM_META,
                                   undo_pop_text_layer_modified,
                                   undo_free_text_layer_modified,
                                   "item", layer,
                                   NULL)))
    {
      TextLayerModifiedUndo *modified_undo = new->data;

      modified_undo->old_modified = layer->modified;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_text_layer_modified (GimpUndo            *undo,
                              GimpUndoMode         undo_mode,
                              GimpUndoAccumulator *accum)
{
  TextLayerModifiedUndo *modified_undo = undo->data;
  GimpTextLayer         *layer;
  gboolean               modified;

  layer = GIMP_TEXT_LAYER (GIMP_ITEM_UNDO (undo)->item);

#if 0
  g_print ("setting layer->modified from %s to %s\n",
           layer->modified ? "TRUE" : "FALSE",
           modified_undo->old_modified ? "TRUE" : "FALSE");
#endif

  modified = layer->modified;
  g_object_set (layer, "modified", modified_undo->old_modified, NULL);
  modified_undo->old_modified = modified;

  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer));

  return TRUE;
}

static void
undo_free_text_layer_modified (GimpUndo     *undo,
                               GimpUndoMode  undo_mode)
{
  g_free (undo->data);
}


/********************************/
/*  Layer Mask Add/Remove Undo  */
/********************************/

typedef struct _LayerMaskUndo LayerMaskUndo;

struct _LayerMaskUndo
{
  GimpLayerMask *mask;
};

static gboolean undo_push_layer_mask (GimpImage           *image,
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
gimp_image_undo_push_layer_mask_add (GimpImage     *image,
                                     const gchar   *undo_desc,
                                     GimpLayer     *layer,
                                     GimpLayerMask *mask)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER_MASK (mask), FALSE);
  g_return_val_if_fail (! gimp_item_is_attached (GIMP_ITEM (mask)), FALSE);

  return undo_push_layer_mask (image, undo_desc, GIMP_UNDO_LAYER_MASK_ADD,
                               layer, mask);
}

gboolean
gimp_image_undo_push_layer_mask_remove (GimpImage     *image,
                                        const gchar   *undo_desc,
                                        GimpLayer     *layer,
                                        GimpLayerMask *mask)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER_MASK (mask), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (mask)), FALSE);
  g_return_val_if_fail (mask->layer == layer, FALSE);
  g_return_val_if_fail (layer->mask == mask, FALSE);

  return undo_push_layer_mask (image, undo_desc, GIMP_UNDO_LAYER_MASK_REMOVE,
                               layer, mask);
}


static gboolean
undo_push_layer_mask (GimpImage     *image,
                      const gchar   *undo_desc,
                      GimpUndoType   type,
                      GimpLayer     *layer,
                      GimpLayerMask *mask)
{
  GimpUndo *new;
  gint64    size;

  size = sizeof (LayerMaskUndo);

  if (type == GIMP_UNDO_LAYER_MASK_REMOVE)
    size += gimp_object_get_memsize (GIMP_OBJECT (mask), NULL);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_ITEM_UNDO,
                                   size, sizeof (LayerMaskUndo),
                                   type, undo_desc,
                                   GIMP_DIRTY_IMAGE_STRUCTURE,
                                   undo_pop_layer_mask,
                                   undo_free_layer_mask,
                                   "item", layer,
                                   NULL)))
    {
      LayerMaskUndo *lmu = new->data;

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
  LayerMaskUndo *lmu   = undo->data;
  GimpLayer     *layer = GIMP_LAYER (GIMP_ITEM_UNDO (undo)->item);

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

      GIMP_ITEM (lmu->mask)->removed = FALSE;
    }

  return TRUE;
}

static void
undo_free_layer_mask (GimpUndo     *undo,
                      GimpUndoMode  undo_mode)
{
  LayerMaskUndo *lmu = undo->data;

  g_object_unref (lmu->mask);
  g_free (lmu);
}


/******************************/
/*  Layer Mask Property Undo  */
/******************************/

typedef struct _LayerMaskPropertyUndo LayerMaskPropertyUndo;

struct _LayerMaskPropertyUndo
{
  gboolean old_apply;
  gboolean old_show;
};

static gboolean undo_push_layer_mask_properties (GimpImage           *image,
                                                 GimpUndoType         undo_type,
                                                 const gchar         *undo_desc,
                                                 GimpLayerMask       *mask);
static gboolean undo_pop_layer_mask_properties  (GimpUndo            *undo,
                                                 GimpUndoMode         undo_mode,
                                                 GimpUndoAccumulator *accum);
static void     undo_free_layer_mask_properties (GimpUndo            *undo,
                                                 GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_layer_mask_apply (GimpImage     *image,
                                       const gchar   *undo_desc,
                                       GimpLayerMask *mask)
{
  return undo_push_layer_mask_properties (image, GIMP_UNDO_LAYER_MASK_APPLY,
                                          undo_desc, mask);
}

gboolean
gimp_image_undo_push_layer_mask_show (GimpImage     *image,
                                      const gchar   *undo_desc,
                                      GimpLayerMask *mask)
{
  return undo_push_layer_mask_properties (image, GIMP_UNDO_LAYER_MASK_SHOW,
                                          undo_desc, mask);
}

static gboolean
undo_push_layer_mask_properties (GimpImage     *image,
                                 GimpUndoType   undo_type,
                                 const gchar   *undo_desc,
                                 GimpLayerMask *mask)
{
  GimpUndo *new;
  gint64    size;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER_MASK (mask), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (mask)), FALSE);

  size = sizeof (LayerMaskPropertyUndo);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_ITEM_UNDO,
                                   size, sizeof (LayerMaskPropertyUndo),
                                   undo_type, undo_desc,
                                   GIMP_DIRTY_ITEM_META,
                                   undo_pop_layer_mask_properties,
                                   undo_free_layer_mask_properties,
                                   "item", mask,
                                   NULL)))
    {
      LayerMaskPropertyUndo *lmp_undo = new->data;

      lmp_undo->old_apply = gimp_layer_mask_get_apply (mask);
      lmp_undo->old_show  = gimp_layer_mask_get_show (mask);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_layer_mask_properties (GimpUndo            *undo,
                                GimpUndoMode         undo_mode,
                                GimpUndoAccumulator *accum)
{
  LayerMaskPropertyUndo *lmp_undo = undo->data;
  GimpLayerMask         *mask;
  gboolean               val;

  mask = GIMP_LAYER_MASK (GIMP_ITEM_UNDO (undo)->item);

  switch (undo->undo_type)
    {
    case GIMP_UNDO_LAYER_MASK_APPLY:
      val = gimp_layer_mask_get_apply (mask);
      gimp_layer_mask_set_apply (mask, lmp_undo->old_apply, FALSE);
      lmp_undo->old_apply = val;
      break;

    case GIMP_UNDO_LAYER_MASK_SHOW:
      val = gimp_layer_mask_get_show (mask);
      gimp_layer_mask_set_show (mask, lmp_undo->old_show, FALSE);
      lmp_undo->old_show = val;
      break;

    default:
      g_return_val_if_reached (FALSE);
      break;
    }

  return TRUE;
}

static void
undo_free_layer_mask_properties (GimpUndo     *undo,
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

static gboolean undo_push_channel (GimpImage           *image,
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
gimp_image_undo_push_channel_add (GimpImage   *image,
                                  const gchar *undo_desc,
                                  GimpChannel *channel,
                                  gint         prev_position,
                                  GimpChannel *prev_channel)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);
  g_return_val_if_fail (! gimp_item_is_attached (GIMP_ITEM (channel)), FALSE);
  g_return_val_if_fail (prev_channel == NULL || GIMP_IS_CHANNEL (prev_channel),
                        FALSE);

  return undo_push_channel (image, undo_desc, GIMP_UNDO_CHANNEL_ADD,
                            channel, prev_position, prev_channel);
}

gboolean
gimp_image_undo_push_channel_remove (GimpImage   *image,
                                     const gchar *undo_desc,
                                     GimpChannel *channel,
                                     gint         prev_position,
                                     GimpChannel *prev_channel)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)), FALSE);
  g_return_val_if_fail (prev_channel == NULL || GIMP_IS_CHANNEL (prev_channel),
                        FALSE);

  return undo_push_channel (image, undo_desc, GIMP_UNDO_CHANNEL_REMOVE,
                            channel, prev_position, prev_channel);
}

static gboolean
undo_push_channel (GimpImage    *image,
                   const gchar  *undo_desc,
                   GimpUndoType  type,
                   GimpChannel  *channel,
                   gint          prev_position,
                   GimpChannel  *prev_channel)
{
  GimpUndo *new;
  gint64    size;

  size = sizeof (ChannelUndo);

  if (type == GIMP_UNDO_CHANNEL_REMOVE)
    size += gimp_object_get_memsize (GIMP_OBJECT (channel), NULL);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_ITEM_UNDO,
                                   size, sizeof (ChannelUndo),
                                   type, undo_desc,
                                   GIMP_DIRTY_IMAGE_STRUCTURE,
                                   undo_pop_channel,
                                   undo_free_channel,
                                   "item", channel,
                                   NULL)))
    {
      ChannelUndo *cu = new->data;

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
  ChannelUndo *cu      = undo->data;
  GimpChannel *channel = GIMP_CHANNEL (GIMP_ITEM_UNDO (undo)->item);

  if ((undo_mode       == GIMP_UNDO_MODE_UNDO &&
       undo->undo_type == GIMP_UNDO_CHANNEL_ADD) ||
      (undo_mode       == GIMP_UNDO_MODE_REDO &&
       undo->undo_type == GIMP_UNDO_CHANNEL_REMOVE))
    {
      /*  remove channel  */

      undo->size += gimp_object_get_memsize (GIMP_OBJECT (channel), NULL);

      /*  record the current position  */
      cu->prev_position = gimp_image_get_channel_index (undo->image,
                                                        channel);

      gimp_container_remove (undo->image->channels, GIMP_OBJECT (channel));
      gimp_item_removed (GIMP_ITEM (channel));

      if (channel == gimp_image_get_active_channel (undo->image))
        {
          if (cu->prev_channel)
            gimp_image_set_active_channel (undo->image, cu->prev_channel);
          else
            gimp_image_unset_active_channel (undo->image);
        }
    }
  else
    {
      /*  restore channel  */

      undo->size -= gimp_object_get_memsize (GIMP_OBJECT (channel), NULL);

      /*  record the active channel  */
      cu->prev_channel = gimp_image_get_active_channel (undo->image);

      gimp_container_insert (undo->image->channels,
                             GIMP_OBJECT (channel), cu->prev_position);
      gimp_image_set_active_channel (undo->image, channel);

      GIMP_ITEM (channel)->removed = FALSE;
    }

  return TRUE;
}

static void
undo_free_channel (GimpUndo     *undo,
                   GimpUndoMode  undo_mode)
{
  g_free (undo->data);
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
gimp_image_undo_push_channel_reposition (GimpImage   *image,
                                         const gchar *undo_desc,
                                         GimpChannel *channel)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_ITEM_UNDO,
                                   sizeof (ChannelRepositionUndo),
                                   sizeof (ChannelRepositionUndo),
                                   GIMP_UNDO_CHANNEL_REPOSITION, undo_desc,
                                   GIMP_DIRTY_IMAGE_STRUCTURE,
                                   undo_pop_channel_reposition,
                                   undo_free_channel_reposition,
                                   "item", channel,
                                   NULL)))
    {
      ChannelRepositionUndo *cru = new->data;

      cru->old_position = gimp_image_get_channel_index (image, channel);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_channel_reposition (GimpUndo            *undo,
                             GimpUndoMode         undo_mode,
                             GimpUndoAccumulator *accum)
{
  ChannelRepositionUndo *cru     = undo->data;
  GimpChannel           *channel = GIMP_CHANNEL (GIMP_ITEM_UNDO (undo)->item);
  gint                   pos;

  pos = gimp_image_get_channel_index (undo->image, channel);
  gimp_image_position_channel (undo->image, channel, cru->old_position,
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
gimp_image_undo_push_channel_color (GimpImage   *image,
                                    const gchar *undo_desc,
                                    GimpChannel *channel)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_ITEM_UNDO,
                                   sizeof (ChannelColorUndo),
                                   sizeof (ChannelColorUndo),
                                   GIMP_UNDO_CHANNEL_COLOR, undo_desc,
                                   GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE,
                                   undo_pop_channel_color,
                                   undo_free_channel_color,
                                   "item", channel,
                                   NULL)))
    {
      ChannelColorUndo *ccu = new->data;

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
  ChannelColorUndo *ccu     = undo->data;
  GimpChannel      *channel = GIMP_CHANNEL (GIMP_ITEM_UNDO (undo)->item);
  GimpRGB           color;

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

static gboolean undo_push_vectors (GimpImage           *image,
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
gimp_image_undo_push_vectors_add (GimpImage   *image,
                                  const gchar *undo_desc,
                                  GimpVectors *vectors,
                                  gint         prev_position,
                                  GimpVectors *prev_vectors)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);
  g_return_val_if_fail (! gimp_item_is_attached (GIMP_ITEM (vectors)), FALSE);
  g_return_val_if_fail (prev_vectors == NULL || GIMP_IS_VECTORS (prev_vectors),
                        FALSE);

  return undo_push_vectors (image, undo_desc, GIMP_UNDO_VECTORS_ADD,
                            vectors, prev_position, prev_vectors);
}

gboolean
gimp_image_undo_push_vectors_remove (GimpImage   *image,
                                     const gchar *undo_desc,
                                     GimpVectors *vectors,
                                     gint         prev_position,
                                     GimpVectors *prev_vectors)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (vectors)), FALSE);
  g_return_val_if_fail (prev_vectors == NULL || GIMP_IS_VECTORS (prev_vectors),
                        FALSE);

  return undo_push_vectors (image, undo_desc, GIMP_UNDO_VECTORS_REMOVE,
                            vectors, prev_position, prev_vectors);
}

static gboolean
undo_push_vectors (GimpImage    *image,
                   const gchar  *undo_desc,
                   GimpUndoType  type,
                   GimpVectors  *vectors,
                   gint          prev_position,
                   GimpVectors  *prev_vectors)
{
  GimpUndo *new;
  gint64    size;

  size = sizeof (VectorsUndo);

  if (type == GIMP_UNDO_VECTORS_REMOVE)
    size += gimp_object_get_memsize (GIMP_OBJECT (vectors), NULL);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_ITEM_UNDO,
                                   size, sizeof (VectorsUndo),
                                   type, undo_desc,
                                   GIMP_DIRTY_IMAGE_STRUCTURE,
                                   undo_pop_vectors,
                                   undo_free_vectors,
                                   "item", vectors,
                                   NULL)))
    {
      VectorsUndo *vu = new->data;

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
  VectorsUndo *vu      = undo->data;
  GimpVectors *vectors = GIMP_VECTORS (GIMP_ITEM_UNDO (undo)->item);

  if ((undo_mode       == GIMP_UNDO_MODE_UNDO &&
       undo->undo_type == GIMP_UNDO_VECTORS_ADD) ||
      (undo_mode       == GIMP_UNDO_MODE_REDO &&
       undo->undo_type == GIMP_UNDO_VECTORS_REMOVE))
    {
      /*  remove vectors  */

      undo->size += gimp_object_get_memsize (GIMP_OBJECT (vectors), NULL);

      /*  record the current position  */
      vu->prev_position = gimp_image_get_vectors_index (undo->image,
                                                        vectors);

      gimp_container_remove (undo->image->vectors, GIMP_OBJECT (vectors));
      gimp_item_removed (GIMP_ITEM (vectors));

      if (vectors == gimp_image_get_active_vectors (undo->image))
        gimp_image_set_active_vectors (undo->image, vu->prev_vectors);
    }
  else
    {
      /*  restore vectors  */

      undo->size -= gimp_object_get_memsize (GIMP_OBJECT (vectors), NULL);

      /*  record the active vectors  */
      vu->prev_vectors = gimp_image_get_active_vectors (undo->image);

      gimp_container_insert (undo->image->vectors,
                             GIMP_OBJECT (vectors), vu->prev_position);
      gimp_image_set_active_vectors (undo->image, vectors);

      GIMP_ITEM (vectors)->removed = FALSE;
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
  GimpVectors *vectors;
};

static gboolean undo_pop_vectors_mod  (GimpUndo            *undo,
                                       GimpUndoMode         undo_mode,
                                       GimpUndoAccumulator *accum);
static void     undo_free_vectors_mod (GimpUndo            *undo,
                                       GimpUndoMode         undo_mode);

gboolean
gimp_image_undo_push_vectors_mod (GimpImage   *image,
                                  const gchar *undo_desc,
                                  GimpVectors *vectors)
{
  GimpVectors *copy;
  GimpUndo    *new;
  gint64       size;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (vectors)), FALSE);

  copy = GIMP_VECTORS (gimp_item_duplicate (GIMP_ITEM (vectors),
                                            G_TYPE_FROM_INSTANCE (vectors),
                                            FALSE));

  size = (sizeof (VectorsModUndo) +
          gimp_object_get_memsize (GIMP_OBJECT (copy), NULL));

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_ITEM_UNDO,
                                   size, sizeof (VectorsModUndo),
                                   GIMP_UNDO_VECTORS_MOD, undo_desc,
                                   GIMP_DIRTY_ITEM | GIMP_DIRTY_VECTORS,
                                   undo_pop_vectors_mod,
                                   undo_free_vectors_mod,
                                   "item", vectors,
                                   NULL)))
    {
      VectorsModUndo *vmu = new->data;

      vmu->vectors = copy;

      return TRUE;
    }

  if (copy)
    g_object_unref (copy);

  return FALSE;
}

static gboolean
undo_pop_vectors_mod (GimpUndo            *undo,
                      GimpUndoMode         undo_mode,
                      GimpUndoAccumulator *accum)
{
  VectorsModUndo *vmu     = undo->data;
  GimpVectors    *vectors = GIMP_VECTORS (GIMP_ITEM_UNDO (undo)->item);
  GimpVectors    *temp;

  undo->size -= gimp_object_get_memsize (GIMP_OBJECT (vmu->vectors), NULL);

  temp = vmu->vectors;

  vmu->vectors =
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

  undo->size += gimp_object_get_memsize (GIMP_OBJECT (vmu->vectors), NULL);

  return TRUE;
}

static void
undo_free_vectors_mod (GimpUndo     *undo,
                       GimpUndoMode  undo_mode)
{
  VectorsModUndo *vmu = undo->data;

  g_object_unref (vmu->vectors);
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
gimp_image_undo_push_vectors_reposition (GimpImage   *image,
                                         const gchar *undo_desc,
                                         GimpVectors *vectors)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (vectors)), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_ITEM_UNDO,
                                   sizeof (VectorsRepositionUndo),
                                   sizeof (VectorsRepositionUndo),
                                   GIMP_UNDO_VECTORS_REPOSITION, undo_desc,
                                   GIMP_DIRTY_IMAGE_STRUCTURE,
                                   undo_pop_vectors_reposition,
                                   undo_free_vectors_reposition,
                                   "item", vectors,
                                   NULL)))
    {
      VectorsRepositionUndo *vru = new->data;

      vru->old_position = gimp_image_get_vectors_index (image, vectors);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_vectors_reposition (GimpUndo            *undo,
                             GimpUndoMode         undo_mode,
                             GimpUndoAccumulator *accum)
{
  VectorsRepositionUndo *vru     = undo->data;
  GimpVectors           *vectors = GIMP_VECTORS (GIMP_ITEM_UNDO (undo)->item);
  gint                   pos;

  /* what's the vectors's current index? */
  pos = gimp_image_get_vectors_index (undo->image, vectors);
  gimp_image_position_vectors (undo->image, vectors, vru->old_position,
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
gimp_image_undo_push_fs_to_layer (GimpImage    *image,
                                  const gchar  *undo_desc,
                                  GimpLayer    *floating_layer,
                                  GimpDrawable *drawable)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (floating_layer), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_UNDO,
                                   sizeof (FStoLayerUndo),
                                   sizeof (FStoLayerUndo),
                                   GIMP_UNDO_FS_TO_LAYER, undo_desc,
                                   GIMP_DIRTY_IMAGE_STRUCTURE,
                                   undo_pop_fs_to_layer,
                                   undo_free_fs_to_layer,
                                   NULL)))
    {
      FStoLayerUndo *fsu = new->data;

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
  FStoLayerUndo *fsu = undo->data;

  switch (undo_mode)
    {
    case GIMP_UNDO_MODE_UNDO:
      /*  Update the preview for the floating sel  */
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (fsu->floating_layer));

      fsu->floating_layer->fs.drawable = fsu->drawable;
      gimp_image_set_active_layer (undo->image, fsu->floating_layer);
      undo->image->floating_sel = fsu->floating_layer;

      /*  store the contents of the drawable  */
      floating_sel_store (fsu->floating_layer,
                          GIMP_ITEM (fsu->floating_layer)->offset_x,
                          GIMP_ITEM (fsu->floating_layer)->offset_y,
                          GIMP_ITEM (fsu->floating_layer)->width,
                          GIMP_ITEM (fsu->floating_layer)->height);
      fsu->floating_layer->fs.initial = TRUE;

      /*  clear the selection  */
      gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (fsu->floating_layer));
      break;

    case GIMP_UNDO_MODE_REDO:
      /*  restore the contents of the drawable  */
      floating_sel_restore (fsu->floating_layer,
                            GIMP_ITEM (fsu->floating_layer)->offset_x,
                            GIMP_ITEM (fsu->floating_layer)->offset_y,
                            GIMP_ITEM (fsu->floating_layer)->width,
                            GIMP_ITEM (fsu->floating_layer)->height);

      /*  Update the preview for the underlying drawable  */
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (fsu->floating_layer));

      /*  clear the selection  */
      gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (fsu->floating_layer));

      /*  update the pointers  */
      fsu->floating_layer->fs.drawable = NULL;
      undo->image->floating_sel       = NULL;
      break;
    }

  gimp_object_name_changed (GIMP_OBJECT (fsu->floating_layer));

  gimp_drawable_update (GIMP_DRAWABLE (fsu->floating_layer),
                        0, 0,
                        GIMP_ITEM (fsu->floating_layer)->width,
                        GIMP_ITEM (fsu->floating_layer)->height);

  gimp_image_floating_selection_changed (undo->image);

  return TRUE;
}

static void
undo_free_fs_to_layer (GimpUndo     *undo,
                       GimpUndoMode  undo_mode)
{
  FStoLayerUndo *fsu = undo->data;

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
gimp_image_undo_push_fs_rigor (GimpImage    *image,
                               const gchar  *undo_desc,
                               GimpLayer    *floating_layer)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (floating_layer), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_ITEM_UNDO,
                                   0, 0,
                                   GIMP_UNDO_FS_RIGOR, undo_desc,
                                   GIMP_DIRTY_NONE,
                                   undo_pop_fs_rigor,
                                   NULL,
                                   "item", floating_layer,
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
gimp_image_undo_push_fs_relax (GimpImage   *image,
                               const gchar *undo_desc,
                               GimpLayer   *floating_layer)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (floating_layer), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_ITEM_UNDO,
                                   0, 0,
                                   GIMP_UNDO_FS_RELAX, undo_desc,
                                   GIMP_DIRTY_NONE,
                                   undo_pop_fs_relax,
                                   NULL,
                                   "item", floating_layer,
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
  GimpImage    *image;
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
gimp_image_undo_push_image_parasite (GimpImage          *image,
                                     const gchar        *undo_desc,
                                     const GimpParasite *parasite)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_UNDO,
                                   sizeof (ParasiteUndo),
                                   sizeof (ParasiteUndo),
                                   GIMP_UNDO_PARASITE_ATTACH, undo_desc,
                                   GIMP_DIRTY_IMAGE_META,
                                   undo_pop_parasite,
                                   undo_free_parasite,
                                   NULL)))
    {
      ParasiteUndo *pu = new->data;

      pu->image    = image;
      pu->item     = NULL;
      pu->name     = g_strdup (gimp_parasite_name (parasite));
      pu->parasite = gimp_parasite_copy (gimp_image_parasite_find (image,
                                                                   pu->name));

      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_image_undo_push_image_parasite_remove (GimpImage   *image,
                                            const gchar *undo_desc,
                                            const gchar *name)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_UNDO,
                                   sizeof (ParasiteUndo),
                                   sizeof (ParasiteUndo),
                                   GIMP_UNDO_PARASITE_REMOVE, undo_desc,
                                   GIMP_DIRTY_IMAGE_META,
                                   undo_pop_parasite,
                                   undo_free_parasite,
                                   NULL)))
    {
      ParasiteUndo *pu = new->data;

      pu->image    = image;
      pu->item     = NULL;
      pu->name     = g_strdup (name);
      pu->parasite = gimp_parasite_copy (gimp_image_parasite_find (image,
                                                                   pu->name));

      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_image_undo_push_item_parasite (GimpImage          *image,
                                    const gchar        *undo_desc,
                                    GimpItem           *item,
                                    const GimpParasite *parasite)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (item), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_UNDO,
                                   sizeof (ParasiteUndo),
                                   sizeof (ParasiteUndo),
                                   GIMP_UNDO_PARASITE_ATTACH, undo_desc,
                                   GIMP_DIRTY_ITEM_META,
                                   undo_pop_parasite,
                                   undo_free_parasite,
                                   NULL)))
    {
      ParasiteUndo *pu = new->data;

      pu->image    = NULL;
      pu->item     = item;
      pu->name     = g_strdup (gimp_parasite_name (parasite));
      pu->parasite = gimp_parasite_copy (gimp_item_parasite_find (item,
                                                                  pu->name));
      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_image_undo_push_item_parasite_remove (GimpImage   *image,
                                           const gchar *undo_desc,
                                           GimpItem    *item,
                                           const gchar *name)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (item), FALSE);

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_UNDO,
                                   sizeof (ParasiteUndo),
                                   sizeof (ParasiteUndo),
                                   GIMP_UNDO_PARASITE_REMOVE, undo_desc,
                                   GIMP_DIRTY_ITEM_META,
                                   undo_pop_parasite,
                                   undo_free_parasite,
                                   NULL)))
    {
      ParasiteUndo *pu = new->data;

      pu->image    = NULL;
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
  ParasiteUndo *pu = undo->data;
  GimpParasite *tmp;

  tmp = pu->parasite;

  if (pu->image)
    {
      pu->parasite = gimp_parasite_copy (gimp_image_parasite_find (undo->image,
                                                                   pu->name));
      if (tmp)
        gimp_parasite_list_add (pu->image->parasites, tmp);
      else
        gimp_parasite_list_remove (pu->image->parasites, pu->name);
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
      pu->parasite = gimp_parasite_copy (gimp_parasite_find (undo->image->gimp,
                                                             pu->name));
      if (tmp)
        gimp_parasite_attach (undo->image->gimp, tmp);
      else
        gimp_parasite_detach (undo->image->gimp, pu->name);
    }

  if (tmp)
    gimp_parasite_free (tmp);

  return TRUE;
}

static void
undo_free_parasite (GimpUndo     *undo,
                    GimpUndoMode  undo_mode)
{
  ParasiteUndo *pu = undo->data;

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
gimp_image_undo_push_cantundo (GimpImage   *image,
                               const gchar *undo_desc)
{
  GimpUndo *new;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  /* This is the sole purpose of this type of undo: the ability to
   * mark an image as having been mutated, without really providing
   * any adequate undo facility.
   */

  if ((new = gimp_image_undo_push (image, GIMP_TYPE_UNDO,
                                   0, 0,
                                   GIMP_UNDO_CANT, undo_desc,
                                   GIMP_DIRTY_ALL,
                                   undo_pop_cantundo,
                                   NULL,
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
      gimp_message (undo->image->gimp, NULL, GIMP_MESSAGE_WARNING,
                    _("Can't undo %s"), GIMP_OBJECT (undo)->name);
      break;

    case GIMP_UNDO_MODE_REDO:
      break;
    }

  return TRUE;
}
