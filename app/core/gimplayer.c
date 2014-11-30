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

#include <stdlib.h>
#include <string.h>

#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/boundary.h"
#include "base/pixel-region.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "paint-funcs/paint-funcs.h"

#include "gegl/gimp-gegl-utils.h"

#include "gimpchannel-select.h"
#include "gimpcontext.h"
#include "gimpcontainer.h"
#include "gimpdrawable-convert.h"
#include "gimpdrawable-invert.h"
#include "gimperror.h"
#include "gimpimage-undo-push.h"
#include "gimpimage-undo.h"
#include "gimpimage.h"
#include "gimplayer-floating-sel.h"
#include "gimplayer.h"
#include "gimplayer-project.h"
#include "gimplayermask.h"
#include "gimpmarshal.h"
#include "gimppickable.h"

#include "gimp-intl.h"


enum
{
  OPACITY_CHANGED,
  MODE_CHANGED,
  LOCK_ALPHA_CHANGED,
  MASK_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_OPACITY,
  PROP_MODE,
  PROP_LOCK_ALPHA,
  PROP_MASK,
  PROP_FLOATING_SELECTION
};


static void   gimp_layer_pickable_iface_init (GimpPickableInterface *iface);

static void       gimp_layer_set_property       (GObject            *object,
                                                 guint               property_id,
                                                 const GValue       *value,
                                                 GParamSpec         *pspec);
static void       gimp_layer_get_property       (GObject            *object,
                                                 guint               property_id,
                                                 GValue             *value,
                                                 GParamSpec         *pspec);
static void       gimp_layer_dispose            (GObject            *object);
static void       gimp_layer_finalize           (GObject            *object);

static void       gimp_layer_name_changed       (GimpObject         *object);
static gint64     gimp_layer_get_memsize        (GimpObject         *object,
                                                 gint64             *gui_size);

static void       gimp_layer_invalidate_preview (GimpViewable       *viewable);
static gchar    * gimp_layer_get_description    (GimpViewable       *viewable,
                                                 gchar             **tooltip);

static void       gimp_layer_removed            (GimpItem           *item);
static void       gimp_layer_unset_removed      (GimpItem           *item);
static gboolean   gimp_layer_is_attached        (const GimpItem     *item);
static GimpItemTree * gimp_layer_get_tree       (GimpItem           *item);
static GimpItem * gimp_layer_duplicate          (GimpItem           *item,
                                                 GType               new_type);
static void       gimp_layer_convert            (GimpItem           *item,
                                                 GimpImage          *dest_image);
static gboolean   gimp_layer_rename             (GimpItem           *item,
                                                 const gchar        *new_name,
                                                 const gchar        *undo_desc,
                                                 GError            **error);
static void       gimp_layer_translate          (GimpItem           *item,
                                                 gint                offset_x,
                                                 gint                offset_y,
                                                 gboolean            push_undo);
static void       gimp_layer_scale              (GimpItem           *item,
                                                 gint                new_width,
                                                 gint                new_height,
                                                 gint                new_offset_x,
                                                 gint                new_offset_y,
                                                 GimpInterpolationType  interp_type,
                                                 GimpProgress       *progress);
static void       gimp_layer_resize             (GimpItem           *item,
                                                 GimpContext        *context,
                                                 gint                new_width,
                                                 gint                new_height,
                                                 gint                offset_x,
                                                 gint                offset_y);
static void       gimp_layer_flip               (GimpItem           *item,
                                                 GimpContext        *context,
                                                 GimpOrientationType flip_type,
                                                 gdouble             axis,
                                                 gboolean            clip_result);
static void       gimp_layer_rotate             (GimpItem           *item,
                                                 GimpContext        *context,
                                                 GimpRotationType    rotate_type,
                                                 gdouble             center_x,
                                                 gdouble             center_y,
                                                 gboolean            clip_result);
static void       gimp_layer_transform          (GimpItem           *item,
                                                 GimpContext        *context,
                                                 const GimpMatrix3  *matrix,
                                                 GimpTransformDirection direction,
                                                 GimpInterpolationType  interpolation_type,
                                                 gint                recursion_level,
                                                 GimpTransformResize clip_result,
                                                 GimpProgress       *progress);
static void       gimp_layer_to_selection       (GimpItem           *item,
                                                 GimpChannelOps      op,
                                                 gboolean            antialias,
                                                 gboolean            feather,
                                                 gdouble             feather_radius_x,
                                                 gdouble             feather_radius_y);
static GeglNode * gimp_layer_get_node           (GimpItem           *item);

static gint64  gimp_layer_estimate_memsize      (const GimpDrawable *drawable,
                                                 gint                width,
                                                 gint                height);
static void    gimp_layer_invalidate_boundary   (GimpDrawable       *drawable);
static void    gimp_layer_get_active_components (const GimpDrawable *drawable,
                                                 gboolean           *active);
static void    gimp_layer_convert_type          (GimpDrawable       *drawable,
                                                 GimpImage          *dest_image,
                                                 GimpImageBaseType   new_base_type,
                                                 gboolean            push_undo);

static gint    gimp_layer_get_opacity_at        (GimpPickable       *pickable,
                                                 gint                x,
                                                 gint                y);

static void       gimp_layer_transform_color    (GimpImage          *image,
                                                 PixelRegion        *srcPR,
                                                 GimpImageType       src_type,
                                                 PixelRegion        *destPR,
                                                 GimpImageType       dest_type);

static void       gimp_layer_layer_mask_update  (GimpDrawable       *layer_mask,
                                                 gint                x,
                                                 gint                y,
                                                 gint                width,
                                                 gint                height,
                                                 GimpLayer          *layer);

static void       gimp_layer_sync_mode_node     (GimpLayer          *layer);


G_DEFINE_TYPE_WITH_CODE (GimpLayer, gimp_layer, GIMP_TYPE_DRAWABLE,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PICKABLE,
                                                gimp_layer_pickable_iface_init))

#define parent_class gimp_layer_parent_class

static guint layer_signals[LAST_SIGNAL] = { 0 };


static void
gimp_layer_class_init (GimpLayerClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class        = GIMP_ITEM_CLASS (klass);
  GimpDrawableClass *drawable_class    = GIMP_DRAWABLE_CLASS (klass);

  layer_signals[OPACITY_CHANGED] =
    g_signal_new ("opacity-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLayerClass, opacity_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  layer_signals[MODE_CHANGED] =
    g_signal_new ("mode-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLayerClass, mode_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  layer_signals[LOCK_ALPHA_CHANGED] =
    g_signal_new ("lock-alpha-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLayerClass, lock_alpha_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  layer_signals[MASK_CHANGED] =
    g_signal_new ("mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLayerClass, mask_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->set_property          = gimp_layer_set_property;
  object_class->get_property          = gimp_layer_get_property;
  object_class->dispose               = gimp_layer_dispose;
  object_class->finalize              = gimp_layer_finalize;

  gimp_object_class->name_changed     = gimp_layer_name_changed;
  gimp_object_class->get_memsize      = gimp_layer_get_memsize;

  viewable_class->default_stock_id    = "gimp-layer";
  viewable_class->invalidate_preview  = gimp_layer_invalidate_preview;
  viewable_class->get_description     = gimp_layer_get_description;

  item_class->removed                 = gimp_layer_removed;
  item_class->unset_removed           = gimp_layer_unset_removed;
  item_class->is_attached             = gimp_layer_is_attached;
  item_class->get_tree                = gimp_layer_get_tree;
  item_class->duplicate               = gimp_layer_duplicate;
  item_class->convert                 = gimp_layer_convert;
  item_class->rename                  = gimp_layer_rename;
  item_class->translate               = gimp_layer_translate;
  item_class->scale                   = gimp_layer_scale;
  item_class->resize                  = gimp_layer_resize;
  item_class->flip                    = gimp_layer_flip;
  item_class->rotate                  = gimp_layer_rotate;
  item_class->transform               = gimp_layer_transform;
  item_class->to_selection            = gimp_layer_to_selection;
  item_class->get_node                = gimp_layer_get_node;
  item_class->default_name            = _("Layer");
  item_class->rename_desc             = C_("undo-type", "Rename Layer");
  item_class->translate_desc          = C_("undo-type", "Move Layer");
  item_class->scale_desc              = C_("undo-type", "Scale Layer");
  item_class->resize_desc             = C_("undo-type", "Resize Layer");
  item_class->flip_desc               = C_("undo-type", "Flip Layer");
  item_class->rotate_desc             = C_("undo-type", "Rotate Layer");
  item_class->transform_desc          = C_("undo-type", "Transform Layer");
  item_class->to_selection_desc       = C_("undo-type", "Alpha to Selection");
  item_class->reorder_desc            = C_("undo-type", "Reorder Layer");
  item_class->raise_desc              = C_("undo-type", "Raise Layer");
  item_class->raise_to_top_desc       = C_("undo-type", "Raise Layer to Top");
  item_class->lower_desc              = C_("undo-type", "Lower Layer");
  item_class->lower_to_bottom_desc    = C_("undo-type", "Lower Layer to Bottom");
  item_class->raise_failed            = _("Layer cannot be raised higher.");
  item_class->lower_failed            = _("Layer cannot be lowered more.");

  drawable_class->estimate_memsize      = gimp_layer_estimate_memsize;
  drawable_class->invalidate_boundary   = gimp_layer_invalidate_boundary;
  drawable_class->get_active_components = gimp_layer_get_active_components;
  drawable_class->convert_type          = gimp_layer_convert_type;
  drawable_class->project_region        = gimp_layer_project_region;

  klass->opacity_changed              = NULL;
  klass->mode_changed                 = NULL;
  klass->lock_alpha_changed           = NULL;
  klass->mask_changed                 = NULL;

  g_object_class_install_property (object_class, PROP_OPACITY,
                                   g_param_spec_double ("opacity", NULL, NULL,
                                                        GIMP_OPACITY_TRANSPARENT,
                                                        GIMP_OPACITY_OPAQUE,
                                                        GIMP_OPACITY_OPAQUE,
                                                        GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_MODE,
                                   g_param_spec_enum ("mode", NULL, NULL,
                                                      GIMP_TYPE_LAYER_MODE_EFFECTS,
                                                      GIMP_NORMAL_MODE,
                                                      GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_LOCK_ALPHA,
                                   g_param_spec_boolean ("lock-alpha",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_MASK,
                                   g_param_spec_object ("mask",
                                                        NULL, NULL,
                                                        GIMP_TYPE_LAYER_MASK,
                                                        GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_FLOATING_SELECTION,
                                   g_param_spec_boolean ("floating-selection",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READABLE));
}

static void
gimp_layer_init (GimpLayer *layer)
{
  layer->opacity    = GIMP_OPACITY_OPAQUE;
  layer->mode       = GIMP_NORMAL_MODE;
  layer->lock_alpha = FALSE;

  layer->mask       = NULL;

  /*  floating selection  */
  layer->fs.drawable       = NULL;
  layer->fs.boundary_known = FALSE;
  layer->fs.segs           = NULL;
  layer->fs.num_segs       = 0;
}

static void
gimp_layer_pickable_iface_init (GimpPickableInterface *iface)
{
  iface->get_opacity_at = gimp_layer_get_opacity_at;
}

static void
gimp_layer_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_layer_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  GimpLayer *layer = GIMP_LAYER (object);

  switch (property_id)
    {
    case PROP_OPACITY:
      g_value_set_double (value, gimp_layer_get_opacity (layer));
      break;
    case PROP_MODE:
      g_value_set_enum (value, gimp_layer_get_mode (layer));
      break;
    case PROP_LOCK_ALPHA:
      g_value_set_boolean (value, gimp_layer_get_lock_alpha (layer));
      break;
    case PROP_MASK:
      g_value_set_object (value, gimp_layer_get_mask (layer));
      break;
    case PROP_FLOATING_SELECTION:
      g_value_set_boolean (value, gimp_layer_is_floating_sel (layer));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_layer_dispose (GObject *object)
{
  GimpLayer *layer = GIMP_LAYER (object);

  if (layer->mask)
    g_signal_handlers_disconnect_by_func (layer->mask,
                                          gimp_layer_layer_mask_update,
                                          layer);

  if (gimp_layer_is_floating_sel (layer))
    {
      GimpDrawable *fs_drawable = gimp_layer_get_floating_sel_drawable (layer);

      /* only detach if this is actually the drawable's fs because the
       * layer might be on the undo stack and not attached to anyhing
       */
      if (gimp_drawable_get_floating_sel (fs_drawable) == layer)
        gimp_drawable_detach_floating_sel (fs_drawable);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_layer_finalize (GObject *object)
{
  GimpLayer *layer = GIMP_LAYER (object);

  if (layer->mask)
    {
      g_object_unref (layer->mask);
      layer->mask = NULL;
    }

  if (layer->fs.segs)
    {
      g_free (layer->fs.segs);
      layer->fs.segs     = NULL;
      layer->fs.num_segs = 0;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_layer_name_changed (GimpObject *object)
{
  GimpLayer *layer = GIMP_LAYER (object);

  if (GIMP_OBJECT_CLASS (parent_class)->name_changed)
    GIMP_OBJECT_CLASS (parent_class)->name_changed (object);

  if (layer->mask)
    {
      gchar *mask_name = g_strdup_printf (_("%s mask"),
                                          gimp_object_get_name (object));

      gimp_object_take_name (GIMP_OBJECT (layer->mask), mask_name);
    }
}

static gint64
gimp_layer_get_memsize (GimpObject *object,
                        gint64     *gui_size)
{
  GimpLayer *layer   = GIMP_LAYER (object);
  gint64     memsize = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (layer->mask), gui_size);

  *gui_size += layer->fs.num_segs * sizeof (BoundSeg);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_layer_invalidate_preview (GimpViewable *viewable)
{
  GimpLayer *layer = GIMP_LAYER (viewable);

  GIMP_VIEWABLE_CLASS (parent_class)->invalidate_preview (viewable);

  if (gimp_layer_is_floating_sel (layer))
    floating_sel_invalidate (layer);
}

static gchar *
gimp_layer_get_description (GimpViewable  *viewable,
                            gchar        **tooltip)
{
  if (gimp_layer_is_floating_sel (GIMP_LAYER (viewable)))
    {
      return g_strdup_printf (_("Floating Selection\n(%s)"),
                              gimp_object_get_name (viewable));
    }

  return GIMP_VIEWABLE_CLASS (parent_class)->get_description (viewable,
                                                              tooltip);
}

static void
gimp_layer_removed (GimpItem *item)
{
  GimpLayer *layer = GIMP_LAYER (item);

  if (layer->mask)
    gimp_item_removed (GIMP_ITEM (layer->mask));

  if (GIMP_ITEM_CLASS (parent_class)->removed)
    GIMP_ITEM_CLASS (parent_class)->removed (item);
}

static void
gimp_layer_unset_removed (GimpItem *item)
{
  GimpLayer *layer = GIMP_LAYER (item);

  if (layer->mask)
    gimp_item_unset_removed (GIMP_ITEM (layer->mask));

  if (GIMP_ITEM_CLASS (parent_class)->unset_removed)
    GIMP_ITEM_CLASS (parent_class)->unset_removed (item);
}

static gboolean
gimp_layer_is_attached (const GimpItem *item)
{
  GimpImage *image = gimp_item_get_image (item);

  return (GIMP_IS_IMAGE (image) &&
          gimp_container_have (gimp_image_get_layers (image),
                               GIMP_OBJECT (item)));
}

static GimpItemTree *
gimp_layer_get_tree (GimpItem *item)
{
  if (gimp_item_is_attached (item))
    {
      GimpImage *image = gimp_item_get_image (item);

      return gimp_image_get_layer_tree (image);
    }

  return NULL;
}

static GimpItem *
gimp_layer_duplicate (GimpItem *item,
                      GType     new_type)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (GIMP_IS_LAYER (new_item))
    {
      GimpLayer *layer     = GIMP_LAYER (item);
      GimpLayer *new_layer = GIMP_LAYER (new_item);

      gimp_layer_set_mode    (new_layer, gimp_layer_get_mode (layer),    FALSE);
      gimp_layer_set_opacity (new_layer, gimp_layer_get_opacity (layer), FALSE);

      if (gimp_layer_can_lock_alpha (new_layer))
        gimp_layer_set_lock_alpha (new_layer,
                                   gimp_layer_get_lock_alpha (layer), FALSE);

      /*  duplicate the layer mask if necessary  */
      if (layer->mask)
        {
          GimpItem *mask;

          mask = gimp_item_duplicate (GIMP_ITEM (layer->mask),
                                      G_TYPE_FROM_INSTANCE (layer->mask));
          gimp_layer_add_mask (new_layer, GIMP_LAYER_MASK (mask), FALSE, NULL);
        }
    }

  return new_item;
}

static void
gimp_layer_convert (GimpItem  *item,
                    GimpImage *dest_image)
{
  GimpLayer         *layer    = GIMP_LAYER (item);
  GimpDrawable      *drawable = GIMP_DRAWABLE (item);
  GimpImageBaseType  old_base_type;
  GimpImageBaseType  new_base_type;

  old_base_type = GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (drawable));
  new_base_type = gimp_image_base_type (dest_image);

  if (old_base_type != new_base_type)
    gimp_drawable_convert_type (drawable, dest_image, new_base_type, FALSE);

  if (layer->mask)
    gimp_item_set_image (GIMP_ITEM (layer->mask), dest_image);

  GIMP_ITEM_CLASS (parent_class)->convert (item, dest_image);
}

static gboolean
gimp_layer_rename (GimpItem     *item,
                   const gchar  *new_name,
                   const gchar  *undo_desc,
                   GError      **error)
{
  GimpLayer *layer = GIMP_LAYER (item);
  GimpImage *image = gimp_item_get_image (item);
  gboolean   attached;
  gboolean   floating_sel;

  attached     = gimp_item_is_attached (item);
  floating_sel = gimp_layer_is_floating_sel (layer);

  if (floating_sel)
    {
      if (GIMP_IS_CHANNEL (gimp_layer_get_floating_sel_drawable (layer)))
        {
          g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			       _("Cannot create a new layer from the floating "
				 "selection because it belongs to a layer mask "
				 "or channel."));
          return FALSE;
        }

      if (attached)
        {
          gimp_image_undo_group_start (image,
                                       GIMP_UNDO_GROUP_ITEM_PROPERTIES,
                                       undo_desc);

          floating_sel_to_layer (layer, NULL);
        }
    }

  GIMP_ITEM_CLASS (parent_class)->rename (item, new_name, undo_desc, error);

  if (attached && floating_sel)
    gimp_image_undo_group_end (image);

  return TRUE;
}

static void
gimp_layer_translate (GimpItem *item,
                      gint      offset_x,
                      gint      offset_y,
                      gboolean  push_undo)
{
  GimpLayer *layer = GIMP_LAYER (item);

  if (push_undo)
    gimp_image_undo_push_item_displace (gimp_item_get_image (item), NULL, item);

  /*  update the old region  */
  gimp_drawable_update (GIMP_DRAWABLE (layer),
                        0, 0,
                        gimp_item_get_width  (item),
                        gimp_item_get_height (item));

  /*  invalidate the selection boundary because of a layer modification  */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (layer));

  GIMP_ITEM_CLASS (parent_class)->translate (item, offset_x, offset_y,
                                             push_undo);

  /*  update the new region  */
  gimp_drawable_update (GIMP_DRAWABLE (layer),
                        0, 0,
                        gimp_item_get_width  (item),
                        gimp_item_get_height (item));

  if (layer->mask)
    {
      gint off_x, off_y;

      gimp_item_get_offset (item, &off_x, &off_y);
      gimp_item_set_offset (GIMP_ITEM (layer->mask), off_x, off_y);

      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer->mask));
    }
}

static void
gimp_layer_scale (GimpItem              *item,
                  gint                   new_width,
                  gint                   new_height,
                  gint                   new_offset_x,
                  gint                   new_offset_y,
                  GimpInterpolationType  interpolation_type,
                  GimpProgress          *progress)
{
  GimpLayer *layer = GIMP_LAYER (item);

  /* scale mask first, see bug 733839 */
  if (layer->mask)
    gimp_item_scale (GIMP_ITEM (layer->mask),
                     new_width, new_height,
                     new_offset_x, new_offset_y,
                     interpolation_type, progress);

  GIMP_ITEM_CLASS (parent_class)->scale (item, new_width, new_height,
                                         new_offset_x, new_offset_y,
                                         interpolation_type, progress);
}

static void
gimp_layer_resize (GimpItem    *item,
                   GimpContext *context,
                   gint         new_width,
                   gint         new_height,
                   gint         offset_x,
                   gint         offset_y)
{
  GimpLayer *layer  = GIMP_LAYER (item);

  GIMP_ITEM_CLASS (parent_class)->resize (item, context, new_width, new_height,
                                          offset_x, offset_y);

  if (layer->mask)
    gimp_item_resize (GIMP_ITEM (layer->mask), context,
                      new_width, new_height, offset_x, offset_y);
}

static void
gimp_layer_flip (GimpItem            *item,
                 GimpContext         *context,
                 GimpOrientationType  flip_type,
                 gdouble              axis,
                 gboolean             clip_result)
{
  GimpLayer *layer = GIMP_LAYER (item);

  GIMP_ITEM_CLASS (parent_class)->flip (item, context, flip_type, axis,
                                        clip_result);

  if (layer->mask)
    gimp_item_flip (GIMP_ITEM (layer->mask), context,
                    flip_type, axis, clip_result);
}

static void
gimp_layer_rotate (GimpItem         *item,
                   GimpContext      *context,
                   GimpRotationType  rotate_type,
                   gdouble           center_x,
                   gdouble           center_y,
                   gboolean          clip_result)
{
  GimpLayer *layer = GIMP_LAYER (item);

  GIMP_ITEM_CLASS (parent_class)->rotate (item, context,
                                          rotate_type, center_x, center_y,
                                          clip_result);

  if (layer->mask)
    gimp_item_rotate (GIMP_ITEM (layer->mask), context,
                      rotate_type, center_x, center_y, clip_result);
}

static void
gimp_layer_transform (GimpItem               *item,
                      GimpContext            *context,
                      const GimpMatrix3      *matrix,
                      GimpTransformDirection  direction,
                      GimpInterpolationType   interpolation_type,
                      gint                    recursion_level,
                      GimpTransformResize     clip_result,
                      GimpProgress           *progress)
{
  GimpLayer *layer = GIMP_LAYER (item);

  /* FIXME: make interpolated transformations work on layers without alpha */
  if (interpolation_type != GIMP_INTERPOLATION_NONE &&
      ! gimp_drawable_has_alpha (GIMP_DRAWABLE (item)))
    gimp_layer_add_alpha (layer);

  GIMP_ITEM_CLASS (parent_class)->transform (item, context, matrix, direction,
                                             interpolation_type,
                                             recursion_level,
                                             clip_result,
                                             progress);

  if (layer->mask)
    gimp_item_transform (GIMP_ITEM (layer->mask), context,
                         matrix, direction,
                         interpolation_type, recursion_level,
                         clip_result, progress);
}

static void
gimp_layer_to_selection (GimpItem       *item,
                         GimpChannelOps  op,
                         gboolean        antialias,
                         gboolean        feather,
                         gdouble         feather_radius_x,
                         gdouble         feather_radius_y)
{
  GimpLayer *layer = GIMP_LAYER (item);
  GimpImage *image = gimp_item_get_image (item);

  gimp_channel_select_alpha (gimp_image_get_mask (image),
                             GIMP_DRAWABLE (layer),
                             op,
                             feather, feather_radius_x, feather_radius_y);
}

static GeglNode *
gimp_layer_get_node (GimpItem *item)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (item);
  GimpLayer    *layer    = GIMP_LAYER (item);
  GeglNode     *node;
  GeglNode     *offset_node;
  GeglNode     *source;
  GeglNode     *mode_node;
  gboolean      source_node_hijacked = FALSE;

  node = GIMP_ITEM_CLASS (parent_class)->get_node (item);

  source = gimp_drawable_get_source_node (drawable);

  /* if the source node already has a parent, we are a floating
   * selection and the source node has been hijacked by the fs'
   * drawable
   */
  if (gegl_node_get_parent (source))
    source_node_hijacked = TRUE;

  if (! source_node_hijacked)
    gegl_node_add_child (node, source);

  g_warn_if_fail (layer->opacity_node == NULL);

  layer->opacity_node = gegl_node_new_child (node,
                                             "operation", "gegl:opacity",
                                             "value",     layer->opacity,
                                             NULL);

  if (! source_node_hijacked)
    gegl_node_connect_to (source,              "output",
                          layer->opacity_node, "input");

  if (layer->mask)
    {
      GeglNode *mask;

      mask = gimp_drawable_get_source_node (GIMP_DRAWABLE (layer->mask));

      gegl_node_connect_to (mask,                "output",
                            layer->opacity_node, "aux");
    }

  offset_node = gimp_item_get_offset_node (GIMP_ITEM (layer));

  gegl_node_connect_to (layer->opacity_node, "output",
                        offset_node,         "input");

  gimp_layer_sync_mode_node (layer);

  mode_node = gimp_drawable_get_mode_node (drawable);

  gegl_node_connect_to (offset_node, "output",
                        mode_node,   "aux");

  return node;
}

static gint64
gimp_layer_estimate_memsize (const GimpDrawable *drawable,
                             gint                width,
                             gint                height)
{
  GimpLayer *layer   = GIMP_LAYER (drawable);
  gint64     memsize = 0;

  if (layer->mask)
    memsize += gimp_drawable_estimate_memsize (GIMP_DRAWABLE (layer->mask),
                                               width, height);

  return memsize + GIMP_DRAWABLE_CLASS (parent_class)->estimate_memsize (drawable,
                                                                         width,
                                                                         height);
}

static void
gimp_layer_invalidate_boundary (GimpDrawable *drawable)
{
  GimpLayer   *layer = GIMP_LAYER (drawable);
  GimpImage   *image;
  GimpChannel *mask;

  if (! (image = gimp_item_get_image (GIMP_ITEM (layer))))
    return;

  /*  Turn the current selection off  */
  gimp_image_selection_invalidate (image);

  /*  get the selection mask channel  */
  mask = gimp_image_get_mask (image);

  /*  Only bother with the bounds if there is a selection  */
  if (! gimp_channel_is_empty (mask))
    {
      mask->bounds_known   = FALSE;
      mask->boundary_known = FALSE;
    }

  if (gimp_layer_is_floating_sel (layer))
    floating_sel_invalidate (layer);
}

static void
gimp_layer_get_active_components (const GimpDrawable *drawable,
                                  gboolean           *active)
{
  GimpLayer *layer = GIMP_LAYER (drawable);
  GimpImage *image = gimp_item_get_image (GIMP_ITEM (drawable));

  /*  first copy the image active channels  */
  gimp_image_get_active_array (image, active);

  if (gimp_drawable_has_alpha (drawable) && layer->lock_alpha)
    active[gimp_drawable_bytes (drawable) - 1] = FALSE;
}

static void
gimp_layer_convert_type (GimpDrawable      *drawable,
                         GimpImage         *dest_image,
                         GimpImageBaseType  new_base_type,
                         gboolean           push_undo)
{
  switch (new_base_type)
    {
    case GIMP_RGB:
    case GIMP_GRAY:
      GIMP_DRAWABLE_CLASS (parent_class)->convert_type (drawable, dest_image,
                                                        new_base_type,
                                                        push_undo);
      break;

    case GIMP_INDEXED:
      {
        GimpItem      *item = GIMP_ITEM (drawable);
        TileManager   *new_tiles;
        GimpImageType  new_type;
        PixelRegion    layerPR;
        PixelRegion    newPR;

        new_type = GIMP_IMAGE_TYPE_FROM_BASE_TYPE (new_base_type);

        if (gimp_drawable_has_alpha (drawable))
          new_type = GIMP_IMAGE_TYPE_WITH_ALPHA (new_type);

        new_tiles = tile_manager_new (gimp_item_get_width  (item),
                                      gimp_item_get_height (item),
                                      GIMP_IMAGE_TYPE_BYTES (new_type));

        pixel_region_init (&layerPR, gimp_drawable_get_tiles (drawable),
                           0, 0,
                           gimp_item_get_width  (item),
                           gimp_item_get_height (item),
                           FALSE);
        pixel_region_init (&newPR, new_tiles,
                           0, 0,
                           gimp_item_get_width  (item),
                           gimp_item_get_height (item),
                           TRUE);

        gimp_layer_transform_color (dest_image,
                                    &layerPR, gimp_drawable_type (drawable),
                                    &newPR,   new_type);

        gimp_drawable_set_tiles (drawable, push_undo, NULL,
                                 new_tiles, new_type);
        tile_manager_unref (new_tiles);
      }
    }
}

static gint
gimp_layer_get_opacity_at (GimpPickable *pickable,
                           gint          x,
                           gint          y)
{
  GimpLayer *layer = GIMP_LAYER (pickable);
  Tile      *tile;
  gint       val   = 0;

  if (x >= 0 && x < gimp_item_get_width  (GIMP_ITEM (layer)) &&
      y >= 0 && y < gimp_item_get_height (GIMP_ITEM (layer)) &&
      gimp_item_is_visible (GIMP_ITEM (layer)))
    {
      /*  If the point is inside, and the layer has no
       *  alpha channel, success!
       */
      if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
        return OPAQUE_OPACITY;

      /*  Otherwise, determine if the alpha value at
       *  the given point is non-zero
       */
      tile = tile_manager_get_tile (gimp_drawable_get_tiles (GIMP_DRAWABLE (layer)),
                                    x, y, TRUE, FALSE);

      val = * ((const guchar *) tile_data_pointer (tile, x, y) +
               tile_bpp (tile) - 1);

      if (layer->mask)
        {
          gint mask_val;

          mask_val = gimp_pickable_get_opacity_at (GIMP_PICKABLE (layer->mask),
                                                   x, y);

          val = val * mask_val / 255;
        }

      tile_release (tile, FALSE);
    }

  return val;
}

static void
gimp_layer_transform_color (GimpImage     *image,
                            PixelRegion   *srcPR,
                            GimpImageType  src_type,
                            PixelRegion   *destPR,
                            GimpImageType  dest_type)
{
  GimpImageBaseType base_type  = GIMP_IMAGE_TYPE_BASE_TYPE (src_type);
  gboolean          src_alpha  = GIMP_IMAGE_TYPE_HAS_ALPHA (src_type);
  gboolean          dest_alpha = GIMP_IMAGE_TYPE_HAS_ALPHA (dest_type);
  gpointer          pr;

  for (pr = pixel_regions_register (2, srcPR, destPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const guchar *src  = srcPR->data;
      guchar       *dest = destPR->data;
      gint          h    = destPR->h;

      while (h--)
        {
          const guchar *s = src;
          guchar       *d = dest;
          gint          i;

          for (i = 0; i < destPR->w; i++)
            {
              gimp_image_transform_color (image, dest_type, d, base_type, s);

              if (dest_alpha)
                d[destPR->bytes - 1] = (src_alpha ?
                                        s[srcPR->bytes - 1] : OPAQUE_OPACITY);

              s += srcPR->bytes;
              d += destPR->bytes;
            }

          src  += srcPR->rowstride;
          dest += destPR->rowstride;
        }
    }
}

static void
gimp_layer_layer_mask_update (GimpDrawable *drawable,
                              gint          x,
                              gint          y,
                              gint          width,
                              gint          height,
                              GimpLayer    *layer)
{
  GimpLayerMask *layer_mask = GIMP_LAYER_MASK (drawable);

  if (gimp_layer_mask_get_apply (layer_mask) ||
      gimp_layer_mask_get_show (layer_mask))
    {
      gimp_drawable_update (GIMP_DRAWABLE (layer),
                            x, y, width, height);
    }
}

static void
gimp_layer_sync_mode_node (GimpLayer *layer)
{
  if (layer->opacity_node)
    {
      GeglNode *mode_node;

      mode_node = gimp_drawable_get_mode_node (GIMP_DRAWABLE (layer));

      switch (layer->mode)
        {
        case GIMP_DISSOLVE_MODE:
        case GIMP_BEHIND_MODE:
        case GIMP_MULTIPLY_MODE:
        case GIMP_SCREEN_MODE:
        case GIMP_OVERLAY_MODE:
        case GIMP_DIFFERENCE_MODE:
        case GIMP_ADDITION_MODE:
        case GIMP_SUBTRACT_MODE:
        case GIMP_DARKEN_ONLY_MODE:
        case GIMP_LIGHTEN_ONLY_MODE:
        case GIMP_HUE_MODE:
        case GIMP_SATURATION_MODE:
        case GIMP_COLOR_MODE:
        case GIMP_VALUE_MODE:
        case GIMP_DIVIDE_MODE:
        case GIMP_DODGE_MODE:
        case GIMP_BURN_MODE:
        case GIMP_HARDLIGHT_MODE:
        case GIMP_SOFTLIGHT_MODE:
        case GIMP_GRAIN_EXTRACT_MODE:
        case GIMP_GRAIN_MERGE_MODE:
        case GIMP_COLOR_ERASE_MODE:
        case GIMP_ERASE_MODE:
        case GIMP_REPLACE_MODE:
        case GIMP_ANTI_ERASE_MODE:
          gegl_node_set (mode_node,
                         "operation",  "gimp:point-layer-mode",
                         "blend-mode", layer->mode,
                         NULL);
          break;

        default:
          gegl_node_set (mode_node,
                         "operation",
                         gimp_layer_mode_to_gegl_operation (layer->mode),
                         NULL);
          break;
        }
    }
}


/*  public functions  */

GimpLayer *
gimp_layer_new (GimpImage            *image,
                gint                  width,
                gint                  height,
                GimpImageType         type,
                const gchar          *name,
                gdouble               opacity,
                GimpLayerModeEffects  mode)
{
  GimpLayer *layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  layer = GIMP_LAYER (gimp_drawable_new (GIMP_TYPE_LAYER,
                                         image, name,
                                         0, 0, width, height,
                                         type));

  opacity = CLAMP (opacity, GIMP_OPACITY_TRANSPARENT, GIMP_OPACITY_OPAQUE);

  layer->opacity = opacity;
  layer->mode    = mode;

  return layer;
}

/**
 * gimp_layer_new_from_tiles:
 * @tiles:      The buffer to make the new layer from.
 * @dest_image: The image the new layer will be added to.
 * @type:       The #GimpImageType of the new layer.
 * @name:       The new layer's name.
 * @opacity:    The new layer's opacity.
 * @mode:       The new layer's mode.
 *
 * Copies %tiles to a layer taking into consideration the
 * possibility of transforming the contents to meet the requirements
 * of the target image type
 *
 * Return value: The new layer.
 **/
GimpLayer *
gimp_layer_new_from_tiles (TileManager          *tiles,
                           GimpImage            *dest_image,
                           GimpImageType         type,
                           const gchar          *name,
                           gdouble               opacity,
                           GimpLayerModeEffects  mode)
{
  PixelRegion bufPR;

  g_return_val_if_fail (tiles != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (dest_image), NULL);

  pixel_region_init (&bufPR, tiles,
                     0, 0,
                     tile_manager_width (tiles),
                     tile_manager_height (tiles),
                     FALSE);

  return gimp_layer_new_from_region (&bufPR, dest_image, type,
                                     name, opacity, mode);
}

/**
 * gimp_layer_new_from_pixbuf:
 * @pixbuf:     The pixbuf to make the new layer from.
 * @dest_image: The image the new layer will be added to.
 * @type:       The #GimpImageType of the new layer.
 * @name:       The new layer's name.
 * @opacity:    The new layer's opacity.
 * @mode:       The new layer's mode.
 *
 * Copies %pixbuf to a layer taking into consideration the
 * possibility of transforming the contents to meet the requirements
 * of the target image type
 *
 * Return value: The new layer.
 **/
GimpLayer *
gimp_layer_new_from_pixbuf (GdkPixbuf            *pixbuf,
                            GimpImage            *dest_image,
                            GimpImageType         type,
                            const gchar          *name,
                            gdouble               opacity,
                            GimpLayerModeEffects  mode)
{
  PixelRegion bufPR = { 0, };

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (GIMP_IMAGE_TYPE_BASE_TYPE (type) ==
                        gimp_image_base_type (dest_image), NULL);

  pixel_region_init_data (&bufPR, gdk_pixbuf_get_pixels (pixbuf),
                          gdk_pixbuf_get_n_channels (pixbuf),
                          gdk_pixbuf_get_rowstride (pixbuf),
                          0, 0,
                          gdk_pixbuf_get_width (pixbuf),
                          gdk_pixbuf_get_height (pixbuf));

  return gimp_layer_new_from_region (&bufPR, dest_image, type,
                                     name, opacity, mode);
}

/**
 * gimp_layer_new_from_region:
 * @region:     A readable pixel region.
 * @dest_image: The image the new layer will be added to.
 * @type:       The #GimpImageType of the new layer.
 * @name:       The new layer's name.
 * @opacity:    The new layer's opacity.
 * @mode:       The new layer's mode.
 *
 * Copies %region to a layer taking into consideration the
 * possibility of transforming the contents to meet the requirements
 * of the target image type
 *
 * Return value: The new layer.
 **/
GimpLayer *
gimp_layer_new_from_region (PixelRegion          *region,
                            GimpImage            *dest_image,
                            GimpImageType         type,
                            const gchar          *name,
                            gdouble               opacity,
                            GimpLayerModeEffects  mode)
{
  GimpLayer     *new_layer;
  PixelRegion    layerPR;
  GimpImageType  src_type;
  gint           width;
  gint           height;

  g_return_val_if_fail (region != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (dest_image), NULL);

  width  = region->w;
  height = region->h;

  switch (region->bytes)
    {
    case 1: src_type = GIMP_GRAY_IMAGE;  break;
    case 2: src_type = GIMP_GRAYA_IMAGE; break;
    case 3: src_type = GIMP_RGB_IMAGE;   break;
    case 4: src_type = GIMP_RGBA_IMAGE;  break;
    default:
      g_return_val_if_reached (NULL);
      break;
    }

  new_layer = gimp_layer_new (dest_image, width, height, type, name,
                              opacity, mode);

  if (! new_layer)
    {
      g_warning ("%s: could not allocate new layer", G_STRFUNC);
      return NULL;
    }

  pixel_region_init (&layerPR,
                     gimp_drawable_get_tiles (GIMP_DRAWABLE (new_layer)),
                     0, 0, width, height,
                     TRUE);

  switch (type)
    {
    case GIMP_RGB_IMAGE:
      switch (src_type)
        {
       case GIMP_RGB_IMAGE:
          copy_region (region, &layerPR);
          break;

        default:
          g_warning ("%s: unhandled type conversion", G_STRFUNC);
          break;
         }
      break;

    case GIMP_RGBA_IMAGE:
      switch (src_type)
        {
        case GIMP_RGBA_IMAGE:
          copy_region (region, &layerPR);
          break;

        case GIMP_RGB_IMAGE:
          add_alpha_region (region, &layerPR);
          break;

        case GIMP_GRAY_IMAGE:
        case GIMP_GRAYA_IMAGE:
          gimp_layer_transform_color (dest_image,
                                      region,   src_type,
                                      &layerPR, type);
          break;

        default:
          g_warning ("%s: unhandled type conversion", G_STRFUNC);
          break;
        }
      break;

    case GIMP_GRAY_IMAGE:
      switch (src_type)
        {
        case GIMP_GRAY_IMAGE:
          copy_region (region, &layerPR);
          break;

        default:
          g_warning ("%s: unhandled type conversion", G_STRFUNC);
          break;
        }
      break;

    case GIMP_GRAYA_IMAGE:
      switch (src_type)
        {
        case GIMP_RGB_IMAGE:
        case GIMP_RGBA_IMAGE:
          gimp_layer_transform_color (dest_image,
                                      region,   src_type,
                                      &layerPR, type);
          break;

        case GIMP_GRAYA_IMAGE:
          copy_region (region, &layerPR);
          break;

        case GIMP_GRAY_IMAGE:
          add_alpha_region (region, &layerPR);
          break;

        default:
          g_warning ("%s: unhandled type conversion", G_STRFUNC);
          break;
        }
      break;

    case GIMP_INDEXED_IMAGE:
      g_warning ("%s: unhandled type conversion", G_STRFUNC);
      break;

    case GIMP_INDEXEDA_IMAGE:
      switch (src_type)
        {
        case GIMP_RGB_IMAGE:
        case GIMP_RGBA_IMAGE:
        case GIMP_GRAY_IMAGE:
        case GIMP_GRAYA_IMAGE:
          gimp_layer_transform_color (dest_image,
                                      region,   src_type,
                                      &layerPR, type);
          break;

        default:
          g_warning ("%s: unhandled type conversion", G_STRFUNC);
          break;
        }
      break;
    }

  return new_layer;
}

GimpLayer *
gimp_layer_get_parent (GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);

  return GIMP_LAYER (gimp_viewable_get_parent (GIMP_VIEWABLE (layer)));
}

GimpLayerMask *
gimp_layer_get_mask (const GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);

  return layer->mask;
}

GimpLayerMask *
gimp_layer_add_mask (GimpLayer      *layer,
                     GimpLayerMask  *mask,
                     gboolean        push_undo,
                     GError        **error)
{
  GimpImage *image;

  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (GIMP_IS_LAYER_MASK (mask), NULL);
  g_return_val_if_fail (gimp_item_get_image (GIMP_ITEM (layer)) ==
                        gimp_item_get_image (GIMP_ITEM (mask)), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! gimp_item_is_attached (GIMP_ITEM (layer)))
    push_undo = FALSE;

  image = gimp_item_get_image (GIMP_ITEM (layer));

  if (layer->mask)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("Unable to add a layer mask since "
			     "the layer already has one."));
      return NULL;
    }

  if ((gimp_item_get_width (GIMP_ITEM (layer)) !=
       gimp_item_get_width (GIMP_ITEM (mask))) ||
      (gimp_item_get_height (GIMP_ITEM (layer)) !=
       gimp_item_get_height (GIMP_ITEM (mask))))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("Cannot add layer mask of different "
			     "dimensions than specified layer."));
      return NULL;
    }

  if (push_undo)
    gimp_image_undo_push_layer_mask_add (image, C_("undo-type", "Add Layer Mask"),
                                         layer, mask);

  layer->mask = g_object_ref_sink (mask);

  gimp_layer_mask_set_layer (mask, layer);

  if (layer->opacity_node)
    {
      GeglNode *mask;

      mask = gimp_drawable_get_source_node (GIMP_DRAWABLE (layer->mask));

      gegl_node_connect_to (mask,                "output",
                            layer->opacity_node, "aux");
    }

  if (gimp_layer_mask_get_apply (mask) ||
      gimp_layer_mask_get_show (mask))
    {
      gimp_drawable_update (GIMP_DRAWABLE (layer),
                            0, 0,
                            gimp_item_get_width  (GIMP_ITEM (layer)),
                            gimp_item_get_height (GIMP_ITEM (layer)));
    }

  g_signal_connect (mask, "update",
                    G_CALLBACK (gimp_layer_layer_mask_update),
                    layer);

  g_signal_emit (layer, layer_signals[MASK_CHANGED], 0);

  g_object_notify (G_OBJECT (layer), "mask");

  /*  if the mask came from the undo stack, reset its "removed" state  */
  if (gimp_item_is_removed (GIMP_ITEM (mask)))
    gimp_item_unset_removed (GIMP_ITEM (mask));

  return layer->mask;
}

GimpLayerMask *
gimp_layer_create_mask (const GimpLayer *layer,
                        GimpAddMaskType  add_mask_type,
                        GimpChannel     *channel)
{
  GimpDrawable  *drawable;
  GimpItem      *item;
  PixelRegion    srcPR;
  PixelRegion    destPR;
  GimpLayerMask *mask;
  GimpImage     *image;
  gchar         *mask_name;
  GimpRGB        black = { 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE };

  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (add_mask_type != GIMP_ADD_CHANNEL_MASK ||
                        GIMP_IS_CHANNEL (channel), NULL);

  drawable = GIMP_DRAWABLE (layer);
  item     = GIMP_ITEM (layer);
  image    = gimp_item_get_image (item);

  mask_name = g_strdup_printf (_("%s mask"),
                               gimp_object_get_name (layer));

  mask = gimp_layer_mask_new (image,
                              gimp_item_get_width  (item),
                              gimp_item_get_height (item),
                              mask_name, &black);

  g_free (mask_name);

  switch (add_mask_type)
    {
    case GIMP_ADD_WHITE_MASK:
      gimp_channel_all (GIMP_CHANNEL (mask), FALSE);
      return mask;

    case GIMP_ADD_BLACK_MASK:
      gimp_channel_clear (GIMP_CHANNEL (mask), NULL, FALSE);
      return mask;

    default:
      break;
    }

  pixel_region_init (&destPR, gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                     0, 0,
                     gimp_item_get_width  (GIMP_ITEM (mask)),
                     gimp_item_get_height (GIMP_ITEM (mask)),
                     TRUE);

  switch (add_mask_type)
    {
    case GIMP_ADD_WHITE_MASK:
    case GIMP_ADD_BLACK_MASK:
      break;

    case GIMP_ADD_ALPHA_MASK:
    case GIMP_ADD_ALPHA_TRANSFER_MASK:
      if (gimp_drawable_has_alpha (drawable))
        {
          pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                             0, 0,
                             gimp_item_get_width  (item),
                             gimp_item_get_height (item),
                             FALSE);

          extract_alpha_region (&srcPR, NULL, &destPR);

          if (add_mask_type == GIMP_ADD_ALPHA_TRANSFER_MASK)
            {
              void   *pr;
              gint    w, h;
              guchar *alpha_ptr;

              gimp_drawable_push_undo (drawable,
                                       C_("undo-type", "Transfer Alpha to Mask"),
                                       0, 0,
                                       gimp_item_get_width  (item),
                                       gimp_item_get_height (item),
                                       NULL, FALSE);

              pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                                 0, 0,
                                 gimp_item_get_width  (item),
                                 gimp_item_get_height (item),
                                 TRUE);

              for (pr = pixel_regions_register (1, &srcPR);
                   pr != NULL;
                   pr = pixel_regions_process (pr))
                {
                  h = srcPR.h;

                  while (h--)
                    {
                      w = srcPR.w;
                      alpha_ptr = (srcPR.data + h * srcPR.rowstride +
                                   srcPR.bytes - 1);

                      while (w--)
                        {
                          *alpha_ptr = OPAQUE_OPACITY;
                          alpha_ptr += srcPR.bytes;
                        }
                    }
                }
            }
        }
      break;

    case GIMP_ADD_SELECTION_MASK:
    case GIMP_ADD_CHANNEL_MASK:
      {
        gboolean channel_empty;
        gint     offset_x, offset_y;
        gint     copy_x, copy_y;
        gint     copy_width, copy_height;

        if (add_mask_type == GIMP_ADD_SELECTION_MASK)
          channel = GIMP_CHANNEL (gimp_image_get_mask (image));

        channel_empty = gimp_channel_is_empty (channel);

        gimp_item_get_offset (item, &offset_x, &offset_y);

        gimp_rectangle_intersect (0, 0,
                                  gimp_image_get_width  (image),
                                  gimp_image_get_height (image),
                                  offset_x, offset_y,
                                  gimp_item_get_width  (item),
                                  gimp_item_get_height (item),
                                  &copy_x, &copy_y,
                                  &copy_width, &copy_height);

        if (copy_width  < gimp_item_get_width  (item) ||
            copy_height < gimp_item_get_height (item) ||
            channel_empty)
          gimp_channel_clear (GIMP_CHANNEL (mask), NULL, FALSE);

        if ((copy_width || copy_height) && ! channel_empty)
          {
            pixel_region_init (&srcPR,
                               gimp_drawable_get_tiles (GIMP_DRAWABLE (channel)),
                               copy_x, copy_y,
                               copy_width, copy_height,
                               FALSE);
            pixel_region_init (&destPR,
                               gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                               copy_x - offset_x, copy_y - offset_y,
                               copy_width, copy_height,
                               TRUE);

            copy_region (&srcPR, &destPR);

            GIMP_CHANNEL (mask)->bounds_known = FALSE;
          }
      }
      break;

    case GIMP_ADD_COPY_MASK:
      {
        TileManager *copy_tiles = NULL;

        if (! gimp_drawable_is_gray (drawable))
          {
            GimpImageType copy_type;

            copy_type = (gimp_drawable_has_alpha (drawable) ?
                         GIMP_GRAYA_IMAGE : GIMP_GRAY_IMAGE);

            copy_tiles = tile_manager_new (gimp_item_get_width  (item),
                                           gimp_item_get_height (item),
                                           GIMP_IMAGE_TYPE_BYTES (copy_type));

            gimp_drawable_convert_tiles_grayscale (drawable, copy_tiles);

            pixel_region_init (&srcPR, copy_tiles,
                               0, 0,
                               gimp_item_get_width  (item),
                               gimp_item_get_height (item),
                               FALSE);
          }
        else
          {
            pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                               0, 0,
                               gimp_item_get_width  (item),
                               gimp_item_get_height (item),
                               FALSE);
          }

        if (gimp_drawable_has_alpha (drawable))
          {
            guchar black_uchar[] = { 0, 0, 0, 0 };

            flatten_region (&srcPR, &destPR, black_uchar);
          }
        else
          {
            copy_region (&srcPR, &destPR);
          }

        if (copy_tiles)
          tile_manager_unref (copy_tiles);
      }

      GIMP_CHANNEL (mask)->bounds_known = FALSE;
      break;
    }

  return mask;
}

void
gimp_layer_apply_mask (GimpLayer         *layer,
                       GimpMaskApplyMode  mode,
                       gboolean           push_undo)
{
  GimpItem      *item;
  GimpImage     *image;
  GimpLayerMask *mask;
  PixelRegion    srcPR, maskPR;
  gboolean       view_changed = FALSE;

  g_return_if_fail (GIMP_IS_LAYER (layer));

  mask = gimp_layer_get_mask (layer);

  if (! mask)
    return;

  /*  APPLY can only be done to layers with an alpha channel  */
  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    g_return_if_fail (mode == GIMP_MASK_DISCARD || push_undo == TRUE);

  item  = GIMP_ITEM (layer);
  image = gimp_item_get_image (item);

  if (! image)
    return;

  if (push_undo)
    {
      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_LAYER_APPLY_MASK,
                                   (mode == GIMP_MASK_APPLY) ?
                                   C_("undo-type", "Apply Layer Mask") :
                                   C_("undo-type", "Delete Layer Mask"));

      gimp_image_undo_push_layer_mask_remove (image, NULL, layer, mask);

      if (mode == GIMP_MASK_APPLY &&
          ! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
        {
          gimp_layer_add_alpha (layer);
        }
    }

  /*  check if applying the mask changes the projection  */
  if (gimp_layer_mask_get_show (mask)                                   ||
      (mode == GIMP_MASK_APPLY   && ! gimp_layer_mask_get_apply (mask)) ||
      (mode == GIMP_MASK_DISCARD &&   gimp_layer_mask_get_apply (mask)))
    {
      view_changed = TRUE;
    }

  if (mode == GIMP_MASK_APPLY)
    {
      if (push_undo)
        gimp_drawable_push_undo (GIMP_DRAWABLE (layer), NULL,
                                 0, 0,
                                 gimp_item_get_width  (item),
                                 gimp_item_get_height (item),
                                 NULL, FALSE);

      /*  Combine the current layer's alpha channel and the mask  */
      pixel_region_init (&srcPR,
                         gimp_drawable_get_tiles (GIMP_DRAWABLE (layer)),
                         0, 0,
                         gimp_item_get_width  (item),
                         gimp_item_get_height (item),
                         TRUE);
      pixel_region_init (&maskPR,
                         gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                         0, 0,
                         gimp_item_get_width  (item),
                         gimp_item_get_height (item),
                         FALSE);

      apply_mask_to_region (&srcPR, &maskPR, OPAQUE_OPACITY);
    }

  g_signal_handlers_disconnect_by_func (mask,
                                        gimp_layer_layer_mask_update,
                                        layer);

  gimp_item_removed (GIMP_ITEM (mask));
  g_object_unref (mask);
  layer->mask = NULL;

  if (push_undo)
    gimp_image_undo_group_end (image);

  if (layer->opacity_node)
    gegl_node_disconnect (layer->opacity_node, "aux");

  /*  If applying actually changed the view  */
  if (view_changed)
    {
      gimp_drawable_update (GIMP_DRAWABLE (layer),
                            0, 0,
                            gimp_item_get_width  (item),
                            gimp_item_get_height (item));
    }
  else
    {
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer));
    }

  g_signal_emit (layer, layer_signals[MASK_CHANGED], 0);

  g_object_notify (G_OBJECT (layer), "mask");
}

void
gimp_layer_add_alpha (GimpLayer *layer)
{
  GimpItem      *item;
  GimpDrawable  *drawable;
  PixelRegion    srcPR, destPR;
  TileManager   *new_tiles;
  GimpImageType  new_type;

  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    return;

  item     = GIMP_ITEM (layer);
  drawable = GIMP_DRAWABLE (layer);

  new_type = gimp_drawable_type_with_alpha (drawable);

  /*  Allocate the new tiles  */
  new_tiles = tile_manager_new (gimp_item_get_width  (item),
                                gimp_item_get_height (item),
                                GIMP_IMAGE_TYPE_BYTES (new_type));

  /*  Configure the pixel regions  */
  pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                     0, 0,
                     gimp_item_get_width  (item),
                     gimp_item_get_height (item),
                     FALSE);
  pixel_region_init (&destPR, new_tiles,
                     0, 0,
                     gimp_item_get_width  (item),
                     gimp_item_get_height (item),
                     TRUE);

  /*  Add an alpha channel  */
  add_alpha_region (&srcPR, &destPR);

  /*  Set the new tiles  */
  gimp_drawable_set_tiles (GIMP_DRAWABLE (layer),
                           gimp_item_is_attached (GIMP_ITEM (layer)),
                           C_("undo-type", "Add Alpha Channel"),
                           new_tiles, new_type);
  tile_manager_unref (new_tiles);
}

void
gimp_layer_flatten (GimpLayer   *layer,
                    GimpContext *context)
{
  GimpItem      *item;
  GimpDrawable  *drawable;
  PixelRegion    srcPR, destPR;
  TileManager   *new_tiles;
  GimpImageType  new_type;
  guchar         bg[4];

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    return;

  item     = GIMP_ITEM (layer);
  drawable = GIMP_DRAWABLE (layer);

  new_type = gimp_drawable_type_without_alpha (drawable);

  gimp_image_get_background (gimp_item_get_image (item), context,
                             gimp_drawable_type (drawable),
                             bg);

  /*  Allocate the new tiles  */
  new_tiles = tile_manager_new (gimp_item_get_width  (item),
                                gimp_item_get_height (item),
                                GIMP_IMAGE_TYPE_BYTES (new_type));

  /*  Configure the pixel regions  */
  pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                     0, 0,
                     gimp_item_get_width  (item),
                     gimp_item_get_height (item),
                     FALSE);
  pixel_region_init (&destPR, new_tiles,
                     0, 0,
                     gimp_item_get_width  (item),
                     gimp_item_get_height (item),
                     TRUE);

  /*  Remove alpha channel  */
  flatten_region (&srcPR, &destPR, bg);

  /*  Set the new tiles  */
  gimp_drawable_set_tiles (GIMP_DRAWABLE (layer),
                           gimp_item_is_attached (GIMP_ITEM (layer)),
                           C_("undo-type", "Remove Alpha Channel"),
                           new_tiles, new_type);
  tile_manager_unref (new_tiles);
}

void
gimp_layer_resize_to_image (GimpLayer   *layer,
                            GimpContext *context)
{
  GimpImage *image;
  gint       offset_x;
  gint       offset_y;

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  image = gimp_item_get_image (GIMP_ITEM (layer));

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_RESIZE,
                               C_("undo-type", "Layer to Image Size"));

  gimp_item_get_offset (GIMP_ITEM (layer), &offset_x, &offset_y);
  gimp_item_resize (GIMP_ITEM (layer), context,
                    gimp_image_get_width  (image),
                    gimp_image_get_height (image),
                    offset_x, offset_y);

  gimp_image_undo_group_end (image);
}

/**********************/
/*  access functions  */
/**********************/

GimpDrawable *
gimp_layer_get_floating_sel_drawable (const GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);

  return layer->fs.drawable;
}

void
gimp_layer_set_floating_sel_drawable (GimpLayer    *layer,
                                      GimpDrawable *drawable)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (drawable == NULL || GIMP_IS_DRAWABLE (drawable));

  if (layer->fs.drawable != drawable)
    {
      if (layer->fs.segs)
        {
          g_free (layer->fs.segs);
          layer->fs.segs     = NULL;
          layer->fs.num_segs = 0;
        }

      layer->fs.drawable = drawable;

      g_object_notify (G_OBJECT (layer), "floating-selection");
    }
}

gboolean
gimp_layer_is_floating_sel (const GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  return (gimp_layer_get_floating_sel_drawable (layer) != NULL);
}

void
gimp_layer_set_opacity (GimpLayer *layer,
                        gdouble    opacity,
                        gboolean   push_undo)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));

  opacity = CLAMP (opacity, GIMP_OPACITY_TRANSPARENT, GIMP_OPACITY_OPAQUE);

  if (layer->opacity != opacity)
    {
      if (push_undo && gimp_item_is_attached (GIMP_ITEM (layer)))
        {
          GimpImage *image = gimp_item_get_image (GIMP_ITEM (layer));

          gimp_image_undo_push_layer_opacity (image, NULL, layer);
        }

      layer->opacity = opacity;

      g_signal_emit (layer, layer_signals[OPACITY_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "opacity");

      if (layer->opacity_node)
        gegl_node_set (layer->opacity_node,
                       "value", layer->opacity,
                       NULL);

      gimp_drawable_update (GIMP_DRAWABLE (layer),
                            0, 0,
                            gimp_item_get_width  (GIMP_ITEM (layer)),
                            gimp_item_get_height (GIMP_ITEM (layer)));
    }
}

gdouble
gimp_layer_get_opacity (const GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), GIMP_OPACITY_OPAQUE);

  return layer->opacity;
}

void
gimp_layer_set_mode (GimpLayer            *layer,
                     GimpLayerModeEffects  mode,
                     gboolean              push_undo)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (layer->mode != mode)
    {
      if (push_undo && gimp_item_is_attached (GIMP_ITEM (layer)))
        {
          GimpImage *image = gimp_item_get_image (GIMP_ITEM (layer));

          gimp_image_undo_push_layer_mode (image, NULL, layer);
        }

      layer->mode = mode;

      g_signal_emit (layer, layer_signals[MODE_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "mode");

      gimp_layer_sync_mode_node (layer);

      gimp_drawable_update (GIMP_DRAWABLE (layer),
                            0, 0,
                            gimp_item_get_width  (GIMP_ITEM (layer)),
                            gimp_item_get_height (GIMP_ITEM (layer)));
    }
}

GimpLayerModeEffects
gimp_layer_get_mode (const GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), GIMP_NORMAL_MODE);

  return layer->mode;
}

void
gimp_layer_set_lock_alpha (GimpLayer *layer,
                           gboolean   lock_alpha,
                           gboolean   push_undo)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_layer_can_lock_alpha (layer));

  lock_alpha = lock_alpha ? TRUE : FALSE;

  if (layer->lock_alpha != lock_alpha)
    {
      if (push_undo && gimp_item_is_attached (GIMP_ITEM (layer)))
        {
          GimpImage *image = gimp_item_get_image (GIMP_ITEM (layer));

          gimp_image_undo_push_layer_lock_alpha (image, NULL, layer);
        }

      layer->lock_alpha = lock_alpha;

      g_signal_emit (layer, layer_signals[LOCK_ALPHA_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "lock-alpha");
    }
}

gboolean
gimp_layer_get_lock_alpha (const GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  return layer->lock_alpha;
}

gboolean
gimp_layer_can_lock_alpha (const GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  if (gimp_viewable_get_children (GIMP_VIEWABLE (layer)))
    return FALSE;

  return TRUE;
}
