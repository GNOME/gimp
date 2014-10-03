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

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "paint/gimppaintcore-stroke.h"
#include "paint/gimppaintoptions.h"

#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-mask.h"
#include "gegl/gimp-gegl-nodes.h"

#include "gimp.h"
#include "gimp-utils.h"
#include "gimpboundary.h"
#include "gimpcontainer.h"
#include "gimperror.h"
#include "gimpimage.h"
#include "gimpimage-quick-mask.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimpchannel.h"
#include "gimpchannel-select.h"
#include "gimpcontext.h"
#include "gimpdrawable-stroke.h"
#include "gimpmarshal.h"
#include "gimppaintinfo.h"
#include "gimppickable.h"
#include "gimpstrokeoptions.h"

#include "gimp-intl.h"


enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};


static void gimp_channel_pickable_iface_init (GimpPickableInterface *iface);

static void       gimp_channel_finalize      (GObject           *object);

static gint64     gimp_channel_get_memsize   (GimpObject        *object,
                                              gint64            *gui_size);

static gchar  * gimp_channel_get_description (GimpViewable      *viewable,
                                              gchar            **tooltip);

static GeglNode * gimp_channel_get_node      (GimpFilter        *filter);

static gboolean   gimp_channel_is_attached   (const GimpItem    *item);
static GimpItemTree * gimp_channel_get_tree  (GimpItem          *item);
static GimpItem * gimp_channel_duplicate     (GimpItem          *item,
                                              GType              new_type);
static void       gimp_channel_convert       (GimpItem          *item,
                                              GimpImage         *dest_image);
static void       gimp_channel_translate     (GimpItem          *item,
                                              gint               off_x,
                                              gint               off_y,
                                              gboolean           push_undo);
static void       gimp_channel_scale         (GimpItem          *item,
                                              gint               new_width,
                                              gint               new_height,
                                              gint               new_offset_x,
                                              gint               new_offset_y,
                                              GimpInterpolationType interp_type,
                                              GimpProgress      *progress);
static void       gimp_channel_resize        (GimpItem          *item,
                                              GimpContext       *context,
                                              gint               new_width,
                                              gint               new_height,
                                              gint               offx,
                                              gint               offy);
static void       gimp_channel_flip          (GimpItem          *item,
                                              GimpContext       *context,
                                              GimpOrientationType flip_type,
                                              gdouble            axis,
                                              gboolean           flip_result);
static void       gimp_channel_rotate        (GimpItem          *item,
                                              GimpContext       *context,
                                              GimpRotationType   flip_type,
                                              gdouble            center_x,
                                              gdouble            center_y,
                                              gboolean           flip_result);
static void       gimp_channel_transform     (GimpItem          *item,
                                              GimpContext       *context,
                                              const GimpMatrix3 *matrix,
                                              GimpTransformDirection direction,
                                              GimpInterpolationType interpolation_type,
                                              GimpTransformResize clip_result,
                                              GimpProgress      *progress);
static gboolean   gimp_channel_stroke        (GimpItem          *item,
                                              GimpDrawable      *drawable,
                                              GimpStrokeOptions *stroke_options,
                                              gboolean           push_undo,
                                              GimpProgress      *progress,
                                              GError           **error);
static void       gimp_channel_to_selection  (GimpItem          *item,
                                              GimpChannelOps     op,
                                              gboolean           antialias,
                                              gboolean           feather,
                                              gdouble            feather_radius_x,
                                              gdouble            feather_radius_y);

static void       gimp_channel_convert_type  (GimpDrawable      *drawable,
                                              GimpImage         *dest_image,
                                              const Babl        *new_format,
                                              GimpImageBaseType  new_base_type,
                                              GimpPrecision      new_precision,
                                              gint               layer_dither_type,
                                              gint               mask_dither_type,
                                              gboolean           push_undo);
static void gimp_channel_invalidate_boundary   (GimpDrawable       *drawable);
static void gimp_channel_get_active_components (const GimpDrawable *drawable,
                                                gboolean           *active);
static GimpComponentMask
                  gimp_channel_get_active_mask (const GimpDrawable *drawable);

static void      gimp_channel_apply_buffer   (GimpDrawable        *drawable,
                                              GeglBuffer          *buffer,
                                              const GeglRectangle *buffer_region,
                                              gboolean             push_undo,
                                              const gchar         *undo_desc,
                                              gdouble              opacity,
                                              GimpLayerModeEffects  mode,
                                              GeglBuffer          *base_buffer,
                                              gint                 base_x,
                                              gint                 base_y);
static void      gimp_channel_replace_buffer (GimpDrawable        *drawable,
                                              GeglBuffer          *buffer,
                                              const GeglRectangle *buffer_region,
                                              gboolean             push_undo,
                                              const gchar         *undo_desc,
                                              gdouble              opacity,
                                              GeglBuffer          *mask,
                                              const GeglRectangle *mask_region,
                                              gint                 x,
                                              gint                 y);
static void      gimp_channel_set_buffer     (GimpDrawable        *drawable,
                                              gboolean             push_undo,
                                              const gchar         *undo_desc,
                                              GeglBuffer          *buffer,
                                              gint                 offset_x,
                                              gint                 offset_y);
static void      gimp_channel_swap_pixels    (GimpDrawable        *drawable,
                                              GeglBuffer          *buffer,
                                              gint                 x,
                                              gint                 y);

static gdouble   gimp_channel_get_opacity_at (GimpPickable        *pickable,
                                              gint                 x,
                                              gint                 y);

static gboolean   gimp_channel_real_boundary (GimpChannel         *channel,
                                              const GimpBoundSeg **segs_in,
                                              const GimpBoundSeg **segs_out,
                                              gint                *num_segs_in,
                                              gint                *num_segs_out,
                                              gint                 x1,
                                              gint                 y1,
                                              gint                 x2,
                                              gint                 y2);
static gboolean   gimp_channel_real_bounds   (GimpChannel         *channel,
                                              gint                *x1,
                                              gint                *y1,
                                              gint                *x2,
                                              gint                *y2);
static gboolean   gimp_channel_real_is_empty (GimpChannel         *channel);
static void       gimp_channel_real_feather  (GimpChannel         *channel,
                                              gdouble              radius_x,
                                              gdouble              radius_y,
                                              gboolean             push_undo);
static void       gimp_channel_real_sharpen  (GimpChannel         *channel,
                                              gboolean             push_undo);
static void       gimp_channel_real_clear    (GimpChannel         *channel,
                                              const gchar         *undo_desc,
                                              gboolean             push_undo);
static void       gimp_channel_real_all      (GimpChannel         *channel,
                                              gboolean             push_undo);
static void       gimp_channel_real_invert   (GimpChannel         *channel,
                                              gboolean             push_undo);
static void       gimp_channel_real_border   (GimpChannel         *channel,
                                              gint                 radius_x,
                                              gint                 radius_y,
                                              gboolean             feather,
                                              gboolean             edge_lock,
                                              gboolean             push_undo);
static void       gimp_channel_real_grow     (GimpChannel         *channel,
                                              gint                 radius_x,
                                              gint                 radius_y,
                                              gboolean             push_undo);
static void       gimp_channel_real_shrink   (GimpChannel         *channel,
                                              gint                 radius_x,
                                              gint                 radius_y,
                                              gboolean             edge_lock,
                                              gboolean             push_undo);


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
  GimpFilterClass   *filter_class      = GIMP_FILTER_CLASS (klass);
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

  object_class->finalize            = gimp_channel_finalize;

  gimp_object_class->get_memsize    = gimp_channel_get_memsize;

  viewable_class->get_description   = gimp_channel_get_description;
  viewable_class->default_icon_name = "gimp-channel";

  filter_class->get_node            = gimp_channel_get_node;

  item_class->is_attached          = gimp_channel_is_attached;
  item_class->get_tree             = gimp_channel_get_tree;
  item_class->duplicate            = gimp_channel_duplicate;
  item_class->convert              = gimp_channel_convert;
  item_class->translate            = gimp_channel_translate;
  item_class->scale                = gimp_channel_scale;
  item_class->resize               = gimp_channel_resize;
  item_class->flip                 = gimp_channel_flip;
  item_class->rotate               = gimp_channel_rotate;
  item_class->transform            = gimp_channel_transform;
  item_class->stroke               = gimp_channel_stroke;
  item_class->to_selection         = gimp_channel_to_selection;
  item_class->default_name         = _("Channel");
  item_class->rename_desc          = C_("undo-type", "Rename Channel");
  item_class->translate_desc       = C_("undo-type", "Move Channel");
  item_class->scale_desc           = C_("undo-type", "Scale Channel");
  item_class->resize_desc          = C_("undo-type", "Resize Channel");
  item_class->flip_desc            = C_("undo-type", "Flip Channel");
  item_class->rotate_desc          = C_("undo-type", "Rotate Channel");
  item_class->transform_desc       = C_("undo-type", "Transform Channel");
  item_class->stroke_desc          = C_("undo-type", "Stroke Channel");
  item_class->to_selection_desc    = C_("undo-type", "Channel to Selection");
  item_class->reorder_desc         = C_("undo-type", "Reorder Channel");
  item_class->raise_desc           = C_("undo-type", "Raise Channel");
  item_class->raise_to_top_desc    = C_("undo-type", "Raise Channel to Top");
  item_class->lower_desc           = C_("undo-type", "Lower Channel");
  item_class->lower_to_bottom_desc = C_("undo-type", "Lower Channel to Bottom");
  item_class->raise_failed         = _("Channel cannot be raised higher.");
  item_class->lower_failed         = _("Channel cannot be lowered more.");

  drawable_class->convert_type          = gimp_channel_convert_type;
  drawable_class->invalidate_boundary   = gimp_channel_invalidate_boundary;
  drawable_class->get_active_components = gimp_channel_get_active_components;
  drawable_class->get_active_mask       = gimp_channel_get_active_mask;
  drawable_class->apply_buffer          = gimp_channel_apply_buffer;
  drawable_class->replace_buffer        = gimp_channel_replace_buffer;
  drawable_class->set_buffer            = gimp_channel_set_buffer;
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

  klass->feather_desc   = C_("undo-type", "Feather Channel");
  klass->sharpen_desc   = C_("undo-type", "Sharpen Channel");
  klass->clear_desc     = C_("undo-type", "Clear Channel");
  klass->all_desc       = C_("undo-type", "Fill Channel");
  klass->invert_desc    = C_("undo-type", "Invert Channel");
  klass->border_desc    = C_("undo-type", "Border Channel");
  klass->grow_desc      = C_("undo-type", "Grow Channel");
  klass->shrink_desc    = C_("undo-type", "Shrink Channel");
}

static void
gimp_channel_init (GimpChannel *channel)
{
  gimp_rgba_set (&channel->color, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);

  channel->show_masked    = FALSE;

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

  *gui_size += channel->num_segs_in  * sizeof (GimpBoundSeg);
  *gui_size += channel->num_segs_out * sizeof (GimpBoundSeg);

  return GIMP_OBJECT_CLASS (parent_class)->get_memsize (object, gui_size);
}

static gchar *
gimp_channel_get_description (GimpViewable  *viewable,
                              gchar        **tooltip)
{
  if (! strcmp (GIMP_IMAGE_QUICK_MASK_NAME,
                gimp_object_get_name (viewable)))
    {
      return g_strdup (_("Quick Mask"));
    }

  return GIMP_VIEWABLE_CLASS (parent_class)->get_description (viewable,
                                                              tooltip);
}

static GeglNode *
gimp_channel_get_node (GimpFilter *filter)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (filter);
  GimpChannel  *channel  = GIMP_CHANNEL (filter);
  GeglNode     *node;
  GeglNode     *source;
  GeglNode     *mode_node;
  const Babl   *color_format;

  node = GIMP_FILTER_CLASS (parent_class)->get_node (filter);

  source = gimp_drawable_get_source_node (drawable);
  gegl_node_add_child (node, source);

  g_warn_if_fail (channel->color_node == NULL);

  if (gimp_drawable_get_linear (drawable))
    color_format = babl_format ("RGBA float");
  else
    color_format = babl_format ("R'G'B'A float");

  channel->color_node = gegl_node_new_child (node,
                                             "operation", "gegl:color",
                                             "format",    color_format,
                                             NULL);
  gimp_gegl_node_set_color (channel->color_node,
                            &channel->color);

  g_warn_if_fail (channel->mask_node == NULL);

  channel->mask_node = gegl_node_new_child (node,
                                            "operation", "gegl:opacity",
                                            NULL);
  gegl_node_connect_to (channel->color_node, "output",
                        channel->mask_node,  "input");

  g_warn_if_fail (channel->invert_node == NULL);

  channel->invert_node = gegl_node_new_child (node,
                                              "operation", "gegl:invert-linear",
                                              NULL);

  if (channel->show_masked)
    {
      gegl_node_connect_to (source,               "output",
                            channel->invert_node, "input");
      gegl_node_connect_to (channel->invert_node, "output",
                            channel->mask_node,   "aux");
    }
  else
    {
      gegl_node_connect_to (source,             "output",
                            channel->mask_node, "aux");
    }

  mode_node = gimp_drawable_get_mode_node (drawable);

  gegl_node_connect_to (channel->mask_node, "output",
                        mode_node,          "aux");

  return node;
}

static gboolean
gimp_channel_is_attached (const GimpItem *item)
{
  GimpImage *image = gimp_item_get_image (item);

  return (GIMP_IS_IMAGE (image) &&
          gimp_container_have (gimp_image_get_channels (image),
                               GIMP_OBJECT (item)));
}

static GimpItemTree *
gimp_channel_get_tree (GimpItem *item)
{
  if (gimp_item_is_attached (item))
    {
      GimpImage *image = gimp_item_get_image (item);

      return gimp_image_get_channel_tree (image);
    }

  return NULL;
}

static GimpItem *
gimp_channel_duplicate (GimpItem *item,
                        GType     new_type)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type);

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
  GimpChannel  *channel  = GIMP_CHANNEL (item);
  GimpDrawable *drawable = GIMP_DRAWABLE (item);

  if (! gimp_drawable_is_gray (drawable))
    {
      gimp_drawable_convert_type (drawable, dest_image, GIMP_GRAY,
                                  gimp_image_get_precision (dest_image),
                                  0, 0,
                                  FALSE);
    }

  if (gimp_drawable_has_alpha (drawable))
    {
      GeglBuffer *new_buffer;
      const Babl *format;
      GimpRGB     background;

      format = gimp_drawable_get_format_without_alpha (drawable);

      new_buffer =
        gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                         gimp_item_get_width (item),
                                         gimp_item_get_height (item)),
                         format);

      gimp_rgba_set (&background, 0.0, 0.0, 0.0, 0.0);

      gimp_gegl_apply_flatten (gimp_drawable_get_buffer (drawable),
                               NULL, NULL,
                               new_buffer, &background);

      gimp_drawable_set_buffer_full (drawable, FALSE, NULL,
                                     new_buffer,
                                     gimp_item_get_offset_x (item),
                                     gimp_item_get_offset_y (item));
      g_object_unref (new_buffer);
    }

  if (G_TYPE_FROM_INSTANCE (channel) == GIMP_TYPE_CHANNEL)
    {
      gint width  = gimp_image_get_width  (dest_image);
      gint height = gimp_image_get_height (dest_image);

      gimp_item_set_offset (item, 0, 0);

      if (gimp_item_get_width  (item) != width ||
          gimp_item_get_height (item) != height)
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
  GimpChannel *channel    = GIMP_CHANNEL (item);
  GeglBuffer  *tmp_buffer = NULL;
  gint         width, height;
  gint         x1, y1, x2, y2;

  gimp_channel_bounds (channel, &x1, &y1, &x2, &y2);

  /*  update the old area  */
  gimp_drawable_update (GIMP_DRAWABLE (item), x1, y1, x2 - x1, y2 - y1);

  if (push_undo)
    gimp_channel_push_undo (channel, NULL);
  else
    gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (channel));

  x1 = CLAMP ((x1 + off_x), 0, gimp_item_get_width  (GIMP_ITEM (channel)));
  y1 = CLAMP ((y1 + off_y), 0, gimp_item_get_height (GIMP_ITEM (channel)));
  x2 = CLAMP ((x2 + off_x), 0, gimp_item_get_width  (GIMP_ITEM (channel)));
  y2 = CLAMP ((y2 + off_y), 0, gimp_item_get_height (GIMP_ITEM (channel)));

  width  = x2 - x1;
  height = y2 - y1;

  /*  make sure width and height are non-zero  */
  if (width != 0 && height != 0)
    {
      /*  copy the portion of the mask we will keep to a temporary
       *  buffer
       */
      tmp_buffer =
        gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                         gimp_drawable_get_format (GIMP_DRAWABLE (channel)));

      gegl_buffer_copy (gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                        GEGL_RECTANGLE (x1 - off_x, y1 - off_y, width, height),
                        tmp_buffer,
                        GEGL_RECTANGLE (0, 0, 0, 0));
    }

  /*  clear the mask  */
  gegl_buffer_clear (gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                     NULL);

  if (width != 0 && height != 0)
    {
      /*  copy the temp mask back to the mask  */

      gegl_buffer_copy (tmp_buffer,
                        NULL,
                        gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                        GEGL_RECTANGLE (x1, y1, 0, 0));

      /*  free the temporary mask  */
      g_object_unref (tmp_buffer);
    }

  /*  calculate new bounds  */
  if (width == 0 || height == 0)
    {
      channel->empty = TRUE;
      channel->x1    = 0;
      channel->y1    = 0;
      channel->x2    = gimp_item_get_width  (GIMP_ITEM (channel));
      channel->y2    = gimp_item_get_height (GIMP_ITEM (channel));
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
  GimpChannel *channel = GIMP_CHANNEL (item);

  if (G_TYPE_FROM_INSTANCE (item) == GIMP_TYPE_CHANNEL)
    {
      new_offset_x = 0;
      new_offset_y = 0;
    }

  /*  don't waste CPU cycles scaling an empty channel  */
  if (channel->bounds_known && channel->empty)
    {
      GimpDrawable *drawable = GIMP_DRAWABLE (item);
      GeglBuffer   *new_buffer;

      new_buffer =
        gegl_buffer_new (GEGL_RECTANGLE (0, 0, new_width, new_height),
                         gimp_drawable_get_format (drawable));

      gimp_drawable_set_buffer_full (drawable,
                                     gimp_item_is_attached (item), NULL,
                                     new_buffer,
                                     new_offset_x, new_offset_y);
      g_object_unref (new_buffer);

      gimp_channel_clear (GIMP_CHANNEL (item), NULL, FALSE);
    }
  else
    {
      GIMP_ITEM_CLASS (parent_class)->scale (item, new_width, new_height,
                                             new_offset_x, new_offset_y,
                                             interpolation_type, progress);
    }
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
      gimp_item_set_offset (item, 0, 0);
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
                        GimpTransformResize     clip_result,
                        GimpProgress           *progress)
{
  if (G_TYPE_FROM_INSTANCE (item) == GIMP_TYPE_CHANNEL)
    clip_result = TRUE;

  GIMP_ITEM_CLASS (parent_class)->transform (item, context, matrix, direction,
                                             interpolation_type,
                                             clip_result,
                                             progress);
}

static gboolean
gimp_channel_stroke (GimpItem           *item,
                     GimpDrawable       *drawable,
                     GimpStrokeOptions  *stroke_options,
                     gboolean            push_undo,
                     GimpProgress       *progress,
                     GError            **error)
{
  GimpChannel        *channel = GIMP_CHANNEL (item);
  const GimpBoundSeg *segs_in;
  const GimpBoundSeg *segs_out;
  gint                n_segs_in;
  gint                n_segs_out;
  gboolean            retval = FALSE;
  gint                offset_x, offset_y;

  if (! gimp_channel_boundary (channel, &segs_in, &segs_out,
                               &n_segs_in, &n_segs_out,
                               0, 0, 0, 0))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("Cannot stroke empty channel."));
      return FALSE;
    }

  gimp_item_get_offset (GIMP_ITEM (channel), &offset_x, &offset_y);

  switch (gimp_stroke_options_get_method (stroke_options))
    {
    case GIMP_STROKE_METHOD_LIBART:
      gimp_drawable_stroke_boundary (drawable,
                                     stroke_options,
                                     segs_in, n_segs_in,
                                     offset_x, offset_y,
                                     push_undo);
      retval = TRUE;
      break;

    case GIMP_STROKE_METHOD_PAINT_CORE:
      {
        GimpPaintInfo    *paint_info;
        GimpPaintCore    *core;
        GimpPaintOptions *paint_options;
        gboolean          emulate_dynamics;

        paint_info = gimp_context_get_paint_info (GIMP_CONTEXT (stroke_options));

        core = g_object_new (paint_info->paint_type, NULL);

        paint_options = gimp_stroke_options_get_paint_options (stroke_options);
        emulate_dynamics = gimp_stroke_options_get_emulate_dynamics (stroke_options);

        retval = gimp_paint_core_stroke_boundary (core, drawable,
                                                  paint_options,
                                                  emulate_dynamics,
                                                  segs_in, n_segs_in,
                                                  offset_x, offset_y,
                                                  push_undo, error);

        g_object_unref (core);
      }
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  return retval;
}

static void
gimp_channel_to_selection (GimpItem       *item,
                           GimpChannelOps  op,
                           gboolean        antialias,
                           gboolean        feather,
                           gdouble         feather_radius_x,
                           gdouble         feather_radius_y)
{
  GimpChannel *channel = GIMP_CHANNEL (item);
  GimpImage   *image   = gimp_item_get_image (item);
  gint         off_x, off_y;

  gimp_item_get_offset (item, &off_x, &off_y);

  gimp_channel_select_channel (gimp_image_get_mask (image),
                               GIMP_ITEM_GET_CLASS (item)->to_selection_desc,
                               channel, off_x, off_y,
                               op,
                               feather, feather_radius_x, feather_radius_x);
}

static void
gimp_channel_convert_type (GimpDrawable      *drawable,
                           GimpImage         *dest_image,
                           const Babl        *new_format,
                           GimpImageBaseType  new_base_type,
                           GimpPrecision      new_precision,
                           gint               layer_dither_type,
                           gint               mask_dither_type,
                           gboolean           push_undo)
{
  GeglBuffer *dest_buffer;

  dest_buffer =
    gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                     gimp_item_get_width  (GIMP_ITEM (drawable)),
                                     gimp_item_get_height (GIMP_ITEM (drawable))),
                     new_format);

  if (mask_dither_type == 0)
    {
      gegl_buffer_copy (gimp_drawable_get_buffer (drawable), NULL,
                        dest_buffer, NULL);
    }
  else
    {
      gint bits;

      bits = (babl_format_get_bytes_per_pixel (new_format) * 8 /
              babl_format_get_n_components (new_format));

      gimp_gegl_apply_color_reduction (gimp_drawable_get_buffer (drawable),
                                       NULL, NULL,
                                       dest_buffer, bits, mask_dither_type);
    }

  gimp_drawable_set_buffer (drawable, push_undo, NULL, dest_buffer);
  g_object_unref (dest_buffer);
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
  active[GRAY]    = TRUE;
  active[ALPHA_G] = FALSE;
}

static GimpComponentMask
gimp_channel_get_active_mask (const GimpDrawable *drawable)
{
  /*  Return all, because that skips the component mask op when painting  */
  return GIMP_COMPONENT_ALL;
}

static void
gimp_channel_apply_buffer (GimpDrawable         *drawable,
                           GeglBuffer           *buffer,
                           const GeglRectangle  *buffer_region,
                           gboolean              push_undo,
                           const gchar          *undo_desc,
                           gdouble               opacity,
                           GimpLayerModeEffects  mode,
                           GeglBuffer           *base_buffer,
                           gint                  base_x,
                           gint                  base_y)
{
  gimp_drawable_invalidate_boundary (drawable);

  GIMP_DRAWABLE_CLASS (parent_class)->apply_buffer (drawable, buffer,
                                                    buffer_region,
                                                    push_undo, undo_desc,
                                                    opacity, mode,
                                                    base_buffer,
                                                    base_x, base_y);

  GIMP_CHANNEL (drawable)->bounds_known = FALSE;
}

static void
gimp_channel_replace_buffer (GimpDrawable        *drawable,
                             GeglBuffer          *buffer,
                             const GeglRectangle *buffer_region,
                             gboolean             push_undo,
                             const gchar         *undo_desc,
                             gdouble              opacity,
                             GeglBuffer          *mask,
                             const GeglRectangle *mask_region,
                             gint                 x,
                             gint                 y)
{
  gimp_drawable_invalidate_boundary (drawable);

  GIMP_DRAWABLE_CLASS (parent_class)->replace_buffer (drawable, buffer,
                                                      buffer_region,
                                                      push_undo, undo_desc,
                                                      opacity,
                                                      mask, mask_region,
                                                      x, y);

  GIMP_CHANNEL (drawable)->bounds_known = FALSE;
}

static void
gimp_channel_set_buffer (GimpDrawable *drawable,
                         gboolean      push_undo,
                         const gchar  *undo_desc,
                         GeglBuffer   *buffer,
                         gint          offset_x,
                         gint          offset_y)
{
  GimpChannel *channel = GIMP_CHANNEL (drawable);

  GIMP_DRAWABLE_CLASS (parent_class)->set_buffer (drawable,
                                                  push_undo, undo_desc,
                                                  buffer,
                                                  offset_x, offset_y);

  channel->bounds_known = FALSE;

  if (gimp_filter_peek_node (GIMP_FILTER (channel)))
    {
      const Babl *color_format;

      if (gimp_drawable_get_linear (drawable))
        color_format = babl_format ("RGBA float");
      else
        color_format = babl_format ("R'G'B'A float");

      gegl_node_set (channel->color_node,
                     "format", color_format,
                     NULL);
    }
}

static void
gimp_channel_swap_pixels (GimpDrawable *drawable,
                          GeglBuffer   *buffer,
                          gint          x,
                          gint          y)
{
  gimp_drawable_invalidate_boundary (drawable);

  GIMP_DRAWABLE_CLASS (parent_class)->swap_pixels (drawable, buffer, x, y);

  GIMP_CHANNEL (drawable)->bounds_known = FALSE;
}

static gdouble
gimp_channel_get_opacity_at (GimpPickable *pickable,
                             gint          x,
                             gint          y)
{
  GimpChannel *channel = GIMP_CHANNEL (pickable);
  gdouble      value   = GIMP_OPACITY_TRANSPARENT;

  if (x >= 0 && x < gimp_item_get_width  (GIMP_ITEM (channel)) &&
      y >= 0 && y < gimp_item_get_height (GIMP_ITEM (channel)))
    {
      if (! channel->bounds_known ||
          (! channel->empty &&
           x >= channel->x1 &&
           x <  channel->x2 &&
           y >= channel->y1 &&
           y <  channel->y2))
        {
          gegl_buffer_sample (gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                              x, y, NULL, &value, babl_format ("Y double"),
                              GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
        }
    }

  return value;
}

static gboolean
gimp_channel_real_boundary (GimpChannel         *channel,
                            const GimpBoundSeg **segs_in,
                            const GimpBoundSeg **segs_out,
                            gint                *num_segs_in,
                            gint                *num_segs_out,
                            gint                 x1,
                            gint                 y1,
                            gint                 x2,
                            gint                 y2)
{
  if (! channel->boundary_known)
    {
      gint x3, y3, x4, y4;

      /* free the out of date boundary segments */
      g_free (channel->segs_in);
      g_free (channel->segs_out);

      if (gimp_channel_bounds (channel, &x3, &y3, &x4, &y4))
        {
          GeglBuffer *buffer;
          GeglRectangle  rect = { x3, y3, x4 - x3, y4 - y3 };

          buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (channel));

          channel->segs_out = gimp_boundary_find (buffer, &rect,
                                                  babl_format ("Y float"),
                                                  GIMP_BOUNDARY_IGNORE_BOUNDS,
                                                  x1, y1, x2, y2,
                                                  GIMP_BOUNDARY_HALF_WAY,
                                                  &channel->num_segs_out);
          x1 = MAX (x1, x3);
          y1 = MAX (y1, y3);
          x2 = MIN (x2, x4);
          y2 = MIN (y2, y4);

          if (x2 > x1 && y2 > y1)
            {
              channel->segs_in = gimp_boundary_find (buffer, NULL,
                                                     babl_format ("Y float"),
                                                     GIMP_BOUNDARY_WITHIN_BOUNDS,
                                                     x1, y1, x2, y2,
                                                     GIMP_BOUNDARY_HALF_WAY,
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
  GeglBuffer *buffer;

  /*  if the channel's bounds have already been reliably calculated...  */
  if (channel->bounds_known)
    {
      *x1 = channel->x1;
      *y1 = channel->y1;
      *x2 = channel->x2;
      *y2 = channel->y2;

      return ! channel->empty;
    }

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (channel));

  channel->empty = ! gimp_gegl_mask_bounds (buffer, x1, y1, x2, y2);

  channel->x1 = *x1;
  channel->y1 = *y1;
  channel->x2 = *x2;
  channel->y2 = *y2;

  channel->bounds_known = TRUE;

  return ! channel->empty;
}

static gboolean
gimp_channel_real_is_empty (GimpChannel *channel)
{
  GeglBuffer *buffer;

  if (channel->bounds_known)
    return channel->empty;

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (channel));

  if (! gimp_gegl_mask_is_empty (buffer))
    return FALSE;

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
  channel->x2             = gimp_item_get_width  (GIMP_ITEM (channel));
  channel->y2             = gimp_item_get_height (GIMP_ITEM (channel));

  return TRUE;
}

static void
gimp_channel_real_feather (GimpChannel *channel,
                           gdouble      radius_x,
                           gdouble      radius_y,
                           gboolean     push_undo)
{
  gint x1, y1, x2, y2;

  if (radius_x <= 0.0 && radius_y <= 0.0)
    return;

  if (! gimp_channel_bounds (channel, &x1, &y1, &x2, &y2))
    return;

  if (gimp_channel_is_empty (channel))
    return;

  x1 = MAX (0, x1 - ceil (radius_x));
  y1 = MAX (0, y1 - ceil (radius_y));

  x2 = MIN (gimp_item_get_width  (GIMP_ITEM (channel)), x2 + ceil (radius_x));
  y2 = MIN (gimp_item_get_height (GIMP_ITEM (channel)), y2 + ceil (radius_y));

  if (push_undo)
    gimp_channel_push_undo (channel,
                            GIMP_CHANNEL_GET_CLASS (channel)->feather_desc);
  else
    gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (channel));

  gimp_gegl_apply_feather (gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                           NULL, NULL,
                           gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                           GEGL_RECTANGLE (x1, y1, x2 - x1, y2 - y1),
                           radius_x,
                           radius_y);

  channel->bounds_known = FALSE;

  gimp_drawable_update (GIMP_DRAWABLE (channel), 0, 0,
                        gimp_item_get_width  (GIMP_ITEM (channel)),
                        gimp_item_get_height (GIMP_ITEM (channel)));
}

static void
gimp_channel_real_sharpen (GimpChannel *channel,
                           gboolean     push_undo)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (channel);

  if (push_undo)
    gimp_channel_push_undo (channel,
                            GIMP_CHANNEL_GET_CLASS (channel)->sharpen_desc);
  else
    gimp_drawable_invalidate_boundary (drawable);

  gimp_gegl_apply_threshold (gimp_drawable_get_buffer (drawable),
                             NULL, NULL,
                             gimp_drawable_get_buffer (drawable),
                             0.5);

  channel->bounds_known = FALSE;

  gimp_drawable_update (GIMP_DRAWABLE (channel), 0, 0,
                        gimp_item_get_width  (GIMP_ITEM (channel)),
                        gimp_item_get_height (GIMP_ITEM (channel)));
}

static void
gimp_channel_real_clear (GimpChannel *channel,
                         const gchar *undo_desc,
                         gboolean     push_undo)
{
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

  if (channel->bounds_known && ! channel->empty)
    {
      gegl_buffer_clear (gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                         GEGL_RECTANGLE (channel->x1, channel->y1,
                                         channel->x2 - channel->x1,
                                         channel->y2 - channel->y1));
    }
  else
    {
      gegl_buffer_clear (gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                         NULL);
    }

  /*  we know the bounds  */
  channel->bounds_known = TRUE;
  channel->empty        = TRUE;
  channel->x1           = 0;
  channel->y1           = 0;
  channel->x2           = gimp_item_get_width  (GIMP_ITEM (channel));
  channel->y2           = gimp_item_get_height (GIMP_ITEM (channel));

  gimp_drawable_update (GIMP_DRAWABLE (channel), 0, 0,
                        gimp_item_get_width  (GIMP_ITEM (channel)),
                        gimp_item_get_height (GIMP_ITEM (channel)));
}

static void
gimp_channel_real_all (GimpChannel *channel,
                       gboolean     push_undo)
{
  GeglColor *color;

  if (push_undo)
    gimp_channel_push_undo (channel,
                            GIMP_CHANNEL_GET_CLASS (channel)->all_desc);
  else
    gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (channel));

  /*  clear the channel  */
  color = gegl_color_new ("#fff");
  gegl_buffer_set_color (gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                         NULL, color);
  g_object_unref (color);

  /*  we know the bounds  */
  channel->bounds_known = TRUE;
  channel->empty        = FALSE;
  channel->x1           = 0;
  channel->y1           = 0;
  channel->x2           = gimp_item_get_width  (GIMP_ITEM (channel));
  channel->y2           = gimp_item_get_height (GIMP_ITEM (channel));

  gimp_drawable_update (GIMP_DRAWABLE (channel), 0, 0,
                        gimp_item_get_width  (GIMP_ITEM (channel)),
                        gimp_item_get_height (GIMP_ITEM (channel)));
}

static void
gimp_channel_real_invert (GimpChannel *channel,
                          gboolean     push_undo)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (channel);

  if (push_undo)
    gimp_channel_push_undo (channel,
                            GIMP_CHANNEL_GET_CLASS (channel)->invert_desc);
  else
    gimp_drawable_invalidate_boundary (drawable);

  if (channel->bounds_known && channel->empty)
    {
      gimp_channel_all (channel, FALSE);
    }
  else
    {
      gimp_gegl_apply_invert_linear (gimp_drawable_get_buffer (drawable),
                                     NULL, NULL,
                                     gimp_drawable_get_buffer (drawable));

      channel->bounds_known = FALSE;

      gimp_drawable_update (GIMP_DRAWABLE (channel), 0, 0,
                            gimp_item_get_width  (GIMP_ITEM (channel)),
                            gimp_item_get_height (GIMP_ITEM (channel)));
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
  gint x1, y1, x2, y2;

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

  if (x2 + radius_x > gimp_item_get_width (GIMP_ITEM (channel)))
    x2 = gimp_item_get_width (GIMP_ITEM (channel));
  else
    x2 += radius_x;

  if (y1 - radius_y < 0)
    y1 = 0;
  else
    y1 -= radius_y;

  if (y2 + radius_y > gimp_item_get_height (GIMP_ITEM (channel)))
    y2 = gimp_item_get_height (GIMP_ITEM (channel));
  else
    y2 += radius_y;

  if (push_undo)
    gimp_channel_push_undo (channel,
                            GIMP_CHANNEL_GET_CLASS (channel)->border_desc);
  else
    gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (channel));

  gimp_gegl_apply_border (gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                          NULL, NULL,
                          gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                          GEGL_RECTANGLE (x1, y1, x2 - x1, y2 - y1),
                          radius_x, radius_y, feather, edge_lock);

  channel->bounds_known = FALSE;

  gimp_drawable_update (GIMP_DRAWABLE (channel), 0, 0,
                        gimp_item_get_width  (GIMP_ITEM (channel)),
                        gimp_item_get_height (GIMP_ITEM (channel)));
}

static void
gimp_channel_real_grow (GimpChannel *channel,
                        gint         radius_x,
                        gint         radius_y,
                        gboolean     push_undo)
{
  gint x1, y1, x2, y2;

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

  if (x2 + radius_x < gimp_item_get_width (GIMP_ITEM (channel)))
    x2 = x2 + radius_x;
  else
    x2 = gimp_item_get_width (GIMP_ITEM (channel));

  if (y2 + radius_y < gimp_item_get_height (GIMP_ITEM (channel)))
    y2 = y2 + radius_y;
  else
    y2 = gimp_item_get_height (GIMP_ITEM (channel));

  if (push_undo)
    gimp_channel_push_undo (channel,
                            GIMP_CHANNEL_GET_CLASS (channel)->grow_desc);
  else
    gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (channel));

  gimp_gegl_apply_grow (gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                        NULL, NULL,
                        gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                        GEGL_RECTANGLE (x1, y1, x2 - x1, y2 - y1),
                        radius_x, radius_y);

  channel->bounds_known = FALSE;

  gimp_drawable_update (GIMP_DRAWABLE (channel), 0, 0,
                        gimp_item_get_width  (GIMP_ITEM (channel)),
                        gimp_item_get_height (GIMP_ITEM (channel)));
}

static void
gimp_channel_real_shrink (GimpChannel *channel,
                          gint         radius_x,
                          gint         radius_y,
                          gboolean     edge_lock,
                          gboolean     push_undo)
{
  gint x1, y1, x2, y2;

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
  if (x2 < gimp_item_get_width (GIMP_ITEM (channel)))
    x2++;
  if (y2 < gimp_item_get_height (GIMP_ITEM (channel)))
    y2++;

  if (push_undo)
    gimp_channel_push_undo (channel,
                            GIMP_CHANNEL_GET_CLASS (channel)->shrink_desc);
  else
    gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (channel));

  gimp_gegl_apply_shrink (gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                          NULL, NULL,
                          gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                          GEGL_RECTANGLE (x1, y1, x2 - x1, y2 - y1),
                          radius_x, radius_y, edge_lock);

  channel->bounds_known = FALSE;

  gimp_drawable_update (GIMP_DRAWABLE (channel), 0, 0,
                        gimp_item_get_width  (GIMP_ITEM (channel)),
                        gimp_item_get_height (GIMP_ITEM (channel)));
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

  channel =
    GIMP_CHANNEL (gimp_drawable_new (GIMP_TYPE_CHANNEL,
                                     image, name,
                                     0, 0, width, height,
                                     gimp_image_get_channel_format (image)));

  if (color)
    channel->color = *color;

  channel->show_masked = TRUE;

  /*  selection mask variables  */
  channel->x2          = width;
  channel->y2          = height;

  return channel;
}

GimpChannel *
gimp_channel_new_from_buffer (GeglBuffer    *buffer,
                              GimpImage     *image,
                              const gchar   *name,
                              const GimpRGB *color)
{
  GimpChannel *channel;
  GeglBuffer  *dest;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  channel = gimp_channel_new (image,
                              gegl_buffer_get_width  (buffer),
                              gegl_buffer_get_height (buffer),
                              name, color);

  dest = gimp_drawable_get_buffer (GIMP_DRAWABLE (channel));
  gegl_buffer_copy (buffer, NULL, dest, NULL);

  return channel;
}

GimpChannel *
gimp_channel_new_from_alpha (GimpImage     *image,
                             GimpDrawable  *drawable,
                             const gchar   *name,
                             const GimpRGB *color)
{
  GimpChannel *channel;
  GeglBuffer  *dest_buffer;
  gint         width;
  gint         height;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_drawable_has_alpha (drawable), NULL);

  width  = gimp_item_get_width  (GIMP_ITEM (drawable));
  height = gimp_item_get_height (GIMP_ITEM (drawable));

  channel = gimp_channel_new (image, width, height, name, color);

  gimp_channel_clear (channel, NULL, FALSE);

  dest_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (channel));

  gegl_buffer_set_format (dest_buffer,
                          gimp_drawable_get_component_format (drawable,
                                                              GIMP_ALPHA_CHANNEL));
  gegl_buffer_copy (gimp_drawable_get_buffer (drawable), NULL,
                    dest_buffer, NULL);
  gegl_buffer_set_format (dest_buffer, NULL);

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
  GeglBuffer  *src_buffer;
  GeglBuffer  *dest_buffer;
  gint         width;
  gint         height;
  const Babl  *format;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  format = gimp_image_get_component_format (image, type);

  g_return_val_if_fail (format != NULL, NULL);

  gimp_pickable_flush (GIMP_PICKABLE (image));

  src_buffer = gimp_pickable_get_buffer (GIMP_PICKABLE (image));
  width  = gegl_buffer_get_width  (src_buffer);
  height = gegl_buffer_get_height (src_buffer);

  channel = gimp_channel_new (image, width, height, name, color);

  dest_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (channel));

  gegl_buffer_set_format (dest_buffer, format);
  gegl_buffer_copy (src_buffer, NULL, dest_buffer, NULL);
  gegl_buffer_set_format (dest_buffer, NULL);

  return channel;
}

GimpChannel *
gimp_channel_get_parent (GimpChannel *channel)
{
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), NULL);

  return GIMP_CHANNEL (gimp_viewable_get_parent (GIMP_VIEWABLE (channel)));
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

          gimp_image_undo_push_channel_color (image, C_("undo-type", "Set Channel Color"),
                                              channel);
        }

      channel->color = *color;

      if (gimp_filter_peek_node (GIMP_FILTER (channel)))
        {
          gimp_gegl_node_set_color (channel->color_node,
                                    &channel->color);
        }

      gimp_drawable_update (GIMP_DRAWABLE (channel),
                            0, 0,
                            gimp_item_get_width  (GIMP_ITEM (channel)),
                            gimp_item_get_height (GIMP_ITEM (channel)));

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

          gimp_image_undo_push_channel_color (image, C_("undo-type", "Set Channel Opacity"),
                                              channel);
        }

      channel->color.a = opacity;

      if (gimp_filter_peek_node (GIMP_FILTER (channel)))
        {
          gimp_gegl_node_set_color (channel->color_node,
                                    &channel->color);
        }

      gimp_drawable_update (GIMP_DRAWABLE (channel),
                            0, 0,
                            gimp_item_get_width  (GIMP_ITEM (channel)),
                            gimp_item_get_height (GIMP_ITEM (channel)));

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

      if (channel->invert_node)
        {
          GeglNode *source;

          source = gimp_drawable_get_source_node (GIMP_DRAWABLE (channel));

          if (channel->show_masked)
            {
              gegl_node_connect_to (source,               "output",
                                    channel->invert_node, "input");
              gegl_node_connect_to (channel->invert_node, "output",
                                    channel->mask_node,   "aux");
            }
          else
            {
              gegl_node_disconnect (channel->invert_node, "input");

              gegl_node_connect_to (source,             "output",
                                    channel->mask_node, "aux");
            }
        }

      gimp_drawable_update (GIMP_DRAWABLE (channel),
                            0, 0,
                            gimp_item_get_width  (GIMP_ITEM (channel)),
                            gimp_item_get_height (GIMP_ITEM (channel)));
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
  GimpChannel *channel;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  channel =
    GIMP_CHANNEL (gimp_drawable_new (GIMP_TYPE_CHANNEL,
                                     image, _("Selection Mask"),
                                     0, 0, width, height,
                                     gimp_image_get_mask_format (image)));

  channel->show_masked = TRUE;
  channel->x2          = width;
  channel->y2          = height;

  gegl_buffer_clear (gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                     NULL);

  return channel;
}

gboolean
gimp_channel_boundary (GimpChannel         *channel,
                       const GimpBoundSeg **segs_in,
                       const GimpBoundSeg **segs_out,
                       gint                *num_segs_in,
                       gint                *num_segs_out,
                       gint                 x1,
                       gint                 y1,
                       gint                 x2,
                       gint                 y2)
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
