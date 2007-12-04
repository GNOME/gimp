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
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"
#include "paint-funcs/scale-region.h"

#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpdrawable-combine.h"
#include "gimpdrawable-preview.h"
#include "gimpdrawable-transform.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimpmarshal.h"
#include "gimppattern.h"
#include "gimppickable.h"
#include "gimppreviewcache.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


enum
{
  UPDATE,
  ALPHA_CHANGED,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void  gimp_drawable_pickable_iface_init (GimpPickableInterface *iface);

static void       gimp_drawable_finalize           (GObject           *object);

static gint64     gimp_drawable_get_memsize        (GimpObject        *object,
                                                    gint64            *gui_size);

static gboolean   gimp_drawable_get_size           (GimpViewable      *viewable,
                                                    gint              *width,
                                                    gint              *height);
static void       gimp_drawable_invalidate_preview (GimpViewable      *viewable);

static GimpItem * gimp_drawable_duplicate          (GimpItem          *item,
                                                    GType              new_type,
                                                    gboolean           add_alpha);
static void       gimp_drawable_translate          (GimpItem          *item,
                                                    gint               offset_x,
                                                    gint               offset_y,
                                                    gboolean           push_undo);
static void       gimp_drawable_scale              (GimpItem          *item,
                                                    gint               new_width,
                                                    gint               new_height,
                                                    gint               new_offset_x,
                                                    gint               new_offset_y,
                                                    GimpInterpolationType interp_type,
                                                    GimpProgress      *progress);
static void       gimp_drawable_resize             (GimpItem          *item,
                                                    GimpContext       *context,
                                                    gint               new_width,
                                                    gint               new_height,
                                                    gint               offset_x,
                                                    gint               offset_y);
static void       gimp_drawable_flip               (GimpItem          *item,
                                                    GimpContext       *context,
                                                    GimpOrientationType  flip_type,
                                                    gdouble            axis,
                                                    gboolean           clip_result);
static void       gimp_drawable_rotate             (GimpItem          *item,
                                                    GimpContext       *context,
                                                    GimpRotationType   rotate_type,
                                                    gdouble            center_x,
                                                    gdouble            center_y,
                                                    gboolean           clip_result);
static void       gimp_drawable_transform          (GimpItem          *item,
                                                    GimpContext       *context,
                                                    const GimpMatrix3 *matrix,
                                                    GimpTransformDirection direction,
                                                    GimpInterpolationType interpolation_type,
                                                    gint               recursion_level,
                                                    GimpTransformResize clip_result,
                                                    GimpProgress      *progress);

static gboolean   gimp_drawable_get_pixel_at       (GimpPickable      *pickable,
                                                    gint               x,
                                                    gint               y,
                                                    guchar            *pixel);
static void       gimp_drawable_real_update        (GimpDrawable      *drawable,
                                                    gint               x,
                                                    gint               y,
                                                    gint               width,
                                                    gint               height);

static gint64  gimp_drawable_real_estimate_memsize (const GimpDrawable *drawable,
                                                    gint               width,
                                                    gint               height);

static void       gimp_drawable_real_set_tiles     (GimpDrawable      *drawable,
                                                    gboolean           push_undo,
                                                    const gchar       *undo_desc,
                                                    TileManager       *tiles,
                                                    GimpImageType      type,
                                                    gint               offset_x,
                                                    gint               offset_y);

static void       gimp_drawable_real_push_undo     (GimpDrawable      *drawable,
                                                    const gchar       *undo_desc,
                                                    TileManager       *tiles,
                                                    gboolean           sparse,
                                                    gint               x,
                                                    gint               y,
                                                    gint               width,
                                                    gint               height);
static void       gimp_drawable_real_swap_pixels   (GimpDrawable      *drawable,
                                                    TileManager       *tiles,
                                                    gboolean           sparse,
                                                    gint               x,
                                                    gint               y,
                                                    gint               width,
                                                    gint               height);


G_DEFINE_TYPE_WITH_CODE (GimpDrawable, gimp_drawable, GIMP_TYPE_ITEM,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PICKABLE,
                                                gimp_drawable_pickable_iface_init))

#define parent_class gimp_drawable_parent_class

static guint gimp_drawable_signals[LAST_SIGNAL] = { 0 };


static void
gimp_drawable_class_init (GimpDrawableClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class        = GIMP_ITEM_CLASS (klass);

  gimp_drawable_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDrawableClass, update),
                  NULL, NULL,
                  gimp_marshal_VOID__INT_INT_INT_INT,
                  G_TYPE_NONE, 4,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  gimp_drawable_signals[ALPHA_CHANGED] =
    g_signal_new ("alpha-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDrawableClass, alpha_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->finalize             = gimp_drawable_finalize;

  gimp_object_class->get_memsize     = gimp_drawable_get_memsize;

  viewable_class->get_size           = gimp_drawable_get_size;
  viewable_class->invalidate_preview = gimp_drawable_invalidate_preview;
  viewable_class->get_preview        = gimp_drawable_get_preview;

  item_class->duplicate              = gimp_drawable_duplicate;
  item_class->translate              = gimp_drawable_translate;
  item_class->scale                  = gimp_drawable_scale;
  item_class->resize                 = gimp_drawable_resize;
  item_class->flip                   = gimp_drawable_flip;
  item_class->rotate                 = gimp_drawable_rotate;
  item_class->transform              = gimp_drawable_transform;

  klass->update                      = gimp_drawable_real_update;
  klass->alpha_changed               = NULL;
  klass->estimate_memsize            = gimp_drawable_real_estimate_memsize;
  klass->invalidate_boundary         = NULL;
  klass->get_active_components       = NULL;
  klass->apply_region                = gimp_drawable_real_apply_region;
  klass->replace_region              = gimp_drawable_real_replace_region;
  klass->set_tiles                   = gimp_drawable_real_set_tiles;
  klass->push_undo                   = gimp_drawable_real_push_undo;
  klass->swap_pixels                 = gimp_drawable_real_swap_pixels;
}

static void
gimp_drawable_init (GimpDrawable *drawable)
{
  drawable->tiles         = NULL;
  drawable->bytes         = 0;
  drawable->type          = -1;
  drawable->has_alpha     = FALSE;
  drawable->preview_cache = NULL;
  drawable->preview_valid = FALSE;
}

/* sorry for the evil casts */

static void
gimp_drawable_pickable_iface_init (GimpPickableInterface *iface)
{
  iface->get_image      = (GimpImage     * (*) (GimpPickable *pickable)) gimp_item_get_image;
  iface->get_image_type = (GimpImageType   (*) (GimpPickable *pickable)) gimp_drawable_type;
  iface->get_bytes      = (gint            (*) (GimpPickable *pickable)) gimp_drawable_bytes;
  iface->get_tiles      = (TileManager   * (*) (GimpPickable *pickable)) gimp_drawable_get_tiles;
  iface->get_pixel_at   = gimp_drawable_get_pixel_at;
}

static void
gimp_drawable_finalize (GObject *object)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (object);

  if (drawable->tiles)
    {
      tile_manager_unref (drawable->tiles);
      drawable->tiles = NULL;
    }

  if (drawable->preview_cache)
    gimp_preview_cache_invalidate (&drawable->preview_cache);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_drawable_get_memsize (GimpObject *object,
                           gint64     *gui_size)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (object);
  gint64        memsize  = 0;

  if (drawable->tiles)
    memsize += tile_manager_get_memsize (drawable->tiles, FALSE);

  if (drawable->preview_cache)
    *gui_size += gimp_preview_cache_get_memsize (drawable->preview_cache);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_drawable_get_size (GimpViewable *viewable,
                        gint         *width,
                        gint         *height)
{
  GimpItem *item = GIMP_ITEM (viewable);

  *width  = item->width;
  *height = item->height;

  return TRUE;
}

static void
gimp_drawable_invalidate_preview (GimpViewable *viewable)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (viewable);

  if (GIMP_VIEWABLE_CLASS (parent_class)->invalidate_preview)
    GIMP_VIEWABLE_CLASS (parent_class)->invalidate_preview (viewable);

  drawable->preview_valid = FALSE;

  if (drawable->preview_cache)
    gimp_preview_cache_invalidate (&drawable->preview_cache);
}

static GimpItem *
gimp_drawable_duplicate (GimpItem *item,
                         GType     new_type,
                         gboolean  add_alpha)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type,
                                                        add_alpha);

  if (GIMP_IS_DRAWABLE (new_item))
    {
      GimpDrawable  *drawable     = GIMP_DRAWABLE (item);
      GimpDrawable  *new_drawable = GIMP_DRAWABLE (new_item);
      GimpImageType  new_image_type;
      PixelRegion    srcPR;
      PixelRegion    destPR;

      if (add_alpha)
        new_image_type = gimp_drawable_type_with_alpha (drawable);
      else
        new_image_type = gimp_drawable_type (drawable);

      gimp_drawable_configure (new_drawable,
                               gimp_item_get_image (item),
                               item->offset_x,
                               item->offset_y,
                               item->width,
                               item->height,
                               new_image_type,
                               GIMP_OBJECT (new_drawable)->name);

      pixel_region_init (&srcPR, drawable->tiles,
                         0, 0,
                         item->width,
                         item->height,
                         FALSE);
      pixel_region_init (&destPR, new_drawable->tiles,
                         0, 0,
                         new_item->width,
                         new_item->height,
                         TRUE);

      if (new_image_type == drawable->type)
        copy_region (&srcPR, &destPR);
      else
        add_alpha_region (&srcPR, &destPR);
    }

  return new_item;
}

static void
gimp_drawable_translate (GimpItem *item,
                         gint      offset_x,
                         gint      offset_y,
                         gboolean  push_undo)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (item);
  GimpImage    *image    = gimp_item_get_image (item);

  if (gimp_drawable_has_floating_sel (drawable))
    floating_sel_relax (gimp_image_floating_sel (image), FALSE);

  GIMP_ITEM_CLASS (parent_class)->translate (item, offset_x, offset_y,
                                             push_undo);

  if (gimp_drawable_has_floating_sel (drawable))
    floating_sel_rigor (gimp_image_floating_sel (image), FALSE);
}

static void
gimp_drawable_scale (GimpItem              *item,
                     gint                   new_width,
                     gint                   new_height,
                     gint                   new_offset_x,
                     gint                   new_offset_y,
                     GimpInterpolationType  interpolation_type,
                     GimpProgress          *progress)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (item);
  PixelRegion   srcPR, destPR;
  TileManager  *new_tiles;

  new_tiles = tile_manager_new (new_width, new_height, drawable->bytes);

  pixel_region_init (&srcPR, drawable->tiles,
                     0, 0, item->width, item->height,
                     FALSE);
  pixel_region_init (&destPR, new_tiles,
                     0, 0, new_width, new_height,
                     TRUE);

  /*  Scale the drawable -
   *   If the drawable is indexed, then we don't use pixel-value
   *   resampling because that doesn't necessarily make sense for indexed
   *   images.
   */
  scale_region (&srcPR, &destPR,
                gimp_drawable_is_indexed (drawable) ?
                GIMP_INTERPOLATION_NONE : interpolation_type,
                progress ? gimp_progress_update_and_flush : NULL,
                progress);

  gimp_drawable_set_tiles_full (drawable, gimp_item_is_attached (item), NULL,
                                new_tiles, gimp_drawable_type (drawable),
                                new_offset_x, new_offset_y);
  tile_manager_unref (new_tiles);
}

static void
gimp_drawable_resize (GimpItem    *item,
                      GimpContext *context,
                      gint         new_width,
                      gint         new_height,
                      gint         offset_x,
                      gint         offset_y)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (item);
  PixelRegion   srcPR, destPR;
  TileManager  *new_tiles;
  gint          new_offset_x;
  gint          new_offset_y;
  gint          copy_x, copy_y;
  gint          copy_width, copy_height;

  /*  if the size doesn't change, this is a nop  */
  if (new_width  == item->width  &&
      new_height == item->height &&
      offset_x   == 0            &&
      offset_y   == 0)
    return;

  new_offset_x = item->offset_x - offset_x;
  new_offset_y = item->offset_y - offset_y;

  gimp_rectangle_intersect (item->offset_x, item->offset_y,
                            item->width, item->height,
                            new_offset_x, new_offset_y,
                            new_width, new_height,
                            &copy_x, &copy_y,
                            &copy_width, &copy_height);

  new_tiles = tile_manager_new (new_width, new_height, drawable->bytes);

  /*  Determine whether the new tiles need to be initially cleared  */
  if (copy_width  != new_width ||
      copy_height != new_height)
    {
      guchar bg[MAX_CHANNELS] = { 0, };

      pixel_region_init (&destPR, new_tiles,
                         0, 0,
                         new_width, new_height,
                         TRUE);

      if (! gimp_drawable_has_alpha (drawable) && ! GIMP_IS_CHANNEL (drawable))
        gimp_image_get_background (gimp_item_get_image (item), context,
                                   gimp_drawable_type (drawable), bg);

      color_region (&destPR, bg);
    }

  /*  Determine whether anything needs to be copied  */
  if (copy_width && copy_height)
    {
      pixel_region_init (&srcPR, drawable->tiles,
                         copy_x - item->offset_x, copy_y - item->offset_y,
                         copy_width, copy_height,
                         FALSE);

      pixel_region_init (&destPR, new_tiles,
                         copy_x - new_offset_x, copy_y - new_offset_y,
                         copy_width, copy_height,
                         TRUE);

      copy_region (&srcPR, &destPR);
    }

  gimp_drawable_set_tiles_full (drawable, gimp_item_is_attached (item), NULL,
                                new_tiles, gimp_drawable_type (drawable),
                                new_offset_x, new_offset_y);
  tile_manager_unref (new_tiles);
}

static void
gimp_drawable_flip (GimpItem            *item,
                    GimpContext         *context,
                    GimpOrientationType  flip_type,
                    gdouble              axis,
                    gboolean             clip_result)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (item);
  TileManager  *tiles;
  gint          off_x, off_y;
  gint          old_off_x, old_off_y;

  gimp_item_offsets (item, &off_x, &off_y);

  tile_manager_get_offsets (drawable->tiles, &old_off_x, &old_off_y);
  tile_manager_set_offsets (drawable->tiles, off_x, off_y);

  tiles = gimp_drawable_transform_tiles_flip (drawable, context,
                                              drawable->tiles,
                                              flip_type, axis,
                                              clip_result);

  tile_manager_set_offsets (drawable->tiles, old_off_x, old_off_y);

  if (tiles)
    {
      gimp_drawable_transform_paste (drawable, tiles, FALSE);
      tile_manager_unref (tiles);
    }
}

static void
gimp_drawable_rotate (GimpItem         *item,
                      GimpContext      *context,
                      GimpRotationType  rotate_type,
                      gdouble           center_x,
                      gdouble           center_y,
                      gboolean          clip_result)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (item);
  TileManager  *tiles;
  gint          off_x, off_y;
  gint          old_off_x, old_off_y;

  gimp_item_offsets (item, &off_x, &off_y);

  tile_manager_get_offsets (drawable->tiles, &old_off_x, &old_off_y);
  tile_manager_set_offsets (drawable->tiles, off_x, off_y);

  tiles = gimp_drawable_transform_tiles_rotate (drawable, context,
                                                drawable->tiles,
                                                rotate_type, center_x, center_y,
                                                clip_result);

  tile_manager_set_offsets (drawable->tiles, old_off_x, old_off_y);

  if (tiles)
    {
      gimp_drawable_transform_paste (drawable, tiles, FALSE);
      tile_manager_unref (tiles);
    }
}

static void
gimp_drawable_transform (GimpItem               *item,
                         GimpContext            *context,
                         const GimpMatrix3      *matrix,
                         GimpTransformDirection  direction,
                         GimpInterpolationType   interpolation_type,
                         gint                    recursion_level,
                         GimpTransformResize     clip_result,
                         GimpProgress           *progress)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (item);
  TileManager  *tiles;
  gint          off_x, off_y;
  gint          old_off_x, old_off_y;

  gimp_item_offsets (item, &off_x, &off_y);

  tile_manager_get_offsets (drawable->tiles, &old_off_x, &old_off_y);
  tile_manager_set_offsets (drawable->tiles, off_x, off_y);

  tiles = gimp_drawable_transform_tiles_affine (drawable, context,
                                                drawable->tiles,
                                                matrix, direction,
                                                interpolation_type,
                                                recursion_level,
                                                clip_result,
                                                progress);

  tile_manager_set_offsets (drawable->tiles, old_off_x, old_off_y);

  if (tiles)
    {
      gimp_drawable_transform_paste (drawable, tiles, FALSE);
      tile_manager_unref (tiles);
    }
}

static gboolean
gimp_drawable_get_pixel_at (GimpPickable *pickable,
                            gint          x,
                            gint          y,
                            guchar       *pixel)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (pickable);

  /* do not make this a g_return_if_fail() */
  if (x < 0 || x >= GIMP_ITEM (drawable)->width ||
      y < 0 || y >= GIMP_ITEM (drawable)->height)
    return FALSE;

  read_pixel_data_1 (gimp_drawable_get_tiles (drawable), x, y, pixel);

  return TRUE;
}

static void
gimp_drawable_real_update (GimpDrawable *drawable,
                           gint          x,
                           gint          y,
                           gint          width,
                           gint          height)
{
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (drawable));
}

static gint64
gimp_drawable_real_estimate_memsize (const GimpDrawable *drawable,
                                     gint                width,
                                     gint                height)
{
  return (gint64) gimp_drawable_bytes (drawable) * width * height;
}

static void
gimp_drawable_real_set_tiles (GimpDrawable *drawable,
                              gboolean      push_undo,
                              const gchar  *undo_desc,
                              TileManager  *tiles,
                              GimpImageType type,
                              gint          offset_x,
                              gint          offset_y)
{
  GimpItem *item;
  gboolean  old_has_alpha;

  g_return_if_fail (tile_manager_bpp (tiles) == GIMP_IMAGE_TYPE_BYTES (type));

  item = GIMP_ITEM (drawable);

  old_has_alpha = gimp_drawable_has_alpha (drawable);

  gimp_drawable_invalidate_boundary (drawable);

  if (push_undo)
    gimp_image_undo_push_drawable_mod (gimp_item_get_image (item), undo_desc,
                                       drawable);

  /*  ref new before unrefing old, they might be the same  */
  tile_manager_ref (tiles);

  if (drawable->tiles)
    tile_manager_unref (drawable->tiles);

  drawable->tiles     = tiles;
  drawable->type      = type;
  drawable->bytes     = tile_manager_bpp (tiles);
  drawable->has_alpha = GIMP_IMAGE_TYPE_HAS_ALPHA (type);

  item->offset_x = offset_x;
  item->offset_y = offset_y;

  if (item->width  != tile_manager_width (tiles) ||
      item->height != tile_manager_height (tiles))
    {
      item->width  = tile_manager_width (tiles);
      item->height = tile_manager_height (tiles);

      gimp_viewable_size_changed (GIMP_VIEWABLE (drawable));
    }

  if (old_has_alpha != gimp_drawable_has_alpha (drawable))
    gimp_drawable_alpha_changed (drawable);
}

static void
gimp_drawable_real_push_undo (GimpDrawable *drawable,
                              const gchar  *undo_desc,
                              TileManager  *tiles,
                              gboolean      sparse,
                              gint          x,
                              gint          y,
                              gint          width,
                              gint          height)
{
  gboolean new_tiles = FALSE;

  if (! tiles)
    {
      PixelRegion srcPR, destPR;

      tiles = tile_manager_new (width, height, gimp_drawable_bytes (drawable));
      pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                         x, y, width, height, FALSE);
      pixel_region_init (&destPR, tiles,
                         0, 0, width, height, TRUE);
      copy_region (&srcPR, &destPR);

      new_tiles = TRUE;
    }

  gimp_image_undo_push_drawable (gimp_item_get_image (GIMP_ITEM (drawable)),
                                 undo_desc, drawable,
                                 tiles, sparse,
                                 x, y, width, height);

  if (new_tiles)
    tile_manager_unref (tiles);
}

static void
gimp_drawable_real_swap_pixels (GimpDrawable *drawable,
                                TileManager  *tiles,
                                gboolean      sparse,
                                gint          x,
                                gint          y,
                                gint          width,
                                gint          height)
{
  if (sparse)
    {
      gint i, j;

      for (i = y; i < (y + height); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
        {
          for (j = x; j < (x + width); j += (TILE_WIDTH - (j % TILE_WIDTH)))
            {
              Tile *src_tile;
              Tile *dest_tile;

              src_tile = tile_manager_get_tile (tiles, j, i, FALSE, FALSE);

              if (tile_is_valid (src_tile))
                {
                  /* swap tiles, not pixels! */

                  src_tile = tile_manager_get_tile (tiles,
                                                    j, i, TRUE, FALSE /*TRUE*/);
                  dest_tile = tile_manager_get_tile (gimp_drawable_get_tiles (drawable),
                                                     j, i, TRUE, FALSE /* TRUE */);

                  tile_manager_map_tile (tiles,
                                         j, i, dest_tile);
                  tile_manager_map_tile (gimp_drawable_get_tiles (drawable),
                                         j, i, src_tile);

                  tile_release (dest_tile, FALSE);
                  tile_release (src_tile, FALSE);
                }
            }
        }
    }
  else
    {
      PixelRegion PR1, PR2;

      pixel_region_init (&PR1, tiles,
                         0, 0, width, height, TRUE);
      pixel_region_init (&PR2, gimp_drawable_get_tiles (drawable),
                         x, y, width, height, TRUE);

      swap_region (&PR1, &PR2);
    }

  gimp_drawable_update (drawable, x, y, width, height);
}


/*  public functions  */

gint64
gimp_drawable_estimate_memsize (const GimpDrawable *drawable,
                                gint                width,
                                gint                height)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), 0);

  return GIMP_DRAWABLE_GET_CLASS (drawable)->estimate_memsize (drawable,
                                                               width, height);
}

void
gimp_drawable_configure (GimpDrawable  *drawable,
                         GimpImage     *image,
                         gint           offset_x,
                         gint           offset_y,
                         gint           width,
                         gint           height,
                         GimpImageType  type,
                         const gchar   *name)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (width > 0 && height > 0);

  gimp_item_configure (GIMP_ITEM (drawable), image,
                       offset_x, offset_y, width, height, name);

  drawable->type      = type;
  drawable->bytes     = GIMP_IMAGE_TYPE_BYTES (type);
  drawable->has_alpha = GIMP_IMAGE_TYPE_HAS_ALPHA (type);

  if (drawable->tiles)
    tile_manager_unref (drawable->tiles);

  drawable->tiles = tile_manager_new (width, height, drawable->bytes);

  /*  preview variables  */
  drawable->preview_cache = NULL;
  drawable->preview_valid = FALSE;
}

void
gimp_drawable_update (GimpDrawable *drawable,
                      gint          x,
                      gint          y,
                      gint          width,
                      gint          height)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  g_signal_emit (drawable, gimp_drawable_signals[UPDATE], 0,
                 x, y, width, height);
}

void
gimp_drawable_alpha_changed (GimpDrawable *drawable)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  g_signal_emit (drawable, gimp_drawable_signals[ALPHA_CHANGED], 0);
}

void
gimp_drawable_invalidate_boundary (GimpDrawable *drawable)
{
  GimpDrawableClass *drawable_class;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  drawable_class = GIMP_DRAWABLE_GET_CLASS (drawable);

  if (drawable_class->invalidate_boundary)
    drawable_class->invalidate_boundary (drawable);
}

void
gimp_drawable_get_active_components (const GimpDrawable *drawable,
                                     gboolean           *active)
{
  GimpDrawableClass *drawable_class;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (active != NULL);

  drawable_class = GIMP_DRAWABLE_GET_CLASS (drawable);

  if (drawable_class->get_active_components)
    drawable_class->get_active_components (drawable, active);
}

void
gimp_drawable_apply_region (GimpDrawable         *drawable,
                            PixelRegion          *src2PR,
                            gboolean              push_undo,
                            const gchar          *undo_desc,
                            gdouble               opacity,
                            GimpLayerModeEffects  mode,
                            TileManager          *src1_tiles,
                            gint                  x,
                            gint                  y)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (src2PR != NULL);

  GIMP_DRAWABLE_GET_CLASS (drawable)->apply_region (drawable, src2PR,
                                                    push_undo, undo_desc,
                                                    opacity, mode,
                                                    src1_tiles,
                                                    x, y);
}

void
gimp_drawable_replace_region (GimpDrawable *drawable,
                              PixelRegion  *src2PR,
                              gboolean      push_undo,
                              const gchar  *undo_desc,
                              gdouble       opacity,
                              PixelRegion  *maskPR,
                              gint          x,
                              gint          y)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (src2PR != NULL);
  g_return_if_fail (maskPR != NULL);

  GIMP_DRAWABLE_GET_CLASS (drawable)->replace_region (drawable, src2PR,
                                                      push_undo, undo_desc,
                                                      opacity, maskPR,
                                                      x, y);
}

TileManager *
gimp_drawable_get_tiles (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return drawable->tiles;
}

void
gimp_drawable_set_tiles (GimpDrawable *drawable,
                         gboolean      push_undo,
                         const gchar  *undo_desc,
                         TileManager  *tiles)
{
  gint offset_x, offset_y;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (tiles != NULL);

  if (! gimp_item_is_attached (GIMP_ITEM (drawable)))
    push_undo = FALSE;

  gimp_item_offsets (GIMP_ITEM (drawable), &offset_x, &offset_y);

  gimp_drawable_set_tiles_full (drawable, push_undo, undo_desc, tiles,
                                gimp_drawable_type (drawable),
                                offset_x, offset_y);
}

void
gimp_drawable_set_tiles_full (GimpDrawable       *drawable,
                              gboolean            push_undo,
                              const gchar        *undo_desc,
                              TileManager        *tiles,
                              GimpImageType       type,
                              gint                offset_x,
                              gint                offset_y)
{
  GimpItem  *item;
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (tiles != NULL);
  g_return_if_fail (tile_manager_bpp (tiles) == GIMP_IMAGE_TYPE_BYTES (type));

  item  = GIMP_ITEM (drawable);
  image = gimp_item_get_image (item);

  if (! gimp_item_is_attached (GIMP_ITEM (drawable)))
    push_undo = FALSE;

  if (item->width    != tile_manager_width (tiles)  ||
      item->height   != tile_manager_height (tiles) ||
      item->offset_x != offset_x                    ||
      item->offset_y != offset_y)
    {
      gimp_drawable_update (drawable, 0, 0, item->width, item->height);
    }

  if (gimp_drawable_has_floating_sel (drawable))
    floating_sel_relax (gimp_image_floating_sel (image), FALSE);

  GIMP_DRAWABLE_GET_CLASS (drawable)->set_tiles (drawable,
                                                 push_undo, undo_desc,
                                                 tiles, type,
                                                 offset_x, offset_y);

  if (gimp_drawable_has_floating_sel (drawable))
    floating_sel_rigor (gimp_image_floating_sel (image), FALSE);

  gimp_drawable_update (drawable, 0, 0, item->width, item->height);
}

void
gimp_drawable_swap_pixels (GimpDrawable *drawable,
                           TileManager  *tiles,
                           gboolean      sparse,
                           gint          x,
                           gint          y,
                           gint          width,
                           gint          height)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (tiles != NULL);

  GIMP_DRAWABLE_GET_CLASS (drawable)->swap_pixels (drawable, tiles, sparse,
                                                   x, y, width, height);
}

void
gimp_drawable_push_undo (GimpDrawable *drawable,
                         const gchar  *undo_desc,
                         gint          x1,
                         gint          y1,
                         gint          x2,
                         gint          y2,
                         TileManager  *tiles,
                         gboolean      sparse)
{
  GimpItem *item;
  gint      x, y;
  gint      width, height;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (sparse == FALSE || tiles != NULL);

  item = GIMP_ITEM (drawable);

  g_return_if_fail (gimp_item_is_attached (item));
  g_return_if_fail (sparse == FALSE ||
                    tile_manager_width (tiles) == gimp_item_width (item));
  g_return_if_fail (sparse == FALSE ||
                    tile_manager_height (tiles) == gimp_item_height (item));

#if 0
  g_printerr ("gimp_drawable_push_undo (%s, %d, %d, %d, %d)\n",
              sparse ? "TRUE" : "FALSE", x1, y1, x2 - x1, y2 - y1);
#endif

  if (! gimp_rectangle_intersect (x1, y1,
                                  x2 - x1, y2 - y1,
                                  0, 0,
                                  gimp_item_width (item),
                                  gimp_item_height (item),
                                  &x, &y, &width, &height))
    {
      g_warning ("%s: tried to push empty region", G_STRFUNC);
      return;
    }

  GIMP_DRAWABLE_GET_CLASS (drawable)->push_undo (drawable, undo_desc,
                                                 tiles, sparse,
                                                 x, y, width, height);
}

TileManager *
gimp_drawable_get_shadow_tiles (GimpDrawable *drawable)
{
  GimpItem *item;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  item = GIMP_ITEM (drawable);

  g_return_val_if_fail (gimp_item_is_attached (item), NULL);

  return gimp_image_get_shadow_tiles (gimp_item_get_image (item),
                                      item->width, item->height,
                                      drawable->bytes);
}

void
gimp_drawable_merge_shadow (GimpDrawable *drawable,
                            gboolean      push_undo,
                            const gchar  *undo_desc)
{
  GimpImage *image;
  gint       x, y, width, height;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  g_return_if_fail (image->shadow != NULL);

  /*  A useful optimization here is to limit the update to the
   *  extents of the selection mask, as it cannot extend beyond
   *  them.
   */
  if (gimp_drawable_mask_intersect (drawable, &x, &y, &width, &height))
    {
      PixelRegion shadowPR;

      pixel_region_init (&shadowPR, image->shadow,
                         x, y, width, height, FALSE);
      gimp_drawable_apply_region (drawable, &shadowPR,
                                  push_undo, undo_desc,
                                  GIMP_OPACITY_OPAQUE, GIMP_REPLACE_MODE,
                                  NULL, x, y);
    }
}

void
gimp_drawable_fill (GimpDrawable      *drawable,
                    const GimpRGB     *color,
                    const GimpPattern *pattern)
{
  GimpItem      *item;
  GimpImage     *image;
  GimpImageType  drawable_type;
  PixelRegion    destPR;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (color != NULL || pattern != NULL);
  g_return_if_fail (pattern == NULL || GIMP_IS_PATTERN (pattern));

  item  = GIMP_ITEM (drawable);
  image = gimp_item_get_image (item);

  drawable_type = gimp_drawable_type (drawable);

  pixel_region_init (&destPR, gimp_drawable_get_tiles (drawable),
                     0, 0, gimp_item_width  (item), gimp_item_height (item),
                     TRUE);

  if (color)
    {
      guchar tmp[MAX_CHANNELS];
      guchar c[MAX_CHANNELS];

      gimp_rgba_get_uchar (color,
                           &tmp[RED_PIX],
                           &tmp[GREEN_PIX],
                           &tmp[BLUE_PIX],
                           &tmp[ALPHA_PIX]);

      gimp_image_transform_color (image, drawable_type, c, GIMP_RGB, tmp);

      if (GIMP_IMAGE_TYPE_HAS_ALPHA (drawable_type))
        c[GIMP_IMAGE_TYPE_BYTES (drawable_type) - 1] = tmp[ALPHA_PIX];
      else
        c[GIMP_IMAGE_TYPE_BYTES (drawable_type)] = OPAQUE_OPACITY;

      color_region (&destPR, c);
    }
  else
    {
      TempBuf  *pat_buf;
      gboolean  new_buf;

      pat_buf = gimp_image_transform_temp_buf (image, drawable_type,
                                               pattern->mask, &new_buf);

      pattern_region (&destPR, NULL, pat_buf, 0, 0);

      if (new_buf)
        temp_buf_free (pat_buf);
    }

  gimp_drawable_update (drawable,
                        0, 0,
                        gimp_item_width  (item),
                        gimp_item_height (item));
}

void
gimp_drawable_fill_by_type (GimpDrawable *drawable,
                            GimpContext  *context,
                            GimpFillType  fill_type)
{
  GimpRGB      color;
  GimpPattern *pattern = NULL;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  switch (fill_type)
    {
    case GIMP_FOREGROUND_FILL:
      gimp_context_get_foreground (context, &color);
      break;

    case GIMP_BACKGROUND_FILL:
      gimp_context_get_background (context, &color);
      break;

    case GIMP_WHITE_FILL:
      gimp_rgba_set (&color, 1.0, 1.0, 1.0, GIMP_OPACITY_OPAQUE);
      break;

    case GIMP_TRANSPARENT_FILL:
      gimp_rgba_set (&color, 0.0, 0.0, 0.0, GIMP_OPACITY_TRANSPARENT);
      break;

    case GIMP_PATTERN_FILL:
      pattern = gimp_context_get_pattern (context);
      break;

    case GIMP_NO_FILL:
      return;

    default:
      g_warning ("%s: unknown fill type %d", G_STRFUNC, fill_type);
      return;
    }

  gimp_drawable_fill (drawable, pattern ? NULL : &color, pattern);
}

gboolean
gimp_drawable_mask_bounds (GimpDrawable *drawable,
                           gint         *x1,
                           gint         *y1,
                           gint         *x2,
                           gint         *y2)
{
  GimpItem    *item;
  GimpImage   *image;
  GimpChannel *selection;
  gint         tmp_x1, tmp_y1;
  gint         tmp_x2, tmp_y2;
  gboolean     retval;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  item = GIMP_ITEM (drawable);

  g_return_val_if_fail (gimp_item_is_attached (item), FALSE);

  image     = gimp_item_get_image (item);
  selection = gimp_image_get_mask (image);

  if (GIMP_DRAWABLE (selection) != drawable &&
      gimp_channel_bounds (selection, &tmp_x1, &tmp_y1, &tmp_x2, &tmp_y2))
    {
      gint off_x, off_y;

      gimp_item_offsets (item, &off_x, &off_y);

      tmp_x1 = CLAMP (tmp_x1 - off_x, 0, gimp_item_width  (item));
      tmp_y1 = CLAMP (tmp_y1 - off_y, 0, gimp_item_height (item));
      tmp_x2 = CLAMP (tmp_x2 - off_x, 0, gimp_item_width  (item));
      tmp_y2 = CLAMP (tmp_y2 - off_y, 0, gimp_item_height (item));

      retval = TRUE;
    }
  else
    {
      tmp_x1 = 0;
      tmp_y1 = 0;
      tmp_x2 = gimp_item_width  (item);
      tmp_y2 = gimp_item_height (item);

      retval = FALSE;
    }

  if (x1) *x1 = tmp_x1;
  if (y1) *y1 = tmp_y1;
  if (x2) *x2 = tmp_x2;
  if (y2) *y2 = tmp_y2;

  return retval;;
}

gboolean
gimp_drawable_mask_intersect (GimpDrawable *drawable,
                              gint         *x,
                              gint         *y,
                              gint         *width,
                              gint         *height)
{
  GimpItem    *item;
  GimpImage   *image;
  GimpChannel *selection;
  gint         tmp_x, tmp_y;
  gint         tmp_width, tmp_height;
  gboolean     retval;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  item = GIMP_ITEM (drawable);

  g_return_val_if_fail (gimp_item_is_attached (item), FALSE);

  image     = gimp_item_get_image (item);
  selection = gimp_image_get_mask (image);

  if (GIMP_DRAWABLE (selection) != drawable &&
      gimp_channel_bounds (selection, &tmp_x, &tmp_y, &tmp_width, &tmp_height))
    {
      gint off_x, off_y;

      gimp_item_offsets (item, &off_x, &off_y);

      retval = gimp_rectangle_intersect (tmp_x - off_x, tmp_y - off_y,
                                         tmp_width - tmp_x, tmp_height - tmp_y,
                                         0, 0,
                                         gimp_item_width (item),
                                         gimp_item_height (item),
                                         &tmp_x, &tmp_y,
                                         &tmp_width, &tmp_height);
    }
  else
    {
      tmp_x      = 0;
      tmp_y      = 0;
      tmp_width  = gimp_item_width  (item);
      tmp_height = gimp_item_height (item);

      retval = TRUE;
    }

  if (x)      *x      = tmp_x;
  if (y)      *y      = tmp_y;
  if (width)  *width  = tmp_width;
  if (height) *height = tmp_height;

  return retval;
}

gboolean
gimp_drawable_has_alpha (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return drawable->has_alpha;
}

GimpImageType
gimp_drawable_type (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return drawable->type;
}

GimpImageType
gimp_drawable_type_with_alpha (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return GIMP_IMAGE_TYPE_WITH_ALPHA (gimp_drawable_type (drawable));
}

GimpImageType
gimp_drawable_type_without_alpha (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return GIMP_IMAGE_TYPE_WITHOUT_ALPHA (gimp_drawable_type (drawable));
}

gboolean
gimp_drawable_is_rgb (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return GIMP_IMAGE_TYPE_IS_RGB (gimp_drawable_type (drawable));
}

gboolean
gimp_drawable_is_gray (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return GIMP_IMAGE_TYPE_IS_GRAY (gimp_drawable_type (drawable));
}

gboolean
gimp_drawable_is_indexed (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return GIMP_IMAGE_TYPE_IS_INDEXED (gimp_drawable_type (drawable));
}

gint
gimp_drawable_bytes (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return drawable->bytes;
}

gint
gimp_drawable_bytes_with_alpha (const GimpDrawable *drawable)
{
  GimpImageType type;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  type = GIMP_IMAGE_TYPE_WITH_ALPHA (gimp_drawable_type (drawable));

  return GIMP_IMAGE_TYPE_BYTES (type);
}

gint
gimp_drawable_bytes_without_alpha (const GimpDrawable *drawable)
{
  GimpImageType type;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  type = GIMP_IMAGE_TYPE_WITHOUT_ALPHA (gimp_drawable_type (drawable));

  return GIMP_IMAGE_TYPE_BYTES (type);
}

gboolean
gimp_drawable_has_floating_sel (const GimpDrawable *drawable)
{
  GimpImage *image;
  GimpLayer *floating_sel;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  floating_sel = gimp_image_floating_sel (image);

  return (floating_sel && floating_sel->fs.drawable == drawable);
}

const guchar *
gimp_drawable_get_colormap (const GimpDrawable *drawable)
{
  GimpImage *image;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  return image ? gimp_image_get_colormap (image) : NULL;
}
