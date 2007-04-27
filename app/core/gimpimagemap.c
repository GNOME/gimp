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

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "paint-funcs/paint-funcs.h"

#include "gimpdrawable.h"
#include "gimpimage.h"
#include "gimpimagemap.h"
#include "gimpmarshal.h"
#include "gimppickable.h"


enum
{
  FLUSH,
  LAST_SIGNAL
};


struct _GimpImageMap
{
  GimpObject             parent_instance;

  GimpDrawable          *drawable;
  gchar                 *undo_desc;

  TileManager           *undo_tiles;
  gint                   undo_offset_x;
  gint                   undo_offset_y;
  GimpImageMapApplyFunc  apply_func;
  gpointer               user_data;
  PixelRegion            srcPR;
  PixelRegion            destPR;
  PixelRegionIterator   *PRI;
  guint                  idle_id;
};


static void   gimp_image_map_pickable_iface_init (GimpPickableInterface *iface);

static void            gimp_image_map_finalize       (GObject      *object);

static GimpImage     * gimp_image_map_get_image      (GimpPickable *pickable);
static GimpImageType   gimp_image_map_get_image_type (GimpPickable *pickable);
static gint            gimp_image_map_get_bytes      (GimpPickable *pickable);
static TileManager   * gimp_image_map_get_tiles      (GimpPickable *pickable);
static gboolean        gimp_image_map_get_pixel_at   (GimpPickable *pickable,
                                                      gint          x,
                                                      gint          y,
                                                      guchar       *pixel);
static guchar        * gimp_image_map_get_color_at   (GimpPickable *pickable,
                                                      gint          x,
                                                      gint          y);

static gboolean        gimp_image_map_do             (GimpImageMap *image_map);


G_DEFINE_TYPE_WITH_CODE (GimpImageMap, gimp_image_map, GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PICKABLE,
                                                gimp_image_map_pickable_iface_init))

#define parent_class gimp_image_map_parent_class

static guint image_map_signals[LAST_SIGNAL] = { 0 };


static void
gimp_image_map_class_init (GimpImageMapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_map_signals[FLUSH] =
    g_signal_new ("flush",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageMapClass, flush),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->finalize = gimp_image_map_finalize;
}

static void
gimp_image_map_pickable_iface_init (GimpPickableInterface *iface)
{
  iface->get_image      = gimp_image_map_get_image;
  iface->get_image_type = gimp_image_map_get_image_type;
  iface->get_bytes      = gimp_image_map_get_bytes;
  iface->get_tiles      = gimp_image_map_get_tiles;
  iface->get_pixel_at   = gimp_image_map_get_pixel_at;
  iface->get_color_at   = gimp_image_map_get_color_at;
}

static void
gimp_image_map_init (GimpImageMap *image_map)
{
  image_map->drawable      = NULL;
  image_map->undo_desc     = NULL;
  image_map->undo_tiles    = NULL;
  image_map->undo_offset_x = 0;
  image_map->undo_offset_y = 0;
  image_map->apply_func    = NULL;
  image_map->user_data     = NULL;
  image_map->PRI           = NULL;
  image_map->idle_id       = 0;
}

static void
gimp_image_map_finalize (GObject *object)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (object);

  if (image_map->undo_desc)
    {
      g_free (image_map->undo_desc);
      image_map->undo_desc = NULL;
    }

  if (image_map->undo_tiles)
    {
      tile_manager_unref (image_map->undo_tiles);
      image_map->undo_tiles = NULL;
    }

  if (image_map->idle_id)
    {
      g_source_remove (image_map->idle_id);
      image_map->idle_id = 0;

      pixel_regions_process_stop (image_map->PRI);
      image_map->PRI = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GimpImage *
gimp_image_map_get_image (GimpPickable *pickable)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (pickable);

  return gimp_pickable_get_image (GIMP_PICKABLE (image_map->drawable));
}

static GimpImageType
gimp_image_map_get_image_type (GimpPickable *pickable)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (pickable);

  return gimp_pickable_get_image_type (GIMP_PICKABLE (image_map->drawable));
}

static gint
gimp_image_map_get_bytes (GimpPickable *pickable)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (pickable);

  return gimp_pickable_get_bytes (GIMP_PICKABLE (image_map->drawable));
}

static TileManager *
gimp_image_map_get_tiles (GimpPickable *pickable)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (pickable);

  if (image_map->undo_tiles)
    return image_map->undo_tiles;
  else
    return gimp_pickable_get_tiles (GIMP_PICKABLE (image_map->drawable));
}

static gboolean
gimp_image_map_get_pixel_at (GimpPickable *pickable,
                             gint          x,
                             gint          y,
                             guchar       *pixel)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (pickable);
  GimpItem     *item      = GIMP_ITEM (image_map->drawable);

  if (x >= 0 && x < gimp_item_width  (item) &&
      y >= 0 && y < gimp_item_height (item))
    {
      /* Check if done damage to original image */
      if (image_map->undo_tiles)
        {
          gint offset_x = image_map->undo_offset_x;
          gint offset_y = image_map->undo_offset_y;
          gint width    = tile_manager_width (image_map->undo_tiles);
          gint height   = tile_manager_height (image_map->undo_tiles);

          if (x >= offset_x && x < offset_x + width &&
              y >= offset_y && y < offset_y + height)
            {
              read_pixel_data_1 (image_map->undo_tiles,
                                 x - offset_x,
                                 y - offset_y,
                                 pixel);

              return TRUE;
            }
        }

      return gimp_pickable_get_pixel_at (GIMP_PICKABLE (image_map->drawable),
                                         x, y, pixel);
    }
  else /* out of bounds error */
    {
      return FALSE;
    }
}

static guchar *
gimp_image_map_get_color_at (GimpPickable *pickable,
                             gint          x,
                             gint          y)
{
  GimpImageMap *image_map = GIMP_IMAGE_MAP (pickable);
  guchar       *dest;
  guchar        pixel[4];

  if (! gimp_image_map_get_pixel_at (pickable, x, y, pixel))
    return NULL;

  dest = g_new (guchar, 5);

  gimp_image_get_color (gimp_item_get_image (GIMP_ITEM (image_map->drawable)),
                        gimp_drawable_type (image_map->drawable),
                        pixel, dest);

  if (gimp_drawable_is_indexed (image_map->drawable))
    dest[4] = pixel[0];
  else
    dest[4] = 0;

  return dest;
}

GimpImageMap *
gimp_image_map_new (GimpDrawable *drawable,
                    const gchar  *undo_desc)
{
  GimpImageMap *image_map;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);

  image_map = g_object_new (GIMP_TYPE_IMAGE_MAP, NULL);

  image_map->drawable  = drawable;
  image_map->undo_desc = g_strdup (undo_desc);

  return image_map;
}

void
gimp_image_map_apply (GimpImageMap          *image_map,
                      GimpImageMapApplyFunc  apply_func,
                      gpointer               apply_data)
{
  gint x, y;
  gint width, height;
  gint undo_offset_x, undo_offset_y;
  gint undo_width, undo_height;

  g_return_if_fail (GIMP_IS_IMAGE_MAP (image_map));
  g_return_if_fail (apply_func != NULL);

  image_map->apply_func = apply_func;
  image_map->user_data  = apply_data;

  /*  If we're still working, remove the timer  */
  if (image_map->idle_id)
    {
      g_source_remove (image_map->idle_id);
      image_map->idle_id = 0;

      pixel_regions_process_stop (image_map->PRI);
      image_map->PRI = NULL;
    }

  /*  Make sure the drawable is still valid  */
  if (! gimp_item_is_attached (GIMP_ITEM (image_map->drawable)))
    return;

  /*  The application should occur only within selection bounds  */
  if (! gimp_drawable_mask_intersect (image_map->drawable,
                                      &x, &y, &width, &height))
    return;

  /*  If undo tiles don't exist, or change size, (re)allocate  */
  if (image_map->undo_tiles)
    {
      undo_offset_x = image_map->undo_offset_x;
      undo_offset_y = image_map->undo_offset_y;
      undo_width    = tile_manager_width (image_map->undo_tiles);
      undo_height   = tile_manager_height (image_map->undo_tiles);
    }
  else
    {
      undo_offset_x = 0;
      undo_offset_y = 0;
      undo_width    = 0;
      undo_height   = 0;
    }

  if (! image_map->undo_tiles ||
      undo_offset_x != x      ||
      undo_offset_y != y      ||
      undo_width    != width  ||
      undo_height   != height)
    {
      /* If either the extents changed or the tiles don't exist,
       * allocate new
       */
      if (! image_map->undo_tiles ||
          undo_width  != width    ||
          undo_height != height)
        {
          /*  Destroy old tiles  */
          if (image_map->undo_tiles)
            tile_manager_unref (image_map->undo_tiles);

          /*  Allocate new tiles  */
          image_map->undo_tiles =
            tile_manager_new (width, height,
                              gimp_drawable_bytes (image_map->drawable));
        }

      /*  Copy from the image to the new tiles  */
      pixel_region_init (&image_map->srcPR,
                         gimp_drawable_get_tiles (image_map->drawable),
                         x, y, width, height, FALSE);
      pixel_region_init (&image_map->destPR, image_map->undo_tiles,
                         0, 0, width, height, TRUE);

      copy_region (&image_map->srcPR, &image_map->destPR);

      /*  Set the offsets  */
      image_map->undo_offset_x = x;
      image_map->undo_offset_y = y;
    }

  /*  Configure the src from the drawable data  */
  pixel_region_init (&image_map->srcPR, image_map->undo_tiles,
                     0, 0, width, height, FALSE);

  /*  Configure the dest as the shadow buffer  */
  pixel_region_init (&image_map->destPR,
                     gimp_drawable_get_shadow_tiles (image_map->drawable),
                     x, y, width, height, TRUE);

  /*  Apply the image transformation to the pixels  */
  image_map->PRI = pixel_regions_register (2,
                                           &image_map->srcPR,
                                           &image_map->destPR);

  /*  Start the intermittant work procedure  */
  image_map->idle_id = g_idle_add ((GSourceFunc) gimp_image_map_do, image_map);
}

void
gimp_image_map_commit (GimpImageMap *image_map)
{
  gint x1, y1, x2, y2;

  g_return_if_fail (GIMP_IS_IMAGE_MAP (image_map));

  if (image_map->idle_id)
    {
      g_source_remove (image_map->idle_id);
      image_map->idle_id = 0;

      /*  Finish the changes  */
      while (gimp_image_map_do (image_map));
    }

  /*  Make sure the drawable is still valid  */
  if (! gimp_item_is_attached (GIMP_ITEM (image_map->drawable)))
    return;

  /*  Register an undo step  */
  if (image_map->undo_tiles)
    {
      x1 = image_map->undo_offset_x;
      y1 = image_map->undo_offset_y;
      x2 = x1 + tile_manager_width (image_map->undo_tiles);
      y2 = y1 + tile_manager_height (image_map->undo_tiles);

      gimp_drawable_push_undo (image_map->drawable,
                               image_map->undo_desc,
                               x1, y1, x2, y2,
                               image_map->undo_tiles, FALSE);

      tile_manager_unref (image_map->undo_tiles);
      image_map->undo_tiles = NULL;
    }
}

void
gimp_image_map_clear (GimpImageMap *image_map)
{
  g_return_if_fail (GIMP_IS_IMAGE_MAP (image_map));

  if (image_map->idle_id)
    {
      g_source_remove (image_map->idle_id);
      image_map->idle_id = 0;

      pixel_regions_process_stop (image_map->PRI);
      image_map->PRI = NULL;
    }

  /*  Make sure the drawable is still valid  */
  if (! gimp_item_is_attached (GIMP_ITEM (image_map->drawable)))
    return;

  /*  restore the original image  */
  if (image_map->undo_tiles)
    {
      PixelRegion srcPR, destPR;
      gint        offset_x = image_map->undo_offset_x;
      gint        offset_y = image_map->undo_offset_y;
      gint        width    = tile_manager_width (image_map->undo_tiles);
      gint        height   = tile_manager_height (image_map->undo_tiles);

      /*  Copy from the drawable to the tiles  */
      pixel_region_init (&srcPR, image_map->undo_tiles,
                         0, 0, width, height, FALSE);
      pixel_region_init (&destPR, gimp_drawable_get_tiles (image_map->drawable),
                         offset_x, offset_y, width, height, TRUE);

      /* if the user has changed the image depth get out quickly */
      if (destPR.bytes != srcPR.bytes)
        {
          g_message ("image depth change, unable to restore original image");
        }
      else
        {
          copy_region (&srcPR, &destPR);

          /*  Update the area  */
          gimp_drawable_update (image_map->drawable,
                                offset_x, offset_y, width, height);
        }

      /*  Free the undo_tiles tile manager  */
      tile_manager_unref (image_map->undo_tiles);
      image_map->undo_tiles = NULL;
    }
}

void
gimp_image_map_abort (GimpImageMap *image_map)
{
  g_return_if_fail (GIMP_IS_IMAGE_MAP (image_map));

  if (! gimp_item_is_attached (GIMP_ITEM (image_map->drawable)))
    return;

  gimp_image_map_clear (image_map);
}


/*  private functions  */

static gboolean
gimp_image_map_do (GimpImageMap *image_map)
{
  GimpImage *image;
  gint       i;

  if (! gimp_item_is_attached (GIMP_ITEM (image_map->drawable)))
    {
      image_map->idle_id = 0;

      return FALSE;
    }

  image = gimp_item_get_image (GIMP_ITEM (image_map->drawable));

  /*  Process up to 16 tiles in one go. This reduces the overhead
   *  caused by updating the display while the imagemap is being
   *  applied and gives us a tiny speedup.
   */
  for (i = 0; i < 16; i++)
    {
      PixelRegion  srcPR;
      PixelRegion  destPR;
      gint         x, y, w, h;

      x = image_map->destPR.x;
      y = image_map->destPR.y;
      w = image_map->destPR.w;
      h = image_map->destPR.h;

      /* Reset to initial drawable conditions. */
      pixel_region_init (&srcPR, image_map->undo_tiles,
                         x - image_map->undo_offset_x,
                         y - image_map->undo_offset_y,
                         w, h, FALSE);
      pixel_region_init (&destPR, gimp_drawable_get_tiles (image_map->drawable),
                         x, y, w, h, TRUE);
      copy_region (&srcPR, &destPR);

      image_map->apply_func (image_map->user_data,
                             &image_map->srcPR,
                             &image_map->destPR);


      pixel_region_init (&srcPR, image->shadow, x, y, w, h, FALSE);

      gimp_drawable_apply_region (image_map->drawable, &srcPR,
                                  FALSE, NULL,
                                  GIMP_OPACITY_OPAQUE, GIMP_REPLACE_MODE,
                                  NULL,
                                  x, y);

      gimp_drawable_update (image_map->drawable, x, y, w, h);

      image_map->PRI = pixel_regions_process (image_map->PRI);

      if (image_map->PRI == NULL)
        {
          image_map->idle_id = 0;

          gimp_image_flush (image);

          return FALSE;
        }
    }

  g_signal_emit (image_map, image_map_signals[FLUSH], 0);

  return TRUE;
}
