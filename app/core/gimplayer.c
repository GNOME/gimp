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

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/boundary.h"
#include "base/pixel-region.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "paint-funcs/paint-funcs.h"

#include "gimpcontext.h"
#include "gimpcontainer.h"
#include "gimpdrawable-convert.h"
#include "gimpdrawable-invert.h"
#include "gimpimage-undo-push.h"
#include "gimpimage-undo.h"
#include "gimpimage.h"
#include "gimplayer-floating-sel.h"
#include "gimplayer.h"
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
  PROP_LOCK_ALPHA
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
static gboolean   gimp_layer_is_attached        (GimpItem           *item);
static GimpItem * gimp_layer_duplicate          (GimpItem           *item,
                                                 GType               new_type,
                                                 gboolean            add_alpha);
static void       gimp_layer_convert            (GimpItem           *item,
                                                 GimpImage          *dest_image);
static gboolean   gimp_layer_rename             (GimpItem           *item,
                                                 const gchar        *new_name,
                                                 const gchar        *undo_desc);
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
                                                 GimpTransformResize    clip_result,
                                                 GimpProgress       *progress);
static void    gimp_layer_invalidate_boundary   (GimpDrawable       *drawable);
static void    gimp_layer_get_active_components (const GimpDrawable *drawable,
                                                 gboolean           *active);
static void    gimp_layer_set_tiles             (GimpDrawable       *drawable,
                                                 gboolean            push_undo,
                                                 const gchar        *undo_desc,
                                                 TileManager        *tiles,
                                                 GimpImageType       type,
                                                 gint                offset_x,
                                                 gint                offset_y);
static gint    gimp_layer_get_opacity_at        (GimpPickable       *pickable,
                                                 gint                x,
                                                 gint                y);

static void       gimp_layer_transform_color    (GimpImage          *image,
                                                 PixelRegion        *layerPR,
                                                 PixelRegion        *bufPR,
                                                 GimpImageType       dest_type,
                                                 GimpImageType       src_type);

static void       gimp_layer_layer_mask_update  (GimpDrawable       *layer_mask,
                                                 gint                x,
                                                 gint                y,
                                                 gint                width,
                                                 gint                height,
                                                 GimpLayer          *layer);


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
  item_class->is_attached             = gimp_layer_is_attached;
  item_class->duplicate               = gimp_layer_duplicate;
  item_class->convert                 = gimp_layer_convert;
  item_class->rename                  = gimp_layer_rename;
  item_class->translate               = gimp_layer_translate;
  item_class->scale                   = gimp_layer_scale;
  item_class->resize                  = gimp_layer_resize;
  item_class->flip                    = gimp_layer_flip;
  item_class->rotate                  = gimp_layer_rotate;
  item_class->transform               = gimp_layer_transform;
  item_class->default_name            = _("Layer");
  item_class->rename_desc             = _("Rename Layer");
  item_class->translate_desc          = _("Move Layer");
  item_class->scale_desc              = _("Scale Layer");
  item_class->resize_desc             = _("Resize Layer");
  item_class->flip_desc               = _("Flip Layer");
  item_class->rotate_desc             = _("Rotate Layer");
  item_class->transform_desc          = _("Transform Layer");

  drawable_class->invalidate_boundary   = gimp_layer_invalidate_boundary;
  drawable_class->get_active_components = gimp_layer_get_active_components;
  drawable_class->set_tiles             = gimp_layer_set_tiles;

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
}

static void
gimp_layer_init (GimpLayer *layer)
{
  layer->opacity    = GIMP_OPACITY_OPAQUE;
  layer->mode       = GIMP_NORMAL_MODE;
  layer->lock_alpha = FALSE;

  layer->mask       = NULL;

  /*  floating selection  */
  layer->fs.backing_store  = NULL;
  layer->fs.drawable       = NULL;
  layer->fs.initial        = TRUE;
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
    case PROP_OPACITY:
    case PROP_MODE:
    case PROP_LOCK_ALPHA:
      g_assert_not_reached ();
      break;
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
      g_value_set_double (value, layer->opacity);
      break;
    case PROP_MODE:
      g_value_set_enum (value, layer->mode);
      break;
    case PROP_LOCK_ALPHA:
      g_value_set_boolean (value, layer->lock_alpha);
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

  if (layer->fs.backing_store)
    {
      tile_manager_unref (layer->fs.backing_store);
      layer->fs.backing_store = NULL;
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

  if (layer->mask)
    memsize += gimp_object_get_memsize (GIMP_OBJECT (layer->mask), gui_size);

  if (layer->fs.backing_store)
    *gui_size += tile_manager_get_memsize (layer->fs.backing_store, FALSE);

  *gui_size += layer->fs.num_segs * sizeof (BoundSeg);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_layer_invalidate_preview (GimpViewable *viewable)
{
  GimpLayer *layer = GIMP_LAYER (viewable);

  if (GIMP_VIEWABLE_CLASS (parent_class)->invalidate_preview)
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
                              gimp_object_get_name (GIMP_OBJECT (viewable)));
    }

  return GIMP_VIEWABLE_CLASS (parent_class)->get_description (viewable,
                                                              tooltip);
}

static void
gimp_layer_get_active_components (const GimpDrawable *drawable,
                                  gboolean           *active)
{
  GimpLayer *layer = GIMP_LAYER (drawable);
  GimpImage *image = gimp_item_get_image (GIMP_ITEM (drawable));
  gint       i;

  /*  first copy the image active channels  */
  for (i = 0; i < MAX_CHANNELS; i++)
    active[i] = image->active[i];

  if (gimp_drawable_has_alpha (drawable) && layer->lock_alpha)
    active[gimp_drawable_bytes (drawable) - 1] = FALSE;
}

static void
gimp_layer_set_tiles (GimpDrawable *drawable,
                      gboolean      push_undo,
                      const gchar  *undo_desc,
                      TileManager  *tiles,
                      GimpImageType type,
                      gint          offset_x,
                      gint          offset_y)
{
  GimpLayer *layer = GIMP_LAYER (drawable);

  if (gimp_layer_is_floating_sel (layer))
    floating_sel_relax (layer, FALSE);

  GIMP_DRAWABLE_CLASS (parent_class)->set_tiles (drawable,
                                                 push_undo, undo_desc,
                                                 tiles, type,
                                                 offset_x, offset_y);

  if (gimp_layer_is_floating_sel (layer))
    floating_sel_rigor (layer, FALSE);
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

static gboolean
gimp_layer_is_attached (GimpItem *item)
{
  return (GIMP_IS_IMAGE (item->image) &&
          gimp_container_have (item->image->layers, GIMP_OBJECT (item)));
}

static GimpItem *
gimp_layer_duplicate (GimpItem *item,
                      GType     new_type,
                      gboolean  add_alpha)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type,
                                                        add_alpha);

  if (GIMP_IS_LAYER (new_item))
    {
      GimpLayer *layer     = GIMP_LAYER (item);
      GimpLayer *new_layer = GIMP_LAYER (new_item);

      new_layer->mode       = layer->mode;
      new_layer->opacity    = layer->opacity;
      new_layer->lock_alpha = layer->lock_alpha;

      /*  duplicate the layer mask if necessary  */
      if (layer->mask)
        {
          GimpItem *new_mask =
            gimp_item_duplicate (GIMP_ITEM (layer->mask),
                                 G_TYPE_FROM_INSTANCE (layer->mask),
                                 FALSE);
          gimp_layer_add_mask (new_layer, GIMP_LAYER_MASK (new_mask), FALSE);
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
    {
      TileManager   *new_tiles;
      GimpImageType  new_type;

      new_type = GIMP_IMAGE_TYPE_FROM_BASE_TYPE (new_base_type);

      if (gimp_drawable_has_alpha (drawable))
        new_type = GIMP_IMAGE_TYPE_WITH_ALPHA (new_type);

      new_tiles = tile_manager_new (gimp_item_width (item),
                                    gimp_item_height (item),
                                    GIMP_IMAGE_TYPE_BYTES (new_type));

      switch (new_base_type)
        {
        case GIMP_RGB:
          gimp_drawable_convert_rgb (drawable,
                                     new_tiles,
                                     old_base_type);
          break;

        case GIMP_GRAY:
          gimp_drawable_convert_grayscale (drawable,
                                           new_tiles,
                                           old_base_type);
          break;

        case GIMP_INDEXED:
          {
            PixelRegion layerPR;
            PixelRegion newPR;

            pixel_region_init (&layerPR, drawable->tiles,
                               0, 0,
                               gimp_item_width (item),
                               gimp_item_height (item),
                               FALSE);
            pixel_region_init (&newPR, new_tiles,
                               0, 0,
                               gimp_item_width (item),
                               gimp_item_height (item),
                               TRUE);

            gimp_layer_transform_color (dest_image,
                                        &newPR, &layerPR,
                                        new_type,
                                        gimp_drawable_type (drawable));
          }
          break;
        }

      gimp_drawable_set_tiles_full (drawable, FALSE, NULL,
                                    new_tiles, new_type,
                                    GIMP_ITEM (layer)->offset_x,
                                    GIMP_ITEM (layer)->offset_y);
      tile_manager_unref (new_tiles);
    }

  if (layer->mask)
    gimp_item_set_image (GIMP_ITEM (layer->mask), dest_image);

  GIMP_ITEM_CLASS (parent_class)->convert (item, dest_image);
}

static gboolean
gimp_layer_rename (GimpItem    *item,
                   const gchar *new_name,
                   const gchar *undo_desc)
{
  GimpLayer *layer = GIMP_LAYER (item);
  GimpImage *image = gimp_item_get_image (item);
  gboolean   attached;
  gboolean   floating_sel;

  attached     = gimp_item_is_attached (item);
  floating_sel = gimp_layer_is_floating_sel (layer);

  if (floating_sel)
    {
      if (GIMP_IS_CHANNEL (layer->fs.drawable))
        return FALSE;

      if (attached)
        {
          gimp_image_undo_group_start (image,
                                       GIMP_UNDO_GROUP_ITEM_PROPERTIES,
                                       undo_desc);

          floating_sel_to_layer (layer);
        }
    }

  GIMP_ITEM_CLASS (parent_class)->rename (item, new_name, undo_desc);

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
  gimp_drawable_update (GIMP_DRAWABLE (layer), 0, 0, item->width, item->height);

  /*  invalidate the selection boundary because of a layer modification  */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (layer));

  if (gimp_layer_is_floating_sel (layer))
    floating_sel_relax (layer, FALSE);

  GIMP_ITEM_CLASS (parent_class)->translate (item, offset_x, offset_y,
                                             push_undo);

  if (gimp_layer_is_floating_sel (layer))
    floating_sel_rigor (layer, FALSE);

  /*  update the new region  */
  gimp_drawable_update (GIMP_DRAWABLE (layer), 0, 0, item->width, item->height);

  if (layer->mask)
    {
      GIMP_ITEM (layer->mask)->offset_x = item->offset_x;
      GIMP_ITEM (layer->mask)->offset_y = item->offset_y;

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

  GIMP_ITEM_CLASS (parent_class)->scale (item, new_width, new_height,
                                         new_offset_x, new_offset_y,
                                         interpolation_type, progress);

  if (layer->mask)
    gimp_item_scale (GIMP_ITEM (layer->mask),
                     new_width, new_height,
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

#ifdef __GNUC__
#warning FIXME: make interpolated transformations work on layers without alpha
#endif
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
gimp_layer_invalidate_boundary (GimpDrawable *drawable)
{
  GimpLayer   *layer = GIMP_LAYER (drawable);
  GimpImage   *image;
  GimpChannel *mask;

  if (! (image = gimp_item_get_image (GIMP_ITEM (layer))))
    return;

  /*  Turn the current selection off  */
  gimp_image_selection_control (image, GIMP_SELECTION_OFF);

  /*  clear the affected region surrounding the layer  */
  gimp_image_selection_control (image, GIMP_SELECTION_LAYER_OFF);

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

static gint
gimp_layer_get_opacity_at (GimpPickable *pickable,
                           gint          x,
                           gint          y)
{
  GimpLayer *layer = GIMP_LAYER (pickable);
  Tile      *tile;
  gint       val   = 0;

  if (x >= 0 && x < GIMP_ITEM (layer)->width &&
      y >= 0 && y < GIMP_ITEM (layer)->height &&
      gimp_item_get_visible (GIMP_ITEM (layer)))
    {
      /*  If the point is inside, and the layer has no
       *  alpha channel, success!
       */
      if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
        return OPAQUE_OPACITY;

      /*  Otherwise, determine if the alpha value at
       *  the given point is non-zero
       */
      tile = tile_manager_get_tile (GIMP_DRAWABLE (layer)->tiles,
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
                            PixelRegion   *layerPR,
                            PixelRegion   *bufPR,
                            GimpImageType  dest_type,
                            GimpImageType  src_type)
{
  GimpImageBaseType base_type = GIMP_IMAGE_TYPE_BASE_TYPE (src_type);
  gboolean          alpha     = GIMP_IMAGE_TYPE_HAS_ALPHA (src_type);
  gpointer          pr;

  for (pr = pixel_regions_register (2, layerPR, bufPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const guchar *src  = bufPR->data;
      guchar       *dest = layerPR->data;
      gint          h    = layerPR->h;

      while (h--)
        {
          const guchar *s = src;
          guchar       *d = dest;
          gint i;

          for (i = 0; i < layerPR->w; i++)
            {
              gimp_image_transform_color (image, dest_type, d, base_type, s);

              /*  alpha channel  */
              d[layerPR->bytes - 1] = (alpha ?
                                       s[bufPR->bytes - 1] : OPAQUE_OPACITY);

              s += bufPR->bytes;
              d += layerPR->bytes;
            }

          src  += bufPR->rowstride;
          dest += layerPR->rowstride;
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

  if (layer_mask->apply_mask || layer_mask->show_mask)
    {
      gimp_drawable_update (GIMP_DRAWABLE (layer),
                            x, y, width, height);
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

  layer = g_object_new (GIMP_TYPE_LAYER, NULL);

  gimp_drawable_configure (GIMP_DRAWABLE (layer),
                           image,
                           0, 0, width, height,
                           type,
                           name);

  opacity = CLAMP (opacity, GIMP_OPACITY_TRANSPARENT, GIMP_OPACITY_OPAQUE);

  layer->opacity = opacity;
  layer->mode    = mode;

  return layer;
}

/**
 * gimp_layer_new_from_tiles:
 * @tiles:       The buffer to make the new layer from.
 * @dest_image: The image the new layer will be added to.
 * @type:        The #GimpImageType of the new layer.
 * @name:        The new layer's name.
 * @opacity:     The new layer's opacity.
 * @mode:        The new layer's mode.
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
  g_return_val_if_fail (name != NULL, NULL);

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
 * @pixbuf:      The pixbuf to make the new layer from.
 * @dest_image: The image the new layer will be added to.
 * @type:        The #GimpImageType of the new layer.
 * @name:        The new layer's name.
 * @opacity:     The new layer's opacity.
 * @mode:        The new layer's mode.
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
  g_return_val_if_fail (name != NULL, NULL);

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
 * @region:      A readable pixel region.
 * @dest_image: The image the new layer will be added to.
 * @type:        The #GimpImageType of the new layer.
 * @name:        The new layer's name.
 * @opacity:     The new layer's opacity.
 * @mode:        The new layer's mode.
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
  g_return_val_if_fail (name != NULL, NULL);

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

  pixel_region_init (&layerPR, GIMP_DRAWABLE (new_layer)->tiles,
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
          gimp_layer_transform_color (dest_image, &layerPR, region,
                                      type, src_type);
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
          gimp_layer_transform_color (dest_image, &layerPR, region,
                                      type, src_type);
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
          gimp_layer_transform_color (dest_image, &layerPR, region,
                                      type, src_type);
          break;
        case GIMP_GRAY_IMAGE:
        case GIMP_GRAYA_IMAGE:
          gimp_layer_transform_color (dest_image, &layerPR, region,
                                      type, src_type);
          break;
        default:
          g_warning ("%s: unhandled type conversion", G_STRFUNC);
          break;
        }
      break;
    }

  return new_layer;
}

GimpLayerMask *
gimp_layer_add_mask (GimpLayer     *layer,
                     GimpLayerMask *mask,
                     gboolean       push_undo)
{
  GimpImage *image;

  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (GIMP_IS_LAYER_MASK (mask), NULL);

  if (! gimp_item_is_attached (GIMP_ITEM (layer)))
    push_undo = FALSE;

  image = gimp_item_get_image (GIMP_ITEM (layer));

  if (! image)
    {
      g_message (_("Cannot add layer mask to layer "
                   "which is not part of an image."));
      return NULL;
    }

  if (layer->mask)
    {
      g_message (_("Unable to add a layer mask since "
                   "the layer already has one."));
      return NULL;
    }

  if ((gimp_item_width (GIMP_ITEM (layer)) !=
       gimp_item_width (GIMP_ITEM (mask))) ||
      (gimp_item_height (GIMP_ITEM (layer)) !=
       gimp_item_height (GIMP_ITEM (mask))))
    {
      g_message (_("Cannot add layer mask of different "
                   "dimensions than specified layer."));
      return NULL;
    }

  if (push_undo)
    gimp_image_undo_push_layer_mask_add (image, _("Add Layer Mask"),
                                         layer, mask);

  layer->mask = g_object_ref_sink (mask);

  gimp_layer_mask_set_layer (mask, layer);

  if (mask->apply_mask || mask->show_mask)
    {
      gimp_drawable_update (GIMP_DRAWABLE (layer),
                            0, 0,
                            GIMP_ITEM (layer)->width,
                            GIMP_ITEM (layer)->height);
    }

  g_signal_connect (mask, "update",
                    G_CALLBACK (gimp_layer_layer_mask_update),
                    layer);

  g_signal_emit (layer, layer_signals[MASK_CHANGED], 0);

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
                               gimp_object_get_name (GIMP_OBJECT (layer)));

  mask = gimp_layer_mask_new (image,
                              item->width,
                              item->height,
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
                     GIMP_ITEM (mask)->width,
                     GIMP_ITEM (mask)->height,
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
                             item->width, item->height,
                             FALSE);

          extract_alpha_region (&srcPR, NULL, &destPR);

          if (add_mask_type == GIMP_ADD_ALPHA_TRANSFER_MASK)
            {
              void   *pr;
              gint    w, h;
              guchar *alpha_ptr;

              gimp_drawable_push_undo (drawable,
                                       _("Transfer Alpha to Mask"),
                                       0, 0,
                                       item->width,
                                       item->height,
                                       NULL, FALSE);

              pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                                 0, 0,
                                 item->width, item->height,
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
        gint     copy_x, copy_y;
        gint     copy_width, copy_height;

        if (add_mask_type == GIMP_ADD_SELECTION_MASK)
          channel = GIMP_CHANNEL (gimp_image_get_mask (image));

        channel_empty = gimp_channel_is_empty (channel);

        gimp_rectangle_intersect (0, 0, image->width, image->height,
                                  item->offset_x, item->offset_y,
                                  item->width, item->height,
                                  &copy_x, &copy_y, &copy_width, &copy_height);

        if (copy_width  < item->width  ||
            copy_height < item->height ||
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
                               copy_x - item->offset_x, copy_y - item->offset_y,
                               copy_width, copy_height,
                               TRUE);

            copy_region (&srcPR, &destPR);

            GIMP_CHANNEL (mask)->bounds_known = FALSE;
          }
      }
      break;

    case GIMP_ADD_COPY_MASK:
      {
        TileManager   *copy_tiles = NULL;
        GimpImageType  layer_type;

        layer_type = drawable->type;

        if (GIMP_IMAGE_TYPE_BASE_TYPE (layer_type) != GIMP_GRAY)
          {
            GimpImageType copy_type;

            copy_type = (GIMP_IMAGE_TYPE_HAS_ALPHA (layer_type) ?
                         GIMP_GRAYA_IMAGE : GIMP_GRAY_IMAGE);

            copy_tiles = tile_manager_new (item->width,
                                           item->height,
                                           GIMP_IMAGE_TYPE_BYTES (copy_type));

            gimp_drawable_convert_grayscale (drawable,
                                             copy_tiles,
                                             GIMP_IMAGE_TYPE_BASE_TYPE (layer_type));

            pixel_region_init (&srcPR, copy_tiles,
                               0, 0,
                               item->width,
                               item->height,
                               FALSE);
          }
        else
          {
            pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                               0, 0,
                               item->width,
                               item->height,
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
  GimpItem    *item;
  GimpImage   *image;
  PixelRegion  srcPR, maskPR;
  gboolean     view_changed = FALSE;

  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (! layer->mask)
    return;

  /*  this operation can only be done to layers with an alpha channel  */
  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    return;

  item = GIMP_ITEM (layer);

  image = gimp_item_get_image (item);

  if (! image)
    return;

  if (push_undo)
    {
      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_LAYER_APPLY_MASK,
                                   (mode == GIMP_MASK_APPLY) ?
                                   _("Apply Layer Mask") :
                                   _("Delete Layer Mask"));

      gimp_image_undo_push_layer_mask_remove (image, NULL, layer, layer->mask);
    }

  /*  check if applying the mask changes the projection  */
  if (layer->mask->show_mask                                   ||
      (mode == GIMP_MASK_APPLY   && ! layer->mask->apply_mask) ||
      (mode == GIMP_MASK_DISCARD &&   layer->mask->apply_mask))
    {
      view_changed = TRUE;
    }

  if (mode == GIMP_MASK_APPLY)
    {
      if (push_undo)
        gimp_drawable_push_undo (GIMP_DRAWABLE (layer), NULL,
                                 0, 0,
                                 item->width,
                                 item->height,
                                 NULL, FALSE);

      /*  Combine the current layer's alpha channel and the mask  */
      pixel_region_init (&srcPR, GIMP_DRAWABLE (layer)->tiles,
                         0, 0,
                         item->width,
                         item->height,
                         TRUE);
      pixel_region_init (&maskPR, GIMP_DRAWABLE (layer->mask)->tiles,
                         0, 0,
                         item->width,
                         item->height,
                         FALSE);

      apply_mask_to_region (&srcPR, &maskPR, OPAQUE_OPACITY);
    }

  g_signal_handlers_disconnect_by_func (layer->mask,
                                        gimp_layer_layer_mask_update,
                                        layer);

  gimp_item_removed (GIMP_ITEM (layer->mask));
  g_object_unref (layer->mask);
  layer->mask = NULL;

  if (push_undo)
    gimp_image_undo_group_end (image);

  /*  If applying actually changed the view  */
  if (view_changed)
    {
      gimp_drawable_update (GIMP_DRAWABLE (layer),
                            0, 0,
                            item->width,
                            item->height);
    }
  else
    {
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer));
    }

  g_signal_emit (layer, layer_signals[MASK_CHANGED], 0);
}

void
gimp_layer_add_alpha (GimpLayer *layer)
{
  PixelRegion    srcPR, destPR;
  TileManager   *new_tiles;
  GimpImageType  new_type;

  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    return;

  new_type = gimp_drawable_type_with_alpha (GIMP_DRAWABLE (layer));

  /*  Allocate the new tiles  */
  new_tiles = tile_manager_new (GIMP_ITEM (layer)->width,
                                GIMP_ITEM (layer)->height,
                                GIMP_IMAGE_TYPE_BYTES (new_type));

  /*  Configure the pixel regions  */
  pixel_region_init (&srcPR, GIMP_DRAWABLE (layer)->tiles,
                     0, 0,
                     GIMP_ITEM (layer)->width,
                     GIMP_ITEM (layer)->height,
                     FALSE);
  pixel_region_init (&destPR, new_tiles,
                     0, 0,
                     GIMP_ITEM (layer)->width,
                     GIMP_ITEM (layer)->height,
                     TRUE);

  /*  Add an alpha channel  */
  add_alpha_region (&srcPR, &destPR);

  /*  Set the new tiles  */
  gimp_drawable_set_tiles_full (GIMP_DRAWABLE (layer),
                                gimp_item_is_attached (GIMP_ITEM (layer)),
                                _("Add Alpha Channel"),
                                new_tiles, new_type,
                                GIMP_ITEM (layer)->offset_x,
                                GIMP_ITEM (layer)->offset_y);
  tile_manager_unref (new_tiles);
}

void
gimp_layer_flatten (GimpLayer   *layer,
                    GimpContext *context)
{
  PixelRegion    srcPR, destPR;
  TileManager   *new_tiles;
  GimpImageType  new_type;
  guchar         bg[4];

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    return;

  new_type = gimp_drawable_type_without_alpha (GIMP_DRAWABLE (layer));

  gimp_image_get_background (gimp_item_get_image (GIMP_ITEM (layer)), context,
                             gimp_drawable_type (GIMP_DRAWABLE (layer)),
                             bg);

  /*  Allocate the new tiles  */
  new_tiles = tile_manager_new (GIMP_ITEM (layer)->width,
                                GIMP_ITEM (layer)->height,
                                GIMP_IMAGE_TYPE_BYTES (new_type));

  /*  Configure the pixel regions  */
  pixel_region_init (&srcPR, GIMP_DRAWABLE (layer)->tiles,
                     0, 0,
                     GIMP_ITEM (layer)->width,
                     GIMP_ITEM (layer)->height,
                     FALSE);
  pixel_region_init (&destPR, new_tiles,
                     0, 0,
                     GIMP_ITEM (layer)->width,
                     GIMP_ITEM (layer)->height,
                     TRUE);

  /*  Remove alpha channel  */
  flatten_region (&srcPR, &destPR, bg);

  /*  Set the new tiles  */
  gimp_drawable_set_tiles_full (GIMP_DRAWABLE (layer),
                                gimp_item_is_attached (GIMP_ITEM (layer)),
                                _("Remove Alpha Channel"),
                                new_tiles, new_type,
                                GIMP_ITEM (layer)->offset_x,
                                GIMP_ITEM (layer)->offset_y);
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
                               _("Layer to Image Size"));

  gimp_item_offsets (GIMP_ITEM (layer), &offset_x, &offset_y);
  gimp_item_resize (GIMP_ITEM (layer), context,
                    image->width, image->height, offset_x, offset_y);

  gimp_image_undo_group_end (image);
}

BoundSeg *
gimp_layer_boundary (GimpLayer *layer,
                     gint      *num_segs)
{
  GimpItem *item;
  BoundSeg *new_segs;

  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);

  item = GIMP_ITEM (layer);

  /*  Create the four boundary segments that encompass this
   *  layer's boundary.
   */
  new_segs  = g_new (BoundSeg, 4);
  *num_segs = 4;

  /*  if the layer is a floating selection  */
  if (gimp_layer_is_floating_sel (layer))
    {
      if (GIMP_IS_CHANNEL (layer->fs.drawable))
        {
          /*  if the owner drawable is a channel, just return nothing  */

          g_free (new_segs);
          *num_segs = 0;
          return NULL;
        }
      else
        {
          /*  otherwise, set the layer to the owner drawable  */

          layer = GIMP_LAYER (layer->fs.drawable);
        }
    }

  new_segs[0].x1   = item->offset_x;
  new_segs[0].y1   = item->offset_y;
  new_segs[0].x2   = item->offset_x;
  new_segs[0].y2   = item->offset_y + item->height;
  new_segs[0].open = 1;

  new_segs[1].x1   = item->offset_x;
  new_segs[1].y1   = item->offset_y;
  new_segs[1].x2   = item->offset_x + item->width;
  new_segs[1].y2   = item->offset_y;
  new_segs[1].open = 1;

  new_segs[2].x1   = item->offset_x + item->width;
  new_segs[2].y1   = item->offset_y;
  new_segs[2].x2   = item->offset_x + item->width;
  new_segs[2].y2   = item->offset_y + item->height;
  new_segs[2].open = 0;

  new_segs[3].x1   = item->offset_x;
  new_segs[3].y1   = item->offset_y + item->height;
  new_segs[3].x2   = item->offset_x + item->width;
  new_segs[3].y2   = item->offset_y + item->height;
  new_segs[3].open = 0;

  return new_segs;
}

/**********************/
/*  access functions  */
/**********************/

GimpLayerMask *
gimp_layer_get_mask (const GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);

  return layer->mask;
}

gboolean
gimp_layer_is_floating_sel (const GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  return (layer->fs.drawable != NULL);
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

      gimp_drawable_update (GIMP_DRAWABLE (layer),
                            0, 0,
                            GIMP_ITEM (layer)->width,
                            GIMP_ITEM (layer)->height);
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

      gimp_drawable_update (GIMP_DRAWABLE (layer),
                            0, 0,
                            GIMP_ITEM (layer)->width,
                            GIMP_ITEM (layer)->height);
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
