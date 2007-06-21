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

#include <string.h>

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/boundary.h"
#include "base/gimplut.h"
#include "base/lut-funcs.h"
#include "base/pixel-processor.h"
#include "base/pixel-region.h"
#include "base/tile.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "paint/gimppaintcore-stroke.h"
#include "paint/gimppaintoptions.h"

#include "gimp.h"
#include "gimp-utils.h"
#include "gimpcontainer.h"
#include "gimpdrawable-convert.h"
#include "gimpimage.h"
#include "gimpimage-quick-mask.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpdrawable-stroke.h"
#include "gimpmarshal.h"
#include "gimppaintinfo.h"
#include "gimppickable.h"
#include "gimpstrokedesc.h"

#include "gimp-intl.h"


enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};


static void gimp_channel_pickable_iface_init (GimpPickableInterface *iface);

static void       gimp_channel_finalize      (GObject          *object);

static gint64     gimp_channel_get_memsize   (GimpObject       *object,
                                              gint64           *gui_size);

static gchar  * gimp_channel_get_description (GimpViewable     *viewable,
                                              gchar           **tooltip);

static gboolean   gimp_channel_is_attached   (GimpItem         *item);
static GimpItem * gimp_channel_duplicate     (GimpItem         *item,
                                              GType             new_type,
                                              gboolean          add_alpha);
static void       gimp_channel_convert       (GimpItem         *item,
                                              GimpImage        *dest_image);
static void       gimp_channel_translate     (GimpItem         *item,
                                              gint              off_x,
                                              gint              off_y,
                                              gboolean          push_undo);
static void       gimp_channel_scale         (GimpItem         *item,
                                              gint              new_width,
                                              gint              new_height,
                                              gint              new_offset_x,
                                              gint              new_offset_y,
                                              GimpInterpolationType interp_type,
                                              GimpProgress     *progress);
static void       gimp_channel_resize        (GimpItem         *item,
                                              GimpContext      *context,
                                              gint              new_width,
                                              gint              new_height,
                                              gint              offx,
                                              gint              offy);
static void       gimp_channel_flip          (GimpItem         *item,
                                              GimpContext      *context,
                                              GimpOrientationType flip_type,
                                              gdouble           axis,
                                              gboolean          flip_result);
static void       gimp_channel_rotate        (GimpItem         *item,
                                              GimpContext      *context,
                                              GimpRotationType  flip_type,
                                              gdouble           center_x,
                                              gdouble           center_y,
                                              gboolean          flip_result);
static void       gimp_channel_transform     (GimpItem         *item,
                                              GimpContext      *context,
                                              const GimpMatrix3 *matrix,
                                              GimpTransformDirection direction,
                                              GimpInterpolationType interpolation_type,
                                              gboolean          supersample,
                                              gint              recursion_level,
                                              GimpTransformResize clip_result,
                                              GimpProgress     *progress);
static gboolean   gimp_channel_stroke        (GimpItem         *item,
                                              GimpDrawable     *drawable,
                                              GimpStrokeDesc   *stroke_desc,
                                              GimpProgress     *progress);

static void gimp_channel_invalidate_boundary   (GimpDrawable       *drawable);
static void gimp_channel_get_active_components (const GimpDrawable *drawable,
                                                gboolean           *active);

static void      gimp_channel_apply_region   (GimpDrawable     *drawable,
                                              PixelRegion      *src2PR,
                                              gboolean          push_undo,
                                              const gchar      *undo_desc,
                                              gdouble           opacity,
                                              GimpLayerModeEffects  mode,
                                              TileManager      *src1_tiles,
                                              gint              x,
                                              gint              y);
static void      gimp_channel_replace_region (GimpDrawable     *drawable,
                                              PixelRegion      *src2PR,
                                              gboolean          push_undo,
                                              const gchar      *undo_desc,
                                              gdouble           opacity,
                                              PixelRegion      *maskPR,
                                              gint              x,
                                              gint              y);
static void      gimp_channel_set_tiles      (GimpDrawable     *drawable,
                                              gboolean          push_undo,
                                              const gchar      *undo_desc,
                                              TileManager      *tiles,
                                              GimpImageType     type,
                                              gint              offset_x,
                                              gint              offset_y);
static void      gimp_channel_swap_pixels    (GimpDrawable     *drawable,
                                              TileManager      *tiles,
                                              gboolean          sparse,
                                              gint              x,
                                              gint              y,
                                              gint              width,
                                              gint              height);

static gint      gimp_channel_get_opacity_at (GimpPickable     *pickable,
                                              gint              x,
                                              gint              y);

static gboolean   gimp_channel_real_boundary (GimpChannel      *channel,
                                              const BoundSeg  **segs_in,
                                              const BoundSeg  **segs_out,
                                              gint             *num_segs_in,
                                              gint             *num_segs_out,
                                              gint              x1,
                                              gint              y1,
                                              gint              x2,
                                              gint              y2);
static gboolean   gimp_channel_real_bounds   (GimpChannel      *channel,
                                              gint             *x1,
                                              gint             *y1,
                                              gint             *x2,
                                              gint             *y2);
static gboolean   gimp_channel_real_is_empty (GimpChannel      *channel);
static void       gimp_channel_real_feather  (GimpChannel      *channel,
                                              gdouble           radius_x,
                                              gdouble           radius_y,
                                              gboolean          push_undo);
static void       gimp_channel_real_sharpen  (GimpChannel      *channel,
                                              gboolean          push_undo);
static void       gimp_channel_real_clear    (GimpChannel      *channel,
                                              const gchar      *undo_desc,
                                              gboolean          push_undo);
static void       gimp_channel_real_all      (GimpChannel      *channel,
                                              gboolean          push_undo);
static void       gimp_channel_real_invert   (GimpChannel      *channel,
                                              gboolean          push_undo);
static void       gimp_channel_real_border   (GimpChannel      *channel,
                                              gint              radius_x,
                                              gint              radius_y,
                                              gboolean          feather,
                                              gboolean          edge_lock,
                                              gboolean          push_undo);
static void       gimp_channel_real_grow     (GimpChannel      *channel,
                                              gint              radius_x,
                                              gint              radius_y,
                                              gboolean          push_undo);
static void       gimp_channel_real_shrink   (GimpChannel      *channel,
                                              gint              radius_x,
                                              gint              radius_y,
                                              gboolean          edge_lock,
                                              gboolean          push_undo);

static void       gimp_channel_validate_tile (TileManager      *tm,
                                              Tile             *tile);


G_DEFINE_TYPE_WITH_CODE (GimpChannel, gimp_channel, GIMP_TYPE_DRAWABLE,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PICKABLE,
                                                gimp_channel_pickable_iface_init))

#define parent_class gimp_channel_parent_class

static guint channel_signals[LAST_SIGNAL] = { 0 };


static void
gimp_channel_class_init (GimpChannelClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class        = GIMP_ITEM_CLASS (klass);
  GimpDrawableClass *drawable_class    = GIMP_DRAWABLE_CLASS (klass);

  channel_signals[COLOR_CHANGED] =
    g_signal_new ("color-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpChannelClass, color_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->finalize           = gimp_channel_finalize;

  gimp_object_class->get_memsize   = gimp_channel_get_memsize;

  viewable_class->get_description  = gimp_channel_get_description;
  viewable_class->default_stock_id = "gimp-channel";

  item_class->is_attached    = gimp_channel_is_attached;
  item_class->duplicate      = gimp_channel_duplicate;
  item_class->convert        = gimp_channel_convert;
  item_class->translate      = gimp_channel_translate;
  item_class->scale          = gimp_channel_scale;
  item_class->resize         = gimp_channel_resize;
  item_class->flip           = gimp_channel_flip;
  item_class->rotate         = gimp_channel_rotate;
  item_class->transform      = gimp_channel_transform;
  item_class->stroke         = gimp_channel_stroke;
  item_class->default_name   = _("Channel");
  item_class->rename_desc    = _("Rename Channel");
  item_class->translate_desc = _("Move Channel");
  item_class->scale_desc     = _("Scale Channel");
  item_class->resize_desc    = _("Resize Channel");
  item_class->flip_desc      = _("Flip Channel");
  item_class->rotate_desc    = _("Rotate Channel");
  item_class->transform_desc = _("Transform Channel");
  item_class->stroke_desc    = _("Stroke Channel");

  drawable_class->invalidate_boundary   = gimp_channel_invalidate_boundary;
  drawable_class->get_active_components = gimp_channel_get_active_components;
  drawable_class->apply_region          = gimp_channel_apply_region;
  drawable_class->replace_region        = gimp_channel_replace_region;
  drawable_class->set_tiles             = gimp_channel_set_tiles;
  drawable_class->swap_pixels           = gimp_channel_swap_pixels;

  klass->boundary       = gimp_channel_real_boundary;
  klass->bounds         = gimp_channel_real_bounds;
  klass->is_empty       = gimp_channel_real_is_empty;
  klass->feather        = gimp_channel_real_feather;
  klass->sharpen        = gimp_channel_real_sharpen;
  klass->clear          = gimp_channel_real_clear;
  klass->all            = gimp_channel_real_all;
  klass->invert         = gimp_channel_real_invert;
  klass->border         = gimp_channel_real_border;
  klass->grow           = gimp_channel_real_grow;
  klass->shrink         = gimp_channel_real_shrink;

  klass->feather_desc   = _("Feather Channel");
  klass->sharpen_desc   = _("Sharpen Channel");
  klass->clear_desc     = _("Clear Channel");
  klass->all_desc       = _("Fill Channel");
  klass->invert_desc    = _("Invert Channel");
  klass->border_desc    = _("Border Channel");
  klass->grow_desc      = _("Grow Channel");
  klass->shrink_desc    = _("Shrink Channel");
}

static void
gimp_channel_init (GimpChannel *channel)
{
  gimp_rgba_set (&channel->color, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);

  channel->show_masked = FALSE;

  /*  Selection mask variables  */
  channel->boundary_known = FALSE;
  channel->segs_in        = NULL;
  channel->segs_out       = NULL;
  channel->num_segs_in    = 0;
  channel->num_segs_out   = 0;
  channel->empty          = FALSE;
  channel->bounds_known   = FALSE;
  channel->x1             = 0;
  channel->y1             = 0;
  channel->x2             = 0;
  channel->y2             = 0;
}

static void
gimp_channel_pickable_iface_init (GimpPickableInterface *iface)
{
  iface->get_opacity_at = gimp_channel_get_opacity_at;
}

static void
gimp_channel_finalize (GObject *object)
{
  GimpChannel *channel = GIMP_CHANNEL (object);

  if (channel->segs_in)
    {
      g_free (channel->segs_in);
      channel->segs_in = NULL;
    }

  if (channel->segs_out)
    {
      g_free (channel->segs_out);
      channel->segs_out = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_channel_get_memsize (GimpObject *object,
                          gint64     *gui_size)
{
  GimpChannel *channel = GIMP_CHANNEL (object);

  *gui_size += channel->num_segs_in  * sizeof (BoundSeg);
  *gui_size += channel->num_segs_out * sizeof (BoundSeg);

  return GIMP_OBJECT_CLASS (parent_class)->get_memsize (object, gui_size);
}

static gchar *
gimp_channel_get_description (GimpViewable  *viewable,
                              gchar        **tooltip)
{
  if (! strcmp (GIMP_IMAGE_QUICK_MASK_NAME,
                gimp_object_get_name (GIMP_OBJECT (viewable))))
    {
      return g_strdup (_("Quick Mask"));
    }

  return GIMP_VIEWABLE_CLASS (parent_class)->get_description (viewable,
                                                              tooltip);
}

static gboolean
gimp_channel_is_attached (GimpItem *item)
{
  return (GIMP_IS_IMAGE (item->image) &&
          gimp_container_have (item->image->channels, GIMP_OBJECT (item)));
}

static GimpItem *
gimp_channel_duplicate (GimpItem *item,
                        GType     new_type,
                        gboolean  add_alpha)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  if (g_type_is_a (new_type, GIMP_TYPE_CHANNEL))
    add_alpha = FALSE;

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type,
                                                        add_alpha);

  if (GIMP_IS_CHANNEL (new_item))
    {
      GimpChannel *channel     = GIMP_CHANNEL (item);
      GimpChannel *new_channel = GIMP_CHANNEL (new_item);

      new_channel->color        = channel->color;
      new_channel->show_masked  = channel->show_masked;

      /*  selection mask variables  */
      new_channel->bounds_known = channel->bounds_known;
      new_channel->empty        = channel->empty;
      new_channel->x1           = channel->x1;
      new_channel->y1           = channel->y1;
      new_channel->x2           = channel->x2;
      new_channel->y2           = channel->y2;
    }

  return new_item;
}

static void
gimp_channel_convert (GimpItem  *item,
                      GimpImage *dest_image)
{
  GimpChannel       *channel  = GIMP_CHANNEL (item);
  GimpDrawable      *drawable = GIMP_DRAWABLE (item);
  GimpImageBaseType  old_base_type;

  old_base_type = GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (drawable));

  if (old_base_type != GIMP_GRAY)
    {
      TileManager   *new_tiles;
      GimpImageType  new_type = GIMP_GRAY_IMAGE;

      if (gimp_drawable_has_alpha (drawable))
        new_type = GIMP_IMAGE_TYPE_WITH_ALPHA (new_type);

      new_tiles = tile_manager_new (gimp_item_width (item),
                                    gimp_item_height (item),
                                    GIMP_IMAGE_TYPE_BYTES (new_type));

      gimp_drawable_convert_grayscale (drawable, new_tiles, old_base_type);

      gimp_drawable_set_tiles_full (drawable, FALSE, NULL,
                                    new_tiles, new_type,
                                    item->offset_x,
                                    item->offset_y);
      tile_manager_unref (new_tiles);
    }

  if (gimp_drawable_has_alpha (drawable))
    {
      TileManager *new_tiles;
      PixelRegion  srcPR;
      PixelRegion  destPR;
      guchar       bg[1] = { 0 };

      new_tiles = tile_manager_new (gimp_item_width (item),
                                    gimp_item_height (item),
                                    GIMP_IMAGE_TYPE_BYTES (GIMP_GRAY_IMAGE));

      pixel_region_init (&srcPR, drawable->tiles,
                         0, 0,
                         gimp_item_width (item),
                         gimp_item_height (item),
                         FALSE);
      pixel_region_init (&destPR, new_tiles,
                         0, 0,
                         gimp_item_width (item),
                         gimp_item_height (item),
                         TRUE);

      flatten_region (&srcPR, &destPR, bg);

      gimp_drawable_set_tiles_full (drawable, FALSE, NULL,
                                    new_tiles, GIMP_GRAY_IMAGE,
                                    item->offset_x,
                                    item->offset_y);
      tile_manager_unref (new_tiles);
    }

  if (G_TYPE_FROM_INSTANCE (channel) == GIMP_TYPE_CHANNEL)
    {
      gint width  = gimp_image_get_width  (dest_image);
      gint height = gimp_image_get_height (dest_image);

      item->offset_x = 0;
      item->offset_y = 0;

      if (gimp_item_width  (item) != width ||
          gimp_item_height (item) != height)
        {
          gimp_item_resize (item, gimp_get_user_context (dest_image->gimp),
                            width, height, 0, 0);
        }
    }

  GIMP_ITEM_CLASS (parent_class)->convert (item, dest_image);
}

static void
gimp_channel_translate (GimpItem *item,
                        gint      off_x,
                        gint      off_y,
                        gboolean  push_undo)
{
  GimpChannel *channel  = GIMP_CHANNEL (item);
  GimpChannel *tmp_mask = NULL;
  gint         width, height;
  PixelRegion  srcPR, destPR;
  guchar       empty = TRANSPARENT_OPACITY;
  gint         x1, y1, x2, y2;

  gimp_channel_bounds (channel, &x1, &y1, &x2, &y2);

  /*  update the old area  */
  gimp_drawable_update (GIMP_DRAWABLE (item), x1, y1, x2 - x1, y2 - y1);

  if (push_undo)
    gimp_channel_push_undo (channel, NULL);
  else
    gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (channel));

  x1 = CLAMP ((x1 + off_x), 0, GIMP_ITEM (channel)->width);
  y1 = CLAMP ((y1 + off_y), 0, GIMP_ITEM (channel)->height);
  x2 = CLAMP ((x2 + off_x), 0, GIMP_ITEM (channel)->width);
  y2 = CLAMP ((y2 + off_y), 0, GIMP_ITEM (channel)->height);

  width  = x2 - x1;
  height = y2 - y1;

  /*  make sure width and height are non-zero  */
  if (width != 0 && height != 0)
    {
      /*  copy the portion of the mask we will keep to a
       *  temporary buffer
       */
      tmp_mask = gimp_channel_new_mask (gimp_item_get_image (item),
                                        width, height);

      pixel_region_init (&srcPR, GIMP_DRAWABLE (channel)->tiles,
                         x1 - off_x, y1 - off_y, width, height, FALSE);
      pixel_region_init (&destPR, GIMP_DRAWABLE (tmp_mask)->tiles,
                         0, 0, width, height, TRUE);
      copy_region (&srcPR, &destPR);
    }

  /*  clear the mask  */
  pixel_region_init (&srcPR, GIMP_DRAWABLE (channel)->tiles,
                     0, 0,
                     GIMP_ITEM (channel)->width,
                     GIMP_ITEM (channel)->height, TRUE);
  color_region (&srcPR, &empty);

  if (width != 0 && height != 0)
    {
      /*  copy the temp mask back to the mask  */
      pixel_region_init (&srcPR, GIMP_DRAWABLE (tmp_mask)->tiles,
                         0, 0, width, height, FALSE);
      pixel_region_init (&destPR, GIMP_DRAWABLE (channel)->tiles,
                         x1, y1, width, height, TRUE);
      copy_region (&srcPR, &destPR);

      /*  free the temporary mask  */
      g_object_unref (tmp_mask);
    }

  /*  calculate new bounds  */
  if (width == 0 || height == 0)
    {
      channel->empty = TRUE;
      channel->x1    = 0;
      channel->y1    = 0;
      channel->x2    = GIMP_ITEM (channel)->width;
      channel->y2    = GIMP_ITEM (channel)->height;
    }
  else
    {
      channel->x1 = x1;
      channel->y1 = y1;
      channel->x2 = x2;
      channel->y2 = y2;
    }

  /*  update the new area  */
  gimp_drawable_update (GIMP_DRAWABLE (item),
                        channel->x1, channel->y1,
                        channel->x2 - channel->x1,
                        channel->y2 - channel->y1);
}

static void
gimp_channel_scale (GimpItem              *item,
                    gint                   new_width,
                    gint                   new_height,
                    gint                   new_offset_x,
                    gint                   new_offset_y,
                    GimpInterpolationType  interpolation_type,
                    GimpProgress          *progress)
{
  if (G_TYPE_FROM_INSTANCE (item) == GIMP_TYPE_CHANNEL)
    {
      new_offset_x = 0;
      new_offset_y = 0;
    }

  GIMP_ITEM_CLASS (parent_class)->scale (item, new_width, new_height,
                                         new_offset_x, new_offset_y,
                                         interpolation_type, progress);
}

static void
gimp_channel_resize (GimpItem    *item,
                     GimpContext *context,
                     gint         new_width,
                     gint         new_height,
                     gint         offset_x,
                     gint         offset_y)
{
  GIMP_ITEM_CLASS (parent_class)->resize (item, context, new_width, new_height,
                                          offset_x, offset_y);

  if (G_TYPE_FROM_INSTANCE (item) == GIMP_TYPE_CHANNEL)
    {
      item->offset_x = 0;
      item->offset_y = 0;
    }
}

static void
gimp_channel_flip (GimpItem            *item,
                   GimpContext         *context,
                   GimpOrientationType  flip_type,
                   gdouble              axis,
                   gboolean             clip_result)
{
  if (G_TYPE_FROM_INSTANCE (item) == GIMP_TYPE_CHANNEL)
    clip_result = TRUE;

  GIMP_ITEM_CLASS (parent_class)->flip (item, context, flip_type, axis,
                                        clip_result);
}

static void
gimp_channel_rotate (GimpItem         *item,
                     GimpContext      *context,
                     GimpRotationType  rotate_type,
                     gdouble           center_x,
                     gdouble           center_y,
                     gboolean          clip_result)
{
  /*  don't default to clip_result == TRUE here  */

  GIMP_ITEM_CLASS (parent_class)->rotate (item, context,
                                          rotate_type, center_x, center_y,
                                          clip_result);
}

static void
gimp_channel_transform (GimpItem               *item,
                        GimpContext            *context,
                        const GimpMatrix3      *matrix,
                        GimpTransformDirection  direction,
                        GimpInterpolationType   interpolation_type,
                        gboolean                supersample,
                        gint                    recursion_level,
                        GimpTransformResize     clip_result,
                        GimpProgress           *progress)
{
  if (G_TYPE_FROM_INSTANCE (item) == GIMP_TYPE_CHANNEL)
    clip_result = TRUE;

  GIMP_ITEM_CLASS (parent_class)->transform (item, context, matrix, direction,
                                             interpolation_type,
                                             supersample, recursion_level,
                                             clip_result, progress);
}

static gboolean
gimp_channel_stroke (GimpItem       *item,
                     GimpDrawable   *drawable,
                     GimpStrokeDesc *stroke_desc,
                     GimpProgress   *progress)

{
  GimpChannel    *channel = GIMP_CHANNEL (item);
  const BoundSeg *segs_in;
  const BoundSeg *segs_out;
  gint            n_segs_in;
  gint            n_segs_out;
  gboolean        retval = FALSE;
  gint            offset_x, offset_y;

  if (! gimp_channel_boundary (channel, &segs_in, &segs_out,
                               &n_segs_in, &n_segs_out,
                               0, 0, 0, 0))
    {
      gimp_message (gimp_item_get_image (item)->gimp, G_OBJECT (progress),
                    GIMP_MESSAGE_WARNING,
                    _("Cannot stroke empty channel."));
      return FALSE;
    }

  gimp_item_offsets (GIMP_ITEM (channel), &offset_x, &offset_y);

  switch (stroke_desc->method)
    {
    case GIMP_STROKE_METHOD_LIBART:
      gimp_drawable_stroke_boundary (drawable,
                                     stroke_desc->stroke_options,
                                     segs_in, n_segs_in,
                                     offset_x, offset_y);
      retval = TRUE;
      break;

    case GIMP_STROKE_METHOD_PAINT_CORE:
      {
        GimpPaintCore *core;

        core = g_object_new (stroke_desc->paint_info->paint_type, NULL);

        retval = gimp_paint_core_stroke_boundary (core, drawable,
                                                  stroke_desc->paint_options,
                                                  segs_in, n_segs_in,
                                                  offset_x, offset_y);

        g_object_unref (core);
      }
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  return retval;
}

static void
gimp_channel_invalidate_boundary (GimpDrawable *drawable)
{
  GIMP_CHANNEL (drawable)->boundary_known = FALSE;
}

static void
gimp_channel_get_active_components (const GimpDrawable *drawable,
                                    gboolean           *active)
{
  /*  Make sure that the alpha channel is not valid.  */
  active[GRAY_PIX]    = TRUE;
  active[ALPHA_G_PIX] = FALSE;
}

static void
gimp_channel_apply_region (GimpDrawable         *drawable,
                           PixelRegion          *src2PR,
                           gboolean              push_undo,
                           const gchar          *undo_desc,
                           gdouble               opacity,
                           GimpLayerModeEffects  mode,
                           TileManager          *src1_tiles,
                           gint                  x,
                           gint                  y)
{
  gimp_drawable_invalidate_boundary (drawable);

  GIMP_DRAWABLE_CLASS (parent_class)->apply_region (drawable, src2PR,
                                                    push_undo, undo_desc,
                                                    opacity, mode,
                                                    src1_tiles,
                                                    x, y);

  GIMP_CHANNEL (drawable)->bounds_known = FALSE;
}

static void
gimp_channel_replace_region (GimpDrawable *drawable,
                             PixelRegion  *src2PR,
                             gboolean      push_undo,
                             const gchar  *undo_desc,
                             gdouble       opacity,
                             PixelRegion  *maskPR,
                             gint          x,
                             gint          y)
{
  gimp_drawable_invalidate_boundary (drawable);

  GIMP_DRAWABLE_CLASS (parent_class)->replace_region (drawable, src2PR,
                                                      push_undo, undo_desc,
                                                      opacity,
                                                      maskPR,
                                                      x, y);

  GIMP_CHANNEL (drawable)->bounds_known = FALSE;
}

static void
gimp_channel_set_tiles (GimpDrawable *drawable,
                        gboolean      push_undo,
                        const gchar  *undo_desc,
                        TileManager  *tiles,
                        GimpImageType type,
                        gint          offset_x,
                        gint          offset_y)
{
  GIMP_DRAWABLE_CLASS (parent_class)->set_tiles (drawable,
                                                 push_undo, undo_desc,
                                                 tiles, type,
                                                 offset_x, offset_y);

  GIMP_CHANNEL (drawable)->bounds_known = FALSE;
}

static void
gimp_channel_swap_pixels (GimpDrawable *drawable,
                          TileManager  *tiles,
                          gboolean      sparse,
                          gint          x,
                          gint          y,
                          gint          width,
                          gint          height)
{
  gimp_drawable_invalidate_boundary (drawable);

  GIMP_DRAWABLE_CLASS (parent_class)->swap_pixels (drawable, tiles, sparse,
                                                   x, y, width, height);

  GIMP_CHANNEL (drawable)->bounds_known = FALSE;
}

static gint
gimp_channel_get_opacity_at (GimpPickable *pickable,
                             gint          x,
                             gint          y)
{
  GimpChannel *channel = GIMP_CHANNEL (pickable);
  Tile        *tile;
  gint         val;

  /*  Some checks to cut back on unnecessary work  */
  if (channel->bounds_known)
    {
      if (channel->empty)
        return 0;
      else if (x < channel->x1 || x >= channel->x2 ||
               y < channel->y1 || y >= channel->y2)
        return 0;
    }
  else
    {
      if (x < 0 || x >= GIMP_ITEM (channel)->width ||
          y < 0 || y >= GIMP_ITEM (channel)->height)
        return 0;
    }

  tile = tile_manager_get_tile (GIMP_DRAWABLE (channel)->tiles, x, y,
                                TRUE, FALSE);
  val = *(guchar *) (tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT));
  tile_release (tile, FALSE);

  return val;
}

static gboolean
gimp_channel_real_boundary (GimpChannel     *channel,
                            const BoundSeg **segs_in,
                            const BoundSeg **segs_out,
                            gint            *num_segs_in,
                            gint            *num_segs_out,
                            gint             x1,
                            gint             y1,
                            gint             x2,
                            gint             y2)
{
  gint         x3, y3, x4, y4;
  PixelRegion  bPR;

  if (! channel->boundary_known)
    {
      /* free the out of date boundary segments */
      g_free (channel->segs_in);
      g_free (channel->segs_out);

      if (gimp_channel_bounds (channel, &x3, &y3, &x4, &y4))
        {
          pixel_region_init (&bPR, GIMP_DRAWABLE (channel)->tiles,
                             x3, y3, x4 - x3, y4 - y3, FALSE);

          channel->segs_out = boundary_find (&bPR, BOUNDARY_IGNORE_BOUNDS,
                                             x1, y1, x2, y2,
                                             BOUNDARY_HALF_WAY,
                                             &channel->num_segs_out);
          x1 = MAX (x1, x3);
          y1 = MAX (y1, y3);
          x2 = MIN (x2, x4);
          y2 = MIN (y2, y4);

          if (x2 > x1 && y2 > y1)
            {
              pixel_region_init (&bPR, GIMP_DRAWABLE (channel)->tiles,
                                 0, 0,
                                 GIMP_ITEM (channel)->width,
                                 GIMP_ITEM (channel)->height, FALSE);

              channel->segs_in = boundary_find (&bPR, BOUNDARY_WITHIN_BOUNDS,
                                                x1, y1, x2, y2,
                                                BOUNDARY_HALF_WAY,
                                                &channel->num_segs_in);
            }
          else
            {
              channel->segs_in     = NULL;
              channel->num_segs_in = 0;
            }
        }
      else
        {
          channel->segs_in      = NULL;
          channel->segs_out     = NULL;
          channel->num_segs_in  = 0;
          channel->num_segs_out = 0;
        }

      channel->boundary_known = TRUE;
    }

  *segs_in      = channel->segs_in;
  *segs_out     = channel->segs_out;
  *num_segs_in  = channel->num_segs_in;
  *num_segs_out = channel->num_segs_out;

  return (! channel->empty);
}

static gboolean
gimp_channel_real_bounds (GimpChannel *channel,
                          gint        *x1,
                          gint        *y1,
                          gint        *x2,
                          gint        *y2)
{
  PixelRegion  maskPR;
  guchar      *data, *data1;
  gint         x, y;
  gint         ex, ey;
  gint         tx1, tx2, ty1, ty2;
  gint         minx, maxx;
  gpointer     pr;

  /*  if the channel's bounds have already been reliably calculated...  */
  if (channel->bounds_known)
    {
      *x1 = channel->x1;
      *y1 = channel->y1;
      *x2 = channel->x2;
      *y2 = channel->y2;

      return ! channel->empty;
    }

  /*  go through and calculate the bounds  */
  tx1 = GIMP_ITEM (channel)->width;
  ty1 = GIMP_ITEM (channel)->height;
  tx2 = 0;
  ty2 = 0;

  pixel_region_init (&maskPR, GIMP_DRAWABLE (channel)->tiles,
                     0, 0,
                     GIMP_ITEM (channel)->width,
                     GIMP_ITEM (channel)->height, FALSE);

  for (pr = pixel_regions_register (1, &maskPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      data1 = data = maskPR.data;
      ex = maskPR.x + maskPR.w;
      ey = maskPR.y + maskPR.h;

      /*  only check the pixels if this tile is not fully within the
       *  currently computed bounds
       */
      if (maskPR.x < tx1 || ex > tx2 ||
          maskPR.y < ty1 || ey > ty2)
        {
          /* Check upper left and lower right corners to see if we can
           * avoid checking the rest of the pixels in this tile
           */
          if (data[0] && data[maskPR.rowstride*(maskPR.h - 1) + maskPR.w - 1])
            {
              if (maskPR.x < tx1)
                tx1 = maskPR.x;
              if (ex > tx2)
                tx2 = ex;
              if (maskPR.y < ty1)
                ty1 = maskPR.y;
              if (ey > ty2)
                ty2 = ey;
            }
          else
            {
              for (y = maskPR.y; y < ey; y++, data1 += maskPR.rowstride)
                {
                  for (x = maskPR.x, data = data1; x < ex; x++, data++)
                    {
                      if (*data)
                        {
                          minx = x;
                          maxx = x;

                          for (; x < ex; x++, data++)
                            if (*data)
                              maxx = x;

                          if (minx < tx1)
                            tx1 = minx;
                          if (maxx > tx2)
                            tx2 = maxx;
                          if (y < ty1)
                            ty1 = y;
                          if (y > ty2)
                            ty2 = y;
                        }
                    }
                }
            }
        }
    }

  tx2 = CLAMP (tx2 + 1, 0, GIMP_ITEM (channel)->width);
  ty2 = CLAMP (ty2 + 1, 0, GIMP_ITEM (channel)->height);

  if (tx1 == GIMP_ITEM (channel)->width && ty1 == GIMP_ITEM (channel)->height)
    {
      channel->empty = TRUE;
      channel->x1    = 0;
      channel->y1    = 0;
      channel->x2    = GIMP_ITEM (channel)->width;
      channel->y2    = GIMP_ITEM (channel)->height;
    }
  else
    {
      channel->empty = FALSE;
      channel->x1    = tx1;
      channel->y1    = ty1;
      channel->x2    = tx2;
      channel->y2    = ty2;
    }

  channel->bounds_known = TRUE;

  *x1 = channel->x1;
  *x2 = channel->x2;
  *y1 = channel->y1;
  *y2 = channel->y2;

  return ! channel->empty;
}

static gboolean
gimp_channel_real_is_empty (GimpChannel *channel)
{
  PixelRegion  maskPR;
  guchar      *data;
  gint         x, y;
  gpointer     pr;

  if (channel->bounds_known)
    return channel->empty;

  pixel_region_init (&maskPR, GIMP_DRAWABLE (channel)->tiles,
                     0, 0,
                     GIMP_ITEM (channel)->width,
                     GIMP_ITEM (channel)->height, FALSE);

  for (pr = pixel_regions_register (1, &maskPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      /*  check if any pixel in the channel is non-zero  */
      data = maskPR.data;

      for (y = 0; y < maskPR.h; y++)
        for (x = 0; x < maskPR.w; x++)
          if (*data++)
            {
              pixel_regions_process_stop (pr);
              return FALSE;
            }
    }

  /*  The mask is empty, meaning we can set the bounds as known  */
  if (channel->segs_in)
    g_free (channel->segs_in);
  if (channel->segs_out)
    g_free (channel->segs_out);

  channel->empty          = TRUE;
  channel->segs_in        = NULL;
  channel->segs_out       = NULL;
  channel->num_segs_in    = 0;
  channel->num_segs_out   = 0;
  channel->bounds_known   = TRUE;
  channel->boundary_known = TRUE;
  channel->x1             = 0;
  channel->y1             = 0;
  channel->x2             = GIMP_ITEM (channel)->width;
  channel->y2             = GIMP_ITEM (channel)->height;

  return TRUE;
}

static void
gimp_channel_real_feather (GimpChannel *channel,
                           gdouble      radius_x,
                           gdouble      radius_y,
                           gboolean     push_undo)
{
  PixelRegion srcPR;

  if (push_undo)
    gimp_channel_push_undo (channel,
                            GIMP_CHANNEL_GET_CLASS (channel)->feather_desc);
  else
    gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (channel));

  pixel_region_init (&srcPR, GIMP_DRAWABLE (channel)->tiles,
                     0, 0,
                     gimp_item_width  (GIMP_ITEM (channel)),
                     gimp_item_height (GIMP_ITEM (channel)),
                     TRUE);
  gaussian_blur_region (&srcPR, radius_x, radius_y);

  channel->bounds_known = FALSE;

  gimp_drawable_update (GIMP_DRAWABLE (channel), 0, 0,
                        gimp_item_width  (GIMP_ITEM (channel)),
                        gimp_item_height (GIMP_ITEM (channel)));
}

static void
gimp_channel_real_sharpen (GimpChannel *channel,
                           gboolean     push_undo)
{
  PixelRegion  maskPR;
  GimpLut     *lut;

  if (push_undo)
    gimp_channel_push_undo (channel,
                            GIMP_CHANNEL_GET_CLASS (channel)->sharpen_desc);
  else
    gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (channel));

  pixel_region_init (&maskPR, GIMP_DRAWABLE (channel)->tiles,
                     0, 0,
                     gimp_item_width  (GIMP_ITEM (channel)),
                     gimp_item_height (GIMP_ITEM (channel)),
                     TRUE);
  lut = threshold_lut_new (0.5, 1);

  pixel_regions_process_parallel ((PixelProcessorFunc) gimp_lut_process_inline,
                                  lut, 1, &maskPR);
  gimp_lut_free (lut);

  channel->bounds_known = FALSE;

  gimp_drawable_update (GIMP_DRAWABLE (channel), 0, 0,
                        gimp_item_width  (GIMP_ITEM (channel)),
                        gimp_item_height (GIMP_ITEM (channel)));
}

static void
gimp_channel_real_clear (GimpChannel *channel,
                         const gchar *undo_desc,
                         gboolean     push_undo)
{
  PixelRegion maskPR;
  guchar      bg = TRANSPARENT_OPACITY;

  if (push_undo)
    {
      if (! undo_desc)
        undo_desc = GIMP_CHANNEL_GET_CLASS (channel)->clear_desc;

      gimp_channel_push_undo (channel, undo_desc);
    }
  else
    {
      gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (channel));
    }

  if (channel->bounds_known && !channel->empty)
    {
      pixel_region_init (&maskPR, GIMP_DRAWABLE (channel)->tiles,
                         channel->x1, channel->y1,
                         (channel->x2 - channel->x1),
                         (channel->y2 - channel->y1), TRUE);
      color_region (&maskPR, &bg);
    }
  else
    {
      /*  clear the mask  */
      pixel_region_init (&maskPR, GIMP_DRAWABLE (channel)->tiles,
                         0, 0,
                         GIMP_ITEM (channel)->width,
                         GIMP_ITEM (channel)->height, TRUE);
      color_region (&maskPR, &bg);
    }

  /*  we know the bounds  */
  channel->bounds_known = TRUE;
  channel->empty        = TRUE;
  channel->x1           = 0;
  channel->y1           = 0;
  channel->x2           = GIMP_ITEM (channel)->width;
  channel->y2           = GIMP_ITEM (channel)->height;

  gimp_drawable_update (GIMP_DRAWABLE (channel), 0, 0,
                        gimp_item_width  (GIMP_ITEM (channel)),
                        gimp_item_height (GIMP_ITEM (channel)));
}

static void
gimp_channel_real_all (GimpChannel *channel,
                       gboolean     push_undo)
{
  PixelRegion maskPR;
  guchar      bg = OPAQUE_OPACITY;

  if (push_undo)
    gimp_channel_push_undo (channel,
                            GIMP_CHANNEL_GET_CLASS (channel)->all_desc);
  else
    gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (channel));

  /*  clear the channel  */
  pixel_region_init (&maskPR, GIMP_DRAWABLE (channel)->tiles,
                     0, 0,
                     GIMP_ITEM (channel)->width,
                     GIMP_ITEM (channel)->height, TRUE);
  color_region (&maskPR, &bg);

  /*  we know the bounds  */
  channel->bounds_known = TRUE;
  channel->empty        = FALSE;
  channel->x1           = 0;
  channel->y1           = 0;
  channel->x2           = GIMP_ITEM (channel)->width;
  channel->y2           = GIMP_ITEM (channel)->height;

  gimp_drawable_update (GIMP_DRAWABLE (channel), 0, 0,
                        gimp_item_width  (GIMP_ITEM (channel)),
                        gimp_item_height (GIMP_ITEM (channel)));
}

static void
gimp_channel_real_invert (GimpChannel *channel,
                          gboolean     push_undo)
{
  if (push_undo)
    gimp_channel_push_undo (channel,
                            GIMP_CHANNEL_GET_CLASS (channel)->invert_desc);
  else
    gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (channel));

  if (channel->bounds_known && channel->empty)
    {
      gimp_channel_all (channel, FALSE);
    }
  else
    {
      PixelRegion  maskPR;
      GimpLut     *lut;

      pixel_region_init (&maskPR, GIMP_DRAWABLE (channel)->tiles,
                         0, 0,
                         GIMP_ITEM (channel)->width,
                         GIMP_ITEM (channel)->height, TRUE);

      lut = invert_lut_new (1);

      pixel_regions_process_parallel ((PixelProcessorFunc)
                                      gimp_lut_process_inline,
                                      lut, 1, &maskPR);

      gimp_lut_free (lut);

      channel->bounds_known = FALSE;

      gimp_drawable_update (GIMP_DRAWABLE (channel), 0, 0,
                            gimp_item_width  (GIMP_ITEM (channel)),
                            gimp_item_height (GIMP_ITEM (channel)));
    }
}

static void
gimp_channel_real_border (GimpChannel *channel,
                          gint         radius_x,
                          gint         radius_y,
                          gboolean     feather,
                          gboolean     edge_lock,
                          gboolean     push_undo)
{
  PixelRegion bPR;
  gint        x1, y1, x2, y2;

  if (radius_x < 0 || radius_y < 0)
    return;

  if (! gimp_channel_bounds (channel, &x1, &y1, &x2, &y2))
    return;

  if (gimp_channel_is_empty (channel))
    return;

  if (x1 - radius_x < 0)
    x1 = 0;
  else
    x1 -= radius_x;

  if (x2 + radius_x > GIMP_ITEM (channel)->width)
    x2 = GIMP_ITEM (channel)->width;
  else
    x2 += radius_x;

  if (y1 - radius_y < 0)
    y1 = 0;
  else
    y1 -= radius_y;

  if (y2 + radius_y > GIMP_ITEM (channel)->height)
    y2 = GIMP_ITEM (channel)->height;
  else
    y2 += radius_y;

  if (push_undo)
    gimp_channel_push_undo (channel,
                            GIMP_CHANNEL_GET_CLASS (channel)->border_desc);
  else
    gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (channel));

  pixel_region_init (&bPR, GIMP_DRAWABLE (channel)->tiles,
                     x1, y1, x2 - x1, y2 - y1, TRUE);

  border_region (&bPR, radius_x, radius_y, feather, edge_lock);

  channel->bounds_known = FALSE;

  gimp_drawable_update (GIMP_DRAWABLE (channel), 0, 0,
                        gimp_item_width  (GIMP_ITEM (channel)),
                        gimp_item_height (GIMP_ITEM (channel)));
}

static void
gimp_channel_real_grow (GimpChannel *channel,
                        gint         radius_x,
                        gint         radius_y,
                        gboolean     push_undo)
{
  PixelRegion bPR;
  gint        x1, y1, x2, y2;

  if (radius_x == 0 && radius_y == 0)
    return;

  if (radius_x <= 0 && radius_y <= 0)
    {
      gimp_channel_shrink (channel, -radius_x, -radius_y, FALSE, push_undo);
      return;
    }

  if (radius_x < 0 || radius_y < 0)
    return;

  if (! gimp_channel_bounds (channel, &x1, &y1, &x2, &y2))
    return;

  if (gimp_channel_is_empty (channel))
    return;

  if (x1 - radius_x > 0)
    x1 = x1 - radius_x;
  else
    x1 = 0;
  if (y1 - radius_y > 0)
    y1 = y1 - radius_y;
  else
    y1 = 0;
  if (x2 + radius_x < GIMP_ITEM (channel)->width)
    x2 = x2 + radius_x;
  else
    x2 = GIMP_ITEM (channel)->width;
  if (y2 + radius_y < GIMP_ITEM (channel)->height)
    y2 = y2 + radius_y;
  else
    y2 = GIMP_ITEM (channel)->height;

  if (push_undo)
    gimp_channel_push_undo (channel,
                            GIMP_CHANNEL_GET_CLASS (channel)->grow_desc);
  else
    gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (channel));

  /*  need full extents for grow, not! */
  pixel_region_init (&bPR, GIMP_DRAWABLE (channel)->tiles,
                     x1, y1, (x2 - x1), (y2 - y1), TRUE);

  fatten_region (&bPR, radius_x, radius_y);

  channel->bounds_known = FALSE;

  gimp_drawable_update (GIMP_DRAWABLE (channel), 0, 0,
                        gimp_item_width  (GIMP_ITEM (channel)),
                        gimp_item_height (GIMP_ITEM (channel)));
}

static void
gimp_channel_real_shrink (GimpChannel *channel,
                          gint         radius_x,
                          gint         radius_y,
                          gboolean     edge_lock,
                          gboolean     push_undo)
{
  PixelRegion bPR;
  gint        x1, y1, x2, y2;

  if (radius_x == 0 && radius_y == 0)
    return;

  if (radius_x <= 0 && radius_y <= 0)
    {
      gimp_channel_grow (channel, -radius_x, -radius_y, push_undo);
      return;
    }

  if (radius_x < 0 || radius_y < 0)
    return;

  if (! gimp_channel_bounds (channel, &x1, &y1, &x2, &y2))
    return;

  if (gimp_channel_is_empty (channel))
    return;

  if (x1 > 0)
    x1--;
  if (y1 > 0)
    y1--;
  if (x2 < GIMP_ITEM (channel)->width)
    x2++;
  if (y2 < GIMP_ITEM (channel)->height)
    y2++;

  if (push_undo)
    gimp_channel_push_undo (channel,
                            GIMP_CHANNEL_GET_CLASS (channel)->shrink_desc);
  else
    gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (channel));

  pixel_region_init (&bPR, GIMP_DRAWABLE (channel)->tiles,
                     x1, y1, (x2 - x1), (y2 - y1), TRUE);

  thin_region (&bPR, radius_x, radius_y, edge_lock);

  channel->bounds_known = FALSE;

  gimp_drawable_update (GIMP_DRAWABLE (channel), 0, 0,
                        gimp_item_width  (GIMP_ITEM (channel)),
                        gimp_item_height (GIMP_ITEM (channel)));
}

static void
gimp_channel_validate_tile (TileManager *tm,
                            Tile        *tile)
{
  /*  Set the contents of the tile to empty  */
  memset (tile_data_pointer (tile, 0, 0),
          TRANSPARENT_OPACITY, tile_size (tile));
}


/*  public functions  */

GimpChannel *
gimp_channel_new (GimpImage     *image,
                  gint           width,
                  gint           height,
                  const gchar   *name,
                  const GimpRGB *color)
{
  GimpChannel *channel;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  channel = g_object_new (GIMP_TYPE_CHANNEL, NULL);

  gimp_drawable_configure (GIMP_DRAWABLE (channel),
                           image,
                           0, 0, width, height,
                           GIMP_GRAY_IMAGE, name);

  if (color)
    channel->color = *color;

  channel->show_masked = TRUE;

  /*  selection mask variables  */
  channel->x2          = width;
  channel->y2          = height;

  return channel;
}

GimpChannel *
gimp_channel_new_from_alpha (GimpImage     *image,
                             GimpDrawable  *drawable,
                             const gchar   *name,
                             const GimpRGB *color)
{
  GimpChannel *channel;
  gint         width;
  gint         height;
  PixelRegion  srcPR, destPR;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_drawable_has_alpha (drawable), NULL);

  width  = gimp_item_width  (GIMP_ITEM (drawable));
  height = gimp_item_height (GIMP_ITEM (drawable));

  channel = gimp_channel_new (image, width, height, name, color);

  gimp_channel_clear (channel, NULL, FALSE);

  pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                     0, 0, width, height, FALSE);
  pixel_region_init (&destPR, gimp_drawable_get_tiles (GIMP_DRAWABLE (channel)),
                     0, 0, width, height, TRUE);

  extract_alpha_region (&srcPR, NULL, &destPR);

  channel->bounds_known = FALSE;

  return channel;
}

GimpChannel *
gimp_channel_new_from_component (GimpImage       *image,
                                 GimpChannelType  type,
                                 const gchar     *name,
                                 const GimpRGB   *color)
{
  GimpChannel *channel;
  TileManager *projection;
  PixelRegion  src;
  PixelRegion  dest;
  gint         width;
  gint         height;
  gint         pixel;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  pixel = gimp_image_get_component_index (image, type);

  g_return_val_if_fail (pixel != -1, NULL);

  gimp_pickable_flush (GIMP_PICKABLE (image->projection));

  projection = gimp_pickable_get_tiles (GIMP_PICKABLE (image->projection));
  width  = tile_manager_width  (projection);
  height = tile_manager_height (projection);

  channel = gimp_channel_new (image, width, height, name, color);

  pixel_region_init (&src, projection,
                     0, 0, width, height, FALSE);
  pixel_region_init (&dest, gimp_drawable_get_tiles (GIMP_DRAWABLE (channel)),
                     0, 0, width, height, TRUE);

  copy_component (&src, &dest, pixel);

  return channel;
}

void
gimp_channel_set_color (GimpChannel   *channel,
                        const GimpRGB *color,
                        gboolean       push_undo)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (color != NULL);

  if (gimp_rgba_distance (&channel->color, color) > 0.0001)
    {
      if (push_undo && gimp_item_is_attached (GIMP_ITEM (channel)))
        {
          GimpImage *image = gimp_item_get_image (GIMP_ITEM (channel));

          gimp_image_undo_push_channel_color (image, _("Set Channel Color"),
                                              channel);
        }

      channel->color = *color;

      gimp_drawable_update (GIMP_DRAWABLE (channel),
                            0, 0,
                            GIMP_ITEM (channel)->width,
                            GIMP_ITEM (channel)->height);

      g_signal_emit (channel, channel_signals[COLOR_CHANGED], 0);
    }
}

void
gimp_channel_get_color (const GimpChannel *channel,
                        GimpRGB           *color)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (color != NULL);

  *color = channel->color;
}

gdouble
gimp_channel_get_opacity (const GimpChannel *channel)
{
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), GIMP_OPACITY_TRANSPARENT);

  return channel->color.a;
}

void
gimp_channel_set_opacity (GimpChannel *channel,
                          gdouble      opacity,
                          gboolean     push_undo)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  opacity = CLAMP (opacity, GIMP_OPACITY_TRANSPARENT, GIMP_OPACITY_OPAQUE);

  if (channel->color.a != opacity)
    {
      if (push_undo && gimp_item_is_attached (GIMP_ITEM (channel)))
        {
          GimpImage *image = gimp_item_get_image (GIMP_ITEM (channel));

          gimp_image_undo_push_channel_color (image, _("Set Channel Opacity"),
                                              channel);
        }

      channel->color.a = opacity;

      gimp_drawable_update (GIMP_DRAWABLE (channel),
                            0, 0,
                            GIMP_ITEM (channel)->width,
                            GIMP_ITEM (channel)->height);

      g_signal_emit (channel, channel_signals[COLOR_CHANGED], 0);
    }
}

gboolean
gimp_channel_get_show_masked (GimpChannel *channel)
{
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  return channel->show_masked;
}

void
gimp_channel_set_show_masked (GimpChannel *channel,
                              gboolean     show_masked)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  if (show_masked != channel->show_masked)
    {
      channel->show_masked = show_masked ? TRUE : FALSE;

      gimp_drawable_update (GIMP_DRAWABLE (channel),
                            0, 0,
                            GIMP_ITEM (channel)->width,
                            GIMP_ITEM (channel)->height);
    }
}

void
gimp_channel_push_undo (GimpChannel *channel,
                        const gchar *undo_desc)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)));

  gimp_image_undo_push_mask (gimp_item_get_image (GIMP_ITEM (channel)),
                             undo_desc, channel);

  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (channel));
}


/******************************/
/*  selection mask functions  */
/******************************/

GimpChannel *
gimp_channel_new_mask (GimpImage *image,
                       gint       width,
                       gint       height)
{
  GimpChannel *new_channel;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  new_channel = gimp_channel_new (image, width, height,
                                  _("Selection Mask"), NULL);

  tile_manager_set_validate_proc (GIMP_DRAWABLE (new_channel)->tiles,
                                  (TileValidateProc) gimp_channel_validate_tile,
                                  NULL);

  return new_channel;
}

gboolean
gimp_channel_boundary (GimpChannel     *channel,
                       const BoundSeg **segs_in,
                       const BoundSeg **segs_out,
                       gint            *num_segs_in,
                       gint            *num_segs_out,
                       gint             x1,
                       gint             y1,
                       gint             x2,
                       gint             y2)
{
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);
  g_return_val_if_fail (segs_in != NULL, FALSE);
  g_return_val_if_fail (segs_out != NULL, FALSE);
  g_return_val_if_fail (num_segs_in != NULL, FALSE);
  g_return_val_if_fail (num_segs_out != NULL, FALSE);

  return GIMP_CHANNEL_GET_CLASS (channel)->boundary (channel,
                                                     segs_in, segs_out,
                                                     num_segs_in, num_segs_out,
                                                     x1, y1,
                                                     x2, y2);
}

gboolean
gimp_channel_bounds (GimpChannel *channel,
                     gint        *x1,
                     gint        *y1,
                     gint        *x2,
                     gint        *y2)
{
  gint     tmp_x1, tmp_y1, tmp_x2, tmp_y2;
  gboolean retval;

  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  retval = GIMP_CHANNEL_GET_CLASS (channel)->bounds (channel,
                                                     &tmp_x1, &tmp_y1,
                                                     &tmp_x2, &tmp_y2);

  if (x1) *x1 = tmp_x1;
  if (y1) *y1 = tmp_y1;
  if (x2) *x2 = tmp_x2;
  if (y2) *y2 = tmp_y2;

  return retval;
}

gboolean
gimp_channel_is_empty (GimpChannel *channel)
{
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  return GIMP_CHANNEL_GET_CLASS (channel)->is_empty (channel);
}

void
gimp_channel_feather (GimpChannel *channel,
                      gdouble      radius_x,
                      gdouble      radius_y,
                      gboolean     push_undo)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  if (! gimp_item_is_attached (GIMP_ITEM (channel)))
    push_undo = FALSE;

  GIMP_CHANNEL_GET_CLASS (channel)->feather (channel, radius_x, radius_y,
                                             push_undo);
}

void
gimp_channel_sharpen (GimpChannel *channel,
                      gboolean     push_undo)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  if (! gimp_item_is_attached (GIMP_ITEM (channel)))
    push_undo = FALSE;

  GIMP_CHANNEL_GET_CLASS (channel)->sharpen (channel, push_undo);
}

void
gimp_channel_clear (GimpChannel *channel,
                    const gchar *undo_desc,
                    gboolean     push_undo)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  if (! gimp_item_is_attached (GIMP_ITEM (channel)))
    push_undo = FALSE;

  GIMP_CHANNEL_GET_CLASS (channel)->clear (channel, undo_desc, push_undo);
}

void
gimp_channel_all (GimpChannel *channel,
                  gboolean     push_undo)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  if (! gimp_item_is_attached (GIMP_ITEM (channel)))
    push_undo = FALSE;

  GIMP_CHANNEL_GET_CLASS (channel)->all (channel, push_undo);
}

void
gimp_channel_invert (GimpChannel *channel,
                     gboolean     push_undo)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  if (! gimp_item_is_attached (GIMP_ITEM (channel)))
    push_undo = FALSE;

  GIMP_CHANNEL_GET_CLASS (channel)->invert (channel, push_undo);
}

void
gimp_channel_border (GimpChannel *channel,
                     gint         radius_x,
                     gint         radius_y,
                     gboolean     feather,
                     gboolean     edge_lock,
                     gboolean     push_undo)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  if (! gimp_item_is_attached (GIMP_ITEM (channel)))
    push_undo = FALSE;

  GIMP_CHANNEL_GET_CLASS (channel)->border (channel,
                                            radius_x, radius_y, feather, edge_lock,
                                            push_undo);
}

void
gimp_channel_grow (GimpChannel *channel,
                   gint         radius_x,
                   gint         radius_y,
                   gboolean     push_undo)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  if (! gimp_item_is_attached (GIMP_ITEM (channel)))
    push_undo = FALSE;

  GIMP_CHANNEL_GET_CLASS (channel)->grow (channel, radius_x, radius_y,
                                          push_undo);
}

void
gimp_channel_shrink (GimpChannel  *channel,
                     gint          radius_x,
                     gint          radius_y,
                     gboolean      edge_lock,
                     gboolean      push_undo)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  if (! gimp_item_is_attached (GIMP_ITEM (channel)))
    push_undo = FALSE;

  GIMP_CHANNEL_GET_CLASS (channel)->shrink (channel, radius_x, radius_y,
                                            edge_lock, push_undo);
}
