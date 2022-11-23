/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmacolor/ligmacolor.h"

#include "core-types.h"

#include "paint/ligmapaintcore-stroke.h"
#include "paint/ligmapaintoptions.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligma-gegl-apply-operation.h"
#include "gegl/ligma-gegl-loops.h"
#include "gegl/ligma-gegl-mask.h"
#include "gegl/ligma-gegl-nodes.h"

#include "ligma.h"
#include "ligma-utils.h"
#include "ligmaboundary.h"
#include "ligmacontainer.h"
#include "ligmaerror.h"
#include "ligmaimage.h"
#include "ligmaimage-quick-mask.h"
#include "ligmaimage-undo.h"
#include "ligmaimage-undo-push.h"
#include "ligmachannel.h"
#include "ligmachannel-select.h"
#include "ligmacontext.h"
#include "ligmadrawable-fill.h"
#include "ligmadrawable-stroke.h"
#include "ligmapaintinfo.h"
#include "ligmapickable.h"
#include "ligmastrokeoptions.h"

#include "ligma-intl.h"


#define RGBA_EPSILON 1e-6

enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};


static void ligma_channel_pickable_iface_init (LigmaPickableInterface *iface);

static void       ligma_channel_finalize      (GObject           *object);

static gint64     ligma_channel_get_memsize   (LigmaObject        *object,
                                              gint64            *gui_size);

static gchar  * ligma_channel_get_description (LigmaViewable      *viewable,
                                              gchar            **tooltip);

static GeglNode * ligma_channel_get_node      (LigmaFilter        *filter);

static gboolean   ligma_channel_is_attached   (LigmaItem          *item);
static LigmaItemTree * ligma_channel_get_tree  (LigmaItem          *item);
static gboolean   ligma_channel_bounds        (LigmaItem          *item,
                                              gdouble           *x,
                                              gdouble           *y,
                                              gdouble           *width,
                                              gdouble           *height);
static LigmaItem * ligma_channel_duplicate     (LigmaItem          *item,
                                              GType              new_type);
static void       ligma_channel_convert       (LigmaItem          *item,
                                              LigmaImage         *dest_image,
                                              GType              old_type);
static void       ligma_channel_translate     (LigmaItem          *item,
                                              gdouble            off_x,
                                              gdouble            off_y,
                                              gboolean           push_undo);
static void       ligma_channel_scale         (LigmaItem          *item,
                                              gint               new_width,
                                              gint               new_height,
                                              gint               new_offset_x,
                                              gint               new_offset_y,
                                              LigmaInterpolationType interp_type,
                                              LigmaProgress      *progress);
static void       ligma_channel_resize        (LigmaItem          *item,
                                              LigmaContext       *context,
                                              LigmaFillType       fill_type,
                                              gint               new_width,
                                              gint               new_height,
                                              gint               offset_x,
                                              gint               offset_y);
static LigmaTransformResize
                  ligma_channel_get_clip      (LigmaItem          *item,
                                              LigmaTransformResize clip_result);
static gboolean   ligma_channel_fill          (LigmaItem          *item,
                                              LigmaDrawable      *drawable,
                                              LigmaFillOptions   *fill_options,
                                              gboolean           push_undo,
                                              LigmaProgress      *progress,
                                              GError           **error);
static gboolean   ligma_channel_stroke        (LigmaItem          *item,
                                              LigmaDrawable      *drawable,
                                              LigmaStrokeOptions *stroke_options,
                                              gboolean           push_undo,
                                              LigmaProgress      *progress,
                                              GError           **error);
static void       ligma_channel_to_selection  (LigmaItem          *item,
                                              LigmaChannelOps     op,
                                              gboolean           antialias,
                                              gboolean           feather,
                                              gdouble            feather_radius_x,
                                              gdouble            feather_radius_y);

static void       ligma_channel_convert_type  (LigmaDrawable      *drawable,
                                              LigmaImage         *dest_image,
                                              const Babl        *new_format,
                                              LigmaColorProfile  *src_profile,
                                              LigmaColorProfile  *dest_profile,
                                              GeglDitherMethod   layer_dither_type,
                                              GeglDitherMethod   mask_dither_type,
                                              gboolean           push_undo,
                                              LigmaProgress      *progress);
static void ligma_channel_invalidate_boundary   (LigmaDrawable       *drawable);
static void ligma_channel_get_active_components (LigmaDrawable       *drawable,
                                                gboolean           *active);

static void      ligma_channel_set_buffer     (LigmaDrawable        *drawable,
                                              gboolean             push_undo,
                                              const gchar         *undo_desc,
                                              GeglBuffer          *buffer,
                                              const GeglRectangle *bounds);

static gdouble   ligma_channel_get_opacity_at (LigmaPickable        *pickable,
                                              gint                 x,
                                              gint                 y);

static gboolean   ligma_channel_real_boundary (LigmaChannel         *channel,
                                              const LigmaBoundSeg **segs_in,
                                              const LigmaBoundSeg **segs_out,
                                              gint                *num_segs_in,
                                              gint                *num_segs_out,
                                              gint                 x1,
                                              gint                 y1,
                                              gint                 x2,
                                              gint                 y2);
static gboolean   ligma_channel_real_is_empty (LigmaChannel         *channel);
static void       ligma_channel_real_feather  (LigmaChannel         *channel,
                                              gdouble              radius_x,
                                              gdouble              radius_y,
                                              gboolean             edge_lock,
                                              gboolean             push_undo);
static void       ligma_channel_real_sharpen  (LigmaChannel         *channel,
                                              gboolean             push_undo);
static void       ligma_channel_real_clear    (LigmaChannel         *channel,
                                              const gchar         *undo_desc,
                                              gboolean             push_undo);
static void       ligma_channel_real_all      (LigmaChannel         *channel,
                                              gboolean             push_undo);
static void       ligma_channel_real_invert   (LigmaChannel         *channel,
                                              gboolean             push_undo);
static void       ligma_channel_real_border   (LigmaChannel         *channel,
                                              gint                 radius_x,
                                              gint                 radius_y,
                                              LigmaChannelBorderStyle style,
                                              gboolean             edge_lock,
                                              gboolean             push_undo);
static void       ligma_channel_real_grow     (LigmaChannel         *channel,
                                              gint                 radius_x,
                                              gint                 radius_y,
                                              gboolean             push_undo);
static void       ligma_channel_real_shrink   (LigmaChannel         *channel,
                                              gint                 radius_x,
                                              gint                 radius_y,
                                              gboolean             edge_lock,
                                              gboolean             push_undo);
static void       ligma_channel_real_flood    (LigmaChannel         *channel,
                                              gboolean             push_undo);


static void      ligma_channel_buffer_changed (GeglBuffer          *buffer,
                                              const GeglRectangle *rect,
                                              LigmaChannel         *channel);


G_DEFINE_TYPE_WITH_CODE (LigmaChannel, ligma_channel, LIGMA_TYPE_DRAWABLE,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_PICKABLE,
                                                ligma_channel_pickable_iface_init))

#define parent_class ligma_channel_parent_class

static guint channel_signals[LAST_SIGNAL] = { 0 };


static void
ligma_channel_class_init (LigmaChannelClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass   *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);
  LigmaFilterClass   *filter_class      = LIGMA_FILTER_CLASS (klass);
  LigmaItemClass     *item_class        = LIGMA_ITEM_CLASS (klass);
  LigmaDrawableClass *drawable_class    = LIGMA_DRAWABLE_CLASS (klass);

  channel_signals[COLOR_CHANGED] =
    g_signal_new ("color-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaChannelClass, color_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize            = ligma_channel_finalize;

  ligma_object_class->get_memsize    = ligma_channel_get_memsize;

  viewable_class->get_description   = ligma_channel_get_description;
  viewable_class->default_icon_name = "ligma-channel";

  filter_class->get_node            = ligma_channel_get_node;

  item_class->is_attached          = ligma_channel_is_attached;
  item_class->get_tree             = ligma_channel_get_tree;
  item_class->bounds               = ligma_channel_bounds;
  item_class->duplicate            = ligma_channel_duplicate;
  item_class->convert              = ligma_channel_convert;
  item_class->translate            = ligma_channel_translate;
  item_class->scale                = ligma_channel_scale;
  item_class->resize               = ligma_channel_resize;
  item_class->get_clip             = ligma_channel_get_clip;
  item_class->fill                 = ligma_channel_fill;
  item_class->stroke               = ligma_channel_stroke;
  item_class->to_selection         = ligma_channel_to_selection;
  item_class->default_name         = _("Channel");
  item_class->rename_desc          = C_("undo-type", "Rename Channel");
  item_class->translate_desc       = C_("undo-type", "Move Channel");
  item_class->scale_desc           = C_("undo-type", "Scale Channel");
  item_class->resize_desc          = C_("undo-type", "Resize Channel");
  item_class->flip_desc            = C_("undo-type", "Flip Channel");
  item_class->rotate_desc          = C_("undo-type", "Rotate Channel");
  item_class->transform_desc       = C_("undo-type", "Transform Channel");
  item_class->fill_desc            = C_("undo-type", "Fill Channel");
  item_class->stroke_desc          = C_("undo-type", "Stroke Channel");
  item_class->to_selection_desc    = C_("undo-type", "Channel to Selection");
  item_class->reorder_desc         = C_("undo-type", "Reorder Channel");
  item_class->raise_desc           = C_("undo-type", "Raise Channel");
  item_class->raise_to_top_desc    = C_("undo-type", "Raise Channel to Top");
  item_class->lower_desc           = C_("undo-type", "Lower Channel");
  item_class->lower_to_bottom_desc = C_("undo-type", "Lower Channel to Bottom");
  item_class->raise_failed         = _("Channel cannot be raised higher.");
  item_class->lower_failed         = _("Channel cannot be lowered more.");

  drawable_class->convert_type          = ligma_channel_convert_type;
  drawable_class->invalidate_boundary   = ligma_channel_invalidate_boundary;
  drawable_class->get_active_components = ligma_channel_get_active_components;
  drawable_class->set_buffer            = ligma_channel_set_buffer;

  klass->boundary       = ligma_channel_real_boundary;
  klass->is_empty       = ligma_channel_real_is_empty;
  klass->feather        = ligma_channel_real_feather;
  klass->sharpen        = ligma_channel_real_sharpen;
  klass->clear          = ligma_channel_real_clear;
  klass->all            = ligma_channel_real_all;
  klass->invert         = ligma_channel_real_invert;
  klass->border         = ligma_channel_real_border;
  klass->grow           = ligma_channel_real_grow;
  klass->shrink         = ligma_channel_real_shrink;
  klass->flood          = ligma_channel_real_flood;

  klass->feather_desc   = C_("undo-type", "Feather Channel");
  klass->sharpen_desc   = C_("undo-type", "Sharpen Channel");
  klass->clear_desc     = C_("undo-type", "Clear Channel");
  klass->all_desc       = C_("undo-type", "Fill Channel");
  klass->invert_desc    = C_("undo-type", "Invert Channel");
  klass->border_desc    = C_("undo-type", "Border Channel");
  klass->grow_desc      = C_("undo-type", "Grow Channel");
  klass->shrink_desc    = C_("undo-type", "Shrink Channel");
  klass->flood_desc     = C_("undo-type", "Flood Channel");
}

static void
ligma_channel_init (LigmaChannel *channel)
{
  ligma_rgba_set (&channel->color, 0.0, 0.0, 0.0, LIGMA_OPACITY_OPAQUE);

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
ligma_channel_pickable_iface_init (LigmaPickableInterface *iface)
{
  iface->get_opacity_at = ligma_channel_get_opacity_at;
}

static void
ligma_channel_finalize (GObject *object)
{
  LigmaChannel *channel = LIGMA_CHANNEL (object);

  g_clear_pointer (&channel->segs_in,  g_free);
  g_clear_pointer (&channel->segs_out, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
ligma_channel_get_memsize (LigmaObject *object,
                          gint64     *gui_size)
{
  LigmaChannel *channel = LIGMA_CHANNEL (object);

  *gui_size += channel->num_segs_in  * sizeof (LigmaBoundSeg);
  *gui_size += channel->num_segs_out * sizeof (LigmaBoundSeg);

  return LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object, gui_size);
}

static gchar *
ligma_channel_get_description (LigmaViewable  *viewable,
                              gchar        **tooltip)
{
  if (! strcmp (LIGMA_IMAGE_QUICK_MASK_NAME,
                ligma_object_get_name (viewable)))
    {
      return g_strdup (_("Quick Mask"));
    }

  return LIGMA_VIEWABLE_CLASS (parent_class)->get_description (viewable,
                                                              tooltip);
}

static GeglNode *
ligma_channel_get_node (LigmaFilter *filter)
{
  LigmaDrawable *drawable = LIGMA_DRAWABLE (filter);
  LigmaChannel  *channel  = LIGMA_CHANNEL (filter);
  GeglNode     *node;
  GeglNode     *source;
  GeglNode     *mode_node;
  const Babl   *color_format;

  node = LIGMA_FILTER_CLASS (parent_class)->get_node (filter);

  source = ligma_drawable_get_source_node (drawable);
  gegl_node_add_child (node, source);

  g_warn_if_fail (channel->color_node == NULL);

  color_format =
    ligma_babl_format (LIGMA_RGB,
                      ligma_babl_precision (LIGMA_COMPONENT_TYPE_FLOAT,
                                           ligma_drawable_get_trc (drawable)),
                      TRUE, NULL);

  channel->color_node = gegl_node_new_child (node,
                                             "operation", "gegl:color",
                                             "format",    color_format,
                                             NULL);
  ligma_gegl_node_set_color (channel->color_node,
                            &channel->color, NULL);

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

  mode_node = ligma_drawable_get_mode_node (drawable);

  gegl_node_connect_to (channel->mask_node, "output",
                        mode_node,          "aux");

  return node;
}

static gboolean
ligma_channel_is_attached (LigmaItem *item)
{
  LigmaImage *image = ligma_item_get_image (item);

  return (LIGMA_IS_IMAGE (image) &&
          ligma_container_have (ligma_image_get_channels (image),
                               LIGMA_OBJECT (item)));
}

static LigmaItemTree *
ligma_channel_get_tree (LigmaItem *item)
{
  if (ligma_item_is_attached (item))
    {
      LigmaImage *image = ligma_item_get_image (item);

      return ligma_image_get_channel_tree (image);
    }

  return NULL;
}

static gboolean
ligma_channel_bounds (LigmaItem *item,
                     gdouble  *x,
                     gdouble  *y,
                     gdouble  *width,
                     gdouble  *height)
{
  LigmaChannel *channel = LIGMA_CHANNEL (item);

  if (! channel->bounds_known)
    {
      GeglBuffer *buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel));

      channel->empty = ! ligma_gegl_mask_bounds (buffer,
                                                &channel->x1,
                                                &channel->y1,
                                                &channel->x2,
                                                &channel->y2);

      channel->bounds_known = TRUE;
    }

  *x      = channel->x1;
  *y      = channel->y1;
  *width  = channel->x2 - channel->x1;
  *height = channel->y2 - channel->y1;

  return ! channel->empty;
}

static LigmaItem *
ligma_channel_duplicate (LigmaItem *item,
                        GType     new_type)
{
  LigmaItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, LIGMA_TYPE_DRAWABLE), NULL);

  new_item = LIGMA_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (LIGMA_IS_CHANNEL (new_item))
    {
      LigmaChannel *channel     = LIGMA_CHANNEL (item);
      LigmaChannel *new_channel = LIGMA_CHANNEL (new_item);

      new_channel->color        = channel->color;
      new_channel->show_masked  = channel->show_masked;

      /*  selection mask variables  */
      new_channel->bounds_known = channel->bounds_known;
      new_channel->empty        = channel->empty;
      new_channel->x1           = channel->x1;
      new_channel->y1           = channel->y1;
      new_channel->x2           = channel->x2;
      new_channel->y2           = channel->y2;

      if (new_type == LIGMA_TYPE_CHANNEL)
        {
          /*  8-bit channel hack: make sure pixels between all sorts
           *  of channels of an image is always copied without any
           *  gamma conversion
           */
          LigmaDrawable *new_drawable = LIGMA_DRAWABLE (new_item);
          LigmaImage    *image        = ligma_item_get_image (item);
          const Babl   *format       = ligma_image_get_channel_format (image);

          if (format != ligma_drawable_get_format (new_drawable))
            {
              GeglBuffer *new_buffer;

              new_buffer =
                gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                 ligma_item_get_width  (new_item),
                                                 ligma_item_get_height (new_item)),
                                 format);

              gegl_buffer_set_format (new_buffer,
                                      ligma_drawable_get_format (new_drawable));
              ligma_gegl_buffer_copy (ligma_drawable_get_buffer (new_drawable),
                                     NULL, GEGL_ABYSS_NONE,
                                     new_buffer, NULL);
              gegl_buffer_set_format (new_buffer, NULL);

              ligma_drawable_set_buffer (new_drawable, FALSE, NULL, new_buffer);
              g_object_unref (new_buffer);
            }
        }
    }

  return new_item;
}

static void
ligma_channel_convert (LigmaItem  *item,
                      LigmaImage *dest_image,
                      GType      old_type)
{
  LigmaChannel  *channel  = LIGMA_CHANNEL (item);
  LigmaDrawable *drawable = LIGMA_DRAWABLE (item);

  if (! ligma_drawable_is_gray (drawable))
    {
      ligma_drawable_convert_type (drawable, dest_image,
                                  LIGMA_GRAY,
                                  ligma_image_get_precision (dest_image),
                                  ligma_drawable_has_alpha (drawable),
                                  NULL, NULL,
                                  GEGL_DITHER_NONE, GEGL_DITHER_NONE,
                                  FALSE, NULL);
    }

  if (ligma_drawable_has_alpha (drawable))
    {
      GeglBuffer *new_buffer;
      const Babl *format;
      LigmaRGB     background;

      format = ligma_drawable_get_format_without_alpha (drawable);

      new_buffer =
        gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                         ligma_item_get_width (item),
                                         ligma_item_get_height (item)),
                         format);

      ligma_rgba_set (&background, 0.0, 0.0, 0.0, 0.0);

      ligma_gegl_apply_flatten (ligma_drawable_get_buffer (drawable),
                               NULL, NULL,
                               new_buffer, &background, NULL,
                               LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR);

      ligma_drawable_set_buffer_full (drawable, FALSE, NULL,
                                     new_buffer,
                                     GEGL_RECTANGLE (
                                       ligma_item_get_offset_x (item),
                                       ligma_item_get_offset_y (item),
                                       0, 0),
                                     TRUE);
      g_object_unref (new_buffer);
    }

  if (G_TYPE_FROM_INSTANCE (channel) == LIGMA_TYPE_CHANNEL)
    {
      gint width  = ligma_image_get_width  (dest_image);
      gint height = ligma_image_get_height (dest_image);

      ligma_item_set_offset (item, 0, 0);

      if (ligma_item_get_width  (item) != width ||
          ligma_item_get_height (item) != height)
        {
          ligma_item_resize (item, ligma_get_user_context (dest_image->ligma),
                            LIGMA_FILL_TRANSPARENT,
                            width, height, 0, 0);
        }
    }

  LIGMA_ITEM_CLASS (parent_class)->convert (item, dest_image, old_type);
}

static void
ligma_channel_translate (LigmaItem *item,
                        gdouble   off_x,
                        gdouble   off_y,
                        gboolean  push_undo)
{
  LigmaChannel *channel = LIGMA_CHANNEL (item);
  gint         x, y, width, height;

  ligma_item_bounds (LIGMA_ITEM (channel), &x, &y, &width, &height);

  /*  update the old area  */
  ligma_drawable_update (LIGMA_DRAWABLE (item), x, y, width, height);

  if (push_undo)
    ligma_channel_push_undo (channel, NULL);

  if (ligma_rectangle_intersect (x + SIGNED_ROUND (off_x),
                                y + SIGNED_ROUND (off_y),
                                width, height,
                                0, 0,
                                ligma_item_get_width  (LIGMA_ITEM (channel)),
                                ligma_item_get_height (LIGMA_ITEM (channel)),
                                &x, &y, &width, &height))
    {
      /*  copy the portion of the mask we will keep to a temporary
       *  buffer
       */
      GeglBuffer *tmp_buffer =
        gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                         ligma_drawable_get_format (LIGMA_DRAWABLE (channel)));

      ligma_gegl_buffer_copy (ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel)),
                             GEGL_RECTANGLE (x - SIGNED_ROUND (off_x),
                                             y - SIGNED_ROUND (off_y),
                                             width, height),
                             GEGL_ABYSS_NONE,
                             tmp_buffer,
                             GEGL_RECTANGLE (0, 0, 0, 0));

      /*  clear the mask  */
      gegl_buffer_clear (ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel)),
                         NULL);

      /*  copy the temp mask back to the mask  */
      ligma_gegl_buffer_copy (tmp_buffer, NULL, GEGL_ABYSS_NONE,
                             ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel)),
                             GEGL_RECTANGLE (x, y, 0, 0));

      /*  free the temporary mask  */
      g_object_unref (tmp_buffer);

      channel->x1 = x;
      channel->y1 = y;
      channel->x2 = x + width;
      channel->y2 = y + height;
    }
  else
    {
      /*  clear the mask  */
      gegl_buffer_clear (ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel)),
                         NULL);

      channel->empty = TRUE;
      channel->x1    = 0;
      channel->y1    = 0;
      channel->x2    = ligma_item_get_width  (LIGMA_ITEM (channel));
      channel->y2    = ligma_item_get_height (LIGMA_ITEM (channel));
    }

  /*  update the new area  */
  ligma_drawable_update (LIGMA_DRAWABLE (item),
                        channel->x1, channel->y1,
                        channel->x2 - channel->x1,
                        channel->y2 - channel->y1);
}

static void
ligma_channel_scale (LigmaItem              *item,
                    gint                   new_width,
                    gint                   new_height,
                    gint                   new_offset_x,
                    gint                   new_offset_y,
                    LigmaInterpolationType  interpolation_type,
                    LigmaProgress          *progress)
{
  LigmaChannel *channel = LIGMA_CHANNEL (item);

  if (G_TYPE_FROM_INSTANCE (item) == LIGMA_TYPE_CHANNEL)
    {
      new_offset_x = 0;
      new_offset_y = 0;
    }

  /*  don't waste CPU cycles scaling an empty channel  */
  if (channel->bounds_known && channel->empty)
    {
      LigmaDrawable *drawable = LIGMA_DRAWABLE (item);
      GeglBuffer   *new_buffer;

      new_buffer =
        gegl_buffer_new (GEGL_RECTANGLE (0, 0, new_width, new_height),
                         ligma_drawable_get_format (drawable));

      ligma_drawable_set_buffer_full (drawable,
                                     ligma_item_is_attached (item), NULL,
                                     new_buffer,
                                     GEGL_RECTANGLE (new_offset_x, new_offset_y,
                                                     0,            0),
                                     TRUE);
      g_object_unref (new_buffer);

      ligma_channel_clear (LIGMA_CHANNEL (item), NULL, FALSE);
    }
  else
    {
      LIGMA_ITEM_CLASS (parent_class)->scale (item, new_width, new_height,
                                             new_offset_x, new_offset_y,
                                             interpolation_type, progress);
    }
}

static void
ligma_channel_resize (LigmaItem     *item,
                     LigmaContext  *context,
                     LigmaFillType  fill_type,
                     gint          new_width,
                     gint          new_height,
                     gint          offset_x,
                     gint          offset_y)
{
  LIGMA_ITEM_CLASS (parent_class)->resize (item, context, LIGMA_FILL_TRANSPARENT,
                                          new_width, new_height,
                                          offset_x, offset_y);

  if (G_TYPE_FROM_INSTANCE (item) == LIGMA_TYPE_CHANNEL)
    {
      ligma_item_set_offset (item, 0, 0);
    }
}

static LigmaTransformResize
ligma_channel_get_clip (LigmaItem            *item,
                       LigmaTransformResize  clip_result)
{
  return LIGMA_TRANSFORM_RESIZE_CLIP;
}

static gboolean
ligma_channel_fill (LigmaItem         *item,
                   LigmaDrawable     *drawable,
                   LigmaFillOptions  *fill_options,
                   gboolean          push_undo,
                   LigmaProgress     *progress,
                   GError          **error)
{
  LigmaChannel        *channel = LIGMA_CHANNEL (item);
  const LigmaBoundSeg *segs_in;
  const LigmaBoundSeg *segs_out;
  gint                n_segs_in;
  gint                n_segs_out;
  gint                offset_x, offset_y;

  if (! ligma_channel_boundary (channel, &segs_in, &segs_out,
                               &n_segs_in, &n_segs_out,
                               0, 0, 0, 0))
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("Cannot fill empty channel."));
      return FALSE;
    }

  ligma_item_get_offset (item, &offset_x, &offset_y);

  ligma_drawable_fill_boundary (drawable,
                               fill_options,
                               segs_in, n_segs_in,
                               offset_x, offset_y,
                               push_undo);

  return TRUE;
}

static gboolean
ligma_channel_stroke (LigmaItem           *item,
                     LigmaDrawable       *drawable,
                     LigmaStrokeOptions  *stroke_options,
                     gboolean            push_undo,
                     LigmaProgress       *progress,
                     GError            **error)
{
  LigmaChannel        *channel = LIGMA_CHANNEL (item);
  const LigmaBoundSeg *segs_in;
  const LigmaBoundSeg *segs_out;
  gint                n_segs_in;
  gint                n_segs_out;
  gboolean            retval = FALSE;
  gint                offset_x, offset_y;

  if (! ligma_channel_boundary (channel, &segs_in, &segs_out,
                               &n_segs_in, &n_segs_out,
                               0, 0, 0, 0))
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("Cannot stroke empty channel."));
      return FALSE;
    }

  ligma_item_get_offset (item, &offset_x, &offset_y);

  switch (ligma_stroke_options_get_method (stroke_options))
    {
    case LIGMA_STROKE_LINE:
      ligma_drawable_stroke_boundary (drawable,
                                     stroke_options,
                                     n_segs_in > 0 ? segs_in   : segs_out,
                                     n_segs_in > 0 ? n_segs_in : n_segs_out,
                                     offset_x, offset_y,
                                     push_undo);
      retval = TRUE;
      break;

    case LIGMA_STROKE_PAINT_METHOD:
      {
        LigmaPaintInfo    *paint_info;
        LigmaPaintCore    *core;
        LigmaPaintOptions *paint_options;
        gboolean          emulate_dynamics;

        paint_info = ligma_context_get_paint_info (LIGMA_CONTEXT (stroke_options));

        core = g_object_new (paint_info->paint_type, NULL);

        paint_options = ligma_stroke_options_get_paint_options (stroke_options);
        emulate_dynamics = ligma_stroke_options_get_emulate_dynamics (stroke_options);

        retval = ligma_paint_core_stroke_boundary (core, drawable,
                                                  paint_options,
                                                  emulate_dynamics,
                                                  n_segs_in > 0 ? segs_in   : segs_out,
                                                  n_segs_in > 0 ? n_segs_in : n_segs_out,
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
ligma_channel_to_selection (LigmaItem       *item,
                           LigmaChannelOps  op,
                           gboolean        antialias,
                           gboolean        feather,
                           gdouble         feather_radius_x,
                           gdouble         feather_radius_y)
{
  LigmaChannel *channel = LIGMA_CHANNEL (item);
  LigmaImage   *image   = ligma_item_get_image (item);
  gint         off_x, off_y;

  ligma_item_get_offset (item, &off_x, &off_y);

  ligma_channel_select_channel (ligma_image_get_mask (image),
                               LIGMA_ITEM_GET_CLASS (item)->to_selection_desc,
                               channel, off_x, off_y,
                               op,
                               feather, feather_radius_x, feather_radius_x);
}

static void
ligma_channel_convert_type (LigmaDrawable     *drawable,
                           LigmaImage        *dest_image,
                           const Babl       *new_format,
                           LigmaColorProfile *src_profile,
                           LigmaColorProfile *dest_profile,
                           GeglDitherMethod  layer_dither_type,
                           GeglDitherMethod  mask_dither_type,
                           gboolean          push_undo,
                           LigmaProgress     *progress)
{
  GeglBuffer *dest_buffer;

  dest_buffer =
    gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                     ligma_item_get_width  (LIGMA_ITEM (drawable)),
                                     ligma_item_get_height (LIGMA_ITEM (drawable))),
                     new_format);

  if (mask_dither_type == GEGL_DITHER_NONE)
    {
      ligma_gegl_buffer_copy (ligma_drawable_get_buffer (drawable), NULL,
                             GEGL_ABYSS_NONE,
                             dest_buffer, NULL);
    }
  else
    {
      gint bits;

      bits = (babl_format_get_bytes_per_pixel (new_format) * 8 /
              babl_format_get_n_components (new_format));

      ligma_gegl_apply_dither (ligma_drawable_get_buffer (drawable),
                              NULL, NULL,
                              dest_buffer, 1 << bits, mask_dither_type);
    }

  ligma_drawable_set_buffer (drawable, push_undo, NULL, dest_buffer);
  g_object_unref (dest_buffer);
}

static void
ligma_channel_invalidate_boundary (LigmaDrawable *drawable)
{
  LigmaChannel *channel = LIGMA_CHANNEL (drawable);

  channel->boundary_known = FALSE;
  channel->bounds_known   = FALSE;
}

static void
ligma_channel_get_active_components (LigmaDrawable *drawable,
                                    gboolean     *active)
{
  /*  Make sure that the alpha channel is not valid.  */
  active[GRAY]    = TRUE;
  active[ALPHA_G] = FALSE;
}

static void
ligma_channel_set_buffer (LigmaDrawable        *drawable,
                         gboolean             push_undo,
                         const gchar         *undo_desc,
                         GeglBuffer          *buffer,
                         const GeglRectangle *bounds)
{
  LigmaChannel *channel    = LIGMA_CHANNEL (drawable);
  GeglBuffer  *old_buffer = ligma_drawable_get_buffer (drawable);

  if (old_buffer)
    {
      g_signal_handlers_disconnect_by_func (old_buffer,
                                            ligma_channel_buffer_changed,
                                            channel);
    }

  LIGMA_DRAWABLE_CLASS (parent_class)->set_buffer (drawable,
                                                  push_undo, undo_desc,
                                                  buffer, bounds);

  gegl_buffer_signal_connect (buffer, "changed",
                              G_CALLBACK (ligma_channel_buffer_changed),
                              channel);

  if (ligma_filter_peek_node (LIGMA_FILTER (channel)))
    {
      const Babl *color_format =
        ligma_babl_format (LIGMA_RGB,
                          ligma_babl_precision (LIGMA_COMPONENT_TYPE_FLOAT,
                                               ligma_drawable_get_trc (drawable)),
                          TRUE, NULL);

      gegl_node_set (channel->color_node,
                     "format", color_format,
                     NULL);
    }
}

static gdouble
ligma_channel_get_opacity_at (LigmaPickable *pickable,
                             gint          x,
                             gint          y)
{
  LigmaChannel *channel = LIGMA_CHANNEL (pickable);
  gdouble      value   = LIGMA_OPACITY_TRANSPARENT;

  if (x >= 0 && x < ligma_item_get_width  (LIGMA_ITEM (channel)) &&
      y >= 0 && y < ligma_item_get_height (LIGMA_ITEM (channel)))
    {
      if (! channel->bounds_known ||
          (! channel->empty &&
           x >= channel->x1 &&
           x <  channel->x2 &&
           y >= channel->y1 &&
           y <  channel->y2))
        {
          gegl_buffer_sample (ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel)),
                              x, y, NULL, &value, babl_format ("Y double"),
                              GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
        }
    }

  return value;
}

static gboolean
ligma_channel_real_boundary (LigmaChannel         *channel,
                            const LigmaBoundSeg **segs_in,
                            const LigmaBoundSeg **segs_out,
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

      if (ligma_item_bounds (LIGMA_ITEM (channel), &x3, &y3, &x4, &y4))
        {
          GeglBuffer    *buffer;
          GeglRectangle  rect = { x3, y3, x4, y4 };

          x4 += x3;
          y4 += y3;

          buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel));

          channel->segs_out = ligma_boundary_find (buffer, &rect,
                                                  babl_format ("Y float"),
                                                  LIGMA_BOUNDARY_IGNORE_BOUNDS,
                                                  x1, y1, x2, y2,
                                                  LIGMA_BOUNDARY_HALF_WAY,
                                                  &channel->num_segs_out);
          x1 = MAX (x1, x3);
          y1 = MAX (y1, y3);
          x2 = MIN (x2, x4);
          y2 = MIN (y2, y4);

          if (x2 > x1 && y2 > y1)
            {
              channel->segs_in = ligma_boundary_find (buffer, NULL,
                                                     babl_format ("Y float"),
                                                     LIGMA_BOUNDARY_WITHIN_BOUNDS,
                                                     x1, y1, x2, y2,
                                                     LIGMA_BOUNDARY_HALF_WAY,
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
ligma_channel_real_is_empty (LigmaChannel *channel)
{
  GeglBuffer *buffer;

  if (channel->bounds_known)
    return channel->empty;

  buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel));

  if (! ligma_gegl_mask_is_empty (buffer))
    return FALSE;

  /*  The mask is empty, meaning we can set the bounds as known  */
  g_clear_pointer (&channel->segs_in,  g_free);
  g_clear_pointer (&channel->segs_out, g_free);

  channel->empty          = TRUE;
  channel->num_segs_in    = 0;
  channel->num_segs_out   = 0;
  channel->bounds_known   = TRUE;
  channel->boundary_known = TRUE;
  channel->x1             = 0;
  channel->y1             = 0;
  channel->x2             = ligma_item_get_width  (LIGMA_ITEM (channel));
  channel->y2             = ligma_item_get_height (LIGMA_ITEM (channel));

  return TRUE;
}

static void
ligma_channel_real_feather (LigmaChannel *channel,
                           gdouble      radius_x,
                           gdouble      radius_y,
                           gboolean     edge_lock,
                           gboolean     push_undo)
{
  gint x1, y1, x2, y2;

  if (radius_x <= 0.0 && radius_y <= 0.0)
    return;

  if (! ligma_item_bounds (LIGMA_ITEM (channel), &x1, &y1, &x2, &y2))
    return;

  x2 += x1;
  y2 += y1;

  if (ligma_channel_is_empty (channel))
    return;

  x1 = MAX (0, x1 - ceil (radius_x));
  y1 = MAX (0, y1 - ceil (radius_y));

  x2 = MIN (ligma_item_get_width  (LIGMA_ITEM (channel)), x2 + ceil (radius_x));
  y2 = MIN (ligma_item_get_height (LIGMA_ITEM (channel)), y2 + ceil (radius_y));

  if (push_undo)
    ligma_channel_push_undo (channel,
                            LIGMA_CHANNEL_GET_CLASS (channel)->feather_desc);

  ligma_gegl_apply_feather (ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel)),
                           NULL, NULL,
                           ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel)),
                           GEGL_RECTANGLE (x1, y1, x2 - x1, y2 - y1),
                           radius_x,
                           radius_y,
                           edge_lock);

  ligma_drawable_update (LIGMA_DRAWABLE (channel), 0, 0, -1, -1);
}

static void
ligma_channel_real_sharpen (LigmaChannel *channel,
                           gboolean     push_undo)
{
  LigmaDrawable *drawable = LIGMA_DRAWABLE (channel);

  if (push_undo)
    ligma_channel_push_undo (channel,
                            LIGMA_CHANNEL_GET_CLASS (channel)->sharpen_desc);

  ligma_gegl_apply_threshold (ligma_drawable_get_buffer (drawable),
                             NULL, NULL,
                             ligma_drawable_get_buffer (drawable),
                             0.5);

  ligma_drawable_update (LIGMA_DRAWABLE (channel), 0, 0, -1, -1);
}

static void
ligma_channel_real_clear (LigmaChannel *channel,
                         const gchar *undo_desc,
                         gboolean     push_undo)
{
  GeglBuffer    *buffer;
  GeglRectangle  rect;
  GeglRectangle  aligned_rect;

  if (channel->bounds_known && channel->empty)
    return;

  if (push_undo)
    {
      if (! undo_desc)
        undo_desc = LIGMA_CHANNEL_GET_CLASS (channel)->clear_desc;

      ligma_channel_push_undo (channel, undo_desc);
    }

  buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel));

  if (channel->bounds_known)
    {
      rect.x      = channel->x1;
      rect.y      = channel->y1;
      rect.width  = channel->x2 - channel->x1;
      rect.height = channel->y2 - channel->y1;
    }
  else
    {
      rect.x      = 0;
      rect.y      = 0;
      rect.width  = ligma_item_get_width  (LIGMA_ITEM (channel));
      rect.height = ligma_item_get_height (LIGMA_ITEM (channel));
    }

  gegl_rectangle_align_to_buffer (&aligned_rect, &rect, buffer,
                                  GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

  gegl_buffer_clear (buffer, &aligned_rect);

  /*  we know the bounds  */
  channel->bounds_known = TRUE;
  channel->empty        = TRUE;
  channel->x1           = 0;
  channel->y1           = 0;
  channel->x2           = ligma_item_get_width  (LIGMA_ITEM (channel));
  channel->y2           = ligma_item_get_height (LIGMA_ITEM (channel));

  ligma_drawable_update (LIGMA_DRAWABLE (channel),
                        rect.x, rect.y, rect.width, rect.height);
}

static void
ligma_channel_real_all (LigmaChannel *channel,
                       gboolean     push_undo)
{
  GeglColor *color;

  if (push_undo)
    ligma_channel_push_undo (channel,
                            LIGMA_CHANNEL_GET_CLASS (channel)->all_desc);

  /*  clear the channel  */
  color = gegl_color_new ("#fff");
  gegl_buffer_set_color (ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel)),
                         NULL, color);
  g_object_unref (color);

  /*  we know the bounds  */
  channel->bounds_known = TRUE;
  channel->empty        = FALSE;
  channel->x1           = 0;
  channel->y1           = 0;
  channel->x2           = ligma_item_get_width  (LIGMA_ITEM (channel));
  channel->y2           = ligma_item_get_height (LIGMA_ITEM (channel));

  ligma_drawable_update (LIGMA_DRAWABLE (channel), 0, 0, -1, -1);
}

static void
ligma_channel_real_invert (LigmaChannel *channel,
                          gboolean     push_undo)
{
  LigmaDrawable *drawable = LIGMA_DRAWABLE (channel);

  if (push_undo)
    ligma_channel_push_undo (channel,
                            LIGMA_CHANNEL_GET_CLASS (channel)->invert_desc);

  if (channel->bounds_known && channel->empty)
    {
      ligma_channel_all (channel, FALSE);
    }
  else
    {
      ligma_gegl_apply_invert_linear (ligma_drawable_get_buffer (drawable),
                                     NULL, NULL,
                                     ligma_drawable_get_buffer (drawable));

      ligma_drawable_update (LIGMA_DRAWABLE (channel), 0, 0, -1, -1);
    }
}

static void
ligma_channel_real_border (LigmaChannel            *channel,
                          gint                    radius_x,
                          gint                    radius_y,
                          LigmaChannelBorderStyle  style,
                          gboolean                edge_lock,
                          gboolean                push_undo)
{
  gint x1, y1, x2, y2;

  if (radius_x == 0 && radius_y == 0)
    {
      /* The relevant GEGL operations require radius_x and radius_y to be > 0.
       * When both are 0 (currently can only be achieved by the user through
       * PDB), the effect should be to clear the channel.
       */
      ligma_channel_clear (channel,
                          LIGMA_CHANNEL_GET_CLASS (channel)->border_desc,
                          push_undo);
      return;
    }
  else if (radius_x <= 0 || radius_y <= 0)
    {
      /* FIXME: Implement the case where only one of radius_x and radius_y is 0.
       * Currently, should never happen.
       */
      g_return_if_reached();
    }

  if (! ligma_item_bounds (LIGMA_ITEM (channel), &x1, &y1, &x2, &y2))
    return;

  x2 += x1;
  y2 += y1;

  if (ligma_channel_is_empty (channel))
    return;

  if (x1 - radius_x < 0)
    x1 = 0;
  else
    x1 -= radius_x;

  if (x2 + radius_x > ligma_item_get_width (LIGMA_ITEM (channel)))
    x2 = ligma_item_get_width (LIGMA_ITEM (channel));
  else
    x2 += radius_x;

  if (y1 - radius_y < 0)
    y1 = 0;
  else
    y1 -= radius_y;

  if (y2 + radius_y > ligma_item_get_height (LIGMA_ITEM (channel)))
    y2 = ligma_item_get_height (LIGMA_ITEM (channel));
  else
    y2 += radius_y;

  if (push_undo)
    ligma_channel_push_undo (channel,
                            LIGMA_CHANNEL_GET_CLASS (channel)->border_desc);

  ligma_gegl_apply_border (ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel)),
                          NULL, NULL,
                          ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel)),
                          GEGL_RECTANGLE (x1, y1, x2 - x1, y2 - y1),
                          radius_x, radius_y, style, edge_lock);

  ligma_drawable_update (LIGMA_DRAWABLE (channel), 0, 0, -1, -1);
}

static void
ligma_channel_real_grow (LigmaChannel *channel,
                        gint         radius_x,
                        gint         radius_y,
                        gboolean     push_undo)
{
  gint x1, y1, x2, y2;

  if (radius_x == 0 && radius_y == 0)
    return;

  if (radius_x <= 0 && radius_y <= 0)
    {
      ligma_channel_shrink (channel, -radius_x, -radius_y, FALSE, push_undo);
      return;
    }

  if (radius_x < 0 || radius_y < 0)
    return;

  if (! ligma_item_bounds (LIGMA_ITEM (channel), &x1, &y1, &x2, &y2))
    return;

  x2 += x1;
  y2 += y1;

  if (ligma_channel_is_empty (channel))
    return;

  if (x1 - radius_x > 0)
    x1 = x1 - radius_x;
  else
    x1 = 0;

  if (y1 - radius_y > 0)
    y1 = y1 - radius_y;
  else
    y1 = 0;

  if (x2 + radius_x < ligma_item_get_width (LIGMA_ITEM (channel)))
    x2 = x2 + radius_x;
  else
    x2 = ligma_item_get_width (LIGMA_ITEM (channel));

  if (y2 + radius_y < ligma_item_get_height (LIGMA_ITEM (channel)))
    y2 = y2 + radius_y;
  else
    y2 = ligma_item_get_height (LIGMA_ITEM (channel));

  if (push_undo)
    ligma_channel_push_undo (channel,
                            LIGMA_CHANNEL_GET_CLASS (channel)->grow_desc);

  ligma_gegl_apply_grow (ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel)),
                        NULL, NULL,
                        ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel)),
                        GEGL_RECTANGLE (x1, y1, x2 - x1, y2 - y1),
                        radius_x, radius_y);

  ligma_drawable_update (LIGMA_DRAWABLE (channel), 0, 0, -1, -1);
}

static void
ligma_channel_real_shrink (LigmaChannel *channel,
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
      ligma_channel_grow (channel, -radius_x, -radius_y, push_undo);
      return;
    }

  if (radius_x < 0 || radius_y < 0)
    return;

  if (! ligma_item_bounds (LIGMA_ITEM (channel), &x1, &y1, &x2, &y2))
    return;

  x2 += x1;
  y2 += y1;

  if (ligma_channel_is_empty (channel))
    return;

  if (x1 > 0)
    x1--;
  if (y1 > 0)
    y1--;
  if (x2 < ligma_item_get_width (LIGMA_ITEM (channel)))
    x2++;
  if (y2 < ligma_item_get_height (LIGMA_ITEM (channel)))
    y2++;

  if (push_undo)
    ligma_channel_push_undo (channel,
                            LIGMA_CHANNEL_GET_CLASS (channel)->shrink_desc);

  ligma_gegl_apply_shrink (ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel)),
                          NULL, NULL,
                          ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel)),
                          GEGL_RECTANGLE (x1, y1, x2 - x1, y2 - y1),
                          radius_x, radius_y, edge_lock);

  ligma_drawable_update (LIGMA_DRAWABLE (channel), 0, 0, -1, -1);
}

static void
ligma_channel_real_flood (LigmaChannel *channel,
                         gboolean     push_undo)
{
  gint x, y, width, height;

  if (! ligma_item_bounds (LIGMA_ITEM (channel), &x, &y, &width, &height))
    return;

  if (ligma_channel_is_empty (channel))
    return;

  if (push_undo)
    ligma_channel_push_undo (channel,
                            LIGMA_CHANNEL_GET_CLASS (channel)->flood_desc);

  ligma_gegl_apply_flood (ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel)),
                         NULL, NULL,
                         ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel)),
                         GEGL_RECTANGLE (x, y, width, height));

  ligma_drawable_update (LIGMA_DRAWABLE (channel), x, y, width, height);
}

static void
ligma_channel_buffer_changed (GeglBuffer          *buffer,
                             const GeglRectangle *rect,
                             LigmaChannel         *channel)
{
  ligma_drawable_invalidate_boundary (LIGMA_DRAWABLE (channel));
}


/*  public functions  */

LigmaChannel *
ligma_channel_new (LigmaImage     *image,
                  gint           width,
                  gint           height,
                  const gchar   *name,
                  const LigmaRGB *color)
{
  LigmaChannel *channel;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  channel =
    LIGMA_CHANNEL (ligma_drawable_new (LIGMA_TYPE_CHANNEL,
                                     image, name,
                                     0, 0, width, height,
                                     ligma_image_get_channel_format (image)));

  if (color)
    channel->color = *color;

  channel->show_masked = TRUE;

  /*  selection mask variables  */
  channel->x2          = width;
  channel->y2          = height;

  return channel;
}

LigmaChannel *
ligma_channel_new_from_buffer (LigmaImage     *image,
                              GeglBuffer    *buffer,
                              const gchar   *name,
                              const LigmaRGB *color)
{
  LigmaChannel *channel;
  GeglBuffer  *dest;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  channel = ligma_channel_new (image,
                              gegl_buffer_get_width  (buffer),
                              gegl_buffer_get_height (buffer),
                              name, color);

  dest = ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel));
  ligma_gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE, dest, NULL);

  return channel;
}

LigmaChannel *
ligma_channel_new_from_alpha (LigmaImage     *image,
                             LigmaDrawable  *drawable,
                             const gchar   *name,
                             const LigmaRGB *color)
{
  LigmaChannel *channel;
  GeglBuffer  *dest_buffer;
  gint         width;
  gint         height;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (ligma_drawable_has_alpha (drawable), NULL);

  width  = ligma_item_get_width  (LIGMA_ITEM (drawable));
  height = ligma_item_get_height (LIGMA_ITEM (drawable));

  channel = ligma_channel_new (image, width, height, name, color);

  ligma_channel_clear (channel, NULL, FALSE);

  dest_buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel));

  gegl_buffer_set_format (dest_buffer,
                          ligma_drawable_get_component_format (drawable,
                                                              LIGMA_CHANNEL_ALPHA));
  ligma_gegl_buffer_copy (ligma_drawable_get_buffer (drawable), NULL,
                         GEGL_ABYSS_NONE,
                         dest_buffer, NULL);
  gegl_buffer_set_format (dest_buffer, NULL);

  return channel;
}

LigmaChannel *
ligma_channel_new_from_component (LigmaImage       *image,
                                 LigmaChannelType  type,
                                 const gchar     *name,
                                 const LigmaRGB   *color)
{
  LigmaChannel *channel;
  GeglBuffer  *src_buffer;
  GeglBuffer  *dest_buffer;
  gint         width;
  gint         height;
  const Babl  *format;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  format = ligma_image_get_component_format (image, type);

  g_return_val_if_fail (format != NULL, NULL);

  ligma_pickable_flush (LIGMA_PICKABLE (image));

  src_buffer = ligma_pickable_get_buffer (LIGMA_PICKABLE (image));
  width  = gegl_buffer_get_width  (src_buffer);
  height = gegl_buffer_get_height (src_buffer);

  channel = ligma_channel_new (image, width, height, name, color);

  dest_buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel));

  gegl_buffer_set_format (dest_buffer, format);
  ligma_gegl_buffer_copy (src_buffer, NULL, GEGL_ABYSS_NONE, dest_buffer, NULL);
  gegl_buffer_set_format (dest_buffer, NULL);

  return channel;
}

LigmaChannel *
ligma_channel_get_parent (LigmaChannel *channel)
{
  g_return_val_if_fail (LIGMA_IS_CHANNEL (channel), NULL);

  return LIGMA_CHANNEL (ligma_viewable_get_parent (LIGMA_VIEWABLE (channel)));
}

void
ligma_channel_set_color (LigmaChannel   *channel,
                        const LigmaRGB *color,
                        gboolean       push_undo)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));
  g_return_if_fail (color != NULL);

  if (ligma_rgba_distance (&channel->color, color) > RGBA_EPSILON)
    {
      if (push_undo && ligma_item_is_attached (LIGMA_ITEM (channel)))
        {
          LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (channel));

          ligma_image_undo_push_channel_color (image, C_("undo-type", "Set Channel Color"),
                                              channel);
        }

      channel->color = *color;

      if (ligma_filter_peek_node (LIGMA_FILTER (channel)))
        {
          ligma_gegl_node_set_color (channel->color_node,
                                    &channel->color, NULL);
        }

      ligma_drawable_update (LIGMA_DRAWABLE (channel), 0, 0, -1, -1);

      g_signal_emit (channel, channel_signals[COLOR_CHANGED], 0);
    }
}

void
ligma_channel_get_color (LigmaChannel *channel,
                        LigmaRGB     *color)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));
  g_return_if_fail (color != NULL);

  *color = channel->color;
}

gdouble
ligma_channel_get_opacity (LigmaChannel *channel)
{
  g_return_val_if_fail (LIGMA_IS_CHANNEL (channel), LIGMA_OPACITY_TRANSPARENT);

  return channel->color.a;
}

void
ligma_channel_set_opacity (LigmaChannel *channel,
                          gdouble      opacity,
                          gboolean     push_undo)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));

  opacity = CLAMP (opacity, LIGMA_OPACITY_TRANSPARENT, LIGMA_OPACITY_OPAQUE);

  if (channel->color.a != opacity)
    {
      if (push_undo && ligma_item_is_attached (LIGMA_ITEM (channel)))
        {
          LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (channel));

          ligma_image_undo_push_channel_color (image, C_("undo-type", "Set Channel Opacity"),
                                              channel);
        }

      channel->color.a = opacity;

      if (ligma_filter_peek_node (LIGMA_FILTER (channel)))
        {
          ligma_gegl_node_set_color (channel->color_node,
                                    &channel->color, NULL);
        }

      ligma_drawable_update (LIGMA_DRAWABLE (channel), 0, 0, -1, -1);

      g_signal_emit (channel, channel_signals[COLOR_CHANGED], 0);
    }
}

gboolean
ligma_channel_get_show_masked (LigmaChannel *channel)
{
  g_return_val_if_fail (LIGMA_IS_CHANNEL (channel), FALSE);

  return channel->show_masked;
}

void
ligma_channel_set_show_masked (LigmaChannel *channel,
                              gboolean     show_masked)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));

  if (show_masked != channel->show_masked)
    {
      channel->show_masked = show_masked ? TRUE : FALSE;

      if (channel->invert_node)
        {
          GeglNode *source;

          source = ligma_drawable_get_source_node (LIGMA_DRAWABLE (channel));

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

      ligma_drawable_update (LIGMA_DRAWABLE (channel), 0, 0, -1, -1);
    }
}

void
ligma_channel_push_undo (LigmaChannel *channel,
                        const gchar *undo_desc)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (channel)));

  ligma_image_undo_push_mask (ligma_item_get_image (LIGMA_ITEM (channel)),
                             undo_desc, channel);
}


/******************************/
/*  selection mask functions  */
/******************************/

LigmaChannel *
ligma_channel_new_mask (LigmaImage *image,
                       gint       width,
                       gint       height)
{
  LigmaChannel *channel;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  channel =
    LIGMA_CHANNEL (ligma_drawable_new (LIGMA_TYPE_CHANNEL,
                                     image, _("Selection Mask"),
                                     0, 0, width, height,
                                     ligma_image_get_mask_format (image)));

  channel->show_masked = TRUE;
  channel->x2          = width;
  channel->y2          = height;

  gegl_buffer_clear (ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel)),
                     NULL);

  return channel;
}

gboolean
ligma_channel_boundary (LigmaChannel         *channel,
                       const LigmaBoundSeg **segs_in,
                       const LigmaBoundSeg **segs_out,
                       gint                *num_segs_in,
                       gint                *num_segs_out,
                       gint                 x1,
                       gint                 y1,
                       gint                 x2,
                       gint                 y2)
{
  g_return_val_if_fail (LIGMA_IS_CHANNEL (channel), FALSE);
  g_return_val_if_fail (segs_in != NULL, FALSE);
  g_return_val_if_fail (segs_out != NULL, FALSE);
  g_return_val_if_fail (num_segs_in != NULL, FALSE);
  g_return_val_if_fail (num_segs_out != NULL, FALSE);

  return LIGMA_CHANNEL_GET_CLASS (channel)->boundary (channel,
                                                     segs_in, segs_out,
                                                     num_segs_in, num_segs_out,
                                                     x1, y1,
                                                     x2, y2);
}

gboolean
ligma_channel_is_empty (LigmaChannel *channel)
{
  g_return_val_if_fail (LIGMA_IS_CHANNEL (channel), TRUE);

  return LIGMA_CHANNEL_GET_CLASS (channel)->is_empty (channel);
}

void
ligma_channel_feather (LigmaChannel *channel,
                      gdouble      radius_x,
                      gdouble      radius_y,
                      gboolean     edge_lock,
                      gboolean     push_undo)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));

  if (! ligma_item_is_attached (LIGMA_ITEM (channel)))
    push_undo = FALSE;

  LIGMA_CHANNEL_GET_CLASS (channel)->feather (channel, radius_x, radius_y,
                                             edge_lock, push_undo);
}

void
ligma_channel_sharpen (LigmaChannel *channel,
                      gboolean     push_undo)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));

  if (! ligma_item_is_attached (LIGMA_ITEM (channel)))
    push_undo = FALSE;

  LIGMA_CHANNEL_GET_CLASS (channel)->sharpen (channel, push_undo);
}

void
ligma_channel_clear (LigmaChannel *channel,
                    const gchar *undo_desc,
                    gboolean     push_undo)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));

  if (! ligma_item_is_attached (LIGMA_ITEM (channel)))
    push_undo = FALSE;

  LIGMA_CHANNEL_GET_CLASS (channel)->clear (channel, undo_desc, push_undo);
}

void
ligma_channel_all (LigmaChannel *channel,
                  gboolean     push_undo)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));

  if (! ligma_item_is_attached (LIGMA_ITEM (channel)))
    push_undo = FALSE;

  LIGMA_CHANNEL_GET_CLASS (channel)->all (channel, push_undo);
}

void
ligma_channel_invert (LigmaChannel *channel,
                     gboolean     push_undo)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));

  if (! ligma_item_is_attached (LIGMA_ITEM (channel)))
    push_undo = FALSE;

  LIGMA_CHANNEL_GET_CLASS (channel)->invert (channel, push_undo);
}

void
ligma_channel_border (LigmaChannel            *channel,
                     gint                    radius_x,
                     gint                    radius_y,
                     LigmaChannelBorderStyle  style,
                     gboolean                edge_lock,
                     gboolean                push_undo)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));

  if (! ligma_item_is_attached (LIGMA_ITEM (channel)))
    push_undo = FALSE;

  LIGMA_CHANNEL_GET_CLASS (channel)->border (channel,
                                            radius_x, radius_y, style, edge_lock,
                                            push_undo);
}

void
ligma_channel_grow (LigmaChannel *channel,
                   gint         radius_x,
                   gint         radius_y,
                   gboolean     push_undo)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));

  if (! ligma_item_is_attached (LIGMA_ITEM (channel)))
    push_undo = FALSE;

  LIGMA_CHANNEL_GET_CLASS (channel)->grow (channel, radius_x, radius_y,
                                          push_undo);
}

void
ligma_channel_shrink (LigmaChannel  *channel,
                     gint          radius_x,
                     gint          radius_y,
                     gboolean      edge_lock,
                     gboolean      push_undo)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));

  if (! ligma_item_is_attached (LIGMA_ITEM (channel)))
    push_undo = FALSE;

  LIGMA_CHANNEL_GET_CLASS (channel)->shrink (channel, radius_x, radius_y,
                                            edge_lock, push_undo);
}

void
ligma_channel_flood (LigmaChannel *channel,
                    gboolean     push_undo)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));

  if (! ligma_item_is_attached (LIGMA_ITEM (channel)))
    push_undo = FALSE;

  LIGMA_CHANNEL_GET_CLASS (channel)->flood (channel, push_undo);
}
