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

#include <stdlib.h>
#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "operations/layer-modes/ligma-layer-modes.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligma-gegl-apply-operation.h"
#include "gegl/ligma-gegl-loops.h"
#include "gegl/ligma-gegl-nodes.h"

#include "ligmaboundary.h"
#include "ligmachannel-select.h"
#include "ligmacontext.h"
#include "ligmacontainer.h"
#include "ligmadrawable-floating-selection.h"
#include "ligmaerror.h"
#include "ligmagrouplayer.h"
#include "ligmaimage-undo-push.h"
#include "ligmaimage-undo.h"
#include "ligmaimage.h"
#include "ligmaimage-color-profile.h"
#include "ligmalayer-floating-selection.h"
#include "ligmalayer.h"
#include "ligmalayermask.h"
#include "ligmaobjectqueue.h"
#include "ligmapickable.h"
#include "ligmaprogress.h"

#include "ligma-intl.h"


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


static void       ligma_color_managed_iface_init (LigmaColorManagedInterface *iface);
static void       ligma_pickable_iface_init      (LigmaPickableInterface     *iface);

static void       ligma_layer_set_property       (GObject            *object,
                                                 guint               property_id,
                                                 const GValue       *value,
                                                 GParamSpec         *pspec);
static void       ligma_layer_get_property       (GObject            *object,
                                                 guint               property_id,
                                                 GValue             *value,
                                                 GParamSpec         *pspec);
static void       ligma_layer_dispose            (GObject            *object);
static void       ligma_layer_finalize           (GObject            *object);
static void       ligma_layer_notify             (GObject            *object,
                                                 GParamSpec         *pspec);

static void       ligma_layer_name_changed       (LigmaObject         *object);
static gint64     ligma_layer_get_memsize        (LigmaObject         *object,
                                                 gint64             *gui_size);

static void       ligma_layer_invalidate_preview (LigmaViewable       *viewable);
static gchar    * ligma_layer_get_description    (LigmaViewable       *viewable,
                                                 gchar             **tooltip);

static GeglNode * ligma_layer_get_node           (LigmaFilter         *filter);

static void       ligma_layer_removed            (LigmaItem           *item);
static void       ligma_layer_unset_removed      (LigmaItem           *item);
static gboolean   ligma_layer_is_attached        (LigmaItem           *item);
static LigmaItemTree * ligma_layer_get_tree       (LigmaItem           *item);
static LigmaItem * ligma_layer_duplicate          (LigmaItem           *item,
                                                 GType               new_type);
static void       ligma_layer_convert            (LigmaItem           *item,
                                                 LigmaImage          *dest_image,
                                                 GType               old_type);
static gboolean   ligma_layer_rename             (LigmaItem           *item,
                                                 const gchar        *new_name,
                                                 const gchar        *undo_desc,
                                                 GError            **error);
static void       ligma_layer_start_move         (LigmaItem           *item,
                                                 gboolean            push_undo);
static void       ligma_layer_end_move           (LigmaItem           *item,
                                                 gboolean            push_undo);
static void       ligma_layer_translate          (LigmaItem           *item,
                                                 gdouble             offset_x,
                                                 gdouble             offset_y,
                                                 gboolean            push_undo);
static void       ligma_layer_scale              (LigmaItem           *item,
                                                 gint                new_width,
                                                 gint                new_height,
                                                 gint                new_offset_x,
                                                 gint                new_offset_y,
                                                 LigmaInterpolationType  interp_type,
                                                 LigmaProgress       *progress);
static void       ligma_layer_resize             (LigmaItem           *item,
                                                 LigmaContext        *context,
                                                 LigmaFillType        fill_type,
                                                 gint                new_width,
                                                 gint                new_height,
                                                 gint                offset_x,
                                                 gint                offset_y);
static void       ligma_layer_flip               (LigmaItem           *item,
                                                 LigmaContext        *context,
                                                 LigmaOrientationType flip_type,
                                                 gdouble             axis,
                                                 gboolean            clip_result);
static void       ligma_layer_rotate             (LigmaItem           *item,
                                                 LigmaContext        *context,
                                                 LigmaRotationType    rotate_type,
                                                 gdouble             center_x,
                                                 gdouble             center_y,
                                                 gboolean            clip_result);
static void       ligma_layer_transform          (LigmaItem           *item,
                                                 LigmaContext        *context,
                                                 const LigmaMatrix3  *matrix,
                                                 LigmaTransformDirection direction,
                                                 LigmaInterpolationType  interpolation_type,
                                                 LigmaTransformResize clip_result,
                                                 LigmaProgress       *progress);
static void       ligma_layer_to_selection       (LigmaItem           *item,
                                                 LigmaChannelOps      op,
                                                 gboolean            antialias,
                                                 gboolean            feather,
                                                 gdouble             feather_radius_x,
                                                 gdouble             feather_radius_y);

static void       ligma_layer_alpha_changed      (LigmaDrawable       *drawable);
static gint64     ligma_layer_estimate_memsize   (LigmaDrawable       *drawable,
                                                 LigmaComponentType   component_type,
                                                 gint                width,
                                                 gint                height);
static gboolean   ligma_layer_supports_alpha     (LigmaDrawable       *drawable);
static void       ligma_layer_convert_type       (LigmaDrawable       *drawable,
                                                 LigmaImage          *dest_image,
                                                 const Babl         *new_format,
                                                 LigmaColorProfile   *src_profile,
                                                 LigmaColorProfile   *dest_profile,
                                                 GeglDitherMethod    layer_dither_type,
                                                 GeglDitherMethod    mask_dither_type,
                                                 gboolean            push_undo,
                                                 LigmaProgress       *progress);
static void    ligma_layer_invalidate_boundary   (LigmaDrawable       *drawable);
static void    ligma_layer_get_active_components (LigmaDrawable       *drawable,
                                                 gboolean           *active);
static LigmaComponentMask
               ligma_layer_get_active_mask       (LigmaDrawable       *drawable);
static void    ligma_layer_set_buffer            (LigmaDrawable       *drawable,
                                                 gboolean            push_undo,
                                                 const gchar        *undo_desc,
                                                 GeglBuffer         *buffer,
                                                 const GeglRectangle *bounds);
static GeglRectangle
               ligma_layer_get_bounding_box      (LigmaDrawable       *drawable);

static LigmaColorProfile *
               ligma_layer_get_color_profile     (LigmaColorManaged   *managed);

static gdouble ligma_layer_get_opacity_at        (LigmaPickable       *pickable,
                                                 gint                x,
                                                 gint                y);
static void    ligma_layer_pixel_to_srgb         (LigmaPickable       *pickable,
                                                 const Babl         *format,
                                                 gpointer            pixel,
                                                 LigmaRGB            *color);
static void    ligma_layer_srgb_to_pixel         (LigmaPickable       *pickable,
                                                 const LigmaRGB      *color,
                                                 const Babl         *format,
                                                 gpointer            pixel);

static gboolean ligma_layer_real_is_alpha_locked (LigmaLayer          *layer,
                                                 LigmaLayer         **locked_layer);
static void       ligma_layer_real_translate     (LigmaLayer          *layer,
                                                 gint                offset_x,
                                                 gint                offset_y);
static void       ligma_layer_real_scale         (LigmaLayer          *layer,
                                                 gint                new_width,
                                                 gint                new_height,
                                                 gint                new_offset_x,
                                                 gint                new_offset_y,
                                                 LigmaInterpolationType  interp_type,
                                                 LigmaProgress       *progress);
static void       ligma_layer_real_resize        (LigmaLayer          *layer,
                                                 LigmaContext        *context,
                                                 LigmaFillType        fill_type,
                                                 gint                new_width,
                                                 gint                new_height,
                                                 gint                offset_x,
                                                 gint                offset_y);
static void       ligma_layer_real_flip          (LigmaLayer          *layer,
                                                 LigmaContext        *context,
                                                 LigmaOrientationType flip_type,
                                                 gdouble             axis,
                                                 gboolean            clip_result);
static void       ligma_layer_real_rotate        (LigmaLayer          *layer,
                                                 LigmaContext        *context,
                                                 LigmaRotationType    rotate_type,
                                                 gdouble             center_x,
                                                 gdouble             center_y,
                                                 gboolean            clip_result);
static void       ligma_layer_real_transform     (LigmaLayer          *layer,
                                                 LigmaContext        *context,
                                                 const LigmaMatrix3  *matrix,
                                                 LigmaTransformDirection direction,
                                                 LigmaInterpolationType  interpolation_type,
                                                 LigmaTransformResize clip_result,
                                                 LigmaProgress       *progress);
static void       ligma_layer_real_convert_type  (LigmaLayer          *layer,
                                                 LigmaImage          *dest_image,
                                                 const Babl         *new_format,
                                                 LigmaColorProfile   *src_profile,
                                                 LigmaColorProfile   *dest_profile,
                                                 GeglDitherMethod    layer_dither_type,
                                                 GeglDitherMethod    mask_dither_type,
                                                 gboolean            push_undo,
                                                 LigmaProgress       *progress);
static GeglRectangle
               ligma_layer_real_get_bounding_box (LigmaLayer          *layer);
static void  ligma_layer_real_get_effective_mode (LigmaLayer          *layer,
                                                 LigmaLayerMode          *mode,
                                                 LigmaLayerColorSpace    *blend_space,
                                                 LigmaLayerColorSpace    *composite_space,
                                                 LigmaLayerCompositeMode *composite_mode);
static gboolean
          ligma_layer_real_get_excludes_backdrop (LigmaLayer          *layer);

static void       ligma_layer_layer_mask_update  (LigmaDrawable       *layer_mask,
                                                 gint                x,
                                                 gint                y,
                                                 gint                width,
                                                 gint                height,
                                                 LigmaLayer          *layer);


G_DEFINE_TYPE_WITH_CODE (LigmaLayer, ligma_layer, LIGMA_TYPE_DRAWABLE,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_COLOR_MANAGED,
                                                ligma_color_managed_iface_init)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_PICKABLE,
                                                ligma_pickable_iface_init))

#define parent_class ligma_layer_parent_class

static guint layer_signals[LAST_SIGNAL] = { 0 };


static void
ligma_layer_class_init (LigmaLayerClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass   *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);
  LigmaFilterClass   *filter_class      = LIGMA_FILTER_CLASS (klass);
  LigmaItemClass     *item_class        = LIGMA_ITEM_CLASS (klass);
  LigmaDrawableClass *drawable_class    = LIGMA_DRAWABLE_CLASS (klass);

  layer_signals[OPACITY_CHANGED] =
    g_signal_new ("opacity-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaLayerClass, opacity_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[MODE_CHANGED] =
    g_signal_new ("mode-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaLayerClass, mode_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[BLEND_SPACE_CHANGED] =
    g_signal_new ("blend-space-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaLayerClass, blend_space_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[COMPOSITE_SPACE_CHANGED] =
    g_signal_new ("composite-space-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaLayerClass, composite_space_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[COMPOSITE_MODE_CHANGED] =
    g_signal_new ("composite-mode-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaLayerClass, composite_mode_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[EFFECTIVE_MODE_CHANGED] =
    g_signal_new ("effective-mode-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaLayerClass, effective_mode_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[EXCLUDES_BACKDROP_CHANGED] =
    g_signal_new ("excludes-backdrop-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaLayerClass, excludes_backdrop_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[LOCK_ALPHA_CHANGED] =
    g_signal_new ("lock-alpha-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaLayerClass, lock_alpha_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[MASK_CHANGED] =
    g_signal_new ("mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaLayerClass, mask_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[APPLY_MASK_CHANGED] =
    g_signal_new ("apply-mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaLayerClass, apply_mask_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[EDIT_MASK_CHANGED] =
    g_signal_new ("edit-mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaLayerClass, edit_mask_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[SHOW_MASK_CHANGED] =
    g_signal_new ("show-mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaLayerClass, show_mask_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->set_property          = ligma_layer_set_property;
  object_class->get_property          = ligma_layer_get_property;
  object_class->dispose               = ligma_layer_dispose;
  object_class->finalize              = ligma_layer_finalize;
  object_class->notify                = ligma_layer_notify;

  ligma_object_class->name_changed     = ligma_layer_name_changed;
  ligma_object_class->get_memsize      = ligma_layer_get_memsize;

  viewable_class->default_icon_name   = "ligma-layer";
  viewable_class->invalidate_preview  = ligma_layer_invalidate_preview;
  viewable_class->get_description     = ligma_layer_get_description;

  filter_class->get_node              = ligma_layer_get_node;

  item_class->removed                 = ligma_layer_removed;
  item_class->unset_removed           = ligma_layer_unset_removed;
  item_class->is_attached             = ligma_layer_is_attached;
  item_class->get_tree                = ligma_layer_get_tree;
  item_class->duplicate               = ligma_layer_duplicate;
  item_class->convert                 = ligma_layer_convert;
  item_class->rename                  = ligma_layer_rename;
  item_class->start_move              = ligma_layer_start_move;
  item_class->end_move                = ligma_layer_end_move;
  item_class->translate               = ligma_layer_translate;
  item_class->scale                   = ligma_layer_scale;
  item_class->resize                  = ligma_layer_resize;
  item_class->flip                    = ligma_layer_flip;
  item_class->rotate                  = ligma_layer_rotate;
  item_class->transform               = ligma_layer_transform;
  item_class->to_selection            = ligma_layer_to_selection;
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

  drawable_class->alpha_changed         = ligma_layer_alpha_changed;
  drawable_class->estimate_memsize      = ligma_layer_estimate_memsize;
  drawable_class->supports_alpha        = ligma_layer_supports_alpha;
  drawable_class->convert_type          = ligma_layer_convert_type;
  drawable_class->invalidate_boundary   = ligma_layer_invalidate_boundary;
  drawable_class->get_active_components = ligma_layer_get_active_components;
  drawable_class->get_active_mask       = ligma_layer_get_active_mask;
  drawable_class->set_buffer            = ligma_layer_set_buffer;
  drawable_class->get_bounding_box      = ligma_layer_get_bounding_box;

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
  klass->is_alpha_locked              = ligma_layer_real_is_alpha_locked;
  klass->translate                    = ligma_layer_real_translate;
  klass->scale                        = ligma_layer_real_scale;
  klass->resize                       = ligma_layer_real_resize;
  klass->flip                         = ligma_layer_real_flip;
  klass->rotate                       = ligma_layer_real_rotate;
  klass->transform                    = ligma_layer_real_transform;
  klass->convert_type                 = ligma_layer_real_convert_type;
  klass->get_bounding_box             = ligma_layer_real_get_bounding_box;
  klass->get_effective_mode           = ligma_layer_real_get_effective_mode;
  klass->get_excludes_backdrop        = ligma_layer_real_get_excludes_backdrop;

  g_object_class_install_property (object_class, PROP_OPACITY,
                                   g_param_spec_double ("opacity", NULL, NULL,
                                                        LIGMA_OPACITY_TRANSPARENT,
                                                        LIGMA_OPACITY_OPAQUE,
                                                        LIGMA_OPACITY_OPAQUE,
                                                        LIGMA_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_MODE,
                                   g_param_spec_enum ("mode", NULL, NULL,
                                                      LIGMA_TYPE_LAYER_MODE,
                                                      LIGMA_LAYER_MODE_NORMAL,
                                                      LIGMA_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_BLEND_SPACE,
                                   g_param_spec_enum ("blend-space",
                                                      NULL, NULL,
                                                      LIGMA_TYPE_LAYER_COLOR_SPACE,
                                                      LIGMA_LAYER_COLOR_SPACE_AUTO,
                                                      LIGMA_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_COMPOSITE_SPACE,
                                   g_param_spec_enum ("composite-space",
                                                      NULL, NULL,
                                                      LIGMA_TYPE_LAYER_COLOR_SPACE,
                                                      LIGMA_LAYER_COLOR_SPACE_AUTO,
                                                      LIGMA_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_COMPOSITE_MODE,
                                   g_param_spec_enum ("composite-mode",
                                                      NULL, NULL,
                                                      LIGMA_TYPE_LAYER_COMPOSITE_MODE,
                                                      LIGMA_LAYER_COMPOSITE_AUTO,
                                                      LIGMA_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_EXCLUDES_BACKDROP,
                                   g_param_spec_boolean ("excludes-backdrop",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_LOCK_ALPHA,
                                   g_param_spec_boolean ("lock-alpha",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_MASK,
                                   g_param_spec_object ("mask",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_LAYER_MASK,
                                                        LIGMA_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_FLOATING_SELECTION,
                                   g_param_spec_boolean ("floating-selection",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READABLE));
}

static void
ligma_layer_init (LigmaLayer *layer)
{
  layer->opacity                   = LIGMA_OPACITY_OPAQUE;
  layer->mode                      = LIGMA_LAYER_MODE_NORMAL;
  layer->blend_space               = LIGMA_LAYER_COLOR_SPACE_AUTO;
  layer->composite_space           = LIGMA_LAYER_COLOR_SPACE_AUTO;
  layer->composite_mode            = LIGMA_LAYER_COMPOSITE_AUTO;
  layer->effective_mode            = layer->mode;
  layer->effective_blend_space     = ligma_layer_get_real_blend_space (layer);
  layer->effective_composite_space = ligma_layer_get_real_composite_space (layer);
  layer->effective_composite_mode  = ligma_layer_get_real_composite_mode (layer);
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
ligma_color_managed_iface_init (LigmaColorManagedInterface *iface)
{
  iface->get_color_profile = ligma_layer_get_color_profile;
}

static void
ligma_pickable_iface_init (LigmaPickableInterface *iface)
{
  iface->get_opacity_at = ligma_layer_get_opacity_at;
  iface->pixel_to_srgb  = ligma_layer_pixel_to_srgb;
  iface->srgb_to_pixel  = ligma_layer_srgb_to_pixel;
}

static void
ligma_layer_set_property (GObject      *object,
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
ligma_layer_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  LigmaLayer *layer = LIGMA_LAYER (object);

  switch (property_id)
    {
    case PROP_OPACITY:
      g_value_set_double (value, ligma_layer_get_opacity (layer));
      break;
    case PROP_MODE:
      g_value_set_enum (value, ligma_layer_get_mode (layer));
      break;
    case PROP_BLEND_SPACE:
      g_value_set_enum (value, ligma_layer_get_blend_space (layer));
      break;
    case PROP_COMPOSITE_SPACE:
      g_value_set_enum (value, ligma_layer_get_composite_space (layer));
      break;
    case PROP_COMPOSITE_MODE:
      g_value_set_enum (value, ligma_layer_get_composite_mode (layer));
      break;
    case PROP_EXCLUDES_BACKDROP:
      g_value_set_boolean (value, ligma_layer_get_excludes_backdrop (layer));
      break;
    case PROP_LOCK_ALPHA:
      g_value_set_boolean (value, ligma_layer_get_lock_alpha (layer));
      break;
    case PROP_MASK:
      g_value_set_object (value, ligma_layer_get_mask (layer));
      break;
    case PROP_FLOATING_SELECTION:
      g_value_set_boolean (value, ligma_layer_is_floating_sel (layer));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_layer_dispose (GObject *object)
{
  LigmaLayer *layer = LIGMA_LAYER (object);

  if (layer->mask)
    g_signal_handlers_disconnect_by_func (layer->mask,
                                          ligma_layer_layer_mask_update,
                                          layer);

  if (ligma_layer_is_floating_sel (layer))
    {
      LigmaDrawable *fs_drawable = ligma_layer_get_floating_sel_drawable (layer);

      /* only detach if this is actually the drawable's fs because the
       * layer might be on the undo stack and not attached to anything
       */
      if (ligma_drawable_get_floating_sel (fs_drawable) == layer)
        ligma_drawable_detach_floating_sel (fs_drawable);

      ligma_layer_set_floating_sel_drawable (layer, NULL);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_layer_finalize (GObject *object)
{
  LigmaLayer *layer = LIGMA_LAYER (object);

  g_clear_object (&layer->mask);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_layer_update_mode_node (LigmaLayer *layer)
{
  GeglNode               *mode_node;
  LigmaLayerMode           visible_mode;
  LigmaLayerColorSpace     visible_blend_space;
  LigmaLayerColorSpace     visible_composite_space;
  LigmaLayerCompositeMode  visible_composite_mode;

  mode_node = ligma_drawable_get_mode_node (LIGMA_DRAWABLE (layer));

  if (layer->mask && layer->show_mask)
    {
      visible_mode            = LIGMA_LAYER_MODE_NORMAL;
      visible_blend_space     = LIGMA_LAYER_COLOR_SPACE_AUTO;
      visible_composite_space = LIGMA_LAYER_COLOR_SPACE_AUTO;
      visible_composite_mode  = LIGMA_LAYER_COMPOSITE_AUTO;

      /* This makes sure that masks of LEGACY-mode layers are
       * composited in PERCEPTUAL space, and non-LEGACY layers in
       * LINEAR space, or whatever composite space was chosen in the
       * layer attributes dialog
       */
      visible_composite_space = ligma_layer_get_real_composite_space (layer);
    }
  else
    {
      visible_mode            = layer->effective_mode;
      visible_blend_space     = layer->effective_blend_space;
      visible_composite_space = layer->effective_composite_space;
      visible_composite_mode  = layer->effective_composite_mode;
    }

  ligma_gegl_mode_node_set_mode (mode_node,
                                visible_mode,
                                visible_blend_space,
                                visible_composite_space,
                                visible_composite_mode);
  ligma_gegl_mode_node_set_opacity (mode_node, layer->opacity);
}

static void
ligma_layer_notify (GObject    *object,
                   GParamSpec *pspec)
{
  if (! strcmp (pspec->name, "is-last-node") &&
      ligma_filter_peek_node (LIGMA_FILTER (object)))
    {
      ligma_layer_update_mode_node (LIGMA_LAYER (object));

      ligma_drawable_update (LIGMA_DRAWABLE (object), 0, 0, -1, -1);
    }
}

static void
ligma_layer_name_changed (LigmaObject *object)
{
  LigmaLayer *layer = LIGMA_LAYER (object);

  if (LIGMA_OBJECT_CLASS (parent_class)->name_changed)
    LIGMA_OBJECT_CLASS (parent_class)->name_changed (object);

  if (layer->mask)
    {
      gchar *mask_name = g_strdup_printf (_("%s mask"),
                                          ligma_object_get_name (object));

      ligma_object_take_name (LIGMA_OBJECT (layer->mask), mask_name);
    }
}

static gint64
ligma_layer_get_memsize (LigmaObject *object,
                        gint64     *gui_size)
{
  LigmaLayer *layer   = LIGMA_LAYER (object);
  gint64     memsize = 0;

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (layer->mask), gui_size);

  *gui_size += layer->fs.num_segs * sizeof (LigmaBoundSeg);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_layer_invalidate_preview (LigmaViewable *viewable)
{
  LigmaLayer *layer = LIGMA_LAYER (viewable);

  LIGMA_VIEWABLE_CLASS (parent_class)->invalidate_preview (viewable);

  if (ligma_layer_is_floating_sel (layer))
    floating_sel_invalidate (layer);
}

static gchar *
ligma_layer_get_description (LigmaViewable  *viewable,
                            gchar        **tooltip)
{
  if (ligma_layer_is_floating_sel (LIGMA_LAYER (viewable)))
    {
      LigmaDrawable *drawable;
      const gchar  *header = _("Floating Selection");

      drawable = ligma_layer_get_floating_sel_drawable (LIGMA_LAYER (viewable));
      if (LIGMA_IS_LAYER_MASK (drawable))
        header = _("Floating Mask");
      else if (LIGMA_IS_LAYER (drawable))
        header = _("Floating Layer");
      /* TRANSLATORS: the first %s will be the type of floating item, i.e.
       * either a "Floating Layer" or "Floating Mask" usually. The second will
       * be a layer name.
       */
      return g_strdup_printf (_("%s\n(%s)"),
                              header, ligma_object_get_name (viewable));
    }

  return LIGMA_VIEWABLE_CLASS (parent_class)->get_description (viewable,
                                                              tooltip);
}

static GeglNode *
ligma_layer_get_node (LigmaFilter *filter)
{
  LigmaDrawable *drawable = LIGMA_DRAWABLE (filter);
  LigmaLayer    *layer    = LIGMA_LAYER (filter);
  GeglNode     *node;
  GeglNode     *input;
  GeglNode     *source;
  GeglNode     *mode_node;
  gboolean      source_node_hijacked = FALSE;

  node = LIGMA_FILTER_CLASS (parent_class)->get_node (filter);

  input = gegl_node_get_input_proxy (node, "input");

  source = ligma_drawable_get_source_node (drawable);

  /* if the source node already has a parent, we are a floating
   * selection and the source node has been hijacked by the fs'
   * drawable
   */
  if (gegl_node_get_parent (source))
    source_node_hijacked = TRUE;

  if (! source_node_hijacked)
    gegl_node_add_child (node, source);

  gegl_node_connect_to (input,  "output",
                        source, "input");

  g_warn_if_fail (layer->layer_offset_node == NULL);
  g_warn_if_fail (layer->mask_offset_node == NULL);

  /* the mode node connects it all, and has aux and aux2 inputs for
   * the layer and its mask
   */
  mode_node = ligma_drawable_get_mode_node (drawable);
  ligma_layer_update_mode_node (layer);

  /* the layer's offset node */
  layer->layer_offset_node = gegl_node_new_child (node,
                                                  "operation", "gegl:translate",
                                                  NULL);
  ligma_item_add_offset_node (LIGMA_ITEM (layer), layer->layer_offset_node);

  /* the layer mask's offset node */
  layer->mask_offset_node = gegl_node_new_child (node,
                                                 "operation", "gegl:translate",
                                                  NULL);
  ligma_item_add_offset_node (LIGMA_ITEM (layer), layer->mask_offset_node);

  if (! source_node_hijacked)
    {
      gegl_node_connect_to (source,                   "output",
                            layer->layer_offset_node, "input");
    }

  if (! (layer->mask && ligma_layer_get_show_mask (layer)))
    {
      gegl_node_connect_to (layer->layer_offset_node, "output",
                            mode_node,                "aux");
    }

  if (layer->mask)
    {
      GeglNode *mask;

      mask = ligma_drawable_get_source_node (LIGMA_DRAWABLE (layer->mask));

      gegl_node_connect_to (mask,                    "output",
                            layer->mask_offset_node, "input");

      if (ligma_layer_get_show_mask (layer))
        {
          gegl_node_connect_to (layer->mask_offset_node, "output",
                                mode_node,               "aux");
        }
      else if (ligma_layer_get_apply_mask (layer))
        {
          gegl_node_connect_to (layer->mask_offset_node, "output",
                                mode_node,               "aux2");
        }
    }

  return node;
}

static void
ligma_layer_removed (LigmaItem *item)
{
  LigmaLayer *layer = LIGMA_LAYER (item);

  if (layer->mask)
    ligma_item_removed (LIGMA_ITEM (layer->mask));

  if (LIGMA_ITEM_CLASS (parent_class)->removed)
    LIGMA_ITEM_CLASS (parent_class)->removed (item);
}

static void
ligma_layer_unset_removed (LigmaItem *item)
{
  LigmaLayer *layer = LIGMA_LAYER (item);

  if (layer->mask)
    ligma_item_unset_removed (LIGMA_ITEM (layer->mask));

  if (LIGMA_ITEM_CLASS (parent_class)->unset_removed)
    LIGMA_ITEM_CLASS (parent_class)->unset_removed (item);
}

static gboolean
ligma_layer_is_attached (LigmaItem *item)
{
  LigmaImage *image = ligma_item_get_image (item);

  return (LIGMA_IS_IMAGE (image) &&
          ligma_container_have (ligma_image_get_layers (image),
                               LIGMA_OBJECT (item)));
}

static LigmaItemTree *
ligma_layer_get_tree (LigmaItem *item)
{
  if (ligma_item_is_attached (item))
    {
      LigmaImage *image = ligma_item_get_image (item);

      return ligma_image_get_layer_tree (image);
    }

  return NULL;
}

static LigmaItem *
ligma_layer_duplicate (LigmaItem *item,
                      GType     new_type)
{
  LigmaItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, LIGMA_TYPE_DRAWABLE), NULL);

  new_item = LIGMA_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (LIGMA_IS_LAYER (new_item))
    {
      LigmaLayer *layer     = LIGMA_LAYER (item);
      LigmaLayer *new_layer = LIGMA_LAYER (new_item);

      ligma_layer_set_mode            (new_layer,
                                      ligma_layer_get_mode (layer), FALSE);
      ligma_layer_set_blend_space     (new_layer,
                                      ligma_layer_get_blend_space (layer), FALSE);
      ligma_layer_set_composite_space (new_layer,
                                      ligma_layer_get_composite_space (layer), FALSE);
      ligma_layer_set_composite_mode  (new_layer,
                                      ligma_layer_get_composite_mode (layer), FALSE);
      ligma_layer_set_opacity         (new_layer,
                                      ligma_layer_get_opacity (layer), FALSE);

      if (ligma_layer_can_lock_alpha (new_layer))
        ligma_layer_set_lock_alpha (new_layer,
                                   ligma_layer_get_lock_alpha (layer), FALSE);

      /*  duplicate the layer mask if necessary  */
      if (layer->mask)
        {
          LigmaItem *mask;

          mask = ligma_item_duplicate (LIGMA_ITEM (layer->mask),
                                      G_TYPE_FROM_INSTANCE (layer->mask));
          ligma_layer_add_mask (new_layer, LIGMA_LAYER_MASK (mask), FALSE, NULL);

          new_layer->apply_mask = layer->apply_mask;
          new_layer->edit_mask  = layer->edit_mask;
          new_layer->show_mask  = layer->show_mask;
        }
    }

  return new_item;
}

static void
ligma_layer_convert (LigmaItem  *item,
                    LigmaImage *dest_image,
                    GType      old_type)
{
  LigmaLayer         *layer    = LIGMA_LAYER (item);
  LigmaDrawable      *drawable = LIGMA_DRAWABLE (item);
  LigmaImageBaseType  old_base_type;
  LigmaImageBaseType  new_base_type;
  LigmaPrecision      old_precision;
  LigmaPrecision      new_precision;
  LigmaColorProfile  *src_profile  = NULL;
  LigmaColorProfile  *dest_profile = NULL;

  old_base_type = ligma_drawable_get_base_type (drawable);
  new_base_type = ligma_image_get_base_type (dest_image);

  old_precision = ligma_drawable_get_precision (drawable);
  new_precision = ligma_image_get_precision (dest_image);

  if (g_type_is_a (old_type, LIGMA_TYPE_LAYER))
    {
      src_profile =
        ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (item));

      dest_profile =
        ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (dest_image));

      if (ligma_color_profile_is_equal (dest_profile, src_profile))
        {
          src_profile  = NULL;
          dest_profile = NULL;
        }
    }

  if (old_base_type != new_base_type ||
      old_precision != new_precision ||
      dest_profile)
    {
      ligma_drawable_convert_type (drawable, dest_image,
                                  new_base_type,
                                  new_precision,
                                  ligma_drawable_has_alpha (drawable),
                                  src_profile,
                                  dest_profile,
                                  GEGL_DITHER_NONE, GEGL_DITHER_NONE,
                                  FALSE, NULL);
    }

  if (layer->mask)
    ligma_item_set_image (LIGMA_ITEM (layer->mask), dest_image);

  LIGMA_ITEM_CLASS (parent_class)->convert (item, dest_image, old_type);
}

static gboolean
ligma_layer_rename (LigmaItem     *item,
                   const gchar  *new_name,
                   const gchar  *undo_desc,
                   GError      **error)
{
  LigmaLayer *layer = LIGMA_LAYER (item);
  LigmaImage *image = ligma_item_get_image (item);
  gboolean   attached;
  gboolean   floating_sel;

  attached     = ligma_item_is_attached (item);
  floating_sel = ligma_layer_is_floating_sel (layer);

  if (floating_sel)
    {
      if (LIGMA_IS_CHANNEL (ligma_layer_get_floating_sel_drawable (layer)))
        {
          g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                               _("Cannot create a new layer from the floating "
                                 "selection because it belongs to a layer mask "
                                 "or channel."));
          return FALSE;
        }

      if (attached)
        {
          ligma_image_undo_group_start (image,
                                       LIGMA_UNDO_GROUP_ITEM_PROPERTIES,
                                       undo_desc);

          floating_sel_to_layer (layer, NULL);
        }
    }

  LIGMA_ITEM_CLASS (parent_class)->rename (item, new_name, undo_desc, error);

  if (attached && floating_sel)
    ligma_image_undo_group_end (image);

  return TRUE;
}

static void
ligma_layer_start_move (LigmaItem *item,
                       gboolean  push_undo)
{
  LigmaLayer *layer     = LIGMA_LAYER (item);
  LigmaLayer *ancestor  = layer;
  GSList    *ancestors = NULL;

  /* suspend mask cropping for all of the layer's ancestors */
  while ((ancestor = ligma_layer_get_parent (ancestor)))
    {
      ligma_group_layer_suspend_mask (LIGMA_GROUP_LAYER (ancestor), push_undo);

      ancestors = g_slist_prepend (ancestors, g_object_ref (ancestor));
    }

  /* we keep the ancestor list around, so that we can resume mask cropping for
   * the same set of groups in ligma_layer_end_move().  note that
   * ligma_image_remove_layer() calls start_move() before removing the layer,
   * while it's still part of the layer tree, and end_move() afterwards, when
   * it's no longer part of the layer tree, and hence we can't use get_parent()
   * in end_move() to get the same set of ancestors.
   */
  layer->move_stack = g_slist_prepend (layer->move_stack, ancestors);

  if (LIGMA_ITEM_CLASS (parent_class)->start_move)
    LIGMA_ITEM_CLASS (parent_class)->start_move (item, push_undo);
}

static void
ligma_layer_end_move (LigmaItem *item,
                     gboolean  push_undo)
{
  LigmaLayer *layer = LIGMA_LAYER (item);
  GSList    *ancestors;
  GSList    *iter;

  g_return_if_fail (layer->move_stack != NULL);

  if (LIGMA_ITEM_CLASS (parent_class)->end_move)
    LIGMA_ITEM_CLASS (parent_class)->end_move (item, push_undo);

  ancestors = layer->move_stack->data;

  layer->move_stack = g_slist_remove (layer->move_stack, ancestors);

  /* resume mask cropping for all of the layer's ancestors */
  for (iter = ancestors; iter; iter = g_slist_next (iter))
    {
      LigmaGroupLayer *ancestor = iter->data;

      ligma_group_layer_resume_mask (ancestor, push_undo);

      g_object_unref (ancestor);
    }

  g_slist_free (ancestors);
}

static void
ligma_layer_translate (LigmaItem *item,
                      gdouble   offset_x,
                      gdouble   offset_y,
                      gboolean  push_undo)
{
  LigmaLayer *layer = LIGMA_LAYER (item);

  if (push_undo)
    ligma_image_undo_push_item_displace (ligma_item_get_image (item), NULL, item);

  LIGMA_LAYER_GET_CLASS (layer)->translate (layer,
                                           SIGNED_ROUND (offset_x),
                                           SIGNED_ROUND (offset_y));

  if (layer->mask)
    {
      gint off_x, off_y;

      ligma_item_get_offset (item, &off_x, &off_y);
      ligma_item_set_offset (LIGMA_ITEM (layer->mask), off_x, off_y);

      ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (layer->mask));
    }
}

static void
ligma_layer_scale (LigmaItem              *item,
                  gint                   new_width,
                  gint                   new_height,
                  gint                   new_offset_x,
                  gint                   new_offset_y,
                  LigmaInterpolationType  interpolation_type,
                  LigmaProgress          *progress)
{
  LigmaLayer       *layer = LIGMA_LAYER (item);
  LigmaObjectQueue *queue = NULL;

  if (progress && layer->mask)
    {
      LigmaLayerMask *mask;

      queue    = ligma_object_queue_new (progress);
      progress = LIGMA_PROGRESS (queue);

      /* temporarily set layer->mask to NULL, so that its size won't be counted
       * when pushing the layer to the queue.
       */
      mask        = layer->mask;
      layer->mask = NULL;

      ligma_object_queue_push (queue, layer);
      ligma_object_queue_push (queue, mask);

      layer->mask = mask;
    }

  if (queue)
    ligma_object_queue_pop (queue);

  LIGMA_LAYER_GET_CLASS (layer)->scale (layer, new_width, new_height,
                                       new_offset_x, new_offset_y,
                                       interpolation_type, progress);

  if (layer->mask)
    {
      if (queue)
        ligma_object_queue_pop (queue);

      ligma_item_scale (LIGMA_ITEM (layer->mask),
                       new_width, new_height,
                       new_offset_x, new_offset_y,
                       interpolation_type, progress);
    }

  g_clear_object (&queue);
}

static void
ligma_layer_resize (LigmaItem     *item,
                   LigmaContext  *context,
                   LigmaFillType  fill_type,
                   gint          new_width,
                   gint          new_height,
                   gint          offset_x,
                   gint          offset_y)
{
  LigmaLayer *layer  = LIGMA_LAYER (item);

  LIGMA_LAYER_GET_CLASS (layer)->resize (layer, context, fill_type,
                                        new_width, new_height,
                                        offset_x, offset_y);

  if (layer->mask)
    ligma_item_resize (LIGMA_ITEM (layer->mask), context, LIGMA_FILL_TRANSPARENT,
                      new_width, new_height, offset_x, offset_y);
}

static void
ligma_layer_flip (LigmaItem            *item,
                 LigmaContext         *context,
                 LigmaOrientationType  flip_type,
                 gdouble              axis,
                 gboolean             clip_result)
{
  LigmaLayer *layer = LIGMA_LAYER (item);

  LIGMA_LAYER_GET_CLASS (layer)->flip (layer, context, flip_type, axis,
                                      clip_result);

  if (layer->mask)
    ligma_item_flip (LIGMA_ITEM (layer->mask), context,
                    flip_type, axis, clip_result);
}

static void
ligma_layer_rotate (LigmaItem         *item,
                   LigmaContext      *context,
                   LigmaRotationType  rotate_type,
                   gdouble           center_x,
                   gdouble           center_y,
                   gboolean          clip_result)
{
  LigmaLayer *layer = LIGMA_LAYER (item);

  LIGMA_LAYER_GET_CLASS (layer)->rotate (layer, context,
                                        rotate_type, center_x, center_y,
                                        clip_result);

  if (layer->mask)
    ligma_item_rotate (LIGMA_ITEM (layer->mask), context,
                      rotate_type, center_x, center_y, clip_result);
}

static void
ligma_layer_transform (LigmaItem               *item,
                      LigmaContext            *context,
                      const LigmaMatrix3      *matrix,
                      LigmaTransformDirection  direction,
                      LigmaInterpolationType   interpolation_type,
                      LigmaTransformResize     clip_result,
                      LigmaProgress           *progress)
{
  LigmaLayer       *layer = LIGMA_LAYER (item);
  LigmaObjectQueue *queue = NULL;

  if (progress && layer->mask)
    {
      LigmaLayerMask *mask;

      queue    = ligma_object_queue_new (progress);
      progress = LIGMA_PROGRESS (queue);

      /* temporarily set layer->mask to NULL, so that its size won't be counted
       * when pushing the layer to the queue.
       */
      mask        = layer->mask;
      layer->mask = NULL;

      ligma_object_queue_push (queue, layer);
      ligma_object_queue_push (queue, mask);

      layer->mask = mask;
    }

  if (queue)
    ligma_object_queue_pop (queue);

  LIGMA_LAYER_GET_CLASS (layer)->transform (layer, context, matrix, direction,
                                           interpolation_type,
                                           clip_result,
                                           progress);

  if (layer->mask)
    {
      if (queue)
        ligma_object_queue_pop (queue);

      ligma_item_transform (LIGMA_ITEM (layer->mask), context,
                           matrix, direction,
                           interpolation_type,
                           clip_result, progress);
    }

  g_clear_object (&queue);
}

static void
ligma_layer_to_selection (LigmaItem       *item,
                         LigmaChannelOps  op,
                         gboolean        antialias,
                         gboolean        feather,
                         gdouble         feather_radius_x,
                         gdouble         feather_radius_y)
{
  LigmaLayer *layer = LIGMA_LAYER (item);
  LigmaImage *image = ligma_item_get_image (item);

  ligma_channel_select_alpha (ligma_image_get_mask (image),
                             LIGMA_DRAWABLE (layer),
                             op,
                             feather, feather_radius_x, feather_radius_y);
}

static void
ligma_layer_alpha_changed (LigmaDrawable *drawable)
{
  if (LIGMA_DRAWABLE_CLASS (parent_class)->alpha_changed)
    LIGMA_DRAWABLE_CLASS (parent_class)->alpha_changed (drawable);

  /* When we add/remove alpha, whatever cached color transforms in
   * view renderers need to be recreated because they cache the wrong
   * lcms formats. See bug 478528.
   */
  ligma_color_managed_profile_changed (LIGMA_COLOR_MANAGED (drawable));
}

static gint64
ligma_layer_estimate_memsize (LigmaDrawable      *drawable,
                             LigmaComponentType  component_type,
                             gint               width,
                             gint               height)
{
  LigmaLayer *layer   = LIGMA_LAYER (drawable);
  gint64     memsize = 0;

  if (layer->mask)
    memsize += ligma_drawable_estimate_memsize (LIGMA_DRAWABLE (layer->mask),
                                               component_type,
                                               width, height);

  return memsize +
         LIGMA_DRAWABLE_CLASS (parent_class)->estimate_memsize (drawable,
                                                               component_type,
                                                               width, height);
}

static gboolean
ligma_layer_supports_alpha (LigmaDrawable *drawable)
{
  return TRUE;
}

static void
ligma_layer_convert_type (LigmaDrawable     *drawable,
                         LigmaImage        *dest_image,
                         const Babl       *new_format,
                         LigmaColorProfile *src_profile,
                         LigmaColorProfile *dest_profile,
                         GeglDitherMethod  layer_dither_type,
                         GeglDitherMethod  mask_dither_type,
                         gboolean          push_undo,
                         LigmaProgress     *progress)
{
  LigmaLayer       *layer = LIGMA_LAYER (drawable);
  LigmaObjectQueue *queue = NULL;
  const Babl      *dest_space;
  const Babl      *space_format;
  gboolean         convert_mask;

  convert_mask = layer->mask &&
                 ligma_babl_format_get_precision (new_format) !=
                 ligma_drawable_get_precision (LIGMA_DRAWABLE (layer->mask));

  if (progress && convert_mask)
    {
      LigmaLayerMask *mask;

      queue    = ligma_object_queue_new (progress);
      progress = LIGMA_PROGRESS (queue);

      /* temporarily set layer->mask to NULL, so that its size won't be counted
       * when pushing the layer to the queue.
       */
      mask        = layer->mask;
      layer->mask = NULL;

      ligma_object_queue_push (queue, layer);
      ligma_object_queue_push (queue, mask);

      layer->mask = mask;
    }

  if (queue)
    ligma_object_queue_pop (queue);

  /*  when called with a dest_profile, always use the space from that
   *  profile, we can't use ligma_image_get_layer_space() during an
   *  image type or precision conversion
   */
  if (dest_profile)
    {
      dest_space =
        ligma_color_profile_get_space (dest_profile,
                                      LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                      NULL);
    }
  else
    {
      dest_space = ligma_image_get_layer_space (dest_image);
    }

  space_format = babl_format_with_space ((const gchar *) new_format,
                                         dest_space);

  LIGMA_LAYER_GET_CLASS (layer)->convert_type (layer, dest_image, space_format,
                                              src_profile, dest_profile,
                                              layer_dither_type,
                                              mask_dither_type, push_undo,
                                              progress);

  if (convert_mask)
    {
      if (queue)
        ligma_object_queue_pop (queue);

      ligma_drawable_convert_type (LIGMA_DRAWABLE (layer->mask), dest_image,
                                  LIGMA_GRAY,
                                  ligma_babl_format_get_precision (new_format),
                                  ligma_drawable_has_alpha (LIGMA_DRAWABLE (layer->mask)),
                                  NULL, NULL,
                                  layer_dither_type, mask_dither_type,
                                  push_undo, progress);
    }

  g_clear_object (&queue);
}

static void
ligma_layer_invalidate_boundary (LigmaDrawable *drawable)
{
  LigmaLayer *layer = LIGMA_LAYER (drawable);

  if (ligma_item_is_attached (LIGMA_ITEM (drawable)) &&
      ligma_item_is_visible (LIGMA_ITEM (drawable)))
    {
      LigmaImage   *image = ligma_item_get_image (LIGMA_ITEM (drawable));
      LigmaChannel *mask  = ligma_image_get_mask (image);

      /*  Turn the current selection off  */
      ligma_image_selection_invalidate (image);

      /*  Only bother with the bounds if there is a selection  */
      if (! ligma_channel_is_empty (mask))
        {
          mask->bounds_known   = FALSE;
          mask->boundary_known = FALSE;
        }
    }

  if (ligma_layer_is_floating_sel (layer))
    floating_sel_invalidate (layer);
}

static void
ligma_layer_get_active_components (LigmaDrawable *drawable,
                                  gboolean     *active)
{
  LigmaLayer  *layer  = LIGMA_LAYER (drawable);
  LigmaImage  *image  = ligma_item_get_image (LIGMA_ITEM (drawable));
  const Babl *format = ligma_drawable_get_format (drawable);

  /*  first copy the image active channels  */
  ligma_image_get_active_array (image, active);

  if (ligma_drawable_has_alpha (drawable) &&
      ligma_layer_is_alpha_locked (layer, NULL))
    active[babl_format_get_n_components (format) - 1] = FALSE;
}

static LigmaComponentMask
ligma_layer_get_active_mask (LigmaDrawable *drawable)
{
  LigmaLayer         *layer = LIGMA_LAYER (drawable);
  LigmaImage         *image = ligma_item_get_image (LIGMA_ITEM (drawable));
  LigmaComponentMask  mask  = ligma_image_get_active_mask (image);

  if (ligma_drawable_has_alpha (drawable) &&
      ligma_layer_is_alpha_locked (layer, NULL))
    mask &= ~LIGMA_COMPONENT_MASK_ALPHA;

  return mask;
}

static void
ligma_layer_set_buffer (LigmaDrawable        *drawable,
                       gboolean             push_undo,
                       const gchar         *undo_desc,
                       GeglBuffer          *buffer,
                       const GeglRectangle *bounds)
{
  GeglBuffer *old_buffer = ligma_drawable_get_buffer (drawable);
  gint        old_trc    = -1;

  if (old_buffer)
    old_trc = ligma_drawable_get_trc (drawable);

  LIGMA_DRAWABLE_CLASS (parent_class)->set_buffer (drawable,
                                                  push_undo, undo_desc,
                                                  buffer, bounds);

  if (ligma_filter_peek_node (LIGMA_FILTER (drawable)))
    {
      if (ligma_drawable_get_trc (drawable) != old_trc)
        ligma_layer_update_mode_node (LIGMA_LAYER (drawable));
    }
}

static GeglRectangle
ligma_layer_get_bounding_box (LigmaDrawable *drawable)
{
  LigmaLayer     *layer = LIGMA_LAYER (drawable);
  LigmaLayerMask *mask  = ligma_layer_get_mask (layer);
  GeglRectangle  bounding_box;

  if (mask && ligma_layer_get_show_mask (layer))
    {
      bounding_box = ligma_drawable_get_bounding_box (LIGMA_DRAWABLE (mask));
    }
  else
    {
      bounding_box = LIGMA_LAYER_GET_CLASS (layer)->get_bounding_box (layer);

      if (mask && ligma_layer_get_apply_mask (layer))
        {
          GeglRectangle mask_bounding_box;

          mask_bounding_box = ligma_drawable_get_bounding_box (
            LIGMA_DRAWABLE (mask));

          gegl_rectangle_intersect (&bounding_box,
                                    &bounding_box, &mask_bounding_box);
        }
    }

  return bounding_box;
}

static LigmaColorProfile *
ligma_layer_get_color_profile (LigmaColorManaged *managed)
{
  LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (managed));

  return ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (image));
}

static gdouble
ligma_layer_get_opacity_at (LigmaPickable *pickable,
                           gint          x,
                           gint          y)
{
  LigmaLayer *layer = LIGMA_LAYER (pickable);
  gdouble    value = LIGMA_OPACITY_TRANSPARENT;

  if (x >= 0 && x < ligma_item_get_width  (LIGMA_ITEM (layer)) &&
      y >= 0 && y < ligma_item_get_height (LIGMA_ITEM (layer)) &&
      ligma_item_is_visible (LIGMA_ITEM (layer)))
    {
      if (! ligma_drawable_has_alpha (LIGMA_DRAWABLE (layer)))
        {
          value = LIGMA_OPACITY_OPAQUE;
        }
      else
        {
          gegl_buffer_sample (ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer)),
                              x, y, NULL, &value, babl_format ("A double"),
                              GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
        }

      if (ligma_layer_get_mask (layer) &&
          ligma_layer_get_apply_mask (layer))
        {
          gdouble mask_value;

          mask_value = ligma_pickable_get_opacity_at (LIGMA_PICKABLE (layer->mask),
                                                     x, y);

          value *= mask_value;
        }
    }

  return value;
}

static void
ligma_layer_pixel_to_srgb (LigmaPickable *pickable,
                          const Babl   *format,
                          gpointer      pixel,
                          LigmaRGB      *color)
{
  LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (pickable));

  ligma_pickable_pixel_to_srgb (LIGMA_PICKABLE (image), format, pixel, color);
}

static void
ligma_layer_srgb_to_pixel (LigmaPickable  *pickable,
                          const LigmaRGB *color,
                          const Babl    *format,
                          gpointer       pixel)
{
  LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (pickable));

  ligma_pickable_srgb_to_pixel (LIGMA_PICKABLE (image), color, format, pixel);
}

static gboolean
ligma_layer_real_is_alpha_locked (LigmaLayer  *layer,
                                 LigmaLayer **locked_layer)
{
  LigmaItem *parent = ligma_item_get_parent (LIGMA_ITEM (layer));

  if (layer->lock_alpha)
    {
      if (locked_layer)
        *locked_layer = layer;
    }
  else if (parent &&
           ligma_layer_is_alpha_locked (LIGMA_LAYER (parent), locked_layer))
    {
      return TRUE;
    }

  return layer->lock_alpha;
}

static void
ligma_layer_real_translate (LigmaLayer *layer,
                           gint       offset_x,
                           gint       offset_y)
{
  /*  update the old region  */
  ligma_drawable_update (LIGMA_DRAWABLE (layer), 0, 0, -1, -1);

  /*  invalidate the selection boundary because of a layer modification  */
  ligma_drawable_invalidate_boundary (LIGMA_DRAWABLE (layer));

  LIGMA_ITEM_CLASS (parent_class)->translate (LIGMA_ITEM (layer),
                                             offset_x, offset_y,
                                             FALSE);

  /*  update the new region  */
  ligma_drawable_update (LIGMA_DRAWABLE (layer), 0, 0, -1, -1);
}

static void
ligma_layer_real_scale (LigmaLayer             *layer,
                       gint                   new_width,
                       gint                   new_height,
                       gint                   new_offset_x,
                       gint                   new_offset_y,
                       LigmaInterpolationType  interpolation_type,
                       LigmaProgress          *progress)
{
  LIGMA_ITEM_CLASS (parent_class)->scale (LIGMA_ITEM (layer),
                                         new_width, new_height,
                                         new_offset_x, new_offset_y,
                                         interpolation_type, progress);
}

static void
ligma_layer_real_resize (LigmaLayer    *layer,
                        LigmaContext  *context,
                        LigmaFillType  fill_type,
                        gint          new_width,
                        gint          new_height,
                        gint          offset_x,
                        gint          offset_y)
{
  if (fill_type == LIGMA_FILL_TRANSPARENT &&
      ! ligma_drawable_has_alpha (LIGMA_DRAWABLE (layer)))
    {
      fill_type = LIGMA_FILL_BACKGROUND;
    }

  LIGMA_ITEM_CLASS (parent_class)->resize (LIGMA_ITEM (layer),
                                          context, fill_type,
                                          new_width, new_height,
                                          offset_x, offset_y);
}

static void
ligma_layer_real_flip (LigmaLayer           *layer,
                      LigmaContext         *context,
                      LigmaOrientationType  flip_type,
                      gdouble              axis,
                      gboolean             clip_result)
{
  LIGMA_ITEM_CLASS (parent_class)->flip (LIGMA_ITEM (layer),
                                        context, flip_type, axis, clip_result);
}

static void
ligma_layer_real_rotate (LigmaLayer        *layer,
                        LigmaContext      *context,
                        LigmaRotationType  rotate_type,
                        gdouble           center_x,
                        gdouble           center_y,
                        gboolean          clip_result)
{
  LIGMA_ITEM_CLASS (parent_class)->rotate (LIGMA_ITEM (layer),
                                          context, rotate_type,
                                          center_x, center_y,
                                          clip_result);
}

static void
ligma_layer_real_transform (LigmaLayer              *layer,
                           LigmaContext            *context,
                           const LigmaMatrix3      *matrix,
                           LigmaTransformDirection  direction,
                           LigmaInterpolationType   interpolation_type,
                           LigmaTransformResize     clip_result,
                           LigmaProgress           *progress)
{
  LIGMA_ITEM_CLASS (parent_class)->transform (LIGMA_ITEM (layer),
                                             context, matrix, direction,
                                             interpolation_type,
                                             clip_result,
                                             progress);
}

static void
ligma_layer_real_convert_type (LigmaLayer        *layer,
                              LigmaImage        *dest_image,
                              const Babl       *new_format,
                              LigmaColorProfile *src_profile,
                              LigmaColorProfile *dest_profile,
                              GeglDitherMethod  layer_dither_type,
                              GeglDitherMethod  mask_dither_type,
                              gboolean          push_undo,
                              LigmaProgress     *progress)
{
  LigmaDrawable *drawable = LIGMA_DRAWABLE (layer);
  GeglBuffer   *src_buffer;
  GeglBuffer   *dest_buffer;

  if (layer_dither_type == GEGL_DITHER_NONE)
    {
      src_buffer = g_object_ref (ligma_drawable_get_buffer (drawable));
    }
  else
    {
      gint bits;

      src_buffer =
        gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                         ligma_item_get_width  (LIGMA_ITEM (layer)),
                                         ligma_item_get_height (LIGMA_ITEM (layer))),
                         ligma_drawable_get_format (drawable));

      bits = (babl_format_get_bytes_per_pixel (new_format) * 8 /
              babl_format_get_n_components (new_format));

      ligma_gegl_apply_dither (ligma_drawable_get_buffer (drawable),
                              NULL, NULL,
                              src_buffer, 1 << bits, layer_dither_type);
    }

  dest_buffer =
    gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                     ligma_item_get_width  (LIGMA_ITEM (layer)),
                                     ligma_item_get_height (LIGMA_ITEM (layer))),
                     new_format);

  if (dest_profile)
    {
      if (! src_profile)
        src_profile =
          ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (layer));

      ligma_gegl_convert_color_profile (src_buffer,  NULL, src_profile,
                                       dest_buffer, NULL, dest_profile,
                                       LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                       TRUE, progress);
    }
  else
    {
      ligma_gegl_buffer_copy (src_buffer, NULL, GEGL_ABYSS_NONE,
                             dest_buffer, NULL);
    }

  ligma_drawable_set_buffer (drawable, push_undo, NULL, dest_buffer);

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);
}

static GeglRectangle
ligma_layer_real_get_bounding_box (LigmaLayer *layer)
{
  return LIGMA_DRAWABLE_CLASS (parent_class)->get_bounding_box (
    LIGMA_DRAWABLE (layer));
}

static void
ligma_layer_real_get_effective_mode (LigmaLayer              *layer,
                                    LigmaLayerMode          *mode,
                                    LigmaLayerColorSpace    *blend_space,
                                    LigmaLayerColorSpace    *composite_space,
                                    LigmaLayerCompositeMode *composite_mode)
{
  *mode            = ligma_layer_get_mode (layer);
  *blend_space     = ligma_layer_get_real_blend_space (layer);
  *composite_space = ligma_layer_get_real_composite_space (layer);
  *composite_mode  = ligma_layer_get_real_composite_mode (layer);
}

static gboolean
ligma_layer_real_get_excludes_backdrop (LigmaLayer *layer)
{
  LigmaLayerCompositeRegion included_region;

  included_region = ligma_layer_mode_get_included_region (layer->mode,
                                                         layer->effective_composite_mode);

  return ! (included_region & LIGMA_LAYER_COMPOSITE_REGION_DESTINATION);
}

static void
ligma_layer_layer_mask_update (LigmaDrawable *drawable,
                              gint          x,
                              gint          y,
                              gint          width,
                              gint          height,
                              LigmaLayer    *layer)
{
  if (ligma_layer_get_apply_mask (layer) ||
      ligma_layer_get_show_mask (layer))
    {
      ligma_drawable_update (LIGMA_DRAWABLE (layer),
                            x, y, width, height);
    }
}


/*  public functions  */

void
ligma_layer_fix_format_space (LigmaLayer *layer,
                             gboolean   copy_buffer,
                             gboolean   push_undo)
{
  LigmaDrawable *drawable;
  const Babl   *format;

  g_return_if_fail (LIGMA_IS_LAYER (layer));
  g_return_if_fail (push_undo == FALSE || copy_buffer == TRUE);

  drawable = LIGMA_DRAWABLE (layer);

  format = ligma_image_get_layer_format (ligma_item_get_image (LIGMA_ITEM (layer)),
                                        ligma_drawable_has_alpha (drawable));

  if (format != ligma_drawable_get_format (drawable))
    {
      ligma_drawable_set_format (drawable, format, copy_buffer, push_undo);
    }
}

LigmaLayer *
ligma_layer_get_parent (LigmaLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), NULL);

  return LIGMA_LAYER (ligma_viewable_get_parent (LIGMA_VIEWABLE (layer)));
}

LigmaLayerMask *
ligma_layer_get_mask (LigmaLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), NULL);

  return layer->mask;
}

LigmaLayerMask *
ligma_layer_add_mask (LigmaLayer      *layer,
                     LigmaLayerMask  *mask,
                     gboolean        push_undo,
                     GError        **error)
{
  LigmaImage *image;

  g_return_val_if_fail (LIGMA_IS_LAYER (layer), NULL);
  g_return_val_if_fail (LIGMA_IS_LAYER_MASK (mask), NULL);
  g_return_val_if_fail (ligma_item_get_image (LIGMA_ITEM (layer)) ==
                        ligma_item_get_image (LIGMA_ITEM (mask)), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! ligma_item_is_attached (LIGMA_ITEM (layer)))
    push_undo = FALSE;

  image = ligma_item_get_image (LIGMA_ITEM (layer));

  if (layer->mask)
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("Unable to add a layer mask since "
                             "the layer already has one."));
      return NULL;
    }

  if ((ligma_item_get_width (LIGMA_ITEM (layer)) !=
       ligma_item_get_width (LIGMA_ITEM (mask))) ||
      (ligma_item_get_height (LIGMA_ITEM (layer)) !=
       ligma_item_get_height (LIGMA_ITEM (mask))))
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("Cannot add layer mask of different "
                             "dimensions than specified layer."));
      return NULL;
    }

  if (push_undo)
    ligma_image_undo_push_layer_mask_add (image, C_("undo-type", "Add Layer Mask"),
                                         layer, mask);

  layer->mask = g_object_ref_sink (mask);
  layer->apply_mask = TRUE;
  layer->edit_mask  = TRUE;
  layer->show_mask  = FALSE;

  ligma_layer_mask_set_layer (mask, layer);

  if (ligma_filter_peek_node (LIGMA_FILTER (layer)))
    {
      GeglNode *mode_node;
      GeglNode *mask;

      mode_node = ligma_drawable_get_mode_node (LIGMA_DRAWABLE (layer));

      mask = ligma_drawable_get_source_node (LIGMA_DRAWABLE (layer->mask));

      gegl_node_connect_to (mask,                    "output",
                            layer->mask_offset_node, "input");

      if (layer->show_mask)
        {
          gegl_node_connect_to (layer->mask_offset_node, "output",
                                mode_node,               "aux");
        }
      else
        {
          gegl_node_connect_to (layer->mask_offset_node, "output",
                                mode_node,               "aux2");
        }

      ligma_layer_update_mode_node (layer);
    }

  ligma_drawable_update_bounding_box (LIGMA_DRAWABLE (layer));

  ligma_layer_update_effective_mode (layer);
  ligma_layer_update_excludes_backdrop (layer);

  if (ligma_layer_get_apply_mask (layer) ||
      ligma_layer_get_show_mask (layer))
    {
      ligma_drawable_update (LIGMA_DRAWABLE (layer), 0, 0, -1, -1);
    }

  g_signal_connect (mask, "update",
                    G_CALLBACK (ligma_layer_layer_mask_update),
                    layer);

  g_signal_emit (layer, layer_signals[MASK_CHANGED], 0);

  g_object_notify (G_OBJECT (layer), "mask");

  /*  if the mask came from the undo stack, reset its "removed" state  */
  if (ligma_item_is_removed (LIGMA_ITEM (mask)))
    ligma_item_unset_removed (LIGMA_ITEM (mask));

  return layer->mask;
}

LigmaLayerMask *
ligma_layer_create_mask (LigmaLayer       *layer,
                        LigmaAddMaskType  add_mask_type,
                        LigmaChannel     *channel)
{
  LigmaDrawable  *drawable;
  LigmaItem      *item;
  LigmaLayerMask *mask;
  LigmaImage     *image;
  gchar         *mask_name;
  LigmaRGB        black = { 0.0, 0.0, 0.0, LIGMA_OPACITY_OPAQUE };

  g_return_val_if_fail (LIGMA_IS_LAYER (layer), NULL);
  g_return_val_if_fail (add_mask_type != LIGMA_ADD_MASK_CHANNEL ||
                        LIGMA_IS_CHANNEL (channel), NULL);

  drawable = LIGMA_DRAWABLE (layer);
  item     = LIGMA_ITEM (layer);
  image    = ligma_item_get_image (item);

  mask_name = g_strdup_printf (_("%s mask"),
                               ligma_object_get_name (layer));

  mask = ligma_layer_mask_new (image,
                              ligma_item_get_width  (item),
                              ligma_item_get_height (item),
                              mask_name, &black);

  g_free (mask_name);

  switch (add_mask_type)
    {
    case LIGMA_ADD_MASK_WHITE:
      ligma_channel_all (LIGMA_CHANNEL (mask), FALSE);
      break;

    case LIGMA_ADD_MASK_BLACK:
      ligma_channel_clear (LIGMA_CHANNEL (mask), NULL, FALSE);
      break;

    case LIGMA_ADD_MASK_ALPHA:
    case LIGMA_ADD_MASK_ALPHA_TRANSFER:
      if (ligma_drawable_has_alpha (drawable))
        {
          GeglBuffer *dest_buffer;
          const Babl *component_format;

          dest_buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (mask));

          component_format =
            ligma_image_get_component_format (image, LIGMA_CHANNEL_ALPHA);

          gegl_buffer_set_format (dest_buffer, component_format);
          ligma_gegl_buffer_copy (ligma_drawable_get_buffer (drawable), NULL,
                                 GEGL_ABYSS_NONE,
                                 dest_buffer, NULL);
          gegl_buffer_set_format (dest_buffer, NULL);

          if (add_mask_type == LIGMA_ADD_MASK_ALPHA_TRANSFER)
            {
              ligma_drawable_push_undo (drawable,
                                       C_("undo-type", "Transfer Alpha to Mask"),
                                       NULL,
                                       0, 0,
                                       ligma_item_get_width  (item),
                                       ligma_item_get_height (item));

              ligma_gegl_apply_set_alpha (ligma_drawable_get_buffer (drawable),
                                         NULL, NULL,
                                         ligma_drawable_get_buffer (drawable),
                                         1.0);
            }
        }
      break;

    case LIGMA_ADD_MASK_SELECTION:
    case LIGMA_ADD_MASK_CHANNEL:
      {
        gboolean channel_empty;
        gint     offset_x, offset_y;
        gint     copy_x, copy_y;
        gint     copy_width, copy_height;

        if (add_mask_type == LIGMA_ADD_MASK_SELECTION)
          channel = LIGMA_CHANNEL (ligma_image_get_mask (image));

        channel_empty = ligma_channel_is_empty (channel);

        ligma_item_get_offset (item, &offset_x, &offset_y);

        ligma_rectangle_intersect (0, 0,
                                  ligma_image_get_width  (image),
                                  ligma_image_get_height (image),
                                  offset_x, offset_y,
                                  ligma_item_get_width  (item),
                                  ligma_item_get_height (item),
                                  &copy_x, &copy_y,
                                  &copy_width, &copy_height);

        if (copy_width  < ligma_item_get_width  (item) ||
            copy_height < ligma_item_get_height (item) ||
            channel_empty)
          ligma_channel_clear (LIGMA_CHANNEL (mask), NULL, FALSE);

        if ((copy_width || copy_height) && ! channel_empty)
          {
            GeglBuffer *src;
            GeglBuffer *dest;
            const Babl *format;

            src  = ligma_drawable_get_buffer (LIGMA_DRAWABLE (channel));
            dest = ligma_drawable_get_buffer (LIGMA_DRAWABLE (mask));

            format = gegl_buffer_get_format (src);

            /* make sure no gamma conversion happens */
            gegl_buffer_set_format (dest, format);
            ligma_gegl_buffer_copy (src,
                                   GEGL_RECTANGLE (copy_x, copy_y,
                                                   copy_width, copy_height),
                                   GEGL_ABYSS_NONE,
                                   dest,
                                   GEGL_RECTANGLE (copy_x - offset_x,
                                                   copy_y - offset_y,
                                                   0, 0));
            gegl_buffer_set_format (dest, NULL);

            LIGMA_CHANNEL (mask)->bounds_known = FALSE;
          }
      }
      break;

    case LIGMA_ADD_MASK_COPY:
      {
        GeglBuffer *src_buffer;
        GeglBuffer *dest_buffer;

        if (! ligma_drawable_is_gray (drawable))
          {
            const Babl *copy_format =
              ligma_image_get_format (image, LIGMA_GRAY,
                                     ligma_drawable_get_precision (drawable),
                                     ligma_drawable_has_alpha (drawable),
                                     NULL);

            src_buffer =
              gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                               ligma_item_get_width  (item),
                                               ligma_item_get_height (item)),
                               copy_format);

            ligma_gegl_buffer_copy (ligma_drawable_get_buffer (drawable), NULL,
                                   GEGL_ABYSS_NONE,
                                   src_buffer, NULL);
          }
        else
          {
            src_buffer = ligma_drawable_get_buffer (drawable);
            g_object_ref (src_buffer);
          }

        dest_buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (mask));

        if (ligma_drawable_has_alpha (drawable))
          {
            LigmaRGB background;

            ligma_rgba_set (&background, 0.0, 0.0, 0.0, 0.0);

            ligma_gegl_apply_flatten (src_buffer, NULL, NULL,
                                     dest_buffer, &background, NULL,
                                     LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR);
          }
        else
          {
            ligma_gegl_buffer_copy (src_buffer, NULL, GEGL_ABYSS_NONE,
                                   dest_buffer, NULL);
          }

        g_object_unref (src_buffer);
      }

      LIGMA_CHANNEL (mask)->bounds_known = FALSE;
      break;
    }

  return mask;
}

void
ligma_layer_apply_mask (LigmaLayer         *layer,
                       LigmaMaskApplyMode  mode,
                       gboolean           push_undo)
{
  LigmaItem      *item;
  LigmaImage     *image;
  LigmaLayerMask *mask;
  gboolean       view_changed = FALSE;

  g_return_if_fail (LIGMA_IS_LAYER (layer));

  mask = ligma_layer_get_mask (layer);

  if (! mask)
    return;

  /*  APPLY can not be done to group layers  */
  g_return_if_fail (! ligma_viewable_get_children (LIGMA_VIEWABLE (layer)) ||
                    mode == LIGMA_MASK_DISCARD);

  /*  APPLY can only be done to layers with an alpha channel  */
  g_return_if_fail (ligma_drawable_has_alpha (LIGMA_DRAWABLE (layer)) ||
                    mode == LIGMA_MASK_DISCARD || push_undo == TRUE);

  item  = LIGMA_ITEM (layer);
  image = ligma_item_get_image (item);

  if (! image)
    return;

  if (push_undo)
    {
      ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_LAYER_APPLY_MASK,
                                   (mode == LIGMA_MASK_APPLY) ?
                                   C_("undo-type", "Apply Layer Mask") :
                                   C_("undo-type", "Delete Layer Mask"));

      ligma_image_undo_push_layer_mask_show (image, NULL, layer);
      ligma_image_undo_push_layer_mask_apply (image, NULL, layer);
      ligma_image_undo_push_layer_mask_remove (image, NULL, layer, mask);

      if (mode == LIGMA_MASK_APPLY &&
          ! ligma_drawable_has_alpha (LIGMA_DRAWABLE (layer)))
        {
          ligma_layer_add_alpha (layer);
        }
    }

  /*  check if applying the mask changes the projection  */
  if (ligma_layer_get_show_mask (layer)                                   ||
      (mode == LIGMA_MASK_APPLY   && ! ligma_layer_get_apply_mask (layer)) ||
      (mode == LIGMA_MASK_DISCARD &&   ligma_layer_get_apply_mask (layer)))
    {
      view_changed = TRUE;
    }

  if (mode == LIGMA_MASK_APPLY)
    {
      GeglBuffer *mask_buffer;
      GeglBuffer *dest_buffer;

      if (push_undo)
        ligma_drawable_push_undo (LIGMA_DRAWABLE (layer), NULL,
                                 NULL,
                                 0, 0,
                                 ligma_item_get_width  (item),
                                 ligma_item_get_height (item));

      /*  Combine the current layer's alpha channel and the mask  */
      mask_buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (mask));
      dest_buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer));

      ligma_gegl_apply_opacity (ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer)),
                               NULL, NULL, dest_buffer,
                               mask_buffer, 0, 0, 1.0);
    }

  g_signal_handlers_disconnect_by_func (mask,
                                        ligma_layer_layer_mask_update,
                                        layer);

  ligma_item_removed (LIGMA_ITEM (mask));
  g_object_unref (mask);
  layer->mask = NULL;

  if (push_undo)
    ligma_image_undo_group_end (image);

  if (ligma_filter_peek_node (LIGMA_FILTER (layer)))
    {
      GeglNode *mode_node;

      mode_node = ligma_drawable_get_mode_node (LIGMA_DRAWABLE (layer));

      if (layer->show_mask)
        {
          gegl_node_connect_to (layer->layer_offset_node, "output",
                                mode_node,                "aux");
        }
      else
        {
          gegl_node_disconnect (mode_node, "aux2");
        }

      ligma_layer_update_mode_node (layer);
    }

  ligma_drawable_update_bounding_box (LIGMA_DRAWABLE (layer));

  ligma_layer_update_effective_mode (layer);
  ligma_layer_update_excludes_backdrop (layer);

  /*  If applying actually changed the view  */
  if (view_changed)
    {
      ligma_drawable_update (LIGMA_DRAWABLE (layer), 0, 0, -1, -1);
    }
  else
    {
      ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (layer));
    }

  g_signal_emit (layer, layer_signals[MASK_CHANGED], 0);

  g_object_notify (G_OBJECT (layer), "mask");
}

void
ligma_layer_set_apply_mask (LigmaLayer *layer,
                           gboolean   apply,
                           gboolean   push_undo)
{
  g_return_if_fail (LIGMA_IS_LAYER (layer));
  g_return_if_fail (layer->mask != NULL);

  if (layer->apply_mask != apply)
    {
      LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (layer));

      if (push_undo && ligma_item_is_attached (LIGMA_ITEM (layer)))
        ligma_image_undo_push_layer_mask_apply (image,
                                               apply ?
                                               C_("undo-type", "Enable Layer Mask") :
                                               C_("undo-type", "Disable Layer Mask"),
                                               layer);

      layer->apply_mask = apply ? TRUE : FALSE;

      if (ligma_filter_peek_node (LIGMA_FILTER (layer)) &&
          ! ligma_layer_get_show_mask (layer))
        {
          GeglNode *mode_node;

          mode_node = ligma_drawable_get_mode_node (LIGMA_DRAWABLE (layer));

          if (layer->apply_mask)
            {
              gegl_node_connect_to (layer->mask_offset_node, "output",
                                    mode_node,               "aux2");
            }
          else
            {
              gegl_node_disconnect (mode_node, "aux2");
            }
        }

      ligma_drawable_update_bounding_box (LIGMA_DRAWABLE (layer));

      ligma_layer_update_effective_mode (layer);
      ligma_layer_update_excludes_backdrop (layer);

      ligma_drawable_update (LIGMA_DRAWABLE (layer), 0, 0, -1, -1);

      g_signal_emit (layer, layer_signals[APPLY_MASK_CHANGED], 0);
    }
}

gboolean
ligma_layer_get_apply_mask (LigmaLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (layer->mask, FALSE);

  return layer->apply_mask;
}

void
ligma_layer_set_edit_mask (LigmaLayer *layer,
                          gboolean   edit)
{
  g_return_if_fail (LIGMA_IS_LAYER (layer));
  g_return_if_fail (layer->mask != NULL);

  if (layer->edit_mask != edit)
    {
      layer->edit_mask = edit ? TRUE : FALSE;

      g_signal_emit (layer, layer_signals[EDIT_MASK_CHANGED], 0);
    }
}

gboolean
ligma_layer_get_edit_mask (LigmaLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (layer->mask, FALSE);

  return layer->edit_mask;
}

void
ligma_layer_set_show_mask (LigmaLayer *layer,
                          gboolean   show,
                          gboolean   push_undo)
{
  g_return_if_fail (LIGMA_IS_LAYER (layer));
  g_return_if_fail (layer->mask != NULL);

  if (layer->show_mask != show)
    {
      LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (layer));

      if (push_undo)
        ligma_image_undo_push_layer_mask_show (image,
                                              C_("undo-type", "Show Layer Mask"),
                                              layer);

      layer->show_mask = show ? TRUE : FALSE;

      if (ligma_filter_peek_node (LIGMA_FILTER (layer)))
        {
          GeglNode *mode_node;

          mode_node = ligma_drawable_get_mode_node (LIGMA_DRAWABLE (layer));

          if (layer->show_mask)
            {
              gegl_node_disconnect (mode_node, "aux2");

              gegl_node_connect_to (layer->mask_offset_node, "output",
                                    mode_node,               "aux");
            }
          else
            {
              gegl_node_connect_to (layer->layer_offset_node, "output",
                                    mode_node,                "aux");

              if (ligma_layer_get_apply_mask (layer))
                {
                  gegl_node_connect_to (layer->mask_offset_node, "output",
                                        mode_node,               "aux2");
                }
            }

          ligma_layer_update_mode_node (layer);
        }

      ligma_drawable_update_bounding_box (LIGMA_DRAWABLE (layer));

      ligma_layer_update_effective_mode (layer);
      ligma_layer_update_excludes_backdrop (layer);

      ligma_drawable_update (LIGMA_DRAWABLE (layer), 0, 0, -1, -1);

      g_signal_emit (layer, layer_signals[SHOW_MASK_CHANGED], 0);
    }
}

gboolean
ligma_layer_get_show_mask (LigmaLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (layer->mask, FALSE);

  return layer->show_mask;
}

void
ligma_layer_add_alpha (LigmaLayer *layer)
{
  LigmaItem     *item;
  LigmaDrawable *drawable;
  GeglBuffer   *new_buffer;

  g_return_if_fail (LIGMA_IS_LAYER (layer));

  if (ligma_drawable_has_alpha (LIGMA_DRAWABLE (layer)))
    return;

  item     = LIGMA_ITEM (layer);
  drawable = LIGMA_DRAWABLE (layer);

  new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                ligma_item_get_width  (item),
                                                ligma_item_get_height (item)),
                                ligma_drawable_get_format_with_alpha (drawable));

  ligma_gegl_buffer_copy (
    ligma_drawable_get_buffer (drawable), NULL, GEGL_ABYSS_NONE,
    new_buffer, NULL);

  ligma_drawable_set_buffer (LIGMA_DRAWABLE (layer),
                            ligma_item_is_attached (LIGMA_ITEM (layer)),
                            C_("undo-type", "Add Alpha Channel"),
                            new_buffer);
  g_object_unref (new_buffer);
}

void
ligma_layer_remove_alpha (LigmaLayer   *layer,
                         LigmaContext *context)
{
  GeglBuffer *new_buffer;
  LigmaRGB     background;

  g_return_if_fail (LIGMA_IS_LAYER (layer));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  if (! ligma_drawable_has_alpha (LIGMA_DRAWABLE (layer)))
    return;

  new_buffer =
    gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                     ligma_item_get_width  (LIGMA_ITEM (layer)),
                                     ligma_item_get_height (LIGMA_ITEM (layer))),
                     ligma_drawable_get_format_without_alpha (LIGMA_DRAWABLE (layer)));

  ligma_context_get_background (context, &background);
  ligma_pickable_srgb_to_image_color (LIGMA_PICKABLE (layer),
                                     &background, &background);

  ligma_gegl_apply_flatten (ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer)),
                           NULL, NULL,
                           new_buffer, &background,
                           ligma_drawable_get_space (LIGMA_DRAWABLE (layer)),
                           ligma_layer_get_real_composite_space (layer));

  ligma_drawable_set_buffer (LIGMA_DRAWABLE (layer),
                            ligma_item_is_attached (LIGMA_ITEM (layer)),
                            C_("undo-type", "Remove Alpha Channel"),
                            new_buffer);
  g_object_unref (new_buffer);
}

void
ligma_layer_resize_to_image (LigmaLayer    *layer,
                            LigmaContext  *context,
                            LigmaFillType  fill_type)
{
  LigmaImage *image;
  gint       offset_x;
  gint       offset_y;

  g_return_if_fail (LIGMA_IS_LAYER (layer));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (layer)));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  image = ligma_item_get_image (LIGMA_ITEM (layer));

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_ITEM_RESIZE,
                               C_("undo-type", "Layer to Image Size"));

  ligma_item_get_offset (LIGMA_ITEM (layer), &offset_x, &offset_y);
  ligma_item_resize (LIGMA_ITEM (layer), context, fill_type,
                    ligma_image_get_width  (image),
                    ligma_image_get_height (image),
                    offset_x, offset_y);

  ligma_image_undo_group_end (image);
}

/**********************/
/*  access functions  */
/**********************/

LigmaDrawable *
ligma_layer_get_floating_sel_drawable (LigmaLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), NULL);

  return layer->fs.drawable;
}

void
ligma_layer_set_floating_sel_drawable (LigmaLayer    *layer,
                                      LigmaDrawable *drawable)
{
  g_return_if_fail (LIGMA_IS_LAYER (layer));
  g_return_if_fail (drawable == NULL || LIGMA_IS_DRAWABLE (drawable));

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
ligma_layer_is_floating_sel (LigmaLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), FALSE);

  return (ligma_layer_get_floating_sel_drawable (layer) != NULL);
}

void
ligma_layer_set_opacity (LigmaLayer *layer,
                        gdouble    opacity,
                        gboolean   push_undo)
{
  g_return_if_fail (LIGMA_IS_LAYER (layer));

  opacity = CLAMP (opacity, LIGMA_OPACITY_TRANSPARENT, LIGMA_OPACITY_OPAQUE);

  if (layer->opacity != opacity)
    {
      if (push_undo && ligma_item_is_attached (LIGMA_ITEM (layer)))
        {
          LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (layer));

          ligma_image_undo_push_layer_opacity (image, NULL, layer);
        }

      layer->opacity = opacity;

      g_signal_emit (layer, layer_signals[OPACITY_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "opacity");

      if (ligma_filter_peek_node (LIGMA_FILTER (layer)))
        ligma_layer_update_mode_node (layer);

      ligma_drawable_update (LIGMA_DRAWABLE (layer), 0, 0, -1, -1);
    }
}

gdouble
ligma_layer_get_opacity (LigmaLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), LIGMA_OPACITY_OPAQUE);

  return layer->opacity;
}

void
ligma_layer_set_mode (LigmaLayer     *layer,
                     LigmaLayerMode  mode,
                     gboolean       push_undo)
{
  g_return_if_fail (LIGMA_IS_LAYER (layer));

  if (ligma_viewable_get_children (LIGMA_VIEWABLE (layer)) == NULL)
    {
      g_return_if_fail (ligma_layer_mode_get_context (mode) &
                        LIGMA_LAYER_MODE_CONTEXT_LAYER);
    }
  else
    {
      g_return_if_fail (ligma_layer_mode_get_context (mode) &
                        LIGMA_LAYER_MODE_CONTEXT_GROUP);
    }

  if (layer->mode != mode)
    {
      if (ligma_item_is_attached (LIGMA_ITEM (layer)))
        {
          LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (layer));

          ligma_image_unset_default_new_layer_mode (image);

          if (push_undo)
            ligma_image_undo_push_layer_mode (image, NULL, layer);
        }

      g_object_freeze_notify (G_OBJECT (layer));

      layer->mode = mode;

      g_signal_emit (layer, layer_signals[MODE_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "mode");

      /*  when changing modes, we always switch to AUTO blend and
       *  composite in order to avoid confusion
       */
      if (layer->blend_space != LIGMA_LAYER_COLOR_SPACE_AUTO)
        {
          layer->blend_space = LIGMA_LAYER_COLOR_SPACE_AUTO;

          g_signal_emit (layer, layer_signals[BLEND_SPACE_CHANGED], 0);
          g_object_notify (G_OBJECT (layer), "blend-space");
        }

      if (layer->composite_space != LIGMA_LAYER_COLOR_SPACE_AUTO)
        {
          layer->composite_space = LIGMA_LAYER_COLOR_SPACE_AUTO;

          g_signal_emit (layer, layer_signals[COMPOSITE_SPACE_CHANGED], 0);
          g_object_notify (G_OBJECT (layer), "composite-space");
        }

      if (layer->composite_mode != LIGMA_LAYER_COMPOSITE_AUTO)
        {
          layer->composite_mode = LIGMA_LAYER_COMPOSITE_AUTO;

          g_signal_emit (layer, layer_signals[COMPOSITE_MODE_CHANGED], 0);
          g_object_notify (G_OBJECT (layer), "composite-mode");
        }

      g_object_thaw_notify (G_OBJECT (layer));

      ligma_layer_update_effective_mode (layer);
      ligma_layer_update_excludes_backdrop (layer);
    }
}

LigmaLayerMode
ligma_layer_get_mode (LigmaLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), LIGMA_LAYER_MODE_NORMAL);

  return layer->mode;
}

void
ligma_layer_set_blend_space (LigmaLayer           *layer,
                            LigmaLayerColorSpace  blend_space,
                            gboolean             push_undo)
{
  g_return_if_fail (LIGMA_IS_LAYER (layer));

  if (! ligma_layer_mode_is_blend_space_mutable (layer->mode))
    return;

  if (layer->blend_space != blend_space)
    {
      if (push_undo && ligma_item_is_attached (LIGMA_ITEM (layer)))
        {
          LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (layer));

          ligma_image_undo_push_layer_mode (image, _("Set layer's blend space"), layer);
        }

      layer->blend_space = blend_space;

      g_signal_emit (layer, layer_signals[BLEND_SPACE_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "blend-space");

      ligma_layer_update_effective_mode (layer);
    }
}

LigmaLayerColorSpace
ligma_layer_get_blend_space (LigmaLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), LIGMA_LAYER_COLOR_SPACE_AUTO);

  return layer->blend_space;
}

LigmaLayerColorSpace
ligma_layer_get_real_blend_space (LigmaLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR);

  if (layer->blend_space == LIGMA_LAYER_COLOR_SPACE_AUTO)
    return ligma_layer_mode_get_blend_space (layer->mode);
  else
    return layer->blend_space;
}

void
ligma_layer_set_composite_space (LigmaLayer           *layer,
                                LigmaLayerColorSpace  composite_space,
                                gboolean             push_undo)
{
  g_return_if_fail (LIGMA_IS_LAYER (layer));

  if (! ligma_layer_mode_is_composite_space_mutable (layer->mode))
    return;

  if (layer->composite_space != composite_space)
    {
      if (push_undo && ligma_item_is_attached (LIGMA_ITEM (layer)))
        {
          LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (layer));

          ligma_image_undo_push_layer_mode (image, _("Set layer's composite space"), layer);
        }

      layer->composite_space = composite_space;

      g_signal_emit (layer, layer_signals[COMPOSITE_SPACE_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "composite-space");

      ligma_layer_update_effective_mode (layer);
    }
}

LigmaLayerColorSpace
ligma_layer_get_composite_space (LigmaLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), LIGMA_LAYER_COLOR_SPACE_AUTO);

  return layer->composite_space;
}

LigmaLayerColorSpace
ligma_layer_get_real_composite_space (LigmaLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR);

  if (layer->composite_space == LIGMA_LAYER_COLOR_SPACE_AUTO)
    return ligma_layer_mode_get_composite_space (layer->mode);
  else
    return layer->composite_space;
}

void
ligma_layer_set_composite_mode (LigmaLayer              *layer,
                               LigmaLayerCompositeMode  composite_mode,
                               gboolean                push_undo)
{
  g_return_if_fail (LIGMA_IS_LAYER (layer));

  if (! ligma_layer_mode_is_composite_mode_mutable (layer->mode))
    return;

  if (layer->composite_mode != composite_mode)
    {
      if (push_undo && ligma_item_is_attached (LIGMA_ITEM (layer)))
        {
          LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (layer));

          ligma_image_undo_push_layer_mode (image, _("Set layer's composite mode"), layer);
        }

      layer->composite_mode = composite_mode;

      g_signal_emit (layer, layer_signals[COMPOSITE_MODE_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "composite-mode");

      ligma_layer_update_effective_mode (layer);
      ligma_layer_update_excludes_backdrop (layer);
    }
}

LigmaLayerCompositeMode
ligma_layer_get_composite_mode (LigmaLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), LIGMA_LAYER_COMPOSITE_AUTO);

  return layer->composite_mode;
}

LigmaLayerCompositeMode
ligma_layer_get_real_composite_mode (LigmaLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), LIGMA_LAYER_COMPOSITE_UNION);

  if (layer->composite_mode == LIGMA_LAYER_COMPOSITE_AUTO)
    return ligma_layer_mode_get_composite_mode (layer->mode);
  else
    return layer->composite_mode;
}

void
ligma_layer_get_effective_mode (LigmaLayer              *layer,
                               LigmaLayerMode          *mode,
                               LigmaLayerColorSpace    *blend_space,
                               LigmaLayerColorSpace    *composite_space,
                               LigmaLayerCompositeMode *composite_mode)
{
  g_return_if_fail (LIGMA_IS_LAYER (layer));

  if (mode)            *mode            = layer->effective_mode;
  if (blend_space)     *blend_space     = layer->effective_blend_space;
  if (composite_space) *composite_space = layer->effective_composite_space;
  if (composite_mode)  *composite_mode  = layer->effective_composite_mode;
}

gboolean
ligma_layer_get_excludes_backdrop (LigmaLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), FALSE);

  return layer->excludes_backdrop;
}

void
ligma_layer_set_lock_alpha (LigmaLayer *layer,
                           gboolean   lock_alpha,
                           gboolean   push_undo)
{
  g_return_if_fail (LIGMA_IS_LAYER (layer));
  g_return_if_fail (ligma_layer_can_lock_alpha (layer));

  lock_alpha = lock_alpha ? TRUE : FALSE;

  if (layer->lock_alpha != lock_alpha)
    {
      if (push_undo && ligma_item_is_attached (LIGMA_ITEM (layer)))
        {
          LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (layer));

          ligma_image_undo_push_layer_lock_alpha (image, NULL, layer);
        }

      layer->lock_alpha = lock_alpha;

      g_signal_emit (layer, layer_signals[LOCK_ALPHA_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "lock-alpha");
    }
}

gboolean
ligma_layer_get_lock_alpha (LigmaLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), FALSE);

  return layer->lock_alpha;
}

gboolean
ligma_layer_can_lock_alpha (LigmaLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), FALSE);

  return TRUE;
}

gboolean
ligma_layer_is_alpha_locked (LigmaLayer  *layer,
                            LigmaLayer **locked_layer)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), FALSE);

  return LIGMA_LAYER_GET_CLASS (layer)->is_alpha_locked (layer, locked_layer);
}

/*  protected functions  */

void
ligma_layer_update_effective_mode (LigmaLayer *layer)
{
  LigmaLayerMode          mode;
  LigmaLayerColorSpace    blend_space;
  LigmaLayerColorSpace    composite_space;
  LigmaLayerCompositeMode composite_mode;

  g_return_if_fail (LIGMA_IS_LAYER (layer));

  if (layer->mask && layer->show_mask)
    {
      mode            = LIGMA_LAYER_MODE_NORMAL;
      blend_space     = LIGMA_LAYER_COLOR_SPACE_AUTO;
      composite_space = LIGMA_LAYER_COLOR_SPACE_AUTO;
      composite_mode  = LIGMA_LAYER_COMPOSITE_AUTO;

      /* This makes sure that masks of LEGACY-mode layers are
       * composited in PERCEPTUAL space, and non-LEGACY layers in
       * LINEAR space, or whatever composite space was chosen in the
       * layer attributes dialog
       */
      composite_space = ligma_layer_get_real_composite_space (layer);
    }
  else
    {
      LIGMA_LAYER_GET_CLASS (layer)->get_effective_mode (layer,
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
      layer->effective_mode            = mode;
      layer->effective_blend_space     = blend_space;
      layer->effective_composite_space = composite_space;
      layer->effective_composite_mode  = composite_mode;

      g_signal_emit (layer, layer_signals[EFFECTIVE_MODE_CHANGED], 0);

      if (ligma_filter_peek_node (LIGMA_FILTER (layer)))
        ligma_layer_update_mode_node (layer);

      ligma_drawable_update (LIGMA_DRAWABLE (layer), 0, 0, -1, -1);
    }
}

void
ligma_layer_update_excludes_backdrop (LigmaLayer *layer)
{
  gboolean excludes_backdrop;

  g_return_if_fail (LIGMA_IS_LAYER (layer));

  excludes_backdrop =
    LIGMA_LAYER_GET_CLASS (layer)->get_excludes_backdrop (layer);

  if (excludes_backdrop != layer->excludes_backdrop)
    {
      layer->excludes_backdrop = excludes_backdrop;

      g_signal_emit (layer, layer_signals[EXCLUDES_BACKDROP_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "excludes-backdrop");
    }
}
