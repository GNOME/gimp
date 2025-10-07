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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-nodes.h"

#include "gimpboundary.h"
#include "gimpchannel-select.h"
#include "gimpcontext.h"
#include "gimpcontainer.h"
#include "gimpdrawable-floating-selection.h"
#include "gimperror.h"
#include "gimpgrouplayer.h"
#include "gimpimage-undo-push.h"
#include "gimpimage-undo.h"
#include "gimpimage.h"
#include "gimpimage-color-profile.h"
#include "gimplayer-floating-selection.h"
#include "gimplayer.h"
#include "gimplayermask.h"
#include "gimpobjectqueue.h"
#include "gimppickable.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


enum
{
  OPACITY_CHANGED,
  MODE_CHANGED,
  BLEND_SPACE_CHANGED,
  COMPOSITE_SPACE_CHANGED,
  COMPOSITE_MODE_CHANGED,
  EFFECTIVE_MODE_CHANGED,
  EXCLUDES_BACKDROP_CHANGED,
  LOCK_ALPHA_CHANGED,
  MASK_CHANGED,
  APPLY_MASK_CHANGED,
  EDIT_MASK_CHANGED,
  SHOW_MASK_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_OPACITY,
  PROP_MODE,
  PROP_BLEND_SPACE,
  PROP_COMPOSITE_SPACE,
  PROP_COMPOSITE_MODE,
  PROP_EXCLUDES_BACKDROP,
  PROP_LOCK_ALPHA,
  PROP_MASK,
  PROP_FLOATING_SELECTION
};


static void       gimp_color_managed_iface_init (GimpColorManagedInterface *iface);
static void       gimp_pickable_iface_init      (GimpPickableInterface     *iface);

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
static void       gimp_layer_notify             (GObject            *object,
                                                 GParamSpec         *pspec);

static void       gimp_layer_name_changed       (GimpObject         *object);
static gint64     gimp_layer_get_memsize        (GimpObject         *object,
                                                 gint64             *gui_size);

static void       gimp_layer_invalidate_preview (GimpViewable       *viewable);
static gchar    * gimp_layer_get_description    (GimpViewable       *viewable,
                                                 gchar             **tooltip);

static GeglNode * gimp_layer_get_node           (GimpFilter         *filter);

static void       gimp_layer_removed            (GimpItem           *item);
static void       gimp_layer_unset_removed      (GimpItem           *item);
static gboolean   gimp_layer_is_attached        (GimpItem           *item);
static GimpItemTree * gimp_layer_get_tree       (GimpItem           *item);
static GimpItem * gimp_layer_duplicate          (GimpItem           *item,
                                                 GType               new_type);
static void       gimp_layer_convert            (GimpItem           *item,
                                                 GimpImage          *dest_image,
                                                 GType               old_type);
static gboolean   gimp_layer_rename             (GimpItem           *item,
                                                 const gchar        *new_name,
                                                 const gchar        *undo_desc,
                                                 GError            **error);
static void       gimp_layer_start_move         (GimpItem           *item,
                                                 gboolean            push_undo);
static void       gimp_layer_end_move           (GimpItem           *item,
                                                 gboolean            push_undo);
static void       gimp_layer_translate          (GimpItem           *item,
                                                 gdouble             offset_x,
                                                 gdouble             offset_y,
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
                                                 GimpFillType        fill_type,
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
                                                 GimpTransformResize clip_result,
                                                 GimpProgress       *progress,
                                                 gboolean            push_undo);
static void       gimp_layer_to_selection       (GimpItem           *item,
                                                 GimpChannelOps      op,
                                                 gboolean            antialias,
                                                 gboolean            feather,
                                                 gdouble             feather_radius_x,
                                                 gdouble             feather_radius_y);

static void       gimp_layer_alpha_changed      (GimpDrawable       *drawable);
static gint64     gimp_layer_estimate_memsize   (GimpDrawable       *drawable,
                                                 GimpComponentType   component_type,
                                                 gint                width,
                                                 gint                height);
static gboolean   gimp_layer_supports_alpha     (GimpDrawable       *drawable);
static void       gimp_layer_convert_type       (GimpDrawable       *drawable,
                                                 GimpImage          *dest_image,
                                                 const Babl         *new_format,
                                                 GimpColorProfile   *src_profile,
                                                 GimpColorProfile   *dest_profile,
                                                 GeglDitherMethod    layer_dither_type,
                                                 GeglDitherMethod    mask_dither_type,
                                                 gboolean            push_undo,
                                                 GimpProgress       *progress);
static void    gimp_layer_invalidate_boundary   (GimpDrawable       *drawable);
static void    gimp_layer_get_active_components (GimpDrawable       *drawable,
                                                 gboolean           *active);
static GimpComponentMask
               gimp_layer_get_active_mask       (GimpDrawable       *drawable);
static void    gimp_layer_set_buffer            (GimpDrawable       *drawable,
                                                 gboolean            push_undo,
                                                 const gchar        *undo_desc,
                                                 GeglBuffer         *buffer,
                                                 const GeglRectangle *bounds);
static GeglRectangle
               gimp_layer_get_bounding_box      (GimpDrawable       *drawable);

static GimpColorProfile *
               gimp_layer_get_color_profile     (GimpColorManaged   *managed);

static gdouble gimp_layer_get_opacity_at        (GimpPickable       *pickable,
                                                 gint                x,
                                                 gint                y);

static gboolean gimp_layer_real_is_alpha_locked (GimpLayer          *layer,
                                                 GimpLayer         **locked_layer);
static void       gimp_layer_real_translate     (GimpLayer          *layer,
                                                 gint                offset_x,
                                                 gint                offset_y);
static void       gimp_layer_real_scale         (GimpLayer          *layer,
                                                 gint                new_width,
                                                 gint                new_height,
                                                 gint                new_offset_x,
                                                 gint                new_offset_y,
                                                 GimpInterpolationType  interp_type,
                                                 GimpProgress       *progress);
static void       gimp_layer_real_resize        (GimpLayer          *layer,
                                                 GimpContext        *context,
                                                 GimpFillType        fill_type,
                                                 gint                new_width,
                                                 gint                new_height,
                                                 gint                offset_x,
                                                 gint                offset_y);
static void       gimp_layer_real_flip          (GimpLayer          *layer,
                                                 GimpContext        *context,
                                                 GimpOrientationType flip_type,
                                                 gdouble             axis,
                                                 gboolean            clip_result);
static void       gimp_layer_real_rotate        (GimpLayer          *layer,
                                                 GimpContext        *context,
                                                 GimpRotationType    rotate_type,
                                                 gdouble             center_x,
                                                 gdouble             center_y,
                                                 gboolean            clip_result);
static void       gimp_layer_real_transform     (GimpLayer          *layer,
                                                 GimpContext        *context,
                                                 const GimpMatrix3  *matrix,
                                                 GimpTransformDirection direction,
                                                 GimpInterpolationType  interpolation_type,
                                                 GimpTransformResize clip_result,
                                                 GimpProgress       *progress,
                                                 gboolean            push_undo);
static void       gimp_layer_real_convert_type  (GimpLayer          *layer,
                                                 GimpImage          *dest_image,
                                                 const Babl         *new_format,
                                                 GimpColorProfile   *src_profile,
                                                 GimpColorProfile   *dest_profile,
                                                 GeglDitherMethod    layer_dither_type,
                                                 GeglDitherMethod    mask_dither_type,
                                                 gboolean            push_undo,
                                                 GimpProgress       *progress);
static GeglRectangle
               gimp_layer_real_get_bounding_box (GimpLayer          *layer);
static void  gimp_layer_real_get_effective_mode (GimpLayer          *layer,
                                                 GimpLayerMode          *mode,
                                                 GimpLayerColorSpace    *blend_space,
                                                 GimpLayerColorSpace    *composite_space,
                                                 GimpLayerCompositeMode *composite_mode);
static gboolean
          gimp_layer_real_get_excludes_backdrop (GimpLayer          *layer);

static void       gimp_layer_layer_mask_update  (GimpDrawable       *layer_mask,
                                                 gint                x,
                                                 gint                y,
                                                 gint                width,
                                                 gint                height,
                                                 GimpLayer          *layer);


G_DEFINE_TYPE_WITH_CODE (GimpLayer, gimp_layer, GIMP_TYPE_DRAWABLE,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_COLOR_MANAGED,
                                                gimp_color_managed_iface_init)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PICKABLE,
                                                gimp_pickable_iface_init))

#define parent_class gimp_layer_parent_class

static guint layer_signals[LAST_SIGNAL] = { 0 };


static void
gimp_layer_class_init (GimpLayerClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpFilterClass   *filter_class      = GIMP_FILTER_CLASS (klass);
  GimpItemClass     *item_class        = GIMP_ITEM_CLASS (klass);
  GimpDrawableClass *drawable_class    = GIMP_DRAWABLE_CLASS (klass);

  layer_signals[OPACITY_CHANGED] =
    g_signal_new ("opacity-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLayerClass, opacity_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[MODE_CHANGED] =
    g_signal_new ("mode-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLayerClass, mode_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[BLEND_SPACE_CHANGED] =
    g_signal_new ("blend-space-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLayerClass, blend_space_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[COMPOSITE_SPACE_CHANGED] =
    g_signal_new ("composite-space-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLayerClass, composite_space_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[COMPOSITE_MODE_CHANGED] =
    g_signal_new ("composite-mode-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLayerClass, composite_mode_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[EFFECTIVE_MODE_CHANGED] =
    g_signal_new ("effective-mode-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLayerClass, effective_mode_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[EXCLUDES_BACKDROP_CHANGED] =
    g_signal_new ("excludes-backdrop-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLayerClass, excludes_backdrop_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[LOCK_ALPHA_CHANGED] =
    g_signal_new ("lock-alpha-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLayerClass, lock_alpha_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[MASK_CHANGED] =
    g_signal_new ("mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLayerClass, mask_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[APPLY_MASK_CHANGED] =
    g_signal_new ("apply-mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLayerClass, apply_mask_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[EDIT_MASK_CHANGED] =
    g_signal_new ("edit-mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLayerClass, edit_mask_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[SHOW_MASK_CHANGED] =
    g_signal_new ("show-mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLayerClass, show_mask_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->set_property          = gimp_layer_set_property;
  object_class->get_property          = gimp_layer_get_property;
  object_class->dispose               = gimp_layer_dispose;
  object_class->finalize              = gimp_layer_finalize;
  object_class->notify                = gimp_layer_notify;

  gimp_object_class->name_changed     = gimp_layer_name_changed;
  gimp_object_class->get_memsize      = gimp_layer_get_memsize;

  viewable_class->default_icon_name   = "gimp-layer";
  viewable_class->default_name        = _("Layer");
  viewable_class->invalidate_preview  = gimp_layer_invalidate_preview;
  viewable_class->get_description     = gimp_layer_get_description;

  filter_class->get_node              = gimp_layer_get_node;

  item_class->removed                 = gimp_layer_removed;
  item_class->unset_removed           = gimp_layer_unset_removed;
  item_class->is_attached             = gimp_layer_is_attached;
  item_class->get_tree                = gimp_layer_get_tree;
  item_class->duplicate               = gimp_layer_duplicate;
  item_class->convert                 = gimp_layer_convert;
  item_class->rename                  = gimp_layer_rename;
  item_class->start_move              = gimp_layer_start_move;
  item_class->end_move                = gimp_layer_end_move;
  item_class->translate               = gimp_layer_translate;
  item_class->scale                   = gimp_layer_scale;
  item_class->resize                  = gimp_layer_resize;
  item_class->flip                    = gimp_layer_flip;
  item_class->rotate                  = gimp_layer_rotate;
  item_class->transform               = gimp_layer_transform;
  item_class->to_selection            = gimp_layer_to_selection;
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

  drawable_class->alpha_changed         = gimp_layer_alpha_changed;
  drawable_class->estimate_memsize      = gimp_layer_estimate_memsize;
  drawable_class->supports_alpha        = gimp_layer_supports_alpha;
  drawable_class->convert_type          = gimp_layer_convert_type;
  drawable_class->invalidate_boundary   = gimp_layer_invalidate_boundary;
  drawable_class->get_active_components = gimp_layer_get_active_components;
  drawable_class->get_active_mask       = gimp_layer_get_active_mask;
  drawable_class->set_buffer            = gimp_layer_set_buffer;
  drawable_class->get_bounding_box      = gimp_layer_get_bounding_box;

  klass->opacity_changed              = NULL;
  klass->mode_changed                 = NULL;
  klass->blend_space_changed          = NULL;
  klass->composite_space_changed      = NULL;
  klass->composite_mode_changed       = NULL;
  klass->excludes_backdrop_changed    = NULL;
  klass->lock_alpha_changed           = NULL;
  klass->mask_changed                 = NULL;
  klass->apply_mask_changed           = NULL;
  klass->edit_mask_changed            = NULL;
  klass->show_mask_changed            = NULL;
  klass->is_alpha_locked              = gimp_layer_real_is_alpha_locked;
  klass->translate                    = gimp_layer_real_translate;
  klass->scale                        = gimp_layer_real_scale;
  klass->resize                       = gimp_layer_real_resize;
  klass->flip                         = gimp_layer_real_flip;
  klass->rotate                       = gimp_layer_real_rotate;
  klass->transform                    = gimp_layer_real_transform;
  klass->convert_type                 = gimp_layer_real_convert_type;
  klass->get_bounding_box             = gimp_layer_real_get_bounding_box;
  klass->get_effective_mode           = gimp_layer_real_get_effective_mode;
  klass->get_excludes_backdrop        = gimp_layer_real_get_excludes_backdrop;

  g_object_class_install_property (object_class, PROP_OPACITY,
                                   g_param_spec_double ("opacity", NULL, NULL,
                                                        GIMP_OPACITY_TRANSPARENT,
                                                        GIMP_OPACITY_OPAQUE,
                                                        GIMP_OPACITY_OPAQUE,
                                                        GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_MODE,
                                   g_param_spec_enum ("mode", NULL, NULL,
                                                      GIMP_TYPE_LAYER_MODE,
                                                      GIMP_LAYER_MODE_NORMAL,
                                                      GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_BLEND_SPACE,
                                   g_param_spec_enum ("blend-space",
                                                      NULL, NULL,
                                                      GIMP_TYPE_LAYER_COLOR_SPACE,
                                                      GIMP_LAYER_COLOR_SPACE_AUTO,
                                                      GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_COMPOSITE_SPACE,
                                   g_param_spec_enum ("composite-space",
                                                      NULL, NULL,
                                                      GIMP_TYPE_LAYER_COLOR_SPACE,
                                                      GIMP_LAYER_COLOR_SPACE_AUTO,
                                                      GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_COMPOSITE_MODE,
                                   g_param_spec_enum ("composite-mode",
                                                      NULL, NULL,
                                                      GIMP_TYPE_LAYER_COMPOSITE_MODE,
                                                      GIMP_LAYER_COMPOSITE_AUTO,
                                                      GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_EXCLUDES_BACKDROP,
                                   g_param_spec_boolean ("excludes-backdrop",
                                                         NULL, NULL,
                                                         FALSE,
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
  layer->opacity                   = GIMP_OPACITY_OPAQUE;
  layer->mode                      = GIMP_LAYER_MODE_NORMAL;
  layer->blend_space               = GIMP_LAYER_COLOR_SPACE_AUTO;
  layer->composite_space           = GIMP_LAYER_COLOR_SPACE_AUTO;
  layer->composite_mode            = GIMP_LAYER_COMPOSITE_AUTO;
  layer->effective_mode            = layer->mode;
  layer->effective_blend_space     = gimp_layer_get_real_blend_space (layer);
  layer->effective_composite_space = gimp_layer_get_real_composite_space (layer);
  layer->effective_composite_mode  = gimp_layer_get_real_composite_mode (layer);
  layer->excludes_backdrop         = FALSE;
  layer->lock_alpha                = FALSE;

  layer->mask       = NULL;
  layer->apply_mask = TRUE;
  layer->edit_mask  = TRUE;
  layer->show_mask  = FALSE;

  /*  floating selection  */
  layer->fs.drawable       = NULL;
  layer->fs.boundary_known = FALSE;
  layer->fs.segs           = NULL;
  layer->fs.num_segs       = 0;
}

static void
gimp_color_managed_iface_init (GimpColorManagedInterface *iface)
{
  iface->get_color_profile = gimp_layer_get_color_profile;
}

static void
gimp_pickable_iface_init (GimpPickableInterface *iface)
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
    case PROP_BLEND_SPACE:
      g_value_set_enum (value, gimp_layer_get_blend_space (layer));
      break;
    case PROP_COMPOSITE_SPACE:
      g_value_set_enum (value, gimp_layer_get_composite_space (layer));
      break;
    case PROP_COMPOSITE_MODE:
      g_value_set_enum (value, gimp_layer_get_composite_mode (layer));
      break;
    case PROP_EXCLUDES_BACKDROP:
      g_value_set_boolean (value, gimp_layer_get_excludes_backdrop (layer));
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
       * layer might be on the undo stack and not attached to anything
       */
      if (gimp_drawable_get_floating_sel (fs_drawable) == layer)
        gimp_drawable_detach_floating_sel (fs_drawable);

      gimp_layer_set_floating_sel_drawable (layer, NULL);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_layer_finalize (GObject *object)
{
  GimpLayer *layer = GIMP_LAYER (object);

  g_clear_object (&layer->mask);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_layer_update_mode_node (GimpLayer *layer)
{
  GeglNode               *mode_node;
  GimpLayerMode           visible_mode;
  GimpLayerColorSpace     visible_blend_space;
  GimpLayerColorSpace     visible_composite_space;
  GimpLayerCompositeMode  visible_composite_mode;

  mode_node = gimp_drawable_get_mode_node (GIMP_DRAWABLE (layer));

  if (layer->mask && layer->show_mask)
    {
      visible_mode            = GIMP_LAYER_MODE_NORMAL;
      visible_blend_space     = GIMP_LAYER_COLOR_SPACE_AUTO;
      visible_composite_space = GIMP_LAYER_COLOR_SPACE_AUTO;
      visible_composite_mode  = GIMP_LAYER_COMPOSITE_AUTO;

      /* This makes sure that masks of LEGACY-mode layers are
       * composited in PERCEPTUAL space, and non-LEGACY layers in
       * LINEAR space, or whatever composite space was chosen in the
       * layer attributes dialog
       */
      visible_composite_space = gimp_layer_get_real_composite_space (layer);
    }
  else
    {
      visible_mode            = layer->effective_mode;
      visible_blend_space     = layer->effective_blend_space;
      visible_composite_space = layer->effective_composite_space;
      visible_composite_mode  = layer->effective_composite_mode;
    }

  gimp_gegl_mode_node_set_mode (mode_node,
                                visible_mode,
                                visible_blend_space,
                                visible_composite_space,
                                visible_composite_mode);
  gimp_gegl_mode_node_set_opacity (mode_node, layer->opacity);
}

static void
gimp_layer_notify (GObject    *object,
                   GParamSpec *pspec)
{
  if (! strcmp (pspec->name, "is-last-node") &&
      gimp_filter_peek_node (GIMP_FILTER (object)))
    {
      gimp_layer_update_mode_node (GIMP_LAYER (object));

      gimp_drawable_update (GIMP_DRAWABLE (object), 0, 0, -1, -1);
    }
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

  *gui_size += layer->fs.num_segs * sizeof (GimpBoundSeg);

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
      GimpDrawable *drawable;
      const gchar  *header = _("Floating Selection");

      drawable = gimp_layer_get_floating_sel_drawable (GIMP_LAYER (viewable));
      if (GIMP_IS_LAYER_MASK (drawable))
        header = _("Floating Mask");
      else if (GIMP_IS_LAYER (drawable))
        header = _("Floating Layer");
      /* TRANSLATORS: the first %s will be the type of floating item, i.e.
       * either a "Floating Layer" or "Floating Mask" usually. The second will
       * be a layer name.
       */
      return g_strdup_printf (_("%s\n(%s)"),
                              header, gimp_object_get_name (viewable));
    }

  return GIMP_VIEWABLE_CLASS (parent_class)->get_description (viewable,
                                                              tooltip);
}

static GeglNode *
gimp_layer_get_node (GimpFilter *filter)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (filter);
  GimpLayer    *layer    = GIMP_LAYER (filter);
  GeglNode     *node;
  GeglNode     *input;
  GeglNode     *source;
  GeglNode     *mode_node;
  gboolean      source_node_hijacked = FALSE;

  node = GIMP_FILTER_CLASS (parent_class)->get_node (filter);

  input = gegl_node_get_input_proxy (node, "input");

  source = gimp_drawable_get_source_node (drawable);

  /* if the source node already has a parent, we are a floating
   * selection and the source node has been hijacked by the fs'
   * drawable
   */
  if (gegl_node_get_parent (source))
    source_node_hijacked = TRUE;

  if (! source_node_hijacked)
    gegl_node_add_child (node, source);

  gegl_node_connect (input, "output", source, "input");

  g_warn_if_fail (layer->layer_offset_node == NULL);
  g_warn_if_fail (layer->mask_offset_node == NULL);

  /* the mode node connects it all, and has aux and aux2 inputs for
   * the layer and its mask
   */
  mode_node = gimp_drawable_get_mode_node (drawable);
  gimp_layer_update_mode_node (layer);

  /* the layer's offset node */
  layer->layer_offset_node = gegl_node_new_child (node,
                                                  "operation", "gegl:translate",
                                                  NULL);
  gimp_item_add_offset_node (GIMP_ITEM (layer), layer->layer_offset_node);

  /* the layer mask's offset node */
  layer->mask_offset_node = gegl_node_new_child (node,
                                                 "operation", "gegl:translate",
                                                  NULL);
  gimp_item_add_offset_node (GIMP_ITEM (layer), layer->mask_offset_node);

  if (! source_node_hijacked)
    {
      gegl_node_connect (source,                   "output",
                         layer->layer_offset_node, "input");
    }

  if (! (layer->mask && gimp_layer_get_show_mask (layer)))
    {
      gegl_node_connect (layer->layer_offset_node, "output",
                         mode_node,                "aux");
    }

  if (layer->mask)
    {
      GeglNode *mask;

      mask = gimp_drawable_get_source_node (GIMP_DRAWABLE (layer->mask));

      gegl_node_connect (mask,                    "output",
                         layer->mask_offset_node, "input");

      if (gimp_layer_get_show_mask (layer))
        {
          gegl_node_connect (layer->mask_offset_node, "output",
                             mode_node,               "aux");
        }
      else if (gimp_layer_get_apply_mask (layer))
        {
          gegl_node_connect (layer->mask_offset_node, "output",
                             mode_node,               "aux2");
        }
    }

  return node;
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
gimp_layer_is_attached (GimpItem *item)
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

      /* PASS_THROUGH mode is invalid for regular layers.
       * We used to change the mode to NORMAL *before* duplicating (see
       * #793714 on bugzilla) but it would change the image's render.
       * Instead we first duplicate so that the group's render is used
       * as-is for the non-group duplicate layer. Then we set NORMAL
       * mode.
       */
      if (gimp_layer_get_mode (layer) == GIMP_LAYER_MODE_PASS_THROUGH &&
          ! GIMP_IS_GROUP_LAYER (new_item))
        {
          GimpLayerColorSpace    blend_space;
          GimpLayerColorSpace    composite_space;
          GimpLayerCompositeMode composite_mode;

          /* keep the group's current blend space, composite space, and composite
           * mode.
           */
          blend_space     = gimp_layer_get_blend_space     (layer);
          composite_space = gimp_layer_get_composite_space (layer);
          composite_mode  = gimp_layer_get_composite_mode  (layer);

          gimp_layer_set_mode            (new_layer, GIMP_LAYER_MODE_NORMAL, FALSE);
          gimp_layer_set_blend_space     (new_layer, blend_space,            FALSE);
          gimp_layer_set_composite_space (new_layer, composite_space,        FALSE);
          gimp_layer_set_composite_mode  (new_layer, composite_mode,         FALSE);
          gimp_layer_set_opacity         (new_layer, 1.0,                    FALSE);
        }
      else
        {
          gimp_layer_set_mode            (new_layer,
                                          gimp_layer_get_mode (layer), FALSE);
          gimp_layer_set_blend_space     (new_layer,
                                          gimp_layer_get_blend_space (layer), FALSE);
          gimp_layer_set_composite_space (new_layer,
                                          gimp_layer_get_composite_space (layer), FALSE);
          gimp_layer_set_composite_mode  (new_layer,
                                          gimp_layer_get_composite_mode (layer), FALSE);
          gimp_layer_set_opacity         (new_layer,
                                          gimp_layer_get_opacity (layer), FALSE);
        }

      if (gimp_layer_can_lock_alpha (new_layer))
        gimp_layer_set_lock_alpha (new_layer,
                                   gimp_layer_get_lock_alpha (layer), FALSE);

      /*  duplicate the layer mask if necessary  */
      if (layer->mask)
        {
          GimpItem *mask;

          mask = gimp_item_duplicate (GIMP_ITEM (layer->mask),
                                      G_TYPE_FROM_INSTANCE (layer->mask));
          gimp_layer_add_mask (new_layer, GIMP_LAYER_MASK (mask),
                               layer->edit_mask, FALSE, NULL);

          new_layer->apply_mask = layer->apply_mask;
          new_layer->show_mask  = layer->show_mask;
        }
    }

  return new_item;
}

static void
gimp_layer_convert (GimpItem  *item,
                    GimpImage *dest_image,
                    GType      old_type)
{
  GimpLayer         *layer    = GIMP_LAYER (item);
  GimpDrawable      *drawable = GIMP_DRAWABLE (item);
  GimpImageBaseType  old_base_type;
  GimpImageBaseType  new_base_type;
  GimpPrecision      old_precision;
  GimpPrecision      new_precision;
  GimpColorProfile  *src_profile  = NULL;
  GimpColorProfile  *dest_profile = NULL;

  old_base_type = gimp_drawable_get_base_type (drawable);
  new_base_type = gimp_image_get_base_type (dest_image);

  old_precision = gimp_drawable_get_precision (drawable);
  new_precision = gimp_image_get_precision (dest_image);

  if (g_type_is_a (old_type, GIMP_TYPE_LAYER))
    {
      src_profile =
        gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (item));

      dest_profile =
        gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (dest_image));

      if (gimp_color_profile_is_equal (dest_profile, src_profile))
        {
          src_profile  = NULL;
          dest_profile = NULL;
        }
    }

  if (old_base_type != new_base_type ||
      old_precision != new_precision ||
      dest_profile)
    {
      gimp_drawable_convert_type (drawable, dest_image,
                                  new_base_type,
                                  new_precision,
                                  gimp_drawable_has_alpha (drawable),
                                  src_profile,
                                  dest_profile,
                                  GEGL_DITHER_NONE, GEGL_DITHER_NONE,
                                  FALSE, NULL);
    }

  if (layer->mask)
    gimp_item_set_image (GIMP_ITEM (layer->mask), dest_image);

  GIMP_ITEM_CLASS (parent_class)->convert (item, dest_image, old_type);
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
gimp_layer_start_move (GimpItem *item,
                       gboolean  push_undo)
{
  GimpLayer *layer     = GIMP_LAYER (item);
  GimpLayer *ancestor  = layer;
  GSList    *ancestors = NULL;

  /* suspend mask cropping for all of the layer's ancestors */
  while ((ancestor = gimp_layer_get_parent (ancestor)))
    {
      gimp_group_layer_suspend_mask (GIMP_GROUP_LAYER (ancestor), push_undo);

      ancestors = g_slist_prepend (ancestors, g_object_ref (ancestor));
    }

  /* we keep the ancestor list around, so that we can resume mask cropping for
   * the same set of groups in gimp_layer_end_move().  note that
   * gimp_image_remove_layer() calls start_move() before removing the layer,
   * while it's still part of the layer tree, and end_move() afterwards, when
   * it's no longer part of the layer tree, and hence we can't use get_parent()
   * in end_move() to get the same set of ancestors.
   */
  layer->move_stack = g_slist_prepend (layer->move_stack, ancestors);

  if (GIMP_ITEM_CLASS (parent_class)->start_move)
    GIMP_ITEM_CLASS (parent_class)->start_move (item, push_undo);
}

static void
gimp_layer_end_move (GimpItem *item,
                     gboolean  push_undo)
{
  GimpLayer *layer = GIMP_LAYER (item);
  GSList    *ancestors;
  GSList    *iter;

  g_return_if_fail (layer->move_stack != NULL);

  if (GIMP_ITEM_CLASS (parent_class)->end_move)
    GIMP_ITEM_CLASS (parent_class)->end_move (item, push_undo);

  ancestors = layer->move_stack->data;

  layer->move_stack = g_slist_remove (layer->move_stack, ancestors);

  /* resume mask cropping for all of the layer's ancestors */
  for (iter = ancestors; iter; iter = g_slist_next (iter))
    {
      GimpGroupLayer *ancestor = iter->data;

      gimp_group_layer_resume_mask (ancestor, push_undo);

      g_object_unref (ancestor);
    }

  g_slist_free (ancestors);
}

static void
gimp_layer_translate (GimpItem *item,
                      gdouble   offset_x,
                      gdouble   offset_y,
                      gboolean  push_undo)
{
  GimpLayer *layer = GIMP_LAYER (item);

  if (push_undo)
    gimp_image_undo_push_item_displace (gimp_item_get_image (item), NULL, item);

  GIMP_LAYER_GET_CLASS (layer)->translate (layer,
                                           SIGNED_ROUND (offset_x),
                                           SIGNED_ROUND (offset_y));

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
  GimpLayer       *layer = GIMP_LAYER (item);
  GimpObjectQueue *queue = NULL;

  if (progress && layer->mask)
    {
      GimpLayerMask *mask;

      queue    = gimp_object_queue_new (progress);
      progress = GIMP_PROGRESS (queue);

      /* temporarily set layer->mask to NULL, so that its size won't be counted
       * when pushing the layer to the queue.
       */
      mask        = layer->mask;
      layer->mask = NULL;

      gimp_object_queue_push (queue, layer);
      gimp_object_queue_push (queue, mask);

      layer->mask = mask;
    }

  if (queue)
    gimp_object_queue_pop (queue);

  GIMP_LAYER_GET_CLASS (layer)->scale (layer, new_width, new_height,
                                       new_offset_x, new_offset_y,
                                       interpolation_type, progress);

  if (layer->mask)
    {
      if (queue)
        gimp_object_queue_pop (queue);

      gimp_item_scale (GIMP_ITEM (layer->mask),
                       new_width, new_height,
                       new_offset_x, new_offset_y,
                       interpolation_type, progress);
    }

  g_clear_object (&queue);
}

static void
gimp_layer_resize (GimpItem     *item,
                   GimpContext  *context,
                   GimpFillType  fill_type,
                   gint          new_width,
                   gint          new_height,
                   gint          offset_x,
                   gint          offset_y)
{
  GimpLayer *layer  = GIMP_LAYER (item);

  GIMP_LAYER_GET_CLASS (layer)->resize (layer, context, fill_type,
                                        new_width, new_height,
                                        offset_x, offset_y);

  if (layer->mask)
    gimp_item_resize (GIMP_ITEM (layer->mask), context, GIMP_FILL_TRANSPARENT,
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

  GIMP_LAYER_GET_CLASS (layer)->flip (layer, context, flip_type, axis,
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

  GIMP_LAYER_GET_CLASS (layer)->rotate (layer, context,
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
                      GimpTransformResize     clip_result,
                      GimpProgress           *progress,
                      gboolean                push_undo)
{
  GimpLayer       *layer = GIMP_LAYER (item);
  GimpObjectQueue *queue = NULL;

  if (progress && layer->mask)
    {
      GimpLayerMask *mask;

      queue    = gimp_object_queue_new (progress);
      progress = GIMP_PROGRESS (queue);

      /* temporarily set layer->mask to NULL, so that its size won't be counted
       * when pushing the layer to the queue.
       */
      mask        = layer->mask;
      layer->mask = NULL;

      gimp_object_queue_push (queue, layer);
      gimp_object_queue_push (queue, mask);

      layer->mask = mask;
    }

  if (queue)
    gimp_object_queue_pop (queue);

  GIMP_LAYER_GET_CLASS (layer)->transform (layer, context, matrix, direction,
                                           interpolation_type,
                                           clip_result,
                                           progress, push_undo);

  if (layer->mask)
    {
      if (queue)
        gimp_object_queue_pop (queue);

      gimp_item_transform (GIMP_ITEM (layer->mask), context,
                           matrix, direction,
                           interpolation_type,
                           clip_result, progress);
    }

  g_clear_object (&queue);
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

static void
gimp_layer_alpha_changed (GimpDrawable *drawable)
{
  if (GIMP_DRAWABLE_CLASS (parent_class)->alpha_changed)
    GIMP_DRAWABLE_CLASS (parent_class)->alpha_changed (drawable);

  /* When we add/remove alpha, whatever cached color transforms in
   * view renderers need to be recreated because they cache the wrong
   * lcms formats. See bug 478528.
   */
  gimp_color_managed_profile_changed (GIMP_COLOR_MANAGED (drawable));
}

static gint64
gimp_layer_estimate_memsize (GimpDrawable      *drawable,
                             GimpComponentType  component_type,
                             gint               width,
                             gint               height)
{
  GimpLayer *layer   = GIMP_LAYER (drawable);
  gint64     memsize = 0;

  if (layer->mask)
    memsize += gimp_drawable_estimate_memsize (GIMP_DRAWABLE (layer->mask),
                                               component_type,
                                               width, height);

  return memsize +
         GIMP_DRAWABLE_CLASS (parent_class)->estimate_memsize (drawable,
                                                               component_type,
                                                               width, height);
}

static gboolean
gimp_layer_supports_alpha (GimpDrawable *drawable)
{
  return TRUE;
}

static void
gimp_layer_convert_type (GimpDrawable     *drawable,
                         GimpImage        *dest_image,
                         const Babl       *new_format,
                         GimpColorProfile *src_profile,
                         GimpColorProfile *dest_profile,
                         GeglDitherMethod  layer_dither_type,
                         GeglDitherMethod  mask_dither_type,
                         gboolean          push_undo,
                         GimpProgress     *progress)
{
  GimpLayer       *layer      = GIMP_LAYER (drawable);
  GimpObjectQueue *queue      = NULL;
  const Babl      *dest_space = NULL;
  const Babl      *space_format;
  gboolean         convert_mask;

  convert_mask = layer->mask &&
                 gimp_babl_format_get_precision (new_format) !=
                 gimp_drawable_get_precision (GIMP_DRAWABLE (layer->mask));

  if (progress && convert_mask)
    {
      GimpLayerMask *mask;

      queue    = gimp_object_queue_new (progress);
      progress = GIMP_PROGRESS (queue);

      /* temporarily set layer->mask to NULL, so that its size won't be counted
       * when pushing the layer to the queue.
       */
      mask        = layer->mask;
      layer->mask = NULL;

      gimp_object_queue_push (queue, layer);
      gimp_object_queue_push (queue, mask);

      layer->mask = mask;
    }

  if (queue)
    gimp_object_queue_pop (queue);

  /*  when called with a dest_profile, always use the space from that
   *  profile, we can't use gimp_image_get_layer_space() during an
   *  image type or precision conversion
   */
  if (dest_profile)
    {
      dest_space =
        gimp_color_profile_get_space (dest_profile,
                                      GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                      NULL);
    }

  space_format = babl_format_with_space ((const gchar *) new_format, dest_space);

  GIMP_LAYER_GET_CLASS (layer)->convert_type (layer, dest_image, space_format,
                                              src_profile, dest_profile,
                                              layer_dither_type,
                                              mask_dither_type, push_undo,
                                              progress);

  if (convert_mask)
    {
      if (queue)
        gimp_object_queue_pop (queue);

      gimp_drawable_convert_type (GIMP_DRAWABLE (layer->mask), dest_image,
                                  GIMP_GRAY,
                                  gimp_babl_format_get_precision (new_format),
                                  gimp_drawable_has_alpha (GIMP_DRAWABLE (layer->mask)),
                                  NULL, NULL,
                                  layer_dither_type, mask_dither_type,
                                  push_undo, progress);
    }

  g_clear_object (&queue);
}

static void
gimp_layer_invalidate_boundary (GimpDrawable *drawable)
{
  GimpLayer *layer = GIMP_LAYER (drawable);

  if (gimp_item_is_attached (GIMP_ITEM (drawable)) &&
      gimp_item_is_visible (GIMP_ITEM (drawable)))
    {
      GimpImage   *image = gimp_item_get_image (GIMP_ITEM (drawable));
      GimpChannel *mask  = gimp_image_get_mask (image);

      /*  Turn the current selection off  */
      gimp_image_selection_invalidate (image);

      /*  Only bother with the bounds if there is a selection  */
      if (! gimp_channel_is_empty (mask))
        {
          mask->bounds_known   = FALSE;
          mask->boundary_known = FALSE;
        }
    }

  if (gimp_layer_is_floating_sel (layer))
    floating_sel_invalidate (layer);
}

static void
gimp_layer_get_active_components (GimpDrawable *drawable,
                                  gboolean     *active)
{
  GimpLayer  *layer  = GIMP_LAYER (drawable);
  GimpImage  *image  = gimp_item_get_image (GIMP_ITEM (drawable));
  const Babl *format = gimp_drawable_get_format (drawable);

  /*  first copy the image active channels  */
  gimp_image_get_active_array (image, active);

  if (gimp_drawable_has_alpha (drawable) &&
      gimp_layer_is_alpha_locked (layer, NULL))
    active[babl_format_get_n_components (format) - 1] = FALSE;
}

static GimpComponentMask
gimp_layer_get_active_mask (GimpDrawable *drawable)
{
  GimpLayer         *layer = GIMP_LAYER (drawable);
  GimpImage         *image = gimp_item_get_image (GIMP_ITEM (drawable));
  GimpComponentMask  mask  = gimp_image_get_active_mask (image);

  if (gimp_drawable_has_alpha (drawable) &&
      gimp_layer_is_alpha_locked (layer, NULL))
    mask &= ~GIMP_COMPONENT_MASK_ALPHA;

  return mask;
}

static void
gimp_layer_set_buffer (GimpDrawable        *drawable,
                       gboolean             push_undo,
                       const gchar         *undo_desc,
                       GeglBuffer          *buffer,
                       const GeglRectangle *bounds)
{
  GeglBuffer *old_buffer = gimp_drawable_get_buffer (drawable);
  gint        old_trc    = -1;

  if (old_buffer)
    old_trc = gimp_drawable_get_trc (drawable);

  GIMP_DRAWABLE_CLASS (parent_class)->set_buffer (drawable,
                                                  push_undo, undo_desc,
                                                  buffer, bounds);

  if (gimp_filter_peek_node (GIMP_FILTER (drawable)))
    {
      if (gimp_drawable_get_trc (drawable) != old_trc)
        gimp_layer_update_mode_node (GIMP_LAYER (drawable));
    }
}

static GeglRectangle
gimp_layer_get_bounding_box (GimpDrawable *drawable)
{
  GimpLayer     *layer = GIMP_LAYER (drawable);
  GimpLayerMask *mask  = gimp_layer_get_mask (layer);
  GeglRectangle  bounding_box;

  if (mask && gimp_layer_get_show_mask (layer))
    {
      bounding_box = gimp_drawable_get_bounding_box (GIMP_DRAWABLE (mask));
    }
  else
    {
      bounding_box = GIMP_LAYER_GET_CLASS (layer)->get_bounding_box (layer);

      if (mask && gimp_layer_get_apply_mask (layer))
        {
          GeglRectangle mask_bounding_box;

          mask_bounding_box = gimp_drawable_get_bounding_box (
            GIMP_DRAWABLE (mask));

          gegl_rectangle_intersect (&bounding_box,
                                    &bounding_box, &mask_bounding_box);
        }
    }

  return bounding_box;
}

static GimpColorProfile *
gimp_layer_get_color_profile (GimpColorManaged *managed)
{
  GimpImage *image = gimp_item_get_image (GIMP_ITEM (managed));

  return gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (image));
}

static gdouble
gimp_layer_get_opacity_at (GimpPickable *pickable,
                           gint          x,
                           gint          y)
{
  GimpLayer *layer = GIMP_LAYER (pickable);
  gdouble    value = GIMP_OPACITY_TRANSPARENT;

  if (x >= 0 && x < gimp_item_get_width  (GIMP_ITEM (layer)) &&
      y >= 0 && y < gimp_item_get_height (GIMP_ITEM (layer)) &&
      gimp_item_is_visible (GIMP_ITEM (layer)))
    {
      if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
        {
          value = GIMP_OPACITY_OPAQUE;
        }
      else
        {
          gegl_buffer_sample (gimp_drawable_get_buffer (GIMP_DRAWABLE (layer)),
                              x, y, NULL, &value, babl_format ("A double"),
                              GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
        }

      if (gimp_layer_get_mask (layer) &&
          gimp_layer_get_apply_mask (layer))
        {
          gdouble mask_value;

          mask_value = gimp_pickable_get_opacity_at (GIMP_PICKABLE (layer->mask),
                                                     x, y);

          value *= mask_value;
        }
    }

  return value;
}

static gboolean
gimp_layer_real_is_alpha_locked (GimpLayer  *layer,
                                 GimpLayer **locked_layer)
{
  GimpItem *parent = gimp_item_get_parent (GIMP_ITEM (layer));

  if (layer->lock_alpha)
    {
      if (locked_layer)
        *locked_layer = layer;
    }
  else if (parent &&
           gimp_layer_is_alpha_locked (GIMP_LAYER (parent), locked_layer))
    {
      return TRUE;
    }

  return layer->lock_alpha;
}

static void
gimp_layer_real_translate (GimpLayer *layer,
                           gint       offset_x,
                           gint       offset_y)
{
  /*  update the old region  */
  gimp_drawable_update (GIMP_DRAWABLE (layer), 0, 0, -1, -1);

  /*  invalidate the selection boundary because of a layer modification  */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (layer));

  GIMP_ITEM_CLASS (parent_class)->translate (GIMP_ITEM (layer),
                                             offset_x, offset_y,
                                             FALSE);

  /*  update the new region  */
  gimp_drawable_update (GIMP_DRAWABLE (layer), 0, 0, -1, -1);
}

static void
gimp_layer_real_scale (GimpLayer             *layer,
                       gint                   new_width,
                       gint                   new_height,
                       gint                   new_offset_x,
                       gint                   new_offset_y,
                       GimpInterpolationType  interpolation_type,
                       GimpProgress          *progress)
{
  GIMP_ITEM_CLASS (parent_class)->scale (GIMP_ITEM (layer),
                                         new_width, new_height,
                                         new_offset_x, new_offset_y,
                                         interpolation_type, progress);
}

static void
gimp_layer_real_resize (GimpLayer    *layer,
                        GimpContext  *context,
                        GimpFillType  fill_type,
                        gint          new_width,
                        gint          new_height,
                        gint          offset_x,
                        gint          offset_y)
{
  if (fill_type == GIMP_FILL_TRANSPARENT &&
      ! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    {
      fill_type = GIMP_FILL_BACKGROUND;
    }

  GIMP_ITEM_CLASS (parent_class)->resize (GIMP_ITEM (layer),
                                          context, fill_type,
                                          new_width, new_height,
                                          offset_x, offset_y);
}

static void
gimp_layer_real_flip (GimpLayer           *layer,
                      GimpContext         *context,
                      GimpOrientationType  flip_type,
                      gdouble              axis,
                      gboolean             clip_result)
{
  GIMP_ITEM_CLASS (parent_class)->flip (GIMP_ITEM (layer),
                                        context, flip_type, axis, clip_result);
}

static void
gimp_layer_real_rotate (GimpLayer        *layer,
                        GimpContext      *context,
                        GimpRotationType  rotate_type,
                        gdouble           center_x,
                        gdouble           center_y,
                        gboolean          clip_result)
{
  GIMP_ITEM_CLASS (parent_class)->rotate (GIMP_ITEM (layer),
                                          context, rotate_type,
                                          center_x, center_y,
                                          clip_result);
}

static void
gimp_layer_real_transform (GimpLayer              *layer,
                           GimpContext            *context,
                           const GimpMatrix3      *matrix,
                           GimpTransformDirection  direction,
                           GimpInterpolationType   interpolation_type,
                           GimpTransformResize     clip_result,
                           GimpProgress           *progress,
                           gboolean                push_undo)
{
  if (! gimp_matrix3_is_simple (matrix) &&
      ! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    gimp_layer_add_alpha (GIMP_LAYER (layer));

  GIMP_ITEM_CLASS (parent_class)->transform (GIMP_ITEM (layer),
                                             context, matrix, direction,
                                             interpolation_type,
                                             clip_result,
                                             progress, push_undo);
}

static void
gimp_layer_real_convert_type (GimpLayer        *layer,
                              GimpImage        *dest_image,
                              const Babl       *new_format,
                              GimpColorProfile *src_profile,
                              GimpColorProfile *dest_profile,
                              GeglDitherMethod  layer_dither_type,
                              GeglDitherMethod  mask_dither_type,
                              gboolean          push_undo,
                              GimpProgress     *progress)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (layer);
  GeglBuffer   *src_buffer;
  GeglBuffer   *dest_buffer;

  if (layer_dither_type == GEGL_DITHER_NONE)
    {
      src_buffer = g_object_ref (gimp_drawable_get_buffer (drawable));
    }
  else
    {
      gint bits;

      src_buffer =
        gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                         gimp_item_get_width  (GIMP_ITEM (layer)),
                                         gimp_item_get_height (GIMP_ITEM (layer))),
                         gimp_drawable_get_format (drawable));

      bits = (babl_format_get_bytes_per_pixel (new_format) * 8 /
              babl_format_get_n_components (new_format));

      gimp_gegl_apply_dither (gimp_drawable_get_buffer (drawable),
                              NULL, NULL,
                              src_buffer, 1 << bits, layer_dither_type);
    }

  dest_buffer =
    gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                     gimp_item_get_width  (GIMP_ITEM (layer)),
                                     gimp_item_get_height (GIMP_ITEM (layer))),
                     new_format);

  if (dest_profile)
    {
      if (! src_profile)
        src_profile =
          gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (layer));

      gimp_gegl_convert_color_profile (src_buffer,  NULL, src_profile,
                                       dest_buffer, NULL, dest_profile,
                                       GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                       TRUE, progress);
    }
  else
    {
      gimp_gegl_buffer_copy (src_buffer, NULL, GEGL_ABYSS_NONE,
                             dest_buffer, NULL);
    }

  gimp_drawable_set_buffer (drawable, push_undo, NULL, dest_buffer);

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);
}

static GeglRectangle
gimp_layer_real_get_bounding_box (GimpLayer *layer)
{
  return GIMP_DRAWABLE_CLASS (parent_class)->get_bounding_box (
    GIMP_DRAWABLE (layer));
}

static void
gimp_layer_real_get_effective_mode (GimpLayer              *layer,
                                    GimpLayerMode          *mode,
                                    GimpLayerColorSpace    *blend_space,
                                    GimpLayerColorSpace    *composite_space,
                                    GimpLayerCompositeMode *composite_mode)
{
  *mode            = gimp_layer_get_mode (layer);
  *blend_space     = gimp_layer_get_real_blend_space (layer);
  *composite_space = gimp_layer_get_real_composite_space (layer);
  *composite_mode  = gimp_layer_get_real_composite_mode (layer);
}

static gboolean
gimp_layer_real_get_excludes_backdrop (GimpLayer *layer)
{
  GimpLayerCompositeRegion included_region;

  included_region = gimp_layer_mode_get_included_region (layer->mode,
                                                         layer->effective_composite_mode);

  return ! (included_region & GIMP_LAYER_COMPOSITE_REGION_DESTINATION);
}

static void
gimp_layer_layer_mask_update (GimpDrawable *drawable,
                              gint          x,
                              gint          y,
                              gint          width,
                              gint          height,
                              GimpLayer    *layer)
{
  if (gimp_layer_get_apply_mask (layer) ||
      gimp_layer_get_show_mask (layer))
    {
      gimp_drawable_update (GIMP_DRAWABLE (layer),
                            x, y, width, height);
    }
}


/*  public functions  */

void
gimp_layer_fix_format_space (GimpLayer *layer,
                             gboolean   copy_buffer,
                             gboolean   push_undo)
{
  GimpDrawable *drawable;
  const Babl   *format;

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (push_undo == FALSE || copy_buffer == TRUE);

  drawable = GIMP_DRAWABLE (layer);

  format = gimp_image_get_layer_format (gimp_item_get_image (GIMP_ITEM (layer)),
                                        gimp_drawable_has_alpha (drawable));

  if (format != gimp_drawable_get_format (drawable))
    {
      gimp_drawable_set_format (drawable, format, copy_buffer, push_undo);
    }
}

GimpLayer *
gimp_layer_get_parent (GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);

  return GIMP_LAYER (gimp_viewable_get_parent (GIMP_VIEWABLE (layer)));
}

GimpLayerMask *
gimp_layer_get_mask (GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);

  return layer->mask;
}

GimpLayerMask *
gimp_layer_add_mask (GimpLayer      *layer,
                     GimpLayerMask  *mask,
                     gboolean        edit_mask,
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
    gimp_image_undo_push_layer_mask_add (image, C_("undo-type", "Add Layer Masks"),
                                         layer, mask);

  layer->mask = g_object_ref_sink (mask);
  layer->apply_mask = TRUE;
  layer->edit_mask  = edit_mask;
  layer->show_mask  = FALSE;

  gimp_layer_mask_set_layer (mask, layer);

  if (gimp_filter_peek_node (GIMP_FILTER (layer)))
    {
      GeglNode *mode_node;
      GeglNode *mask;

      mode_node = gimp_drawable_get_mode_node (GIMP_DRAWABLE (layer));

      mask = gimp_drawable_get_source_node (GIMP_DRAWABLE (layer->mask));

      gegl_node_connect (mask,                    "output",
                         layer->mask_offset_node, "input");

      if (layer->show_mask)
        {
          gegl_node_connect (layer->mask_offset_node, "output",
                             mode_node,               "aux");
        }
      else
        {
          gegl_node_connect (layer->mask_offset_node, "output",
                             mode_node,               "aux2");
        }

      gimp_layer_update_mode_node (layer);
    }

  gimp_drawable_update_bounding_box (GIMP_DRAWABLE (layer));

  gimp_layer_update_effective_mode (layer);
  gimp_layer_update_excludes_backdrop (layer);

  if (gimp_layer_get_apply_mask (layer) ||
      gimp_layer_get_show_mask (layer))
    {
      gimp_drawable_update (GIMP_DRAWABLE (layer), 0, 0, -1, -1);
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
gimp_layer_create_mask (GimpLayer       *layer,
                        GimpAddMaskType  add_mask_type,
                        GimpChannel     *channel)
{
  GimpDrawable  *drawable;
  GimpItem      *item;
  GimpLayerMask *mask;
  GimpImage     *image;
  gchar         *mask_name;
  GeglColor     *black = gegl_color_new ("black");

  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (add_mask_type != GIMP_ADD_MASK_CHANNEL ||
                        GIMP_IS_CHANNEL (channel), NULL);

  drawable = GIMP_DRAWABLE (layer);
  item     = GIMP_ITEM (layer);
  image    = gimp_item_get_image (item);

  mask_name = g_strdup_printf (_("%s mask"),
                               gimp_object_get_name (layer));

  mask = gimp_layer_mask_new (image,
                              gimp_item_get_width  (item),
                              gimp_item_get_height (item),
                              mask_name, black);

  g_free (mask_name);
  g_object_unref (black);

  switch (add_mask_type)
    {
    case GIMP_ADD_MASK_WHITE:
      gimp_channel_all (GIMP_CHANNEL (mask), FALSE);
      break;

    case GIMP_ADD_MASK_BLACK:
      gimp_channel_clear (GIMP_CHANNEL (mask), NULL, FALSE);
      break;

    case GIMP_ADD_MASK_ALPHA:
    case GIMP_ADD_MASK_ALPHA_TRANSFER:
      if (gimp_drawable_has_alpha (drawable))
        {
          GeglBuffer *dest_buffer;
          const Babl *component_format;

          dest_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));

          component_format =
            gimp_image_get_component_format (image, GIMP_CHANNEL_ALPHA);

          gegl_buffer_set_format (dest_buffer, component_format);
          gimp_gegl_buffer_copy (gimp_drawable_get_buffer (drawable), NULL,
                                 GEGL_ABYSS_NONE,
                                 dest_buffer, NULL);
          gegl_buffer_set_format (dest_buffer, NULL);

          if (add_mask_type == GIMP_ADD_MASK_ALPHA_TRANSFER)
            {
              gimp_drawable_push_undo (drawable,
                                       C_("undo-type", "Transfer Alpha to Mask"),
                                       NULL,
                                       0, 0,
                                       gimp_item_get_width  (item),
                                       gimp_item_get_height (item));

              gimp_gegl_apply_set_alpha (gimp_drawable_get_buffer (drawable),
                                         NULL, NULL,
                                         gimp_drawable_get_buffer (drawable),
                                         1.0);
            }
        }
      else
        {
          gimp_channel_all (GIMP_CHANNEL (mask), FALSE);
        }
      break;

    case GIMP_ADD_MASK_SELECTION:
    case GIMP_ADD_MASK_CHANNEL:
      {
        gboolean channel_empty;
        gint     offset_x, offset_y;
        gint     copy_x, copy_y;
        gint     copy_width, copy_height;

        if (add_mask_type == GIMP_ADD_MASK_SELECTION)
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
            GeglBuffer *src;
            GeglBuffer *dest;
            const Babl *format;

            src  = gimp_drawable_get_buffer (GIMP_DRAWABLE (channel));
            dest = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));

            format = gegl_buffer_get_format (src);

            /* make sure no gamma conversion happens */
            gegl_buffer_set_format (dest, format);
            gimp_gegl_buffer_copy (src,
                                   GEGL_RECTANGLE (copy_x, copy_y,
                                                   copy_width, copy_height),
                                   GEGL_ABYSS_NONE,
                                   dest,
                                   GEGL_RECTANGLE (copy_x - offset_x,
                                                   copy_y - offset_y,
                                                   0, 0));
            gegl_buffer_set_format (dest, NULL);

            GIMP_CHANNEL (mask)->bounds_known = FALSE;
          }
      }
      break;

    case GIMP_ADD_MASK_COPY:
      {
        GeglBuffer *src_buffer;
        GeglBuffer *dest_buffer;

        if (! gimp_drawable_is_gray (drawable))
          {
            const Babl *copy_format =
              gimp_image_get_format (image, GIMP_GRAY,
                                     gimp_drawable_get_precision (drawable),
                                     gimp_drawable_has_alpha (drawable),
                                     NULL);

            src_buffer =
              gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                               gimp_item_get_width  (item),
                                               gimp_item_get_height (item)),
                               copy_format);

            gimp_gegl_buffer_copy (gimp_drawable_get_buffer (drawable), NULL,
                                   GEGL_ABYSS_NONE,
                                   src_buffer, NULL);
          }
        else
          {
            src_buffer = gimp_drawable_get_buffer (drawable);
            g_object_ref (src_buffer);
          }

        dest_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));

        if (gimp_drawable_has_alpha (drawable))
          {
            GeglColor *background = gegl_color_new ("transparent");

            gimp_gegl_apply_flatten (src_buffer, NULL, NULL,
                                     dest_buffer, background,
                                     GIMP_LAYER_COLOR_SPACE_RGB_LINEAR);

            g_object_unref (background);
          }
        else
          {
            gimp_gegl_buffer_copy (src_buffer, NULL, GEGL_ABYSS_NONE,
                                   dest_buffer, NULL);
          }

        g_object_unref (src_buffer);
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
  gboolean       view_changed = FALSE;

  g_return_if_fail (GIMP_IS_LAYER (layer));

  mask = gimp_layer_get_mask (layer);

  if (! mask)
    return;

  /*  APPLY can not be done to group layers  */
  g_return_if_fail (! gimp_viewable_get_children (GIMP_VIEWABLE (layer)) ||
                    mode == GIMP_MASK_DISCARD);

  /*  APPLY can only be done to layers with an alpha channel  */
  g_return_if_fail (gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)) ||
                    mode == GIMP_MASK_DISCARD || push_undo == TRUE);

  item  = GIMP_ITEM (layer);
  image = gimp_item_get_image (item);

  if (! image)
    return;

  if (push_undo)
    {
      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_LAYER_APPLY_MASK,
                                   (mode == GIMP_MASK_APPLY) ?
                                   C_("undo-type", "Apply Layer Masks") :
                                   C_("undo-type", "Delete Layer Masks"));

      gimp_image_undo_push_layer_mask_show (image, NULL, layer);
      gimp_image_undo_push_layer_mask_apply (image, NULL, layer);
      gimp_image_undo_push_layer_mask_remove (image, NULL, layer, mask);

      if (mode == GIMP_MASK_APPLY &&
          ! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
        {
          gimp_layer_add_alpha (layer);
        }
    }

  /*  check if applying the mask changes the projection  */
  if (gimp_layer_get_show_mask (layer)                                   ||
      (mode == GIMP_MASK_APPLY   && ! gimp_layer_get_apply_mask (layer)) ||
      (mode == GIMP_MASK_DISCARD &&   gimp_layer_get_apply_mask (layer)))
    {
      view_changed = TRUE;
    }

  if (mode == GIMP_MASK_APPLY)
    {
      GeglBuffer *mask_buffer;
      GeglBuffer *dest_buffer;

      if (push_undo)
        gimp_drawable_push_undo (GIMP_DRAWABLE (layer), NULL,
                                 NULL,
                                 0, 0,
                                 gimp_item_get_width  (item),
                                 gimp_item_get_height (item));

      /*  Combine the current layer's alpha channel and the mask  */
      mask_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));
      dest_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

      gimp_gegl_apply_opacity (gimp_drawable_get_buffer (GIMP_DRAWABLE (layer)),
                               NULL, NULL, dest_buffer,
                               mask_buffer, 0, 0, 1.0);
    }

  g_signal_handlers_disconnect_by_func (mask,
                                        gimp_layer_layer_mask_update,
                                        layer);

  gimp_item_removed (GIMP_ITEM (mask));
  g_object_unref (mask);
  layer->mask = NULL;

  if (push_undo)
    gimp_image_undo_group_end (image);

  if (gimp_filter_peek_node (GIMP_FILTER (layer)))
    {
      GeglNode *mode_node;

      mode_node = gimp_drawable_get_mode_node (GIMP_DRAWABLE (layer));

      if (layer->show_mask)
        {
          gegl_node_connect (layer->layer_offset_node, "output",
                             mode_node,                "aux");
        }
      else
        {
          gegl_node_disconnect (mode_node, "aux2");
        }

      gimp_layer_update_mode_node (layer);
    }

  gimp_drawable_update_bounding_box (GIMP_DRAWABLE (layer));

  gimp_layer_update_effective_mode (layer);
  gimp_layer_update_excludes_backdrop (layer);

  /*  If applying actually changed the view  */
  if (view_changed)
    {
      gimp_drawable_update (GIMP_DRAWABLE (layer), 0, 0, -1, -1);
    }
  else
    {
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer));
    }

  g_signal_emit (layer, layer_signals[MASK_CHANGED], 0);

  g_object_notify (G_OBJECT (layer), "mask");
}

void
gimp_layer_set_apply_mask (GimpLayer *layer,
                           gboolean   apply,
                           gboolean   push_undo)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (layer->mask != NULL);

  if (layer->apply_mask != apply)
    {
      GimpImage *image = gimp_item_get_image (GIMP_ITEM (layer));

      if (push_undo && gimp_item_is_attached (GIMP_ITEM (layer)))
        gimp_image_undo_push_layer_mask_apply (image,
                                               apply ?
                                               C_("undo-type", "Enable Layer Masks") :
                                               C_("undo-type", "Disable Layer Masks"),
                                               layer);

      layer->apply_mask = apply ? TRUE : FALSE;

      if (gimp_filter_peek_node (GIMP_FILTER (layer)) &&
          ! gimp_layer_get_show_mask (layer))
        {
          GeglNode *mode_node;

          mode_node = gimp_drawable_get_mode_node (GIMP_DRAWABLE (layer));

          if (layer->apply_mask)
            {
              gegl_node_connect (layer->mask_offset_node, "output",
                                 mode_node,               "aux2");
            }
          else
            {
              gegl_node_disconnect (mode_node, "aux2");
            }
        }

      gimp_drawable_update_bounding_box (GIMP_DRAWABLE (layer));

      gimp_layer_update_effective_mode (layer);
      gimp_layer_update_excludes_backdrop (layer);

      gimp_drawable_update (GIMP_DRAWABLE (layer), 0, 0, -1, -1);

      g_signal_emit (layer, layer_signals[APPLY_MASK_CHANGED], 0);
    }
}

gboolean
gimp_layer_get_apply_mask (GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (layer->mask, FALSE);

  return layer->apply_mask;
}

void
gimp_layer_set_edit_mask (GimpLayer *layer,
                          gboolean   edit)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (layer->mask != NULL);

  if (layer->edit_mask != edit)
    {
      layer->edit_mask = edit ? TRUE : FALSE;

      g_signal_emit (layer, layer_signals[EDIT_MASK_CHANGED], 0);
    }
}

gboolean
gimp_layer_get_edit_mask (GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (layer->mask, FALSE);

  return layer->edit_mask;
}

void
gimp_layer_set_show_mask (GimpLayer *layer,
                          gboolean   show,
                          gboolean   push_undo)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (layer->mask != NULL);

  if (layer->show_mask != show)
    {
      GimpImage *image = gimp_item_get_image (GIMP_ITEM (layer));

      if (push_undo)
        gimp_image_undo_push_layer_mask_show (image,
                                              C_("undo-type", "Show Layer Masks"),
                                              layer);

      layer->show_mask = show ? TRUE : FALSE;

      if (gimp_filter_peek_node (GIMP_FILTER (layer)))
        {
          GeglNode *mode_node;

          mode_node = gimp_drawable_get_mode_node (GIMP_DRAWABLE (layer));

          if (layer->show_mask)
            {
              gegl_node_disconnect (mode_node, "aux2");

              gegl_node_connect (layer->mask_offset_node, "output",
                                 mode_node,               "aux");
            }
          else
            {
              gegl_node_connect (layer->layer_offset_node, "output",
                                 mode_node,                "aux");

              if (gimp_layer_get_apply_mask (layer))
                {
                  gegl_node_connect (layer->mask_offset_node, "output",
                                     mode_node,               "aux2");
                }
            }

          gimp_layer_update_mode_node (layer);
        }

      gimp_drawable_update_bounding_box (GIMP_DRAWABLE (layer));

      gimp_layer_update_effective_mode (layer);
      gimp_layer_update_excludes_backdrop (layer);

      gimp_drawable_update (GIMP_DRAWABLE (layer), 0, 0, -1, -1);

      g_signal_emit (layer, layer_signals[SHOW_MASK_CHANGED], 0);
    }
}

gboolean
gimp_layer_get_show_mask (GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (layer->mask, FALSE);

  return layer->show_mask;
}

void
gimp_layer_add_alpha (GimpLayer *layer)
{
  GimpItem     *item;
  GimpDrawable *drawable;
  GeglBuffer   *new_buffer;

  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    return;

  item     = GIMP_ITEM (layer);
  drawable = GIMP_DRAWABLE (layer);

  new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                gimp_item_get_width  (item),
                                                gimp_item_get_height (item)),
                                gimp_drawable_get_format_with_alpha (drawable));

  gimp_gegl_buffer_copy (
    gimp_drawable_get_buffer (drawable), NULL, GEGL_ABYSS_NONE,
    new_buffer, NULL);

  gimp_drawable_set_buffer (GIMP_DRAWABLE (layer),
                            gimp_item_is_attached (GIMP_ITEM (layer)),
                            C_("undo-type", "Add Alpha Channel"),
                            new_buffer);
  g_object_unref (new_buffer);
}

void
gimp_layer_remove_alpha (GimpLayer   *layer,
                         GimpContext *context)
{
  GeglBuffer *new_buffer;

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    return;

  new_buffer =
    gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                     gimp_item_get_width  (GIMP_ITEM (layer)),
                                     gimp_item_get_height (GIMP_ITEM (layer))),
                     gimp_drawable_get_format_without_alpha (GIMP_DRAWABLE (layer)));

  gimp_gegl_apply_flatten (gimp_drawable_get_buffer (GIMP_DRAWABLE (layer)),
                           NULL, NULL, new_buffer,
                           gimp_context_get_background (context),
                           gimp_layer_get_real_composite_space (layer));

  gimp_drawable_set_buffer (GIMP_DRAWABLE (layer),
                            gimp_item_is_attached (GIMP_ITEM (layer)),
                            C_("undo-type", "Remove Alpha Channel"),
                            new_buffer);
  g_object_unref (new_buffer);
}

void
gimp_layer_resize_to_image (GimpLayer    *layer,
                            GimpContext  *context,
                            GimpFillType  fill_type)
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
  gimp_item_resize (GIMP_ITEM (layer), context, fill_type,
                    gimp_image_get_width  (image),
                    gimp_image_get_height (image),
                    offset_x, offset_y);

  gimp_image_undo_group_end (image);
}

/**********************/
/*  access functions  */
/**********************/

GimpDrawable *
gimp_layer_get_floating_sel_drawable (GimpLayer *layer)
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

  if (g_set_object (&layer->fs.drawable, drawable))
    {
      if (layer->fs.segs)
        {
          g_clear_pointer (&layer->fs.segs, g_free);
          layer->fs.num_segs = 0;
        }

      g_object_notify (G_OBJECT (layer), "floating-selection");
    }
}

gboolean
gimp_layer_is_floating_sel (GimpLayer *layer)
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

      if (gimp_filter_peek_node (GIMP_FILTER (layer)))
        gimp_layer_update_mode_node (layer);

      gimp_drawable_update (GIMP_DRAWABLE (layer), 0, 0, -1, -1);
    }
}

gdouble
gimp_layer_get_opacity (GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), GIMP_OPACITY_OPAQUE);

  return layer->opacity;
}

void
gimp_layer_set_mode (GimpLayer     *layer,
                     GimpLayerMode  mode,
                     gboolean       push_undo)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (gimp_viewable_get_children (GIMP_VIEWABLE (layer)) == NULL)
    {
      g_return_if_fail (gimp_layer_mode_get_context (mode) &
                        GIMP_LAYER_MODE_CONTEXT_LAYER);
    }
  else
    {
      g_return_if_fail (gimp_layer_mode_get_context (mode) &
                        GIMP_LAYER_MODE_CONTEXT_GROUP);
    }

  if (layer->mode != mode)
    {
      if (gimp_item_is_attached (GIMP_ITEM (layer)))
        {
          GimpImage *image = gimp_item_get_image (GIMP_ITEM (layer));

          gimp_image_unset_default_new_layer_mode (image);

          if (push_undo)
            gimp_image_undo_push_layer_mode (image, NULL, layer);
        }

      g_object_freeze_notify (G_OBJECT (layer));

      layer->mode = mode;

      g_signal_emit (layer, layer_signals[MODE_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "mode");

      /*  when changing modes, we always switch to AUTO blend and
       *  composite in order to avoid confusion
       */
      if (layer->blend_space != GIMP_LAYER_COLOR_SPACE_AUTO)
        {
          layer->blend_space = GIMP_LAYER_COLOR_SPACE_AUTO;

          g_signal_emit (layer, layer_signals[BLEND_SPACE_CHANGED], 0);
          g_object_notify (G_OBJECT (layer), "blend-space");
        }

      if (layer->composite_space != GIMP_LAYER_COLOR_SPACE_AUTO)
        {
          layer->composite_space = GIMP_LAYER_COLOR_SPACE_AUTO;

          g_signal_emit (layer, layer_signals[COMPOSITE_SPACE_CHANGED], 0);
          g_object_notify (G_OBJECT (layer), "composite-space");
        }

      if (layer->composite_mode != GIMP_LAYER_COMPOSITE_AUTO)
        {
          layer->composite_mode = GIMP_LAYER_COMPOSITE_AUTO;

          g_signal_emit (layer, layer_signals[COMPOSITE_MODE_CHANGED], 0);
          g_object_notify (G_OBJECT (layer), "composite-mode");
        }

      g_object_thaw_notify (G_OBJECT (layer));

      gimp_layer_update_effective_mode (layer);
      gimp_layer_update_excludes_backdrop (layer);
    }
}

GimpLayerMode
gimp_layer_get_mode (GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), GIMP_LAYER_MODE_NORMAL);

  return layer->mode;
}

void
gimp_layer_set_blend_space (GimpLayer           *layer,
                            GimpLayerColorSpace  blend_space,
                            gboolean             push_undo)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (! gimp_layer_mode_is_blend_space_mutable (layer->mode))
    return;

  if (layer->blend_space != blend_space)
    {
      if (push_undo && gimp_item_is_attached (GIMP_ITEM (layer)))
        {
          GimpImage *image = gimp_item_get_image (GIMP_ITEM (layer));

          gimp_image_undo_push_layer_mode (image, _("Set layer's blend space"), layer);
        }

      layer->blend_space = blend_space;

      g_signal_emit (layer, layer_signals[BLEND_SPACE_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "blend-space");

      gimp_layer_update_effective_mode (layer);
    }
}

GimpLayerColorSpace
gimp_layer_get_blend_space (GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), GIMP_LAYER_COLOR_SPACE_AUTO);

  return layer->blend_space;
}

GimpLayerColorSpace
gimp_layer_get_real_blend_space (GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), GIMP_LAYER_COLOR_SPACE_RGB_LINEAR);

  if (layer->blend_space == GIMP_LAYER_COLOR_SPACE_AUTO)
    return gimp_layer_mode_get_blend_space (layer->mode);
  else
    return layer->blend_space;
}

void
gimp_layer_set_composite_space (GimpLayer           *layer,
                                GimpLayerColorSpace  composite_space,
                                gboolean             push_undo)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (! gimp_layer_mode_is_composite_space_mutable (layer->mode))
    return;

  if (layer->composite_space != composite_space)
    {
      if (push_undo && gimp_item_is_attached (GIMP_ITEM (layer)))
        {
          GimpImage *image = gimp_item_get_image (GIMP_ITEM (layer));

          gimp_image_undo_push_layer_mode (image, _("Set layer's composite space"), layer);
        }

      layer->composite_space = composite_space;

      g_signal_emit (layer, layer_signals[COMPOSITE_SPACE_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "composite-space");

      gimp_layer_update_effective_mode (layer);
    }
}

GimpLayerColorSpace
gimp_layer_get_composite_space (GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), GIMP_LAYER_COLOR_SPACE_AUTO);

  return layer->composite_space;
}

GimpLayerColorSpace
gimp_layer_get_real_composite_space (GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), GIMP_LAYER_COLOR_SPACE_RGB_LINEAR);

  if (layer->composite_space == GIMP_LAYER_COLOR_SPACE_AUTO)
    return gimp_layer_mode_get_composite_space (layer->mode);
  else
    return layer->composite_space;
}

void
gimp_layer_set_composite_mode (GimpLayer              *layer,
                               GimpLayerCompositeMode  composite_mode,
                               gboolean                push_undo)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (! gimp_layer_mode_is_composite_mode_mutable (layer->mode))
    return;

  if (layer->composite_mode != composite_mode)
    {
      if (push_undo && gimp_item_is_attached (GIMP_ITEM (layer)))
        {
          GimpImage *image = gimp_item_get_image (GIMP_ITEM (layer));

          gimp_image_undo_push_layer_mode (image, _("Set layer's composite mode"), layer);
        }

      layer->composite_mode = composite_mode;

      g_signal_emit (layer, layer_signals[COMPOSITE_MODE_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "composite-mode");

      gimp_layer_update_effective_mode (layer);
      gimp_layer_update_excludes_backdrop (layer);
    }
}

GimpLayerCompositeMode
gimp_layer_get_composite_mode (GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), GIMP_LAYER_COMPOSITE_AUTO);

  return layer->composite_mode;
}

GimpLayerCompositeMode
gimp_layer_get_real_composite_mode (GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), GIMP_LAYER_COMPOSITE_UNION);

  if (layer->composite_mode == GIMP_LAYER_COMPOSITE_AUTO)
    return gimp_layer_mode_get_composite_mode (layer->mode);
  else
    return layer->composite_mode;
}

void
gimp_layer_get_effective_mode (GimpLayer              *layer,
                               GimpLayerMode          *mode,
                               GimpLayerColorSpace    *blend_space,
                               GimpLayerColorSpace    *composite_space,
                               GimpLayerCompositeMode *composite_mode)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (mode)            *mode            = layer->effective_mode;
  if (blend_space)     *blend_space     = layer->effective_blend_space;
  if (composite_space) *composite_space = layer->effective_composite_space;
  if (composite_mode)  *composite_mode  = layer->effective_composite_mode;
}

gboolean
gimp_layer_get_excludes_backdrop (GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  return layer->excludes_backdrop;
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
gimp_layer_get_lock_alpha (GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  return layer->lock_alpha;
}

gboolean
gimp_layer_can_lock_alpha (GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  return TRUE;
}

gboolean
gimp_layer_is_alpha_locked (GimpLayer  *layer,
                            GimpLayer **locked_layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  return GIMP_LAYER_GET_CLASS (layer)->is_alpha_locked (layer, locked_layer);
}

/*  protected functions  */

void
gimp_layer_update_effective_mode (GimpLayer *layer)
{
  GimpLayerMode          mode;
  GimpLayerColorSpace    blend_space;
  GimpLayerColorSpace    composite_space;
  GimpLayerCompositeMode composite_mode;

  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (layer->mask && layer->show_mask)
    {
      mode            = GIMP_LAYER_MODE_NORMAL;
      blend_space     = GIMP_LAYER_COLOR_SPACE_AUTO;
      composite_space = GIMP_LAYER_COLOR_SPACE_AUTO;
      composite_mode  = GIMP_LAYER_COMPOSITE_AUTO;

      /* This makes sure that masks of LEGACY-mode layers are
       * composited in PERCEPTUAL space, and non-LEGACY layers in
       * LINEAR space, or whatever composite space was chosen in the
       * layer attributes dialog
       */
      composite_space = gimp_layer_get_real_composite_space (layer);
    }
  else
    {
      GIMP_LAYER_GET_CLASS (layer)->get_effective_mode (layer,
                                                        &mode,
                                                        &blend_space,
                                                        &composite_space,
                                                        &composite_mode);
    }

  if (mode            != layer->effective_mode            ||
      blend_space     != layer->effective_blend_space     ||
      composite_space != layer->effective_composite_space ||
      composite_mode  != layer->effective_composite_mode)
    {
      GeglRectangle rect = GIMP_LAYER_GET_CLASS (layer)->get_bounding_box (layer);
      GeglRectangle rect2;

      layer->effective_mode            = mode;
      layer->effective_blend_space     = blend_space;
      layer->effective_composite_space = composite_space;
      layer->effective_composite_mode  = composite_mode;

      g_signal_emit (layer, layer_signals[EFFECTIVE_MODE_CHANGED], 0);

      if (gimp_filter_peek_node (GIMP_FILTER (layer)))
        gimp_layer_update_mode_node (layer);

      /* The effective mode change might be enough to shrink the
       * effective bounding box. This may happen in particular for
       * pass-through group layers changed to other modes. So make sure
       * we update a box containing both the old and new boundaries.
       */
      rect2 = GIMP_LAYER_GET_CLASS (layer)->get_bounding_box (layer);
      gegl_rectangle_bounding_box (&rect, &rect, &rect2);

      gimp_drawable_update (GIMP_DRAWABLE (layer), rect.x, rect.y, rect.width, rect.height);
    }
}

void
gimp_layer_update_excludes_backdrop (GimpLayer *layer)
{
  gboolean excludes_backdrop;

  g_return_if_fail (GIMP_IS_LAYER (layer));

  excludes_backdrop =
    GIMP_LAYER_GET_CLASS (layer)->get_excludes_backdrop (layer);

  if (excludes_backdrop != layer->excludes_backdrop)
    {
      layer->excludes_backdrop = excludes_backdrop;

      g_signal_emit (layer, layer_signals[EXCLUDES_BACKDROP_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "excludes-backdrop");
    }
}
