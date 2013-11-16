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

#include <cairo.h>
#include <gegl.h>
#include <gegl-plugin.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"
#include "paint-funcs/scale-region.h"

#include "gegl/gimp-gegl-utils.h"

#include "gimp.h" /* temp for gimp_use_gegl() */
#include "gimp-utils.h" /* temp for GIMP_TIMER */
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpdrawable-combine.h"
#include "gimpdrawable-convert.h"
#include "gimpdrawable-operation.h"
#include "gimpdrawable-preview.h"
#include "gimpdrawable-private.h"
#include "gimpdrawable-shadow.h"
#include "gimpdrawable-transform.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimpmarshal.h"
#include "gimppattern.h"
#include "gimppickable.h"
#include "gimppreviewcache.h"
#include "gimpprogress.h"

#include "gimp-log.h"

#include "gimp-intl.h"


enum
{
  UPDATE,
  ALPHA_CHANGED,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void  gimp_drawable_pickable_iface_init (GimpPickableInterface *iface);

static void       gimp_drawable_dispose            (GObject           *object);
static void       gimp_drawable_finalize           (GObject           *object);

static gint64     gimp_drawable_get_memsize        (GimpObject        *object,
                                                    gint64            *gui_size);

static gboolean   gimp_drawable_get_size           (GimpViewable      *viewable,
                                                    gint              *width,
                                                    gint              *height);
static void       gimp_drawable_invalidate_preview (GimpViewable      *viewable);

static void       gimp_drawable_removed            (GimpItem          *item);
static void       gimp_drawable_visibility_changed (GimpItem          *item);
static GimpItem * gimp_drawable_duplicate          (GimpItem          *item,
                                                    GType              new_type);
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

static void       gimp_drawable_real_convert_type  (GimpDrawable      *drawable,
                                                    GimpImage         *dest_image,
                                                    GimpImageBaseType  new_base_type,
                                                    gboolean           push_undo);

static TileManager * gimp_drawable_real_get_tiles  (GimpDrawable      *drawable);
static void       gimp_drawable_real_set_tiles     (GimpDrawable      *drawable,
                                                    gboolean           push_undo,
                                                    const gchar       *undo_desc,
                                                    TileManager       *tiles,
                                                    GimpImageType      type,
                                                    gint               offset_x,
                                                    gint               offset_y);
static GeglNode * gimp_drawable_get_node           (GimpItem          *item);

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

static void       gimp_drawable_sync_source_node   (GimpDrawable      *drawable,
                                                    gboolean           detach_fs);
static void       gimp_drawable_fs_notify          (GimpLayer         *fs,
                                                    const GParamSpec  *pspec,
                                                    GimpDrawable      *drawable);
static void       gimp_drawable_fs_update          (GimpLayer         *fs,
                                                    gint               x,
                                                    gint               y,
                                                    gint               width,
                                                    gint               height,
                                                    GimpDrawable      *drawable);


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

  object_class->dispose              = gimp_drawable_dispose;
  object_class->finalize             = gimp_drawable_finalize;

  gimp_object_class->get_memsize     = gimp_drawable_get_memsize;

  viewable_class->get_size           = gimp_drawable_get_size;
  viewable_class->invalidate_preview = gimp_drawable_invalidate_preview;
  viewable_class->get_preview        = gimp_drawable_get_preview;

  item_class->removed                = gimp_drawable_removed;
  item_class->visibility_changed     = gimp_drawable_visibility_changed;
  item_class->duplicate              = gimp_drawable_duplicate;
  item_class->scale                  = gimp_drawable_scale;
  item_class->resize                 = gimp_drawable_resize;
  item_class->flip                   = gimp_drawable_flip;
  item_class->rotate                 = gimp_drawable_rotate;
  item_class->transform              = gimp_drawable_transform;
  item_class->get_node               = gimp_drawable_get_node;

  klass->update                      = gimp_drawable_real_update;
  klass->alpha_changed               = NULL;
  klass->estimate_memsize            = gimp_drawable_real_estimate_memsize;
  klass->invalidate_boundary         = NULL;
  klass->get_active_components       = NULL;
  klass->convert_type                = gimp_drawable_real_convert_type;
  klass->apply_region                = gimp_drawable_real_apply_region;
  klass->replace_region              = gimp_drawable_real_replace_region;
  klass->get_tiles                   = gimp_drawable_real_get_tiles;
  klass->set_tiles                   = gimp_drawable_real_set_tiles;
  klass->push_undo                   = gimp_drawable_real_push_undo;
  klass->swap_pixels                 = gimp_drawable_real_swap_pixels;

  g_type_class_add_private (klass, sizeof (GimpDrawablePrivate));
}

static void
gimp_drawable_init (GimpDrawable *drawable)
{
  drawable->private = G_TYPE_INSTANCE_GET_PRIVATE (drawable,
                                                   GIMP_TYPE_DRAWABLE,
                                                   GimpDrawablePrivate);

  drawable->private->type = -1;
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
gimp_drawable_dispose (GObject *object)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (object);

  if (gimp_drawable_get_floating_sel (drawable))
    gimp_drawable_detach_floating_sel (drawable);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_drawable_finalize (GObject *object)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (object);

  if (drawable->private->tiles)
    {
      tile_manager_unref (drawable->private->tiles);
      drawable->private->tiles = NULL;
    }

  gimp_drawable_free_shadow_tiles (drawable);

  if (drawable->private->source_node)
    {
      g_object_unref (drawable->private->source_node);
      drawable->private->source_node = NULL;
    }

  if (drawable->private->preview_cache)
    gimp_preview_cache_invalidate (&drawable->private->preview_cache);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_drawable_get_memsize (GimpObject *object,
                           gint64     *gui_size)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (object);
  gint64        memsize  = 0;

  memsize += tile_manager_get_memsize (gimp_drawable_get_tiles (drawable),
                                       FALSE);
  memsize += tile_manager_get_memsize (drawable->private->shadow, FALSE);

  *gui_size += gimp_preview_cache_get_memsize (drawable->private->preview_cache);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_drawable_get_size (GimpViewable *viewable,
                        gint         *width,
                        gint         *height)
{
  GimpItem *item = GIMP_ITEM (viewable);

  *width  = gimp_item_get_width  (item);
  *height = gimp_item_get_height (item);

  return TRUE;
}

static void
gimp_drawable_invalidate_preview (GimpViewable *viewable)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (viewable);

  GIMP_VIEWABLE_CLASS (parent_class)->invalidate_preview (viewable);

  drawable->private->preview_valid = FALSE;

  if (drawable->private->preview_cache)
    gimp_preview_cache_invalidate (&drawable->private->preview_cache);
}

static void
gimp_drawable_removed (GimpItem *item)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (item);

  gimp_drawable_free_shadow_tiles (drawable);

  if (GIMP_ITEM_CLASS (parent_class)->removed)
    GIMP_ITEM_CLASS (parent_class)->removed (item);
}

static void
gimp_drawable_visibility_changed (GimpItem *item)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (item);
  GeglNode     *node;

  /*  don't use gimp_item_get_node() because that would create
   *  the node.
   */
  node = gimp_item_peek_node (item);

  if (node)
    {
      GeglNode *input  = gegl_node_get_input_proxy  (node, "input");
      GeglNode *output = gegl_node_get_output_proxy (node, "output");

      if (gimp_item_get_visible (item) &&
          ! (GIMP_IS_LAYER (item) &&
             gimp_layer_is_floating_sel (GIMP_LAYER (item))))
        {
          gegl_node_connect_to (input,                        "output",
                                drawable->private->mode_node, "input");
          gegl_node_connect_to (drawable->private->mode_node, "output",
                                output,                       "input");
        }
      else
        {
          gegl_node_disconnect (drawable->private->mode_node, "input");

          gegl_node_connect_to (input,  "output",
                                output, "input");
        }

      /* FIXME: chain up again when above floating sel special case is gone */
      return;
    }

  GIMP_ITEM_CLASS (parent_class)->visibility_changed (item);
}

static GimpItem *
gimp_drawable_duplicate (GimpItem *item,
                         GType     new_type)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (GIMP_IS_DRAWABLE (new_item))
    {
      GimpDrawable  *drawable     = GIMP_DRAWABLE (item);
      GimpDrawable  *new_drawable = GIMP_DRAWABLE (new_item);
      GimpImageType  image_type   = gimp_drawable_type (drawable);
      PixelRegion    srcPR;
      PixelRegion    destPR;

      new_drawable->private->type = image_type;

      if (new_drawable->private->tiles)
        tile_manager_unref (new_drawable->private->tiles);

      new_drawable->private->tiles =
        tile_manager_new (gimp_item_get_width  (new_item),
                          gimp_item_get_height (new_item),
                          gimp_drawable_bytes (new_drawable));

      pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                         0, 0,
                         gimp_item_get_width  (item),
                         gimp_item_get_height (item),
                         FALSE);
      pixel_region_init (&destPR, gimp_drawable_get_tiles (new_drawable),
                         0, 0,
                         gimp_item_get_width  (new_item),
                         gimp_item_get_height (new_item),
                         TRUE);

      copy_region (&srcPR, &destPR);
    }

  return new_item;
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
  TileManager  *new_tiles;

  new_tiles = tile_manager_new (new_width, new_height,
                                gimp_drawable_bytes (drawable));

#ifdef GIMP_UNSTABLE
  GIMP_TIMER_START ();
#endif

  if (gimp_use_gegl (gimp_item_get_image (item)->gimp) &&
      ! gimp_drawable_is_indexed (drawable)            &&
      interpolation_type != GIMP_INTERPOLATION_LANCZOS)
    {
      GeglNode *scale;

      scale = g_object_new (GEGL_TYPE_NODE,
                            "operation", "gegl:scale",
                            NULL);

      gegl_node_set (scale,
                     "origin-x",   0.0,
                     "origin-y",   0.0,
                     "filter",     gimp_interpolation_to_gegl_filter (interpolation_type),
                     "hard-edges", FALSE,
                     "x",          ((gdouble) new_width /
                                    gimp_item_get_width  (item)),
                     "y",          ((gdouble) new_height /
                                    gimp_item_get_height (item)),
                     NULL);

      gimp_drawable_apply_operation_to_tiles (drawable, progress, C_("undo-type", "Scale"),
                                              scale, TRUE, new_tiles);
      g_object_unref (scale);
    }
  else
    {
      PixelRegion srcPR, destPR;

      pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                         0, 0,
                         gimp_item_get_width  (item),
                         gimp_item_get_height (item),
                         FALSE);
      pixel_region_init (&destPR, new_tiles,
                         0, 0,
                         new_width, new_height,
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
    }

#ifdef GIMP_UNSTABLE
  GIMP_TIMER_END ("scaling");
#endif

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
  gboolean      intersect;

  /*  if the size doesn't change, this is a nop  */
  if (new_width  == gimp_item_get_width  (item) &&
      new_height == gimp_item_get_height (item) &&
      offset_x   == 0                           &&
      offset_y   == 0)
    return;

  new_offset_x = gimp_item_get_offset_x (item) - offset_x;
  new_offset_y = gimp_item_get_offset_y (item) - offset_y;

  intersect = gimp_rectangle_intersect (gimp_item_get_offset_x (item),
                                        gimp_item_get_offset_y (item),
                                        gimp_item_get_width (item),
                                        gimp_item_get_height (item),
                                        new_offset_x,
                                        new_offset_y,
                                        new_width,
                                        new_height,
                                        &copy_x,
                                        &copy_y,
                                        &copy_width,
                                        &copy_height);

  new_tiles = tile_manager_new (new_width, new_height,
                                gimp_drawable_bytes (drawable));

  /*  Determine whether the new tiles need to be initially cleared  */
  if (! intersect              ||
      copy_width  != new_width ||
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
  if (intersect && copy_width && copy_height)
    {
      pixel_region_init (&srcPR,
                         gimp_drawable_get_tiles (drawable),
                         copy_x - gimp_item_get_offset_x (item),
                         copy_y - gimp_item_get_offset_y (item),
                         copy_width,
                         copy_height,
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
  gint          new_off_x, new_off_y;

  gimp_item_get_offset (item, &off_x, &off_y);

  tiles = gimp_drawable_transform_tiles_flip (drawable, context,
                                              gimp_drawable_get_tiles (drawable),
                                              off_x, off_y,
                                              flip_type, axis,
                                              clip_result,
                                              &new_off_x, &new_off_y);

  if (tiles)
    {
      gimp_drawable_transform_paste (drawable, tiles,
                                     new_off_x, new_off_y, FALSE);
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
  gint          new_off_x, new_off_y;

  gimp_item_get_offset (item, &off_x, &off_y);

  tiles = gimp_drawable_transform_tiles_rotate (drawable, context,
                                                gimp_drawable_get_tiles (drawable),
                                                off_x, off_y,
                                                rotate_type, center_x, center_y,
                                                clip_result,
                                                &new_off_x, &new_off_y);

  if (tiles)
    {
      gimp_drawable_transform_paste (drawable, tiles,
                                     new_off_x, new_off_y, FALSE);
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
  gint          new_off_x, new_off_y;

  gimp_item_get_offset (item, &off_x, &off_y);

  tiles = gimp_drawable_transform_tiles_affine (drawable, context,
                                                gimp_drawable_get_tiles (drawable),
                                                off_x, off_y,
                                                matrix, direction,
                                                interpolation_type,
                                                recursion_level,
                                                clip_result,
                                                &new_off_x, &new_off_y,
                                                progress);

  if (tiles)
    {
      gimp_drawable_transform_paste (drawable, tiles,
                                     new_off_x, new_off_y, FALSE);
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
  if (x < 0 || x >= gimp_item_get_width  (GIMP_ITEM (drawable)) ||
      y < 0 || y >= gimp_item_get_height (GIMP_ITEM (drawable)))
    return FALSE;

  tile_manager_read_pixel_data_1 (gimp_drawable_get_tiles (drawable), x, y,
                                  pixel);

  return TRUE;
}

static void
gimp_drawable_real_update (GimpDrawable *drawable,
                           gint          x,
                           gint          y,
                           gint          width,
                           gint          height)
{
  if (drawable->private->tile_source_node)
    {
      GObject       *operation;
      GeglRectangle  rect;

      g_object_get (drawable->private->tile_source_node,
                    "gegl-operation", &operation,
                    NULL);

      rect.x      = x;
      rect.y      = y;
      rect.width  = width;
      rect.height = height;

      gegl_operation_invalidate (GEGL_OPERATION (operation), &rect, FALSE);

      g_object_unref (operation);
    }

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
gimp_drawable_real_convert_type (GimpDrawable      *drawable,
                                 GimpImage         *dest_image,
                                 GimpImageBaseType  new_base_type,
                                 gboolean           push_undo)
{
  g_return_if_fail (new_base_type != GIMP_INDEXED);

  switch (new_base_type)
    {
    case GIMP_RGB:
      gimp_drawable_convert_rgb (drawable, push_undo);
      break;

    case GIMP_GRAY:
      gimp_drawable_convert_grayscale (drawable, push_undo);
      break;

    default:
      break;
    }
}

static TileManager *
gimp_drawable_real_get_tiles (GimpDrawable *drawable)
{
  return drawable->private->tiles;
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
                                       drawable, FALSE);

  /*  ref new before unrefing old, they might be the same  */
  tile_manager_ref (tiles);

  if (drawable->private->tiles)
    tile_manager_unref (drawable->private->tiles);

  drawable->private->tiles = tiles;
  drawable->private->type  = type;

  gimp_item_set_offset (item, offset_x, offset_y);
  gimp_item_set_size (item,
                      tile_manager_width  (tiles),
                      tile_manager_height (tiles));

  if (old_has_alpha != gimp_drawable_has_alpha (drawable))
    gimp_drawable_alpha_changed (drawable);

  if (drawable->private->tile_source_node)
    gegl_node_set (drawable->private->tile_source_node,
                   "tile-manager", gimp_drawable_get_tiles (drawable),
                   NULL);
}

static GeglNode *
gimp_drawable_get_node (GimpItem *item)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (item);
  GeglNode     *node;
  GeglNode     *input;
  GeglNode     *output;

  node = GIMP_ITEM_CLASS (parent_class)->get_node (item);

  g_warn_if_fail (drawable->private->mode_node == NULL);

  drawable->private->mode_node = gegl_node_new_child (node,
                                                      "operation", "gegl:over",
                                                      NULL);

  input  = gegl_node_get_input_proxy  (node, "input");
  output = gegl_node_get_output_proxy (node, "output");

  if (gimp_item_get_visible (GIMP_ITEM (drawable)) &&
      ! (GIMP_IS_LAYER (drawable) &&
         gimp_layer_is_floating_sel (GIMP_LAYER (drawable))))
    {
      gegl_node_connect_to (input,                        "output",
                            drawable->private->mode_node, "input");
      gegl_node_connect_to (drawable->private->mode_node, "output",
                            output,                       "input");
    }
  else
    {
      gegl_node_connect_to (input,  "output",
                            output, "input");
    }

  return node;
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

static void
gimp_drawable_sync_source_node (GimpDrawable *drawable,
                                gboolean      detach_fs)
{
  GimpLayer *fs = gimp_drawable_get_floating_sel (drawable);
  GeglNode  *output;

  if (! drawable->private->source_node)
    return;

  output = gegl_node_get_output_proxy (drawable->private->source_node, "output");

  if (fs && ! detach_fs)
    {
      gint off_x, off_y;
      gint fs_off_x, fs_off_y;

      if (! drawable->private->fs_crop_node)
        {
          GeglNode *fs_source;

          fs_source = gimp_drawable_get_source_node (GIMP_DRAWABLE (fs));

          /* rip the fs' source node out of its graph */
          if (fs->opacity_node)
            {
              gegl_node_disconnect (fs->opacity_node, "input");
              gegl_node_remove_child (gimp_item_get_node (GIMP_ITEM (fs)),
                                      fs_source);
            }

          gegl_node_add_child (drawable->private->source_node, fs_source);

          drawable->private->fs_crop_node =
            gegl_node_new_child (drawable->private->source_node,
                                 "operation", "gegl:crop",
                                 NULL);

          gegl_node_connect_to (fs_source,                       "output",
                                drawable->private->fs_crop_node, "input");

          drawable->private->fs_opacity_node =
            gegl_node_new_child (drawable->private->source_node,
                                 "operation", "gegl:opacity",
                                 NULL);

          gegl_node_connect_to (drawable->private->fs_crop_node,    "output",
                                drawable->private->fs_opacity_node, "input");

          drawable->private->fs_offset_node =
            gegl_node_new_child (drawable->private->source_node,
                                 "operation", "gegl:translate",
                                 NULL);

          gegl_node_connect_to (drawable->private->fs_opacity_node, "output",
                                drawable->private->fs_offset_node,  "input");

          drawable->private->fs_mode_node =
            gegl_node_new_child (drawable->private->source_node,
                                 "operation", "gimp:point-layer-mode",
                                 NULL);

          gegl_node_connect_to (drawable->private->tile_source_node, "output",
                                drawable->private->fs_mode_node,     "input");
          gegl_node_connect_to (drawable->private->fs_offset_node,   "output",
                                drawable->private->fs_mode_node,     "aux");

          gegl_node_connect_to (drawable->private->fs_mode_node, "output",
                                output,                          "input");

          g_signal_connect (fs, "notify",
                            G_CALLBACK (gimp_drawable_fs_notify),
                            drawable);
        }

      gegl_node_set (drawable->private->fs_opacity_node,
                     "value", gimp_layer_get_opacity (fs),
                     NULL);

      gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);
      gimp_item_get_offset (GIMP_ITEM (fs), &fs_off_x, &fs_off_y);

      gegl_node_set (drawable->private->fs_crop_node,
                     "x",      (gdouble) (off_x - fs_off_x),
                     "y",      (gdouble) (off_y - fs_off_y),
                     "width",  (gdouble) gimp_item_get_width  (GIMP_ITEM (drawable)),
                     "height", (gdouble) gimp_item_get_height (GIMP_ITEM (drawable)),
                     NULL);

      gegl_node_set (drawable->private->fs_offset_node,
                     "x", (gdouble) (fs_off_x - off_x),
                     "y", (gdouble) (fs_off_y - off_y),
                     NULL);

      gegl_node_set (drawable->private->fs_mode_node,
                     "blend-mode", gimp_layer_get_mode (fs),
                     NULL);
    }
  else
    {
      if (drawable->private->fs_crop_node)
        {
          GeglNode *fs_source;

          gegl_node_disconnect (drawable->private->fs_crop_node, "input");
          gegl_node_disconnect (drawable->private->fs_opacity_node, "input");
          gegl_node_disconnect (drawable->private->fs_offset_node, "input");
          gegl_node_disconnect (drawable->private->fs_mode_node, "input");
          gegl_node_disconnect (drawable->private->fs_mode_node, "aux");

          fs_source = gimp_drawable_get_source_node (GIMP_DRAWABLE (fs));
          gegl_node_remove_child (drawable->private->source_node,
                                  fs_source);

          /* plug the fs' source node back into its graph */
          if (fs->opacity_node)
            {
              gegl_node_add_child (gimp_item_get_node (GIMP_ITEM (fs)),
                                   fs_source);
              gegl_node_connect_to (fs_source,        "output",
                                    fs->opacity_node, "input");
            }

          gegl_node_remove_child (drawable->private->source_node,
                                  drawable->private->fs_crop_node);
          drawable->private->fs_crop_node = NULL;

          gegl_node_remove_child (drawable->private->source_node,
                                  drawable->private->fs_opacity_node);
          drawable->private->fs_opacity_node = NULL;

          gegl_node_remove_child (drawable->private->source_node,
                                  drawable->private->fs_offset_node);
          drawable->private->fs_offset_node = NULL;

          gegl_node_remove_child (drawable->private->source_node,
                                  drawable->private->fs_mode_node);
          drawable->private->fs_mode_node = NULL;

          g_signal_handlers_disconnect_by_func (fs,
                                                gimp_drawable_fs_notify,
                                                drawable);
        }

      gegl_node_connect_to (drawable->private->tile_source_node, "output",
                            output,                              "input");
    }
}

static void
gimp_drawable_fs_notify (GimpLayer        *fs,
                         const GParamSpec *pspec,
                         GimpDrawable     *drawable)
{
  if (! strcmp (pspec->name, "offset-x") ||
      ! strcmp (pspec->name, "offset-y") ||
      ! strcmp (pspec->name, "visible")  ||
      ! strcmp (pspec->name, "mode")     ||
      ! strcmp (pspec->name, "opacity"))
    {
      gimp_drawable_sync_source_node (drawable, FALSE);
    }
}

static void
gimp_drawable_fs_update (GimpLayer    *fs,
                         gint          x,
                         gint          y,
                         gint          width,
                         gint          height,
                         GimpDrawable *drawable)
{
  gint fs_off_x, fs_off_y;
  gint off_x, off_y;
  gint dr_x, dr_y, dr_width, dr_height;

  gimp_item_get_offset (GIMP_ITEM (fs), &fs_off_x, &fs_off_y);
  gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

  if (gimp_rectangle_intersect (x + fs_off_x,
                                y + fs_off_y,
                                width,
                                height,
                                off_x,
                                off_y,
                                gimp_item_get_width  (GIMP_ITEM (drawable)),
                                gimp_item_get_height (GIMP_ITEM (drawable)),
                                &dr_x,
                                &dr_y,
                                &dr_width,
                                &dr_height))
    {
      gimp_drawable_update (drawable,
                            dr_x - off_x, dr_y - off_y,
                            dr_width, dr_height);
    }
}


/*  public functions  */

GimpDrawable *
gimp_drawable_new (GType          type,
                   GimpImage     *image,
                   const gchar   *name,
                   gint           offset_x,
                   gint           offset_y,
                   gint           width,
                   gint           height,
                   GimpImageType  image_type)
{
  GimpDrawable *drawable;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (g_type_is_a (type, GIMP_TYPE_DRAWABLE), NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  drawable = GIMP_DRAWABLE (gimp_item_new (type,
                                           image, name,
                                           offset_x, offset_y,
                                           width, height));

  drawable->private->type  = image_type;
  drawable->private->tiles = tile_manager_new (width, height,
                                               gimp_drawable_bytes (drawable));

  return drawable;
}

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
gimp_drawable_convert_type (GimpDrawable      *drawable,
                            GimpImage         *dest_image,
                            GimpImageBaseType  new_base_type,
                            gboolean           push_undo)
{
  GimpImageType type;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (dest_image == NULL || GIMP_IS_IMAGE (dest_image));
  g_return_if_fail (new_base_type != GIMP_INDEXED || GIMP_IS_IMAGE (dest_image));

  if (! gimp_item_is_attached (GIMP_ITEM (drawable)))
    push_undo = FALSE;

  type = gimp_drawable_type (drawable);

  g_return_if_fail (new_base_type != GIMP_IMAGE_TYPE_BASE_TYPE (type));

  GIMP_DRAWABLE_GET_CLASS (drawable)->convert_type (drawable, dest_image,
                                                    new_base_type, push_undo);
}

void
gimp_drawable_apply_region (GimpDrawable         *drawable,
                            PixelRegion          *src2PR,
                            gboolean              push_undo,
                            const gchar          *undo_desc,
                            gdouble               opacity,
                            GimpLayerModeEffects  mode,
                            TileManager          *src1_tiles,
                            PixelRegion          *destPR,
                            gint                  x,
                            gint                  y)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (src2PR != NULL);

  GIMP_DRAWABLE_GET_CLASS (drawable)->apply_region (drawable, src2PR,
                                                    push_undo, undo_desc,
                                                    opacity, mode,
                                                    src1_tiles, destPR,
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

void
gimp_drawable_project_region (GimpDrawable *drawable,
                              gint          x,
                              gint          y,
                              gint          width,
                              gint          height,
                              PixelRegion  *projPR,
                              gboolean      combine)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (projPR != NULL);

  GIMP_DRAWABLE_GET_CLASS (drawable)->project_region (drawable,
                                                      x, y, width, height,
                                                      projPR, combine);
}

void
gimp_drawable_init_src_region (GimpDrawable  *drawable,
                               PixelRegion   *srcPR,
                               gint           x,
                               gint           y,
                               gint           width,
                               gint           height,
                               TileManager  **temp_tiles)
{
  GimpLayer *fs;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (srcPR != NULL);
  g_return_if_fail (temp_tiles != NULL);

  fs = gimp_drawable_get_floating_sel (drawable);

  if (fs)
    {
      gint off_x, off_y;
      gint fs_off_x, fs_off_y;
      gint combine_x, combine_y;
      gint combine_width, combine_height;

      gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);
      gimp_item_get_offset (GIMP_ITEM (fs), &fs_off_x, &fs_off_y);

      if (gimp_item_get_visible (GIMP_ITEM (fs)) &&
          gimp_rectangle_intersect (x + off_x, y + off_y,
                                    width, height,
                                    fs_off_x, fs_off_y,
                                    gimp_item_get_width  (GIMP_ITEM (fs)),
                                    gimp_item_get_height (GIMP_ITEM (fs)),
                                    &combine_x, &combine_y,
                                    &combine_width, &combine_height))
        {
          PixelRegion tempPR;
          PixelRegion destPR;
          PixelRegion fsPR;
          gboolean    lock_alpha = FALSE;

          /*  a temporary buffer for the compisition of the drawable and
           *  its floating selection
           */
          *temp_tiles = tile_manager_new (width, height,
                                          gimp_drawable_bytes (drawable));

          /*  first, initialize the entire buffer with the drawable's
           *  contents
           */
          pixel_region_init (&tempPR, gimp_drawable_get_tiles (drawable),
                             x, y, width, height,
                             FALSE);
          pixel_region_init (&destPR, *temp_tiles,
                             0, 0, width, height,
                             TRUE);
          copy_region (&tempPR, &destPR);

          /*  then, apply the floating selection onto the buffer just as
           *  we apply it onto the drawable when anchoring the floating
           *  selection
           */
          pixel_region_init (&fsPR,
                             gimp_drawable_get_tiles (GIMP_DRAWABLE (fs)),
                             combine_x - fs_off_x,
                             combine_y - fs_off_y,
                             combine_width, combine_height,
                             FALSE);
          pixel_region_init (&destPR, *temp_tiles,
                             combine_x - x - off_x,
                             combine_y - y - off_y,
                             combine_width, combine_height,
                             TRUE);

          if (GIMP_IS_LAYER (drawable))
            {
              lock_alpha = gimp_layer_get_lock_alpha (GIMP_LAYER (drawable));

              if (lock_alpha)
                gimp_layer_set_lock_alpha (GIMP_LAYER (drawable), FALSE, FALSE);
            }

          gimp_drawable_apply_region (drawable, &fsPR,
                                      FALSE, NULL,
                                      gimp_layer_get_opacity (fs),
                                      gimp_layer_get_mode (fs),
                                      NULL, &destPR,
                                      combine_x - off_x,
                                      combine_y - off_y);

          if (lock_alpha)
            gimp_layer_set_lock_alpha (GIMP_LAYER (drawable), TRUE, FALSE);

          /*  finally, return a PixelRegion on the composited buffer instead
           *  of the drawable's tiles
           */
          pixel_region_init (srcPR, *temp_tiles,
                             0, 0, width, height,
                             FALSE);

          return;
       }
    }

  pixel_region_init (srcPR, gimp_drawable_get_tiles (drawable),
                     x, y, width, height,
                     FALSE);
  *temp_tiles = NULL;
}

TileManager *
gimp_drawable_get_tiles (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return GIMP_DRAWABLE_GET_CLASS (drawable)->get_tiles (drawable);
}

void
gimp_drawable_set_tiles (GimpDrawable  *drawable,
                         gboolean       push_undo,
                         const gchar   *undo_desc,
                         TileManager   *tiles,
                         GimpImageType  type)
{
  gint offset_x, offset_y;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (tiles != NULL);

  if (! gimp_item_is_attached (GIMP_ITEM (drawable)))
    push_undo = FALSE;

  gimp_item_get_offset (GIMP_ITEM (drawable), &offset_x, &offset_y);

  gimp_drawable_set_tiles_full (drawable, push_undo, undo_desc, tiles, type,
                                offset_x, offset_y);
}

void
gimp_drawable_set_tiles_full (GimpDrawable  *drawable,
                              gboolean       push_undo,
                              const gchar   *undo_desc,
                              TileManager   *tiles,
                              GimpImageType  type,
                              gint           offset_x,
                              gint           offset_y)
{
  GimpItem *item;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (tiles != NULL);
  g_return_if_fail (tile_manager_bpp (tiles) == GIMP_IMAGE_TYPE_BYTES (type));

  item = GIMP_ITEM (drawable);

  if (! gimp_item_is_attached (GIMP_ITEM (drawable)))
    push_undo = FALSE;

  if (gimp_item_get_width  (item)   != tile_manager_width (tiles)  ||
      gimp_item_get_height (item)   != tile_manager_height (tiles) ||
      gimp_item_get_offset_x (item) != offset_x                    ||
      gimp_item_get_offset_y (item) != offset_y)
    {
      gimp_drawable_update (drawable,
                            0, 0,
                            gimp_item_get_width  (item),
                            gimp_item_get_height (item));
    }

  g_object_freeze_notify (G_OBJECT (drawable));

  GIMP_DRAWABLE_GET_CLASS (drawable)->set_tiles (drawable,
                                                 push_undo, undo_desc,
                                                 tiles, type,
                                                 offset_x, offset_y);

  g_object_thaw_notify (G_OBJECT (drawable));

  gimp_drawable_update (drawable,
                        0, 0,
                        gimp_item_get_width  (item),
                        gimp_item_get_height (item));
}

GeglNode *
gimp_drawable_get_source_node (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  if (drawable->private->source_node)
    return drawable->private->source_node;

  drawable->private->source_node = gegl_node_new ();

  drawable->private->tile_source_node =
    gegl_node_new_child (drawable->private->source_node,
                         "operation",    "gimp:tilemanager-source",
                         "tile-manager", gimp_drawable_get_tiles (drawable),
                         "linear",       TRUE,
                         NULL);

  gimp_drawable_sync_source_node (drawable, FALSE);

  return drawable->private->source_node;
}

GeglNode *
gimp_drawable_get_mode_node (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  if (! drawable->private->mode_node)
    gimp_item_get_node (GIMP_ITEM (drawable));

  return drawable->private->mode_node;
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
                         gint          x,
                         gint          y,
                         gint          width,
                         gint          height,
                         TileManager  *tiles,
                         gboolean      sparse)
{
  GimpItem *item;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (sparse == FALSE || tiles != NULL);

  item = GIMP_ITEM (drawable);

  g_return_if_fail (gimp_item_is_attached (item));
  g_return_if_fail (sparse == FALSE ||
                    tile_manager_width (tiles) == gimp_item_get_width (item));
  g_return_if_fail (sparse == FALSE ||
                    tile_manager_height (tiles) == gimp_item_get_height (item));

#if 0
  g_printerr ("gimp_drawable_push_undo (%s, %d, %d, %d, %d)\n",
              sparse ? "TRUE" : "FALSE", x, y, width, height);
#endif

  if (! gimp_rectangle_intersect (x, y,
                                  width, height,
                                  0, 0,
                                  gimp_item_get_width (item),
                                  gimp_item_get_height (item),
                                  &x, &y, &width, &height))
    {
      g_warning ("%s: tried to push empty region", G_STRFUNC);
      return;
    }

  GIMP_DRAWABLE_GET_CLASS (drawable)->push_undo (drawable, undo_desc,
                                                 tiles, sparse,
                                                 x, y, width, height);
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
                     0, 0, gimp_item_get_width  (item), gimp_item_get_height (item),
                     TRUE);

  if (color)
    {
      guchar tmp[MAX_CHANNELS];
      guchar c[MAX_CHANNELS];

      gimp_rgba_get_uchar (color,
                           &tmp[RED],
                           &tmp[GREEN],
                           &tmp[BLUE],
                           &tmp[ALPHA]);

      gimp_image_transform_color (image, drawable_type, c, GIMP_RGB, tmp);

      if (GIMP_IMAGE_TYPE_HAS_ALPHA (drawable_type))
        c[GIMP_IMAGE_TYPE_BYTES (drawable_type) - 1] = tmp[ALPHA];
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
                        gimp_item_get_width  (item),
                        gimp_item_get_height (item));
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
gimp_drawable_has_alpha (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return GIMP_IMAGE_TYPE_HAS_ALPHA (gimp_drawable_type (drawable));
}

GimpImageType
gimp_drawable_type (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return drawable->private->type;
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

  return GIMP_IMAGE_TYPE_BYTES (drawable->private->type);
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

const guchar *
gimp_drawable_get_colormap (const GimpDrawable *drawable)
{
  GimpImage *image;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  return image ? gimp_image_get_colormap (image) : NULL;
}

GimpLayer *
gimp_drawable_get_floating_sel (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return drawable->private->floating_selection;
}

void
gimp_drawable_attach_floating_sel (GimpDrawable *drawable,
                                   GimpLayer    *floating_sel)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (gimp_drawable_get_floating_sel (drawable) == NULL);
  g_return_if_fail (GIMP_IS_LAYER (floating_sel));

  GIMP_LOG (FLOATING_SELECTION, "%s", G_STRFUNC);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  drawable->private->floating_selection = floating_sel;
  gimp_image_set_floating_selection (image, floating_sel);

  /*  clear the selection  */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (floating_sel));

  gimp_drawable_sync_source_node (drawable, FALSE);

  g_signal_connect (floating_sel, "update",
                    G_CALLBACK (gimp_drawable_fs_update),
                    drawable);

  gimp_drawable_fs_update (floating_sel,
                           0, 0,
                           gimp_item_get_width  (GIMP_ITEM (floating_sel)),
                           gimp_item_get_height (GIMP_ITEM (floating_sel)),
                           drawable);
}

void
gimp_drawable_detach_floating_sel (GimpDrawable *drawable)
{
  GimpImage *image;
  GimpLayer *floating_sel;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_drawable_get_floating_sel (drawable) != NULL);

  GIMP_LOG (FLOATING_SELECTION, "%s", G_STRFUNC);

  image        = gimp_item_get_image (GIMP_ITEM (drawable));
  floating_sel = drawable->private->floating_selection;

  gimp_drawable_sync_source_node (drawable, TRUE);

  g_signal_handlers_disconnect_by_func (floating_sel,
                                        gimp_drawable_fs_update,
                                        drawable);

  gimp_drawable_fs_update (floating_sel,
                           0, 0,
                           gimp_item_get_width  (GIMP_ITEM (floating_sel)),
                           gimp_item_get_height (GIMP_ITEM (floating_sel)),
                           drawable);

  /*  clear the selection  */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (floating_sel));

  gimp_image_set_floating_selection (image, NULL);
  drawable->private->floating_selection = NULL;
}
