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
#include <time.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <gexiv2/gexiv2.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"
#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "config/ligmacoreconfig.h"

#include "operations/layer-modes/ligma-layer-modes.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligma-gegl-loops.h"

#include "ligma.h"
#include "ligma-memsize.h"
#include "ligma-parasites.h"
#include "ligma-utils.h"
#include "ligmacontext.h"
#include "ligmadrawable-floating-selection.h"
#include "ligmadrawablestack.h"
#include "ligmagrid.h"
#include "ligmaerror.h"
#include "ligmaguide.h"
#include "ligmaidtable.h"
#include "ligmaimage.h"
#include "ligmaimage-color-profile.h"
#include "ligmaimage-colormap.h"
#include "ligmaimage-guides.h"
#include "ligmaimage-item-list.h"
#include "ligmaimage-metadata.h"
#include "ligmaimage-sample-points.h"
#include "ligmaimage-preview.h"
#include "ligmaimage-private.h"
#include "ligmaimage-quick-mask.h"
#include "ligmaimage-symmetry.h"
#include "ligmaimage-undo.h"
#include "ligmaimage-undo-push.h"
#include "ligmaitemlist.h"
#include "ligmaitemtree.h"
#include "ligmalayer.h"
#include "ligmalayer-floating-selection.h"
#include "ligmalayermask.h"
#include "ligmalayerstack.h"
#include "ligmamarshal.h"
#include "ligmapalette.h"
#include "ligmaparasitelist.h"
#include "ligmapickable.h"
#include "ligmaprojectable.h"
#include "ligmaprojection.h"
#include "ligmasamplepoint.h"
#include "ligmaselection.h"
#include "ligmasymmetry.h"
#include "ligmatempbuf.h"
#include "ligmatemplate.h"
#include "ligmaundostack.h"

#include "vectors/ligmavectors.h"

#include "ligma-log.h"
#include "ligma-intl.h"


#ifdef DEBUG
#define TRC(x) g_printerr x
#else
#define TRC(x)
#endif


enum
{
  MODE_CHANGED,
  PRECISION_CHANGED,
  ALPHA_CHANGED,
  FLOATING_SELECTION_CHANGED,
  SELECTED_CHANNELS_CHANGED,
  SELECTED_VECTORS_CHANGED,
  SELECTED_LAYERS_CHANGED,
  COMPONENT_VISIBILITY_CHANGED,
  COMPONENT_ACTIVE_CHANGED,
  MASK_CHANGED,
  RESOLUTION_CHANGED,
  SIZE_CHANGED_DETAILED,
  UNIT_CHANGED,
  QUICK_MASK_CHANGED,
  SELECTION_INVALIDATE,
  CLEAN,
  DIRTY,
  SAVING,
  SAVED,
  EXPORTED,
  GUIDE_ADDED,
  GUIDE_REMOVED,
  GUIDE_MOVED,
  SAMPLE_POINT_ADDED,
  SAMPLE_POINT_REMOVED,
  SAMPLE_POINT_MOVED,
  PARASITE_ATTACHED,
  PARASITE_DETACHED,
  COLORMAP_CHANGED,
  UNDO_EVENT,
  LAYER_SETS_CHANGED,
  CHANNEL_SETS_CHANGED,
  VECTORS_SETS_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_LIGMA,
  PROP_ID,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_BASE_TYPE,
  PROP_PRECISION,
  PROP_METADATA,
  PROP_BUFFER,
  PROP_SYMMETRY
};


/*  local function prototypes  */

static void     ligma_color_managed_iface_init    (LigmaColorManagedInterface *iface);
static void     ligma_projectable_iface_init      (LigmaProjectableInterface  *iface);
static void     ligma_pickable_iface_init         (LigmaPickableInterface     *iface);

static void     ligma_image_constructed           (GObject           *object);
static void     ligma_image_set_property          (GObject           *object,
                                                  guint              property_id,
                                                  const GValue      *value,
                                                  GParamSpec        *pspec);
static void     ligma_image_get_property          (GObject           *object,
                                                  guint              property_id,
                                                  GValue            *value,
                                                  GParamSpec        *pspec);
static void     ligma_image_dispose               (GObject           *object);
static void     ligma_image_finalize              (GObject           *object);

static void     ligma_image_name_changed          (LigmaObject        *object);
static gint64   ligma_image_get_memsize           (LigmaObject        *object,
                                                  gint64            *gui_size);

static gboolean ligma_image_get_size              (LigmaViewable      *viewable,
                                                  gint              *width,
                                                  gint              *height);
static void     ligma_image_size_changed          (LigmaViewable      *viewable);
static gchar  * ligma_image_get_description       (LigmaViewable      *viewable,
                                                  gchar            **tooltip);

static void     ligma_image_real_mode_changed     (LigmaImage         *image);
static void     ligma_image_real_precision_changed(LigmaImage         *image);
static void     ligma_image_real_resolution_changed(LigmaImage        *image);
static void     ligma_image_real_size_changed_detailed
                                                 (LigmaImage         *image,
                                                  gint               previous_origin_x,
                                                  gint               previous_origin_y,
                                                  gint               previous_width,
                                                  gint               previous_height);
static void     ligma_image_real_unit_changed     (LigmaImage         *image);
static void     ligma_image_real_colormap_changed (LigmaImage         *image,
                                                  gint               color_index);

static const guint8 *
        ligma_image_color_managed_get_icc_profile (LigmaColorManaged  *managed,
                                                  gsize             *len);
static LigmaColorProfile *
      ligma_image_color_managed_get_color_profile (LigmaColorManaged  *managed);
static void
        ligma_image_color_managed_profile_changed (LigmaColorManaged  *managed);

static LigmaColorProfile *
      ligma_image_color_managed_get_simulation_profile     (LigmaColorManaged  *managed);
static void
      ligma_image_color_managed_simulation_profile_changed (LigmaColorManaged  *managed);

static LigmaColorRenderingIntent
      ligma_image_color_managed_get_simulation_intent      (LigmaColorManaged  *managed);
static void
      ligma_image_color_managed_simulation_intent_changed  (LigmaColorManaged  *managed);

static gboolean
      ligma_image_color_managed_get_simulation_bpc         (LigmaColorManaged  *managed);
static void
      ligma_image_color_managed_simulation_bpc_changed     (LigmaColorManaged  *managed);

static void        ligma_image_projectable_flush  (LigmaProjectable   *projectable,
                                                  gboolean           invalidate_preview);
static GeglRectangle ligma_image_get_bounding_box (LigmaProjectable   *projectable);
static GeglNode   * ligma_image_get_graph         (LigmaProjectable   *projectable);
static LigmaImage  * ligma_image_get_image         (LigmaProjectable   *projectable);
static const Babl * ligma_image_get_proj_format   (LigmaProjectable   *projectable);

static void         ligma_image_pickable_flush    (LigmaPickable      *pickable);
static GeglBuffer * ligma_image_get_buffer        (LigmaPickable      *pickable);
static gboolean     ligma_image_get_pixel_at      (LigmaPickable      *pickable,
                                                  gint               x,
                                                  gint               y,
                                                  const Babl        *format,
                                                  gpointer           pixel);
static gdouble      ligma_image_get_opacity_at    (LigmaPickable      *pickable,
                                                  gint               x,
                                                  gint               y);
static void         ligma_image_get_pixel_average (LigmaPickable      *pickable,
                                                  const GeglRectangle *rect,
                                                  const Babl        *format,
                                                  gpointer           pixel);
static void         ligma_image_pixel_to_srgb     (LigmaPickable      *pickable,
                                                  const Babl        *format,
                                                  gpointer           pixel,
                                                  LigmaRGB           *color);
static void         ligma_image_srgb_to_pixel     (LigmaPickable      *pickable,
                                                  const LigmaRGB     *color,
                                                  const Babl        *format,
                                                  gpointer           pixel);

static void     ligma_image_projection_buffer_notify
                                                 (LigmaProjection    *projection,
                                                  const GParamSpec  *pspec,
                                                  LigmaImage         *image);
static void     ligma_image_mask_update           (LigmaDrawable      *drawable,
                                                  gint               x,
                                                  gint               y,
                                                  gint               width,
                                                  gint               height,
                                                  LigmaImage         *image);
static void     ligma_image_layers_changed        (LigmaContainer     *container,
                                                  LigmaChannel       *channel,
                                                  LigmaImage         *image);
static void     ligma_image_layer_offset_changed  (LigmaDrawable      *drawable,
                                                  const GParamSpec  *pspec,
                                                  LigmaImage         *image);
static void     ligma_image_layer_bounding_box_changed
                                                 (LigmaDrawable      *drawable,
                                                  LigmaImage         *image);
static void     ligma_image_layer_alpha_changed   (LigmaDrawable      *drawable,
                                                  LigmaImage         *image);
static void     ligma_image_channel_add           (LigmaContainer     *container,
                                                  LigmaChannel       *channel,
                                                  LigmaImage         *image);
static void     ligma_image_channel_remove        (LigmaContainer     *container,
                                                  LigmaChannel       *channel,
                                                  LigmaImage         *image);
static void     ligma_image_channel_name_changed  (LigmaChannel       *channel,
                                                  LigmaImage         *image);
static void     ligma_image_channel_color_changed (LigmaChannel       *channel,
                                                  LigmaImage         *image);

static void     ligma_image_selected_layers_notify   (LigmaItemTree      *tree,
                                                     const GParamSpec  *pspec,
                                                     LigmaImage         *image);
static void     ligma_image_selected_channels_notify (LigmaItemTree      *tree,
                                                     const GParamSpec  *pspec,
                                                     LigmaImage         *image);
static void     ligma_image_selected_vectors_notify  (LigmaItemTree      *tree,
                                                     const GParamSpec  *pspec,
                                                     LigmaImage         *image);

static void     ligma_image_freeze_bounding_box   (LigmaImage         *image);
static void     ligma_image_thaw_bounding_box     (LigmaImage         *image);
static void     ligma_image_update_bounding_box   (LigmaImage         *image);

static gint     ligma_image_layer_stack_cmp         (GList           *layers1,
                                                    GList           *layers2);
static void ligma_image_rec_remove_layer_stack_dups (LigmaImage       *image,
                                                    GSList          *start);
static void     ligma_image_clean_layer_stack       (LigmaImage       *image);
static void     ligma_image_remove_from_layer_stack (LigmaImage       *image,
                                                    LigmaLayer       *layer);
static gint     ligma_image_selected_is_descendant  (LigmaViewable    *selected,
                                                    LigmaViewable    *viewable);

G_DEFINE_TYPE_WITH_CODE (LigmaImage, ligma_image, LIGMA_TYPE_VIEWABLE,
                         G_ADD_PRIVATE (LigmaImage)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_COLOR_MANAGED,
                                                ligma_color_managed_iface_init)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_PROJECTABLE,
                                                ligma_projectable_iface_init)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_PICKABLE,
                                                ligma_pickable_iface_init))

#define parent_class ligma_image_parent_class

static guint ligma_image_signals[LAST_SIGNAL] = { 0 };


static void
ligma_image_class_init (LigmaImageClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass   *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);

  ligma_image_signals[MODE_CHANGED] =
    g_signal_new ("mode-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, mode_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_image_signals[PRECISION_CHANGED] =
    g_signal_new ("precision-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, precision_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_image_signals[ALPHA_CHANGED] =
    g_signal_new ("alpha-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, alpha_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_image_signals[FLOATING_SELECTION_CHANGED] =
    g_signal_new ("floating-selection-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, floating_selection_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_image_signals[SELECTED_LAYERS_CHANGED] =
    g_signal_new ("selected-layers-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, selected_layers_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_image_signals[SELECTED_CHANNELS_CHANGED] =
    g_signal_new ("selected-channels-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, selected_channels_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_image_signals[SELECTED_VECTORS_CHANGED] =
    g_signal_new ("selected-vectors-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, selected_vectors_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_image_signals[COMPONENT_VISIBILITY_CHANGED] =
    g_signal_new ("component-visibility-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, component_visibility_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_CHANNEL_TYPE);

  ligma_image_signals[COMPONENT_ACTIVE_CHANGED] =
    g_signal_new ("component-active-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, component_active_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_CHANNEL_TYPE);

  ligma_image_signals[MASK_CHANGED] =
    g_signal_new ("mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, mask_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_image_signals[RESOLUTION_CHANGED] =
    g_signal_new ("resolution-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, resolution_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_image_signals[SIZE_CHANGED_DETAILED] =
    g_signal_new ("size-changed-detailed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, size_changed_detailed),
                  NULL, NULL,
                  ligma_marshal_VOID__INT_INT_INT_INT,
                  G_TYPE_NONE, 4,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  ligma_image_signals[UNIT_CHANGED] =
    g_signal_new ("unit-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, unit_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_image_signals[QUICK_MASK_CHANGED] =
    g_signal_new ("quick-mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, quick_mask_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_image_signals[SELECTION_INVALIDATE] =
    g_signal_new ("selection-invalidate",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, selection_invalidate),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_image_signals[CLEAN] =
    g_signal_new ("clean",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, clean),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_DIRTY_MASK);

  ligma_image_signals[DIRTY] =
    g_signal_new ("dirty",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, dirty),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_DIRTY_MASK);

  ligma_image_signals[SAVING] =
    g_signal_new ("saving",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, saving),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_image_signals[SAVED] =
    g_signal_new ("saved",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, saved),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_FILE);

  ligma_image_signals[EXPORTED] =
    g_signal_new ("exported",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, exported),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_FILE);

  ligma_image_signals[GUIDE_ADDED] =
    g_signal_new ("guide-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, guide_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_GUIDE);

  ligma_image_signals[GUIDE_REMOVED] =
    g_signal_new ("guide-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, guide_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_GUIDE);

  ligma_image_signals[GUIDE_MOVED] =
    g_signal_new ("guide-moved",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, guide_moved),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_GUIDE);

  ligma_image_signals[SAMPLE_POINT_ADDED] =
    g_signal_new ("sample-point-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, sample_point_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_SAMPLE_POINT);

  ligma_image_signals[SAMPLE_POINT_REMOVED] =
    g_signal_new ("sample-point-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, sample_point_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_SAMPLE_POINT);

  ligma_image_signals[SAMPLE_POINT_MOVED] =
    g_signal_new ("sample-point-moved",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, sample_point_moved),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_SAMPLE_POINT);

  ligma_image_signals[PARASITE_ATTACHED] =
    g_signal_new ("parasite-attached",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, parasite_attached),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  ligma_image_signals[PARASITE_DETACHED] =
    g_signal_new ("parasite-detached",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, parasite_detached),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  ligma_image_signals[COLORMAP_CHANGED] =
    g_signal_new ("colormap-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, colormap_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  ligma_image_signals[UNDO_EVENT] =
    g_signal_new ("undo-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, undo_event),
                  NULL, NULL,
                  ligma_marshal_VOID__ENUM_OBJECT,
                  G_TYPE_NONE, 2,
                  LIGMA_TYPE_UNDO_EVENT,
                  LIGMA_TYPE_UNDO);

  ligma_image_signals[LAYER_SETS_CHANGED] =
    g_signal_new ("layer-sets-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, layer_sets_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_image_signals[CHANNEL_SETS_CHANGED] =
    g_signal_new ("channel-sets-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, channel_sets_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_image_signals[VECTORS_SETS_CHANGED] =
    g_signal_new ("vectors-sets-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaImageClass, vectors_sets_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed           = ligma_image_constructed;
  object_class->set_property          = ligma_image_set_property;
  object_class->get_property          = ligma_image_get_property;
  object_class->dispose               = ligma_image_dispose;
  object_class->finalize              = ligma_image_finalize;

  ligma_object_class->name_changed     = ligma_image_name_changed;
  ligma_object_class->get_memsize      = ligma_image_get_memsize;

  viewable_class->default_icon_name   = "ligma-image";
  viewable_class->get_size            = ligma_image_get_size;
  viewable_class->size_changed        = ligma_image_size_changed;
  viewable_class->get_preview_size    = ligma_image_get_preview_size;
  viewable_class->get_popup_size      = ligma_image_get_popup_size;
  viewable_class->get_new_preview     = ligma_image_get_new_preview;
  viewable_class->get_new_pixbuf      = ligma_image_get_new_pixbuf;
  viewable_class->get_description     = ligma_image_get_description;

  klass->mode_changed                 = ligma_image_real_mode_changed;
  klass->precision_changed            = ligma_image_real_precision_changed;
  klass->alpha_changed                = NULL;
  klass->floating_selection_changed   = NULL;
  klass->selected_layers_changed      = NULL;
  klass->selected_channels_changed    = NULL;
  klass->selected_vectors_changed     = NULL;
  klass->component_visibility_changed = NULL;
  klass->component_active_changed     = NULL;
  klass->mask_changed                 = NULL;
  klass->resolution_changed           = ligma_image_real_resolution_changed;
  klass->size_changed_detailed        = ligma_image_real_size_changed_detailed;
  klass->unit_changed                 = ligma_image_real_unit_changed;
  klass->quick_mask_changed           = NULL;
  klass->selection_invalidate         = NULL;

  klass->clean                        = NULL;
  klass->dirty                        = NULL;
  klass->saving                       = NULL;
  klass->saved                        = NULL;
  klass->exported                     = NULL;
  klass->guide_added                  = NULL;
  klass->guide_removed                = NULL;
  klass->guide_moved                  = NULL;
  klass->sample_point_added           = NULL;
  klass->sample_point_removed         = NULL;
  klass->sample_point_moved           = NULL;
  klass->parasite_attached            = NULL;
  klass->parasite_detached            = NULL;
  klass->colormap_changed             = ligma_image_real_colormap_changed;
  klass->undo_event                   = NULL;

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma", NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_int ("id", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     LIGMA_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_WIDTH,
                                   g_param_spec_int ("width", NULL, NULL,
                                                     1, LIGMA_MAX_IMAGE_SIZE, 1,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", NULL, NULL,
                                                     1, LIGMA_MAX_IMAGE_SIZE, 1,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_BASE_TYPE,
                                   g_param_spec_enum ("base-type", NULL, NULL,
                                                      LIGMA_TYPE_IMAGE_BASE_TYPE,
                                                      LIGMA_RGB,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PRECISION,
                                   g_param_spec_enum ("precision", NULL, NULL,
                                                      LIGMA_TYPE_PRECISION,
                                                      LIGMA_PRECISION_U8_NON_LINEAR,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_METADATA,
                                   g_param_spec_object ("metadata", NULL, NULL,
                                                        GEXIV2_TYPE_METADATA,
                                                        LIGMA_PARAM_READABLE));

  g_object_class_override_property (object_class, PROP_BUFFER, "buffer");

  g_object_class_install_property (object_class, PROP_SYMMETRY,
                                   g_param_spec_gtype ("symmetry",
                                                       NULL, _("Symmetry"),
                                                       LIGMA_TYPE_SYMMETRY,
                                                       LIGMA_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT));
}

static void
ligma_color_managed_iface_init (LigmaColorManagedInterface *iface)
{
  iface->get_icc_profile            = ligma_image_color_managed_get_icc_profile;
  iface->get_color_profile          = ligma_image_color_managed_get_color_profile;
  iface->profile_changed            = ligma_image_color_managed_profile_changed;
  iface->get_simulation_profile     = ligma_image_color_managed_get_simulation_profile;
  iface->simulation_profile_changed = ligma_image_color_managed_simulation_profile_changed;
  iface->get_simulation_intent      = ligma_image_color_managed_get_simulation_intent;
  iface->simulation_intent_changed  = ligma_image_color_managed_simulation_intent_changed;
  iface->get_simulation_bpc         = ligma_image_color_managed_get_simulation_bpc;
  iface->simulation_bpc_changed     = ligma_image_color_managed_simulation_bpc_changed;
}

static void
ligma_projectable_iface_init (LigmaProjectableInterface *iface)
{
  iface->flush              = ligma_image_projectable_flush;
  iface->get_image          = ligma_image_get_image;
  iface->get_format         = ligma_image_get_proj_format;
  iface->get_bounding_box   = ligma_image_get_bounding_box;
  iface->get_graph          = ligma_image_get_graph;
  iface->invalidate_preview = (void (*) (LigmaProjectable*)) ligma_viewable_invalidate_preview;
}

static void
ligma_pickable_iface_init (LigmaPickableInterface *iface)
{
  iface->flush                 = ligma_image_pickable_flush;
  iface->get_image             = (LigmaImage  * (*) (LigmaPickable *pickable)) ligma_image_get_image;
  iface->get_format            = (const Babl * (*) (LigmaPickable *pickable)) ligma_image_get_proj_format;
  iface->get_format_with_alpha = (const Babl * (*) (LigmaPickable *pickable)) ligma_image_get_proj_format;
  iface->get_buffer            = ligma_image_get_buffer;
  iface->get_pixel_at          = ligma_image_get_pixel_at;
  iface->get_opacity_at        = ligma_image_get_opacity_at;
  iface->get_pixel_average     = ligma_image_get_pixel_average;
  iface->pixel_to_srgb         = ligma_image_pixel_to_srgb;
  iface->srgb_to_pixel         = ligma_image_srgb_to_pixel;
}

static void
ligma_image_init (LigmaImage *image)
{
  LigmaImagePrivate *private = ligma_image_get_instance_private (image);
  gint              i;

  image->priv = private;

  private->ID                  = 0;

  private->load_proc           = NULL;
  private->save_proc           = NULL;

  private->width               = 0;
  private->height              = 0;
  private->xresolution         = 1.0;
  private->yresolution         = 1.0;
  private->resolution_set      = FALSE;
  private->resolution_unit     = LIGMA_UNIT_INCH;
  private->base_type           = LIGMA_RGB;
  private->precision           = LIGMA_PRECISION_U8_NON_LINEAR;
  private->new_layer_mode      = -1;

  private->show_all            = 0;
  private->bounding_box.x      = 0;
  private->bounding_box.y      = 0;
  private->bounding_box.width  = 0;
  private->bounding_box.height = 0;
  private->pickable_buffer     = NULL;

  private->palette             = NULL;

  private->metadata            = NULL;

  private->simulation_profile  = NULL;
  private->simulation_intent   = LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
  private->simulation_bpc      = FALSE;

  private->dirty               = 1;
  private->dirty_time          = 0;
  private->undo_freeze_count   = 0;

  private->export_dirty        = 1;

  private->instance_count      = 0;
  private->disp_count          = 0;

  private->tattoo_state        = 0;

  private->projection          = ligma_projection_new (LIGMA_PROJECTABLE (image));

  private->symmetries          = NULL;
  private->active_symmetry     = NULL;

  private->guides              = NULL;
  private->grid                = NULL;
  private->sample_points       = NULL;

  private->layers              = ligma_item_tree_new (image,
                                                     LIGMA_TYPE_LAYER_STACK,
                                                     LIGMA_TYPE_LAYER);
  private->channels            = ligma_item_tree_new (image,
                                                     LIGMA_TYPE_DRAWABLE_STACK,
                                                     LIGMA_TYPE_CHANNEL);
  private->vectors             = ligma_item_tree_new (image,
                                                     LIGMA_TYPE_ITEM_STACK,
                                                     LIGMA_TYPE_VECTORS);
  private->layer_stack         = NULL;

  private->stored_layer_sets   = NULL;
  private->stored_channel_sets = NULL;
  private->stored_vectors_sets = NULL;

  g_signal_connect (private->projection, "notify::buffer",
                    G_CALLBACK (ligma_image_projection_buffer_notify),
                    image);

  g_signal_connect (private->layers, "notify::selected-items",
                    G_CALLBACK (ligma_image_selected_layers_notify),
                    image);
  g_signal_connect (private->channels, "notify::selected-items",
                    G_CALLBACK (ligma_image_selected_channels_notify),
                    image);
  g_signal_connect (private->vectors, "notify::selected-items",
                    G_CALLBACK (ligma_image_selected_vectors_notify),
                    image);

  g_signal_connect_swapped (private->layers->container, "update",
                            G_CALLBACK (ligma_image_invalidate),
                            image);

  private->layer_offset_x_handler =
    ligma_container_add_handler (private->layers->container, "notify::offset-x",
                                G_CALLBACK (ligma_image_layer_offset_changed),
                                image);
  private->layer_offset_y_handler =
    ligma_container_add_handler (private->layers->container, "notify::offset-y",
                                G_CALLBACK (ligma_image_layer_offset_changed),
                                image);
  private->layer_bounding_box_handler =
    ligma_container_add_handler (private->layers->container, "bounding-box-changed",
                                G_CALLBACK (ligma_image_layer_bounding_box_changed),
                                image);
  private->layer_alpha_handler =
    ligma_container_add_handler (private->layers->container, "alpha-changed",
                                G_CALLBACK (ligma_image_layer_alpha_changed),
                                image);

  g_signal_connect (private->layers->container, "add",
                    G_CALLBACK (ligma_image_layers_changed),
                    image);
  g_signal_connect (private->layers->container, "remove",
                    G_CALLBACK (ligma_image_layers_changed),
                    image);

  g_signal_connect_swapped (private->channels->container, "update",
                            G_CALLBACK (ligma_image_invalidate),
                            image);

  private->channel_name_changed_handler =
    ligma_container_add_handler (private->channels->container, "name-changed",
                                G_CALLBACK (ligma_image_channel_name_changed),
                                image);
  private->channel_color_changed_handler =
    ligma_container_add_handler (private->channels->container, "color-changed",
                                G_CALLBACK (ligma_image_channel_color_changed),
                                image);

  g_signal_connect (private->channels->container, "add",
                    G_CALLBACK (ligma_image_channel_add),
                    image);
  g_signal_connect (private->channels->container, "remove",
                    G_CALLBACK (ligma_image_channel_remove),
                    image);

  private->floating_sel        = NULL;
  private->selection_mask      = NULL;

  private->parasites           = ligma_parasite_list_new ();

  for (i = 0; i < MAX_CHANNELS; i++)
    {
      private->visible[i] = TRUE;
      private->active[i]  = TRUE;
    }

  private->quick_mask_state    = FALSE;
  private->quick_mask_inverted = FALSE;
  ligma_rgba_set (&private->quick_mask_color, 1.0, 0.0, 0.0, 0.5);

  private->undo_stack          = ligma_undo_stack_new (image);
  private->redo_stack          = ligma_undo_stack_new (image);
  private->group_count         = 0;
  private->pushing_undo_group  = LIGMA_UNDO_GROUP_NONE;

  private->flush_accum.alpha_changed              = FALSE;
  private->flush_accum.mask_changed               = FALSE;
  private->flush_accum.floating_selection_changed = FALSE;
  private->flush_accum.preview_invalidated        = FALSE;
}

static void
ligma_image_constructed (GObject *object)
{
  LigmaImage        *image   = LIGMA_IMAGE (object);
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);
  LigmaChannel      *selection;
  LigmaCoreConfig   *config;
  LigmaTemplate     *template;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_LIGMA (image->ligma));

  config = image->ligma->config;

  private->ID = ligma_id_table_insert (image->ligma->image_table, image);

  template = config->default_image;

  private->xresolution     = ligma_template_get_resolution_x (template);
  private->yresolution     = ligma_template_get_resolution_y (template);
  private->resolution_unit = ligma_template_get_resolution_unit (template);

  private->grid = ligma_config_duplicate (LIGMA_CONFIG (config->default_grid));

  private->quick_mask_color = config->quick_mask_color;

  ligma_image_update_bounding_box (image);

  if (private->base_type == LIGMA_INDEXED)
    ligma_image_colormap_init (image);

  selection = ligma_selection_new (image,
                                  ligma_image_get_width  (image),
                                  ligma_image_get_height (image));
  ligma_image_take_mask (image, selection);

  g_signal_connect_object (config, "notify::transparency-type",
                           G_CALLBACK (ligma_item_stack_invalidate_previews),
                           private->layers->container, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::transparency-size",
                           G_CALLBACK (ligma_item_stack_invalidate_previews),
                           private->layers->container, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::transparency-custom-color1",
                           G_CALLBACK (ligma_item_stack_invalidate_previews),
                           private->layers->container, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::transparency-custom-color2",
                           G_CALLBACK (ligma_item_stack_invalidate_previews),
                           private->layers->container, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::layer-previews",
                           G_CALLBACK (ligma_viewable_size_changed),
                           image, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::group-layer-previews",
                           G_CALLBACK (ligma_viewable_size_changed),
                           image, G_CONNECT_SWAPPED);

  ligma_container_add (image->ligma->images, LIGMA_OBJECT (image));
}

static void
ligma_image_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  LigmaImage        *image   = LIGMA_IMAGE (object);
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);

  switch (property_id)
    {
    case PROP_LIGMA:
      image->ligma = g_value_get_object (value);
      break;

    case PROP_WIDTH:
      private->width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      private->height = g_value_get_int (value);
      break;

    case PROP_BASE_TYPE:
      private->base_type = g_value_get_enum (value);
      _ligma_image_free_color_transforms (image);
      break;

    case PROP_PRECISION:
      private->precision = g_value_get_enum (value);
      _ligma_image_free_color_transforms (image);
      break;

    case PROP_SYMMETRY:
      {
        GList *iter;
        GType  type = g_value_get_gtype (value);

        if (private->active_symmetry)
          g_object_set (private->active_symmetry,
                        "active", FALSE,
                        NULL);
        private->active_symmetry = NULL;

        for (iter = private->symmetries; iter; iter = g_list_next (iter))
          {
            LigmaSymmetry *sym = iter->data;

            if (type == G_TYPE_FROM_INSTANCE (sym))
              private->active_symmetry = iter->data;
          }

        if (! private->active_symmetry &&
            g_type_is_a (type, LIGMA_TYPE_SYMMETRY))
          {
            LigmaSymmetry *sym = ligma_image_symmetry_new (image, type);

            ligma_image_symmetry_add (image, sym);
            g_object_unref (sym);

            private->active_symmetry = sym;
          }

        if (private->active_symmetry)
          g_object_set (private->active_symmetry,
                        "active", TRUE,
                        NULL);
      }
      break;

    case PROP_ID:
    case PROP_METADATA:
    case PROP_BUFFER:
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_image_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  LigmaImage        *image   = LIGMA_IMAGE (object);
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);

  switch (property_id)
    {
    case PROP_LIGMA:
      g_value_set_object (value, image->ligma);
      break;
    case PROP_ID:
      g_value_set_int (value, private->ID);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, private->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, private->height);
      break;
    case PROP_BASE_TYPE:
      g_value_set_enum (value, private->base_type);
      break;
    case PROP_PRECISION:
      g_value_set_enum (value, private->precision);
      break;
    case PROP_METADATA:
      g_value_set_object (value, ligma_image_get_metadata (image));
      break;
    case PROP_BUFFER:
      g_value_set_object (value, ligma_image_get_buffer (LIGMA_PICKABLE (image)));
      break;
    case PROP_SYMMETRY:
      g_value_set_gtype (value,
                         private->active_symmetry ?
                         G_TYPE_FROM_INSTANCE (private->active_symmetry) :
                         G_TYPE_NONE);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_image_dispose (GObject *object)
{
  LigmaImage        *image   = LIGMA_IMAGE (object);
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (private->palette)
    ligma_image_colormap_dispose (image);

  ligma_image_undo_free (image);

  g_list_free_full (private->stored_layer_sets, g_object_unref);
  g_list_free_full (private->stored_channel_sets, g_object_unref);
  g_list_free_full (private->stored_vectors_sets, g_object_unref);

  g_signal_handlers_disconnect_by_func (private->layers->container,
                                        ligma_image_invalidate,
                                        image);

  ligma_container_remove_handler (private->layers->container,
                                 private->layer_offset_x_handler);
  ligma_container_remove_handler (private->layers->container,
                                 private->layer_offset_y_handler);
  ligma_container_remove_handler (private->layers->container,
                                 private->layer_bounding_box_handler);
  ligma_container_remove_handler (private->layers->container,
                                 private->layer_alpha_handler);

  g_signal_handlers_disconnect_by_func (private->layers->container,
                                        ligma_image_layers_changed,
                                        image);

  g_signal_handlers_disconnect_by_func (private->channels->container,
                                        ligma_image_invalidate,
                                        image);

  ligma_container_remove_handler (private->channels->container,
                                 private->channel_name_changed_handler);
  ligma_container_remove_handler (private->channels->container,
                                 private->channel_color_changed_handler);

  g_signal_handlers_disconnect_by_func (private->channels->container,
                                        ligma_image_channel_add,
                                        image);
  g_signal_handlers_disconnect_by_func (private->channels->container,
                                        ligma_image_channel_remove,
                                        image);

  g_object_run_dispose (G_OBJECT (private->layers));
  g_object_run_dispose (G_OBJECT (private->channels));
  g_object_run_dispose (G_OBJECT (private->vectors));

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_image_finalize (GObject *object)
{
  LigmaImage        *image   = LIGMA_IMAGE (object);
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);

  g_clear_object (&private->projection);
  g_clear_object (&private->graph);
  private->visible_mask = NULL;

  if (private->palette)
    ligma_image_colormap_free (image);

  _ligma_image_free_color_profile (image);

  g_clear_object (&private->pickable_buffer);
  g_clear_object (&private->metadata);
  g_clear_object (&private->file);
  g_clear_object (&private->imported_file);
  g_clear_object (&private->exported_file);
  g_clear_object (&private->save_a_copy_file);
  g_clear_object (&private->untitled_file);
  g_clear_object (&private->layers);
  g_clear_object (&private->channels);
  g_clear_object (&private->vectors);

  if (private->layer_stack)
    {
      g_slist_free_full (private->layer_stack,
                         (GDestroyNotify) g_list_free);
      private->layer_stack = NULL;
    }

  g_clear_object (&private->selection_mask);
  g_clear_object (&private->parasites);

  if (private->guides)
    {
      g_list_free_full (private->guides, (GDestroyNotify) g_object_unref);
      private->guides = NULL;
    }

  if (private->symmetries)
    {
      g_list_free_full (private->symmetries, g_object_unref);
      private->symmetries = NULL;
    }

  g_clear_object (&private->grid);

  if (private->sample_points)
    {
      g_list_free_full (private->sample_points,
                        (GDestroyNotify) g_object_unref);
      private->sample_points = NULL;
    }

  g_clear_object (&private->undo_stack);
  g_clear_object (&private->redo_stack);

  if (image->ligma && image->ligma->image_table)
    {
      ligma_id_table_remove (image->ligma->image_table, private->ID);
      image->ligma = NULL;
    }

  g_clear_pointer (&private->display_name, g_free);
  g_clear_pointer (&private->display_path, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);

  /* Don't just free the list silently. It is supposed to be empty.
   * Instead just assert this fact to warn on improper management of
   * hidden items.
   */
  if (private->hidden_items != NULL)
    {
      g_warning ("%s: the hidden items list should be empty (%d items remaining).",
                 G_STRFUNC, g_list_length (private->hidden_items));
      g_list_free (private->hidden_items);
    }
}

static void
ligma_image_name_changed (LigmaObject *object)
{
  LigmaImage        *image   = LIGMA_IMAGE (object);
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);
  const gchar      *name;

  if (LIGMA_OBJECT_CLASS (parent_class)->name_changed)
    LIGMA_OBJECT_CLASS (parent_class)->name_changed (object);

  g_clear_pointer (&private->display_name, g_free);
  g_clear_pointer (&private->display_path, g_free);

  /* We never want the empty string as a name, so change empty strings
   * to NULL strings (without emitting the "name-changed" signal
   * again)
   */
  name = ligma_object_get_name (object);
  if (name && strlen (name) == 0)
    {
      ligma_object_name_free (object);
      name = NULL;
    }

  g_clear_object (&private->file);

  if (name)
    private->file = g_file_new_for_uri (name);
}

static gint64
ligma_image_get_memsize (LigmaObject *object,
                        gint64     *gui_size)
{
  LigmaImage        *image   = LIGMA_IMAGE (object);
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);
  gint64            memsize = 0;

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (private->palette),
                                      gui_size);

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (private->projection),
                                      gui_size);

  memsize += ligma_g_list_get_memsize (ligma_image_get_guides (image),
                                      sizeof (LigmaGuide));

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (private->grid), gui_size);

  memsize += ligma_g_list_get_memsize (ligma_image_get_sample_points (image),
                                      sizeof (LigmaSamplePoint));

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (private->layers),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (private->channels),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (private->vectors),
                                      gui_size);

  memsize += ligma_g_slist_get_memsize (private->layer_stack, 0);

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (private->selection_mask),
                                      gui_size);

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (private->parasites),
                                      gui_size);

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (private->undo_stack),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (private->redo_stack),
                                      gui_size);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
ligma_image_get_size (LigmaViewable *viewable,
                     gint         *width,
                     gint         *height)
{
  LigmaImage *image = LIGMA_IMAGE (viewable);

  *width  = ligma_image_get_width  (image);
  *height = ligma_image_get_height (image);

  return TRUE;
}

static void
ligma_image_size_changed (LigmaViewable *viewable)
{
  LigmaImage *image = LIGMA_IMAGE (viewable);
  GList     *all_items;
  GList     *list;

  if (LIGMA_VIEWABLE_CLASS (parent_class)->size_changed)
    LIGMA_VIEWABLE_CLASS (parent_class)->size_changed (viewable);

  all_items = ligma_image_get_layer_list (image);
  for (list = all_items; list; list = g_list_next (list))
    {
      LigmaLayerMask *mask = ligma_layer_get_mask (LIGMA_LAYER (list->data));

      ligma_viewable_size_changed (LIGMA_VIEWABLE (list->data));

      if (mask)
        ligma_viewable_size_changed (LIGMA_VIEWABLE (mask));
    }
  g_list_free (all_items);

  all_items = ligma_image_get_channel_list (image);
  g_list_free_full (all_items, (GDestroyNotify) ligma_viewable_size_changed);

  all_items = ligma_image_get_vectors_list (image);
  g_list_free_full (all_items, (GDestroyNotify) ligma_viewable_size_changed);

  ligma_viewable_size_changed (LIGMA_VIEWABLE (ligma_image_get_mask (image)));

  ligma_image_metadata_update_pixel_size (image);

  g_clear_object (&LIGMA_IMAGE_GET_PRIVATE (image)->pickable_buffer);

  ligma_image_update_bounding_box (image);
}

static gchar *
ligma_image_get_description (LigmaViewable  *viewable,
                            gchar        **tooltip)
{
  LigmaImage *image = LIGMA_IMAGE (viewable);

  if (tooltip)
    *tooltip = g_strdup (ligma_image_get_display_path (image));

  return g_strdup_printf ("%s-%d",
                          ligma_image_get_display_name (image),
                          ligma_image_get_id (image));
}

static void
ligma_image_real_mode_changed (LigmaImage *image)
{
  ligma_projectable_structure_changed (LIGMA_PROJECTABLE (image));
}

static void
ligma_image_real_precision_changed (LigmaImage *image)
{
  ligma_image_metadata_update_bits_per_sample (image);

  ligma_projectable_structure_changed (LIGMA_PROJECTABLE (image));
}

static void
ligma_image_real_resolution_changed (LigmaImage *image)
{
  ligma_image_metadata_update_resolution (image);
}

static void
ligma_image_real_size_changed_detailed (LigmaImage *image,
                                       gint       previous_origin_x,
                                       gint       previous_origin_y,
                                       gint       previous_width,
                                       gint       previous_height)
{
  /* Whenever LigmaImage::size-changed-detailed is emitted, so is
   * LigmaViewable::size-changed. Clients choose what signal to listen
   * to depending on how much info they need.
   */
  ligma_viewable_size_changed (LIGMA_VIEWABLE (image));
}

static void
ligma_image_real_unit_changed (LigmaImage *image)
{
  ligma_image_metadata_update_resolution (image);
}

static void
ligma_image_real_colormap_changed (LigmaImage *image,
                                  gint       color_index)
{
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);

  ligma_image_colormap_update_formats (image);

  if (ligma_image_get_base_type (image) == LIGMA_INDEXED)
    {
      /* A colormap alteration affects the whole image */
      ligma_image_invalidate_all (image);

      ligma_item_stack_invalidate_previews (LIGMA_ITEM_STACK (private->layers->container));
    }
}

static const guint8 *
ligma_image_color_managed_get_icc_profile (LigmaColorManaged *managed,
                                          gsize            *len)
{
  return ligma_image_get_icc_profile (LIGMA_IMAGE (managed), len);
}

static LigmaColorProfile *
ligma_image_color_managed_get_color_profile (LigmaColorManaged *managed)
{
  LigmaImage        *image = LIGMA_IMAGE (managed);
  LigmaColorProfile *profile;

  profile = ligma_image_get_color_profile (image);

  if (! profile)
    profile = ligma_image_get_builtin_color_profile (image);

  return profile;
}

static void
ligma_image_color_managed_profile_changed (LigmaColorManaged *managed)
{
  LigmaImage     *image  = LIGMA_IMAGE (managed);
  LigmaItemStack *layers = LIGMA_ITEM_STACK (ligma_image_get_layers (image));

  ligma_image_metadata_update_colorspace (image);

  ligma_projectable_structure_changed (LIGMA_PROJECTABLE (image));
  ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (image));
  ligma_item_stack_profile_changed (layers);
}

static LigmaColorProfile *
ligma_image_color_managed_get_simulation_profile (LigmaColorManaged *managed)
{
  LigmaImage        *image = LIGMA_IMAGE (managed);
  LigmaColorProfile *profile;

  profile = ligma_image_get_simulation_profile (image);

  return profile;
}

static void
ligma_image_color_managed_simulation_profile_changed (LigmaColorManaged *managed)
{
  LigmaImage *image = LIGMA_IMAGE (managed);

  ligma_projectable_structure_changed (LIGMA_PROJECTABLE (image));
  ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (image));
}

static LigmaColorRenderingIntent
ligma_image_color_managed_get_simulation_intent (LigmaColorManaged *managed)
{
  LigmaImage *image = LIGMA_IMAGE (managed);

  return ligma_image_get_simulation_intent (image);
}

static void
ligma_image_color_managed_simulation_intent_changed (LigmaColorManaged *managed)
{
  LigmaImage *image = LIGMA_IMAGE (managed);

  ligma_projectable_structure_changed (LIGMA_PROJECTABLE (image));
  ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (image));
}

static gboolean
ligma_image_color_managed_get_simulation_bpc (LigmaColorManaged *managed)
{
  LigmaImage *image = LIGMA_IMAGE (managed);

  return ligma_image_get_simulation_bpc (image);
}

static void
ligma_image_color_managed_simulation_bpc_changed (LigmaColorManaged *managed)
{
  LigmaImage *image = LIGMA_IMAGE (managed);

  ligma_projectable_structure_changed (LIGMA_PROJECTABLE (image));
  ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (image));
}

static void
ligma_image_projectable_flush (LigmaProjectable *projectable,
                              gboolean         invalidate_preview)
{
  LigmaImage        *image   = LIGMA_IMAGE (projectable);
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (private->flush_accum.alpha_changed)
    {
      ligma_image_alpha_changed (image);
      private->flush_accum.alpha_changed = FALSE;
    }

  if (private->flush_accum.mask_changed)
    {
      ligma_image_mask_changed (image);
      private->flush_accum.mask_changed = FALSE;
    }

  if (private->flush_accum.floating_selection_changed)
    {
      ligma_image_floating_selection_changed (image);
      private->flush_accum.floating_selection_changed = FALSE;
    }

  if (private->flush_accum.preview_invalidated)
    {
      /*  don't invalidate the preview here, the projection does this when
       *  it is completely constructed.
       */
      private->flush_accum.preview_invalidated = FALSE;
    }
}

static LigmaImage *
ligma_image_get_image (LigmaProjectable *projectable)
{
  return LIGMA_IMAGE (projectable);
}

static const Babl *
ligma_image_get_proj_format (LigmaProjectable *projectable)
{
  LigmaImage        *image   = LIGMA_IMAGE (projectable);
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);

  switch (private->base_type)
    {
    case LIGMA_RGB:
    case LIGMA_INDEXED:
      return ligma_image_get_format (image, LIGMA_RGB,
                                    ligma_image_get_precision (image), TRUE,
                                    ligma_image_get_layer_space (image));

    case LIGMA_GRAY:
      return ligma_image_get_format (image, LIGMA_GRAY,
                                    ligma_image_get_precision (image), TRUE,
                                    ligma_image_get_layer_space (image));
    }

  g_return_val_if_reached (NULL);
}

static void
ligma_image_pickable_flush (LigmaPickable *pickable)
{
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (pickable);

  return ligma_pickable_flush (LIGMA_PICKABLE (private->projection));
}

static GeglBuffer *
ligma_image_get_buffer (LigmaPickable *pickable)
{
  LigmaImage        *image   = LIGMA_IMAGE (pickable);
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (! private->pickable_buffer)
    {
      GeglBuffer *buffer;

      buffer = ligma_pickable_get_buffer (LIGMA_PICKABLE (private->projection));

      if (! private->show_all)
        {
          private->pickable_buffer = g_object_ref (buffer);
        }
      else
        {
          private->pickable_buffer = gegl_buffer_create_sub_buffer (
            buffer,
            GEGL_RECTANGLE (0, 0,
                            ligma_image_get_width  (image),
                            ligma_image_get_height (image)));
        }
    }

  return private->pickable_buffer;
}

static gboolean
ligma_image_get_pixel_at (LigmaPickable *pickable,
                         gint          x,
                         gint          y,
                         const Babl   *format,
                         gpointer      pixel)
{
  LigmaImage        *image   = LIGMA_IMAGE (pickable);
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (x >= 0                            &&
      y >= 0                            &&
      x < ligma_image_get_width  (image) &&
      y < ligma_image_get_height (image))
    {
      return ligma_pickable_get_pixel_at (LIGMA_PICKABLE (private->projection),
                                         x, y, format, pixel);
    }

  return FALSE;
}

static gdouble
ligma_image_get_opacity_at (LigmaPickable *pickable,
                           gint          x,
                           gint          y)
{
  LigmaImage        *image   = LIGMA_IMAGE (pickable);
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (x >= 0                            &&
      y >= 0                            &&
      x < ligma_image_get_width  (image) &&
      y < ligma_image_get_height (image))
    {
      return ligma_pickable_get_opacity_at (LIGMA_PICKABLE (private->projection),
                                           x, y);
    }

  return FALSE;
}

static void
ligma_image_get_pixel_average (LigmaPickable        *pickable,
                              const GeglRectangle *rect,
                              const Babl          *format,
                              gpointer             pixel)
{
  GeglBuffer *buffer = ligma_pickable_get_buffer (pickable);

  return ligma_gegl_average_color (buffer, rect, TRUE, GEGL_ABYSS_NONE, format,
                                  pixel);
}

static void
ligma_image_pixel_to_srgb (LigmaPickable *pickable,
                          const Babl   *format,
                          gpointer      pixel,
                          LigmaRGB      *color)
{
  ligma_image_color_profile_pixel_to_srgb (LIGMA_IMAGE (pickable),
                                          format, pixel, color);
}

static void
ligma_image_srgb_to_pixel (LigmaPickable  *pickable,
                          const LigmaRGB *color,
                          const Babl    *format,
                          gpointer       pixel)
{
  ligma_image_color_profile_srgb_to_pixel (LIGMA_IMAGE (pickable),
                                          color, format, pixel);
}

static GeglRectangle
ligma_image_get_bounding_box (LigmaProjectable *projectable)
{
  LigmaImage *image = LIGMA_IMAGE (projectable);

  return LIGMA_IMAGE_GET_PRIVATE (image)->bounding_box;
}

static GeglNode *
ligma_image_get_graph (LigmaProjectable *projectable)
{
  LigmaImage         *image   = LIGMA_IMAGE (projectable);
  LigmaImagePrivate  *private = LIGMA_IMAGE_GET_PRIVATE (image);
  GeglNode          *layers_node;
  GeglNode          *channels_node;
  GeglNode          *output;
  LigmaComponentMask  mask;

  if (private->graph)
    return private->graph;

  private->graph = gegl_node_new ();

  layers_node =
    ligma_filter_stack_get_graph (LIGMA_FILTER_STACK (private->layers->container));

  gegl_node_add_child (private->graph, layers_node);

  mask = ~ligma_image_get_visible_mask (image) & LIGMA_COMPONENT_MASK_ALL;

  private->visible_mask =
    gegl_node_new_child (private->graph,
                         "operation", "ligma:mask-components",
                         "mask",      mask,
                         "alpha",     1.0,
                         NULL);

  gegl_node_connect_to (layers_node,           "output",
                        private->visible_mask, "input");

  channels_node =
    ligma_filter_stack_get_graph (LIGMA_FILTER_STACK (private->channels->container));

  gegl_node_add_child (private->graph, channels_node);

  gegl_node_connect_to (private->visible_mask, "output",
                        channels_node,         "input");

  output = gegl_node_get_output_proxy (private->graph, "output");

  gegl_node_connect_to (channels_node, "output",
                        output,        "input");

  return private->graph;
}

static void
ligma_image_projection_buffer_notify (LigmaProjection   *projection,
                                     const GParamSpec *pspec,
                                     LigmaImage        *image)
{
  g_clear_object (&LIGMA_IMAGE_GET_PRIVATE (image)->pickable_buffer);
}

static void
ligma_image_mask_update (LigmaDrawable *drawable,
                        gint          x,
                        gint          y,
                        gint          width,
                        gint          height,
                        LigmaImage    *image)
{
  LIGMA_IMAGE_GET_PRIVATE (image)->flush_accum.mask_changed = TRUE;
}

static void
ligma_image_layers_changed (LigmaContainer *container,
                           LigmaChannel   *channel,
                           LigmaImage     *image)
{
  ligma_image_update_bounding_box (image);
}

static void
ligma_image_layer_offset_changed (LigmaDrawable     *drawable,
                                 const GParamSpec *pspec,
                                 LigmaImage        *image)
{
  ligma_image_update_bounding_box (image);
}

static void
ligma_image_layer_bounding_box_changed (LigmaDrawable *drawable,
                                       LigmaImage    *image)
{
  ligma_image_update_bounding_box (image);
}

static void
ligma_image_layer_alpha_changed (LigmaDrawable *drawable,
                                LigmaImage    *image)
{
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (ligma_container_get_n_children (private->layers->container) == 1)
    private->flush_accum.alpha_changed = TRUE;
}

static void
ligma_image_channel_add (LigmaContainer *container,
                        LigmaChannel   *channel,
                        LigmaImage     *image)
{
  if (! strcmp (LIGMA_IMAGE_QUICK_MASK_NAME,
                ligma_object_get_name (channel)))
    {
      ligma_image_set_quick_mask_state (image, TRUE);
    }
}

static void
ligma_image_channel_remove (LigmaContainer *container,
                           LigmaChannel   *channel,
                           LigmaImage     *image)
{
  if (! strcmp (LIGMA_IMAGE_QUICK_MASK_NAME,
                ligma_object_get_name (channel)))
    {
      ligma_image_set_quick_mask_state (image, FALSE);
    }
}

static void
ligma_image_channel_name_changed (LigmaChannel *channel,
                                 LigmaImage   *image)
{
  if (! strcmp (LIGMA_IMAGE_QUICK_MASK_NAME,
                ligma_object_get_name (channel)))
    {
      ligma_image_set_quick_mask_state (image, TRUE);
    }
  else if (ligma_image_get_quick_mask_state (image) &&
           ! ligma_image_get_quick_mask (image))
    {
      ligma_image_set_quick_mask_state (image, FALSE);
    }
}

static void
ligma_image_channel_color_changed (LigmaChannel *channel,
                                  LigmaImage   *image)
{
  if (! strcmp (LIGMA_IMAGE_QUICK_MASK_NAME,
                ligma_object_get_name (channel)))
    {
      LIGMA_IMAGE_GET_PRIVATE (image)->quick_mask_color = channel->color;
    }
}

static void
ligma_image_selected_layers_notify (LigmaItemTree     *tree,
                                   const GParamSpec *pspec,
                                   LigmaImage        *image)
{
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);
  GList            *layers  = ligma_image_get_selected_layers (image);

  if (layers)
    {
      /*  Configure the layer stack to reflect this change  */
      GSList *prev_layers;

      while ((prev_layers = g_slist_find_custom (private->layer_stack, layers,
                                                (GCompareFunc) ligma_image_layer_stack_cmp)))
        {
          g_list_free (prev_layers->data);
          private->layer_stack = g_slist_delete_link (private->layer_stack,
                                                      prev_layers);
        }
      private->layer_stack = g_slist_prepend (private->layer_stack, g_list_copy (layers));
    }

  g_signal_emit (image, ligma_image_signals[SELECTED_LAYERS_CHANGED], 0);

  if (layers && ligma_image_get_selected_channels (image))
    ligma_image_set_selected_channels (image, NULL);
}

static void
ligma_image_selected_channels_notify (LigmaItemTree     *tree,
                                     const GParamSpec *pspec,
                                     LigmaImage        *image)
{
  GList *channels = ligma_image_get_selected_channels (image);

  g_signal_emit (image, ligma_image_signals[SELECTED_CHANNELS_CHANGED], 0);

  if (channels && ligma_image_get_selected_layers (image))
    ligma_image_set_selected_layers (image, NULL);
}

static void
ligma_image_selected_vectors_notify (LigmaItemTree     *tree,
                                    const GParamSpec *pspec,
                                    LigmaImage        *image)
{
  g_signal_emit (image, ligma_image_signals[SELECTED_VECTORS_CHANGED], 0);
}

static void
ligma_image_freeze_bounding_box (LigmaImage *image)
{
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);

  private->bounding_box_freeze_count++;
}

static void
ligma_image_thaw_bounding_box (LigmaImage *image)
{
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);

  private->bounding_box_freeze_count--;

  if (private->bounding_box_freeze_count == 0 &&
      private->bounding_box_update_pending)
    {
      private->bounding_box_update_pending = FALSE;

      ligma_image_update_bounding_box (image);
    }
}

static void
ligma_image_update_bounding_box (LigmaImage *image)
{
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);
  GeglRectangle     bounding_box;

  if (private->bounding_box_freeze_count > 0)
    {
      private->bounding_box_update_pending = TRUE;

      return;
    }

  bounding_box.x      = 0;
  bounding_box.y      = 0;
  bounding_box.width  = ligma_image_get_width  (image);
  bounding_box.height = ligma_image_get_height (image);

  if (private->show_all)
    {
      GList *iter;

      for (iter = ligma_image_get_layer_iter (image);
           iter;
           iter = g_list_next (iter))
        {
          LigmaLayer     *layer = iter->data;
          GeglRectangle  layer_bounding_box;
          gint           offset_x;
          gint           offset_y;

          ligma_item_get_offset (LIGMA_ITEM (layer), &offset_x, &offset_y);

          layer_bounding_box = ligma_drawable_get_bounding_box (
            LIGMA_DRAWABLE (layer));

          layer_bounding_box.x += offset_x;
          layer_bounding_box.y += offset_y;

          gegl_rectangle_bounding_box (&bounding_box,
                                       &bounding_box, &layer_bounding_box);
        }
    }

  if (! gegl_rectangle_equal (&bounding_box, &private->bounding_box))
    {
      private->bounding_box = bounding_box;

      ligma_projectable_bounds_changed (LIGMA_PROJECTABLE (image), 0, 0);
    }
}

static gint
ligma_image_layer_stack_cmp (GList *layers1,
                            GList *layers2)
{
  if (g_list_length (layers1) != g_list_length (layers2))
    {
      /* We don't really need to order lists of layers, and only care
       * about identity.
       */
      return 1;
    }
  else
    {
      GList *iter;

      for (iter = layers1; iter; iter = iter->next)
        {
          if (! g_list_find (layers2, iter->data))
            return 1;
        }
      return 0;
    }
}

/**
 * ligma_image_rec_remove_layer_stack_dups:
 * @image:
 *
 * Recursively remove duplicates from the layer stack. You should not
 * call this directly, call ligma_image_clean_layer_stack() instead.
 */
static void
ligma_image_rec_remove_layer_stack_dups (LigmaImage *image,
                                        GSList    *start)
{
  LigmaImagePrivate *private;
  GSList           *dup_layers;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (start == NULL || start->next == NULL)
    return;

  while ((dup_layers = g_slist_find_custom (start->next, start->data,
                                            (GCompareFunc) ligma_image_layer_stack_cmp)))
    {
      g_list_free (dup_layers->data);
      /* We can safely remove the duplicate then search again because we
       * know that @start is never removed as we search after it.
       */
      private->layer_stack = g_slist_delete_link (private->layer_stack,
                                                  dup_layers);
    }

  ligma_image_rec_remove_layer_stack_dups (image, start->next);
}

/**
 * ligma_image_clean_layer_stack:
 * @image:
 *
 * Remove any duplicate and empty selections in the layer stack.
 */
static void
ligma_image_clean_layer_stack (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  /* First remove all empty layer lists. */
  private->layer_stack = g_slist_remove_all (private->layer_stack, NULL);
  /* Then remove all duplicates. */
  ligma_image_rec_remove_layer_stack_dups (image, private->layer_stack);
}

static void
ligma_image_remove_from_layer_stack (LigmaImage *image,
                                    LigmaLayer *layer)
{
  LigmaImagePrivate *private;
  GSList           *slist;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_LAYER (layer));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  /* Remove layer itself from the MRU layer stack. */
  for (slist = private->layer_stack; slist; slist = slist->next)
    slist->data = g_list_remove (slist->data, layer);

  /*  Also remove all children of a group layer from the layer_stack  */
  if (ligma_viewable_get_children (LIGMA_VIEWABLE (layer)))
    {
      LigmaContainer *stack = ligma_viewable_get_children (LIGMA_VIEWABLE (layer));
      GList         *children;
      GList         *list;

      children = ligma_item_stack_get_item_list (LIGMA_ITEM_STACK (stack));

      for (list = children; list; list = g_list_next (list))
        {
          LigmaLayer *child = list->data;

          for (slist = private->layer_stack; slist; slist = slist->next)
            slist->data = g_list_remove (slist->data, child);
        }

      g_list_free (children);
    }

  ligma_image_clean_layer_stack (image);
}

static gint
ligma_image_selected_is_descendant (LigmaViewable *selected,
                                   LigmaViewable *viewable)
{
  /* Used as a GCompareFunc to g_list_find_custom() in order to know if
   * one of the selected items is a descendant to @viewable.
   */
  if (ligma_viewable_is_ancestor (viewable, selected))
    return 0;
  else
    return 1;
}


/*  public functions  */

LigmaImage *
ligma_image_new (Ligma              *ligma,
                gint               width,
                gint               height,
                LigmaImageBaseType  base_type,
                LigmaPrecision      precision)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (ligma_babl_is_valid (base_type, precision), NULL);

  return g_object_new (LIGMA_TYPE_IMAGE,
                       "ligma",      ligma,
                       "width",     width,
                       "height",    height,
                       "base-type", base_type,
                       "precision", precision,
                       NULL);
}

gint64
ligma_image_estimate_memsize (LigmaImage         *image,
                             LigmaComponentType  component_type,
                             gint               width,
                             gint               height)
{
  GList  *drawables;
  GList  *list;
  gint    current_width;
  gint    current_height;
  gint64  current_size;
  gint64  scalable_size = 0;
  gint64  scaled_size   = 0;
  gint64  new_size;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), 0);

  current_width  = ligma_image_get_width (image);
  current_height = ligma_image_get_height (image);
  current_size   = ligma_object_get_memsize (LIGMA_OBJECT (image), NULL);

  /*  the part of the image's memsize that scales linearly with the image  */
  drawables = ligma_image_item_list_get_list (image,
                                             LIGMA_ITEM_TYPE_LAYERS |
                                             LIGMA_ITEM_TYPE_CHANNELS,
                                             LIGMA_ITEM_SET_ALL);

  ligma_image_item_list_filter (drawables);

  drawables = g_list_prepend (drawables, ligma_image_get_mask (image));

  for (list = drawables; list; list = g_list_next (list))
    {
      LigmaDrawable *drawable = list->data;
      gdouble       drawable_width;
      gdouble       drawable_height;

      drawable_width  = ligma_item_get_width  (LIGMA_ITEM (drawable));
      drawable_height = ligma_item_get_height (LIGMA_ITEM (drawable));

      scalable_size += ligma_drawable_estimate_memsize (drawable,
                                                       ligma_drawable_get_component_type (drawable),
                                                       drawable_width,
                                                       drawable_height);

      scaled_size += ligma_drawable_estimate_memsize (drawable,
                                                     component_type,
                                                     drawable_width * width /
                                                     current_width,
                                                     drawable_height * height /
                                                     current_height);
    }

  g_list_free (drawables);

  scalable_size +=
    ligma_projection_estimate_memsize (ligma_image_get_base_type (image),
                                      ligma_image_get_component_type (image),
                                      ligma_image_get_width (image),
                                      ligma_image_get_height (image));

  scaled_size +=
    ligma_projection_estimate_memsize (ligma_image_get_base_type (image),
                                      component_type,
                                      width, height);

  LIGMA_LOG (IMAGE_SCALE,
            "scalable_size = %"G_GINT64_FORMAT"  scaled_size = %"G_GINT64_FORMAT,
            scalable_size, scaled_size);

  new_size = current_size - scalable_size + scaled_size;

  return new_size;
}

LigmaImageBaseType
ligma_image_get_base_type (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), -1);

  return LIGMA_IMAGE_GET_PRIVATE (image)->base_type;
}

LigmaComponentType
ligma_image_get_component_type (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), -1);

  return ligma_babl_component_type (LIGMA_IMAGE_GET_PRIVATE (image)->precision);
}

LigmaPrecision
ligma_image_get_precision (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), -1);

  return LIGMA_IMAGE_GET_PRIVATE (image)->precision;
}

const Babl *
ligma_image_get_format (LigmaImage         *image,
                       LigmaImageBaseType  base_type,
                       LigmaPrecision      precision,
                       gboolean           with_alpha,
                       const Babl        *space)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  switch (base_type)
    {
    case LIGMA_RGB:
    case LIGMA_GRAY:
      return ligma_babl_format (base_type, precision, with_alpha, space);

    case LIGMA_INDEXED:
      if (precision == LIGMA_PRECISION_U8_NON_LINEAR)
        {
          if (with_alpha)
            return ligma_image_colormap_get_rgba_format (image);
          else
            return ligma_image_colormap_get_rgb_format (image);
        }
    }

  g_return_val_if_reached (NULL);
}

const Babl *
ligma_image_get_layer_space (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->layer_space;
}

const Babl *
ligma_image_get_layer_format (LigmaImage *image,
                             gboolean   with_alpha)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return ligma_image_get_format (image,
                                ligma_image_get_base_type (image),
                                ligma_image_get_precision (image),
                                with_alpha,
                                ligma_image_get_layer_space (image));
}

const Babl *
ligma_image_get_channel_format (LigmaImage *image)
{
  LigmaPrecision precision;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  precision = ligma_image_get_precision (image);

  if (precision == LIGMA_PRECISION_U8_NON_LINEAR)
    return ligma_image_get_format (image, LIGMA_GRAY,
                                  ligma_image_get_precision (image),
                                  FALSE, NULL);

  return ligma_babl_mask_format (precision);
}

const Babl *
ligma_image_get_mask_format (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return ligma_babl_mask_format (ligma_image_get_precision (image));
}

LigmaLayerMode
ligma_image_get_default_new_layer_mode (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), LIGMA_LAYER_MODE_NORMAL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (private->new_layer_mode == -1)
    {
      GList *layers = ligma_image_get_layer_list (image);

      if (layers)
        {
          GList *list;

          for (list = layers; list; list = g_list_next (list))
            {
              LigmaLayer     *layer = list->data;
              LigmaLayerMode  mode  = ligma_layer_get_mode (layer);

              if (! ligma_layer_mode_is_legacy (mode))
                {
                  /*  any non-legacy layer switches the mode to non-legacy
                   */
                  private->new_layer_mode = LIGMA_LAYER_MODE_NORMAL;
                  break;
                }
            }

          /*  only if all layers are legacy, the mode is also legacy
           */
          if (! list)
            private->new_layer_mode = LIGMA_LAYER_MODE_NORMAL_LEGACY;

          g_list_free (layers);
        }
      else
        {
          /*  empty images are never considered legacy
           */
          private->new_layer_mode = LIGMA_LAYER_MODE_NORMAL;
        }
    }

  return private->new_layer_mode;
}

void
ligma_image_unset_default_new_layer_mode (LigmaImage *image)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  LIGMA_IMAGE_GET_PRIVATE (image)->new_layer_mode = -1;
}

gint
ligma_image_get_id (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), -1);

  return LIGMA_IMAGE_GET_PRIVATE (image)->ID;
}

LigmaImage *
ligma_image_get_by_id (Ligma *ligma,
                      gint  image_id)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  if (ligma->image_table == NULL)
    return NULL;

  return (LigmaImage *) ligma_id_table_lookup (ligma->image_table, image_id);
}

void
ligma_image_set_file (LigmaImage *image,
                     GFile     *file)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (private->file != file)
    {
      ligma_object_take_name (LIGMA_OBJECT (image),
                             file ? g_file_get_uri (file) : NULL);
    }
}

/**
 * ligma_image_get_untitled_file:
 *
 * Returns: A #GFile saying "Untitled" for newly created images.
 **/
GFile *
ligma_image_get_untitled_file (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (! private->untitled_file)
    private->untitled_file = g_file_new_for_uri (_("Untitled"));

  return private->untitled_file;
}

/**
 * ligma_image_get_file_or_untitled:
 * @image: A #LigmaImage.
 *
 * Get the file of the XCF image, or the "Untitled" file if there is no file.
 *
 * Returns: A #GFile.
 **/
GFile *
ligma_image_get_file_or_untitled (LigmaImage *image)
{
  GFile *file;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  file = ligma_image_get_file (image);

  if (! file)
    file = ligma_image_get_untitled_file (image);

  return file;
}

/**
 * ligma_image_get_file:
 * @image: A #LigmaImage.
 *
 * Get the file of the XCF image, or %NULL if there is no file.
 *
 * Returns: (nullable): The file, or %NULL.
 **/
GFile *
ligma_image_get_file (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->file;
}

/**
 * ligma_image_get_imported_file:
 * @image: A #LigmaImage.
 *
 * Returns: (nullable): The file of the imported image, or %NULL if the image
 * has been saved as XCF after it was imported.
 **/
GFile *
ligma_image_get_imported_file (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->imported_file;
}

/**
 * ligma_image_get_exported_file:
 * @image: A #LigmaImage.
 *
 * Returns: (nullable): The file of the image last exported from this XCF file,
 * or %NULL if the image has never been exported.
 **/
GFile *
ligma_image_get_exported_file (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->exported_file;
}

/**
 * ligma_image_get_save_a_copy_file:
 * @image: A #LigmaImage.
 *
 * Returns: The URI of the last copy that was saved of this XCF file.
 **/
GFile *
ligma_image_get_save_a_copy_file (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->save_a_copy_file;
}

/**
 * ligma_image_get_any_file:
 * @image: A #LigmaImage.
 *
 * Returns: The XCF file, the imported file, or the exported file, in
 * that order of precedence.
 **/
GFile *
ligma_image_get_any_file (LigmaImage *image)
{
  GFile *file;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  file = ligma_image_get_file (image);
  if (! file)
    {
      file = ligma_image_get_imported_file (image);
      if (! file)
        {
          file = ligma_image_get_exported_file (image);
        }
    }

  return file;
}

/**
 * ligma_image_set_imported_uri:
 * @image: A #LigmaImage.
 * @file:
 *
 * Sets the URI this file was imported from.
 **/
void
ligma_image_set_imported_file (LigmaImage *image,
                              GFile     *file)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (g_set_object (&private->imported_file, file))
    {
      ligma_object_name_changed (LIGMA_OBJECT (image));
    }

  if (! private->resolution_set && file != NULL)
    {
      /* Unlike new files (which follow technological progress and will
       * use higher default resolution, or explicitly chosen templates),
       * imported files have a more backward-compatible value.
       *
       * 72 PPI is traditionally the default value when none other had
       * been explicitly set (for instance it is the default when no
       * resolution metadata was set in Exif version 2.32, and below,
       * standard). This historical value will only ever apply to loaded
       * images. New images will continue having more modern or
       * templated defaults.
       */
      private->xresolution     = 72.0;
      private->yresolution     = 72.0;
      private->resolution_unit = LIGMA_UNIT_INCH;
    }
}

/**
 * ligma_image_set_exported_file:
 * @image: A #LigmaImage.
 * @file:
 *
 * Sets the file this image was last exported to. Note that saving as
 * XCF is not "exporting".
 **/
void
ligma_image_set_exported_file (LigmaImage *image,
                              GFile     *file)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (g_set_object (&private->exported_file, file))
    {
      ligma_object_name_changed (LIGMA_OBJECT (image));
    }
}

/**
 * ligma_image_set_save_a_copy_file:
 * @image: A #LigmaImage.
 * @uri:
 *
 * Set the URI to the last copy this XCF file was saved to through the
 * "save a copy" action.
 **/
void
ligma_image_set_save_a_copy_file (LigmaImage *image,
                                 GFile     *file)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  g_set_object (&private->save_a_copy_file, file);
}

static gchar *
ligma_image_format_display_uri (LigmaImage *image,
                               gboolean   basename)
{
  const gchar *uri_format    = NULL;
  const gchar *export_status = NULL;
  GFile       *file          = NULL;
  GFile       *source        = NULL;
  GFile       *dest          = NULL;
  GFile       *display_file  = NULL;
  gboolean     is_imported;
  gboolean     is_exported;
  gchar       *display_uri   = NULL;
  gchar       *format_string;
  gchar       *tmp;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  file   = ligma_image_get_file (image);
  source = ligma_image_get_imported_file (image);
  dest   = ligma_image_get_exported_file (image);

  is_imported = (source != NULL);
  is_exported = (dest   != NULL);

  if (file)
    {
      display_file = g_object_ref (file);
      uri_format   = "%s";
    }
  else
    {
      if (is_imported)
        display_file = source;

      /* Calculate filename suffix */
      if (! ligma_image_is_export_dirty (image))
        {
          if (is_exported)
            {
              display_file  = dest;
              export_status = _(" (exported)");
            }
          else if (is_imported)
            {
              export_status = _(" (overwritten)");
            }
          else
            {
              g_warning ("Unexpected code path, Save+export implementation is buggy!");
            }
        }
      else if (is_imported)
        {
          export_status = _(" (imported)");
        }

      if (display_file)
        display_file = ligma_file_with_new_extension (display_file, NULL);

      uri_format = "[%s]";
    }

  if (! display_file)
    display_file = g_object_ref (ligma_image_get_untitled_file (image));

  if (basename)
    display_uri = g_path_get_basename (ligma_file_get_utf8_name (display_file));
  else
    display_uri = g_strdup (ligma_file_get_utf8_name (display_file));

  g_object_unref (display_file);

  format_string = g_strconcat (uri_format, export_status, NULL);

  tmp = g_strdup_printf (format_string, display_uri);
  g_free (display_uri);
  display_uri = tmp;

  g_free (format_string);

  return display_uri;
}

const gchar *
ligma_image_get_display_name (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (! private->display_name)
    private->display_name = ligma_image_format_display_uri (image, TRUE);

  return private->display_name;
}

const gchar *
ligma_image_get_display_path (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (! private->display_path)
    private->display_path = ligma_image_format_display_uri (image, FALSE);

  return private->display_path;
}

void
ligma_image_set_load_proc (LigmaImage           *image,
                          LigmaPlugInProcedure *proc)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  LIGMA_IMAGE_GET_PRIVATE (image)->load_proc = proc;
}

LigmaPlugInProcedure *
ligma_image_get_load_proc (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->load_proc;
}

void
ligma_image_set_save_proc (LigmaImage           *image,
                          LigmaPlugInProcedure *proc)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  LIGMA_IMAGE_GET_PRIVATE (image)->save_proc = proc;
}

LigmaPlugInProcedure *
ligma_image_get_save_proc (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->save_proc;
}

void
ligma_image_set_export_proc (LigmaImage           *image,
                            LigmaPlugInProcedure *proc)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  LIGMA_IMAGE_GET_PRIVATE (image)->export_proc = proc;
}

LigmaPlugInProcedure *
ligma_image_get_export_proc (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->export_proc;
}

gint
ligma_image_get_xcf_version (LigmaImage    *image,
                            gboolean      zlib_compression,
                            gint         *ligma_version,
                            const gchar **version_string,
                            gchar       **version_reason)
{
  GList       *items;
  GList       *list;
  GList       *reasons = NULL;
  gint         version = 0;  /* default to oldest */
  const gchar *enum_desc;

#define ADD_REASON(_reason)                                       \
  if (version_reason) {                                           \
    gchar *tmp = _reason;                                         \
    if (g_list_find_custom (reasons, tmp, (GCompareFunc) strcmp)) \
      g_free (tmp);                                               \
    else                                                          \
      reasons = g_list_prepend (reasons, tmp); }

  /* need version 1 for colormaps */
  if (ligma_image_get_colormap_palette (image))
    version = 1;

  items = ligma_image_get_layer_list (image);
  for (list = items; list; list = g_list_next (list))
    {
      LigmaLayer *layer = LIGMA_LAYER (list->data);

      switch (ligma_layer_get_mode (layer))
        {
          /*  Modes that exist since ancient times  */
        case LIGMA_LAYER_MODE_NORMAL_LEGACY:
        case LIGMA_LAYER_MODE_DISSOLVE:
        case LIGMA_LAYER_MODE_BEHIND_LEGACY:
        case LIGMA_LAYER_MODE_MULTIPLY_LEGACY:
        case LIGMA_LAYER_MODE_SCREEN_LEGACY:
        case LIGMA_LAYER_MODE_OVERLAY_LEGACY:
        case LIGMA_LAYER_MODE_DIFFERENCE_LEGACY:
        case LIGMA_LAYER_MODE_ADDITION_LEGACY:
        case LIGMA_LAYER_MODE_SUBTRACT_LEGACY:
        case LIGMA_LAYER_MODE_DARKEN_ONLY_LEGACY:
        case LIGMA_LAYER_MODE_LIGHTEN_ONLY_LEGACY:
        case LIGMA_LAYER_MODE_HSV_HUE_LEGACY:
        case LIGMA_LAYER_MODE_HSV_SATURATION_LEGACY:
        case LIGMA_LAYER_MODE_HSL_COLOR_LEGACY:
        case LIGMA_LAYER_MODE_HSV_VALUE_LEGACY:
        case LIGMA_LAYER_MODE_DIVIDE_LEGACY:
        case LIGMA_LAYER_MODE_DODGE_LEGACY:
        case LIGMA_LAYER_MODE_BURN_LEGACY:
        case LIGMA_LAYER_MODE_HARDLIGHT_LEGACY:
          break;

          /*  Since 2.6  */
        case LIGMA_LAYER_MODE_SOFTLIGHT_LEGACY:
        case LIGMA_LAYER_MODE_GRAIN_EXTRACT_LEGACY:
        case LIGMA_LAYER_MODE_GRAIN_MERGE_LEGACY:
        case LIGMA_LAYER_MODE_COLOR_ERASE_LEGACY:
          ligma_enum_get_value (LIGMA_TYPE_LAYER_MODE,
                               ligma_layer_get_mode (layer),
                               NULL, NULL, &enum_desc, NULL);
          ADD_REASON (g_strdup_printf (_("Layer mode '%s' was added in %s"),
                                       enum_desc, "LIGMA 2.6"));
          version = MAX (2, version);
          break;

          /*  Since 2.10  */
        case LIGMA_LAYER_MODE_OVERLAY:
        case LIGMA_LAYER_MODE_LCH_HUE:
        case LIGMA_LAYER_MODE_LCH_CHROMA:
        case LIGMA_LAYER_MODE_LCH_COLOR:
        case LIGMA_LAYER_MODE_LCH_LIGHTNESS:
          ligma_enum_get_value (LIGMA_TYPE_LAYER_MODE,
                               ligma_layer_get_mode (layer),
                               NULL, NULL, &enum_desc, NULL);
          ADD_REASON (g_strdup_printf (_("Layer mode '%s' was added in %s"),
                                       enum_desc, "LIGMA 2.10"));
          version = MAX (9, version);
          break;

          /*  Since 2.10  */
        case LIGMA_LAYER_MODE_NORMAL:
        case LIGMA_LAYER_MODE_BEHIND:
        case LIGMA_LAYER_MODE_MULTIPLY:
        case LIGMA_LAYER_MODE_SCREEN:
        case LIGMA_LAYER_MODE_DIFFERENCE:
        case LIGMA_LAYER_MODE_ADDITION:
        case LIGMA_LAYER_MODE_SUBTRACT:
        case LIGMA_LAYER_MODE_DARKEN_ONLY:
        case LIGMA_LAYER_MODE_LIGHTEN_ONLY:
        case LIGMA_LAYER_MODE_HSV_HUE:
        case LIGMA_LAYER_MODE_HSV_SATURATION:
        case LIGMA_LAYER_MODE_HSL_COLOR:
        case LIGMA_LAYER_MODE_HSV_VALUE:
        case LIGMA_LAYER_MODE_DIVIDE:
        case LIGMA_LAYER_MODE_DODGE:
        case LIGMA_LAYER_MODE_BURN:
        case LIGMA_LAYER_MODE_HARDLIGHT:
        case LIGMA_LAYER_MODE_SOFTLIGHT:
        case LIGMA_LAYER_MODE_GRAIN_EXTRACT:
        case LIGMA_LAYER_MODE_GRAIN_MERGE:
        case LIGMA_LAYER_MODE_VIVID_LIGHT:
        case LIGMA_LAYER_MODE_PIN_LIGHT:
        case LIGMA_LAYER_MODE_LINEAR_LIGHT:
        case LIGMA_LAYER_MODE_HARD_MIX:
        case LIGMA_LAYER_MODE_EXCLUSION:
        case LIGMA_LAYER_MODE_LINEAR_BURN:
        case LIGMA_LAYER_MODE_LUMA_DARKEN_ONLY:
        case LIGMA_LAYER_MODE_LUMA_LIGHTEN_ONLY:
        case LIGMA_LAYER_MODE_LUMINANCE:
        case LIGMA_LAYER_MODE_COLOR_ERASE:
        case LIGMA_LAYER_MODE_ERASE:
        case LIGMA_LAYER_MODE_MERGE:
        case LIGMA_LAYER_MODE_SPLIT:
        case LIGMA_LAYER_MODE_PASS_THROUGH:
          ligma_enum_get_value (LIGMA_TYPE_LAYER_MODE,
                               ligma_layer_get_mode (layer),
                               NULL, NULL, &enum_desc, NULL);
          ADD_REASON (g_strdup_printf (_("Layer mode '%s' was added in %s"),
                                       enum_desc, "LIGMA 2.10"));
          version = MAX (10, version);
          break;

          /*  Just here instead of default so we get compiler warnings  */
        case LIGMA_LAYER_MODE_REPLACE:
        case LIGMA_LAYER_MODE_ANTI_ERASE:
        case LIGMA_LAYER_MODE_SEPARATOR:
          break;
        }

      /* need version 3 for layer trees */
      if (ligma_viewable_get_children (LIGMA_VIEWABLE (layer)))
        {
          ADD_REASON (g_strdup_printf (_("Layer groups were added in %s"),
                                       "LIGMA 2.8"));
          version = MAX (3, version);

          /* need version 13 for group layers with masks */
          if (ligma_layer_get_mask (layer))
            {
              ADD_REASON (g_strdup_printf (_("Masks on layer groups were "
                                             "added in %s"), "LIGMA 2.10"));
              version = MAX (13, version);
            }

          if (ligma_item_get_lock_position (LIGMA_ITEM (layer)))
            {
              ADD_REASON (g_strdup_printf (_("Position locks on layer groups were added in %s"),
                                           "LIGMA 3.0"));
              version = MAX (17, version);
            }

          if (ligma_layer_get_lock_alpha (layer))
            {
              ADD_REASON (g_strdup_printf (_("Alpha channel locks on layer groups were added in %s"),
                                           "LIGMA 3.0"));
              version = MAX (17, version);
            }
        }

      if (ligma_item_get_lock_visibility (LIGMA_ITEM (layer)))
        {
          ADD_REASON (g_strdup_printf (_("Visibility locks were added in %s"),
                                       "LIGMA 3.0"));
          version = MAX (17, version);
        }
    }
  g_list_free (items);

  items = ligma_image_get_channel_list (image);
  for (list = items; list; list = g_list_next (list))
    {
      LigmaChannel *channel = LIGMA_CHANNEL (list->data);

      if (ligma_item_get_lock_visibility (LIGMA_ITEM (channel)))
        {
          ADD_REASON (g_strdup_printf (_("Visibility locks were added in %s"),
                                       "LIGMA 3.0"));
          version = MAX (17, version);
        }
    }
  g_list_free (items);

  if (g_list_length (ligma_image_get_selected_vectors (image)) > 1)
    {
      ADD_REASON (g_strdup_printf (_("Multiple path selection was "
                                     "added in %s"), "LIGMA 3.0.0"));
      version = MAX (18, version);
    }

  items = ligma_image_get_vectors_list (image);
  for (list = items; list; list = g_list_next (list))
    {
      LigmaVectors *vectors = LIGMA_VECTORS (list->data);

      if (ligma_item_get_color_tag (LIGMA_ITEM (vectors)) != LIGMA_COLOR_TAG_NONE)
        {
          ADD_REASON (g_strdup_printf (_("Storing color tags in path was "
                                         "added in %s"), "LIGMA 3.0.0"));
          version = MAX (18, version);
        }
      if (ligma_item_get_lock_content (LIGMA_ITEM (list->data)) ||
          ligma_item_get_lock_position (LIGMA_ITEM (list->data)))
        {
          ADD_REASON (g_strdup_printf (_("Storing locks in path was "
                                         "added in %s"), "LIGMA 3.0.0"));
          version = MAX (18, version);
        }
    }
  g_list_free (items);

  /* version 6 for new metadata has been dropped since they are
   * saved through parasites, which is compatible with older versions.
   */

  /* need version 7 for != 8-bit gamma images */
  if (ligma_image_get_precision (image) != LIGMA_PRECISION_U8_NON_LINEAR)
    {
      ADD_REASON (g_strdup_printf (_("High bit-depth images were added "
                                     "in %s"), "LIGMA 2.10"));
      version = MAX (7, version);
    }

  /* need version 12 for > 8-bit images for proper endian swapping */
  if (ligma_image_get_component_type (image) > LIGMA_COMPONENT_TYPE_U8)
    {
      ADD_REASON (g_strdup_printf (_("Encoding of high bit-depth images was "
                                     "fixed in %s"), "LIGMA 2.10"));
      version = MAX (12, version);
    }

  /* need version 8 for zlib compression */
  if (zlib_compression)
    {
      ADD_REASON (g_strdup_printf (_("Internal zlib compression was "
                                     "added in %s"), "LIGMA 2.10"));
      version = MAX (8, version);
    }

  /* if version is 10 (lots of new layer modes), go to version 11 with
   * 64 bit offsets right away
   */
  if (version == 10)
    version = 11;

  /* use the image's in-memory size as an upper bound to estimate the
   * need for 64 bit file offsets inside the XCF, this is a *very*
   * conservative estimate and should never fail
   */
  if (ligma_object_get_memsize (LIGMA_OBJECT (image), NULL) >= ((gint64) 1 << 32))
    {
      ADD_REASON (g_strdup_printf (_("Support for image files larger than "
                                     "4GB was added in %s"), "LIGMA 2.10"));
      version = MAX (11, version);
    }

  if (g_list_length (ligma_image_get_selected_layers (image)) > 1)
    {
      ADD_REASON (g_strdup_printf (_("Multiple layer selection was "
                                     "added in %s"), "LIGMA 3.0.0"));
      version = MAX (14, version);
    }

  if ((list = ligma_image_get_guides (image)))
    {
      for (; list; list = g_list_next (list))
        {
          gint32 position = ligma_guide_get_position (list->data);

          if (position < 0 ||
              (ligma_guide_get_orientation (list->data) == LIGMA_ORIENTATION_HORIZONTAL &&
               position > ligma_image_get_height (image)) ||
              (ligma_guide_get_orientation (list->data) == LIGMA_ORIENTATION_VERTICAL &&
               position > ligma_image_get_width (image)))
            {
              ADD_REASON (g_strdup_printf (_("Off-canvas guides "
                                             "added in %s"), "LIGMA 3.0.0"));
              version = MAX (15, version);
              break;
            }
        }
    }

  if (ligma_image_get_stored_item_sets (image, LIGMA_TYPE_LAYER) ||
      ligma_image_get_stored_item_sets (image, LIGMA_TYPE_CHANNEL))
    {
      ADD_REASON (g_strdup_printf (_("Item set and pattern search in item's name were "
                                     "added in %s"), "LIGMA 3.0.0"));
      version = MAX (16, version);
    }
  if (g_list_length (ligma_image_get_selected_channels (image)) > 1)
    {
      ADD_REASON (g_strdup_printf (_("Multiple channel selection was "
                                     "added in %s"), "LIGMA 3.0.0"));
      version = MAX (16, version);
    }


#undef ADD_REASON

  switch (version)
    {
    case 0:
    case 1:
    case 2:
      if (ligma_version)   *ligma_version   = 206;
      if (version_string) *version_string = "LIGMA 2.6";
      break;

    case 3:
      if (ligma_version)   *ligma_version   = 208;
      if (version_string) *version_string = "LIGMA 2.8";
      break;

    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
      if (ligma_version)   *ligma_version   = 210;
      if (version_string) *version_string = "LIGMA 2.10";
      break;
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
      if (ligma_version)   *ligma_version   = 300;
      if (version_string) *version_string = "LIGMA 3.0";
      break;
    }

  if (version_reason && reasons)
    {
      GString *reason = g_string_new (NULL);

      reasons = g_list_sort (reasons, (GCompareFunc) strcmp);

      for (list = reasons; list; list = g_list_next (list))
        {
          g_string_append (reason, list->data);
          if (g_list_next (list))
            g_string_append_c (reason, '\n');
        }

      *version_reason = g_string_free (reason, FALSE);
    }
  if (reasons)
    g_list_free_full (reasons, g_free);

  return version;
}

void
ligma_image_set_xcf_compression (LigmaImage *image,
                                gboolean   compression)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  LIGMA_IMAGE_GET_PRIVATE (image)->xcf_compression = compression;
}

gboolean
ligma_image_get_xcf_compression (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  return LIGMA_IMAGE_GET_PRIVATE (image)->xcf_compression;
}

void
ligma_image_set_resolution (LigmaImage *image,
                           gdouble    xresolution,
                           gdouble    yresolution)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  /* don't allow to set the resolution out of bounds */
  if (xresolution < LIGMA_MIN_RESOLUTION || xresolution > LIGMA_MAX_RESOLUTION ||
      yresolution < LIGMA_MIN_RESOLUTION || yresolution > LIGMA_MAX_RESOLUTION)
    return;

  private->resolution_set = TRUE;

  if ((ABS (private->xresolution - xresolution) >= 1e-5) ||
      (ABS (private->yresolution - yresolution) >= 1e-5))
    {
      ligma_image_undo_push_image_resolution (image,
                                             C_("undo-type", "Change Image Resolution"));

      private->xresolution = xresolution;
      private->yresolution = yresolution;

      ligma_image_resolution_changed (image);
      ligma_image_size_changed_detailed (image,
                                        0,
                                        0,
                                        ligma_image_get_width (image),
                                        ligma_image_get_height (image));
    }
}

void
ligma_image_get_resolution (LigmaImage *image,
                           gdouble   *xresolution,
                           gdouble   *yresolution)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (xresolution != NULL && yresolution != NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  *xresolution = private->xresolution;
  *yresolution = private->yresolution;
}

void
ligma_image_resolution_changed (LigmaImage *image)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  g_signal_emit (image, ligma_image_signals[RESOLUTION_CHANGED], 0);
}

void
ligma_image_set_unit (LigmaImage *image,
                     LigmaUnit   unit)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (unit > LIGMA_UNIT_PIXEL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (private->resolution_unit != unit)
    {
      ligma_image_undo_push_image_resolution (image,
                                             C_("undo-type", "Change Image Unit"));

      private->resolution_unit = unit;
      ligma_image_unit_changed (image);
    }
}

LigmaUnit
ligma_image_get_unit (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), LIGMA_UNIT_INCH);

  return LIGMA_IMAGE_GET_PRIVATE (image)->resolution_unit;
}

void
ligma_image_unit_changed (LigmaImage *image)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  g_signal_emit (image, ligma_image_signals[UNIT_CHANGED], 0);
}

gint
ligma_image_get_width (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), 0);

  return LIGMA_IMAGE_GET_PRIVATE (image)->width;
}

gint
ligma_image_get_height (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), 0);

  return LIGMA_IMAGE_GET_PRIVATE (image)->height;
}

gboolean
ligma_image_has_alpha (LigmaImage *image)
{
  LigmaImagePrivate *private;
  LigmaLayer        *layer;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), TRUE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  layer = LIGMA_LAYER (ligma_container_get_first_child (private->layers->container));

  return ((ligma_image_get_n_layers (image) > 1) ||
          (layer && ligma_drawable_has_alpha (LIGMA_DRAWABLE (layer))));
}

gboolean
ligma_image_is_empty (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), TRUE);

  return ligma_container_is_empty (LIGMA_IMAGE_GET_PRIVATE (image)->layers->container);
}

void
ligma_image_set_floating_selection (LigmaImage *image,
                                   LigmaLayer *floating_sel)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (floating_sel == NULL || LIGMA_IS_LAYER (floating_sel));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (private->floating_sel != floating_sel)
    {
      private->floating_sel = floating_sel;

      private->flush_accum.floating_selection_changed = TRUE;
    }
}

LigmaLayer *
ligma_image_get_floating_selection (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->floating_sel;
}

void
ligma_image_floating_selection_changed (LigmaImage *image)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  g_signal_emit (image, ligma_image_signals[FLOATING_SELECTION_CHANGED], 0);
}

LigmaChannel *
ligma_image_get_mask (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->selection_mask;
}

void
ligma_image_mask_changed (LigmaImage *image)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  g_signal_emit (image, ligma_image_signals[MASK_CHANGED], 0);
}

/**
 * ligma_item_mask_intersect:
 * @image:   the #LigmaImage
 * @items:   a list of #LigmaItem
 * @x: (out) (optional): return location for x
 * @y: (out) (optional): return location for y
 * @width: (out) (optional): return location for the width
 * @height: (out) (optional): return location for the height
 *
 * Intersect the area of the @items and its image's selection mask.
 * The computed area is the bounding box of the selection intersection
 * within the image. These values are only returned if the function
 * returns %TRUE.
 *
 * Note that even though the items bounding box may not be empty, it is
 * possible to get a return value of %FALSE. Imagine disjoint items, one
 * on the left, one on the right, and a selection in the middle not
 * intersecting with any of the items.
 *
 * Returns: %TRUE if the selection intersects with any of the @items.
 */
gboolean
ligma_image_mask_intersect (LigmaImage *image,
                           GList     *items,
                           gint      *x,
                           gint      *y,
                           gint      *width,
                           gint      *height)
{
  LigmaChannel *selection;
  GList       *iter;
  gint         sel_x, sel_y, sel_width, sel_height;
  gint         x1 = G_MAXINT;
  gint         y1 = G_MAXINT;
  gint         x2 = G_MININT;
  gint         y2 = G_MININT;
  gboolean     intersect = FALSE;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  for (iter = items; iter; iter = iter->next)
    {
      g_return_val_if_fail (LIGMA_IS_ITEM (iter->data), FALSE);
      g_return_val_if_fail (ligma_item_is_attached (iter->data), FALSE);
      g_return_val_if_fail (ligma_item_get_image (iter->data) == image, FALSE);
    }

  selection = ligma_image_get_mask (image);
  if (selection)
    ligma_item_bounds (LIGMA_ITEM (selection),
                      &sel_x, &sel_y, &sel_width, &sel_height);

  for (iter = items; iter; iter = iter->next)
    {
      LigmaItem *item = iter->data;
      gboolean  item_intersect;
      gint      tmp_x, tmp_y;
      gint      tmp_width, tmp_height;

      ligma_item_get_offset (item, &tmp_x, &tmp_y);

      /* check for is_empty() before intersecting so we ignore the
       * selection if it is suspended (like when stroking)
       */
      if (LIGMA_ITEM (selection) != item       &&
          ! ligma_channel_is_empty (selection))
        {
          item_intersect = ligma_rectangle_intersect (sel_x, sel_y, sel_width, sel_height,
                                                     tmp_x, tmp_y,
                                                     ligma_item_get_width  (item),
                                                     ligma_item_get_height (item),
                                                     &tmp_x, &tmp_y,
                                                     &tmp_width, &tmp_height);
        }
      else
        {
          tmp_width  = ligma_item_get_width  (item);
          tmp_height = ligma_item_get_height (item);

          item_intersect = TRUE;
        }

      if (item_intersect)
        {
          x1 = MIN (x1, tmp_x);
          y1 = MIN (y1, tmp_y);
          x2 = MAX (x2, tmp_x + tmp_width);
          y2 = MAX (y2, tmp_y + tmp_height);

          intersect = TRUE;
        }
    }

  if (intersect)
    {
      if (x)      *x      = x1;
      if (y)      *y      = y1;
      if (width)  *width  = x2 - x1;
      if (height) *height = y2 - y1;
    }

  return intersect;
}

void
ligma_image_take_mask (LigmaImage   *image,
                      LigmaChannel *mask)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_SELECTION (mask));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (private->selection_mask)
    g_object_unref (private->selection_mask);

  private->selection_mask = g_object_ref_sink (mask);

  g_signal_connect (private->selection_mask, "update",
                    G_CALLBACK (ligma_image_mask_update),
                    image);
}


/*  image components  */

const Babl *
ligma_image_get_component_format (LigmaImage       *image,
                                 LigmaChannelType  channel)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  switch (channel)
    {
    case LIGMA_CHANNEL_RED:
      return ligma_babl_component_format (LIGMA_RGB,
                                         ligma_image_get_precision (image),
                                         RED);

    case LIGMA_CHANNEL_GREEN:
      return ligma_babl_component_format (LIGMA_RGB,
                                         ligma_image_get_precision (image),
                                         GREEN);

    case LIGMA_CHANNEL_BLUE:
      return ligma_babl_component_format (LIGMA_RGB,
                                         ligma_image_get_precision (image),
                                         BLUE);

    case LIGMA_CHANNEL_ALPHA:
      return ligma_babl_component_format (LIGMA_RGB,
                                         ligma_image_get_precision (image),
                                         ALPHA);

    case LIGMA_CHANNEL_GRAY:
      return ligma_babl_component_format (LIGMA_GRAY,
                                         ligma_image_get_precision (image),
                                         GRAY);

    case LIGMA_CHANNEL_INDEXED:
      return babl_format ("Y u8"); /* will extract grayscale, the best
                                    * we can do here */
    }

  return NULL;
}

gint
ligma_image_get_component_index (LigmaImage       *image,
                                LigmaChannelType  channel)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), -1);

  switch (channel)
    {
    case LIGMA_CHANNEL_RED:     return RED;
    case LIGMA_CHANNEL_GREEN:   return GREEN;
    case LIGMA_CHANNEL_BLUE:    return BLUE;
    case LIGMA_CHANNEL_GRAY:    return GRAY;
    case LIGMA_CHANNEL_INDEXED: return INDEXED;
    case LIGMA_CHANNEL_ALPHA:
      switch (ligma_image_get_base_type (image))
        {
        case LIGMA_RGB:     return ALPHA;
        case LIGMA_GRAY:    return ALPHA_G;
        case LIGMA_INDEXED: return ALPHA_I;
        }
    }

  return -1;
}

void
ligma_image_set_component_active (LigmaImage       *image,
                                 LigmaChannelType  channel,
                                 gboolean         active)
{
  LigmaImagePrivate *private;
  gint              index = -1;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  index = ligma_image_get_component_index (image, channel);

  if (index != -1 && active != private->active[index])
    {
      private->active[index] = active ? TRUE : FALSE;

      /*  If there is an active channel and we mess with the components,
       *  the active channel gets unset...
       */
      ligma_image_unset_selected_channels (image);

      g_signal_emit (image,
                     ligma_image_signals[COMPONENT_ACTIVE_CHANGED], 0,
                     channel);
    }
}

gboolean
ligma_image_get_component_active (LigmaImage       *image,
                                 LigmaChannelType  channel)
{
  gint index = -1;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  index = ligma_image_get_component_index (image, channel);

  if (index != -1)
    return LIGMA_IMAGE_GET_PRIVATE (image)->active[index];

  return FALSE;
}

void
ligma_image_get_active_array (LigmaImage *image,
                             gboolean  *components)
{
  LigmaImagePrivate *private;
  gint              i;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (components != NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  for (i = 0; i < MAX_CHANNELS; i++)
    components[i] = private->active[i];
}

LigmaComponentMask
ligma_image_get_active_mask (LigmaImage *image)
{
  LigmaImagePrivate  *private;
  LigmaComponentMask  mask = 0;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), 0);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  switch (ligma_image_get_base_type (image))
    {
    case LIGMA_RGB:
      mask |= (private->active[RED])   ? LIGMA_COMPONENT_MASK_RED   : 0;
      mask |= (private->active[GREEN]) ? LIGMA_COMPONENT_MASK_GREEN : 0;
      mask |= (private->active[BLUE])  ? LIGMA_COMPONENT_MASK_BLUE  : 0;
      mask |= (private->active[ALPHA]) ? LIGMA_COMPONENT_MASK_ALPHA : 0;
      break;

    case LIGMA_GRAY:
    case LIGMA_INDEXED:
      mask |= (private->active[GRAY])    ? LIGMA_COMPONENT_MASK_RED   : 0;
      mask |= (private->active[GRAY])    ? LIGMA_COMPONENT_MASK_GREEN : 0;
      mask |= (private->active[GRAY])    ? LIGMA_COMPONENT_MASK_BLUE  : 0;
      mask |= (private->active[ALPHA_G]) ? LIGMA_COMPONENT_MASK_ALPHA : 0;
      break;
    }

  return mask;
}

void
ligma_image_set_component_visible (LigmaImage       *image,
                                  LigmaChannelType  channel,
                                  gboolean         visible)
{
  LigmaImagePrivate *private;
  gint              index = -1;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  index = ligma_image_get_component_index (image, channel);

  if (index != -1 && visible != private->visible[index])
    {
      private->visible[index] = visible ? TRUE : FALSE;

      if (private->visible_mask)
        {
          LigmaComponentMask mask;

          mask = ~ligma_image_get_visible_mask (image) & LIGMA_COMPONENT_MASK_ALL;

          gegl_node_set (private->visible_mask,
                         "mask", mask,
                         NULL);
        }

      g_signal_emit (image,
                     ligma_image_signals[COMPONENT_VISIBILITY_CHANGED], 0,
                     channel);

      ligma_image_invalidate_all (image);
    }
}

gboolean
ligma_image_get_component_visible (LigmaImage       *image,
                                  LigmaChannelType  channel)
{
  gint index = -1;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  index = ligma_image_get_component_index (image, channel);

  if (index != -1)
    return LIGMA_IMAGE_GET_PRIVATE (image)->visible[index];

  return FALSE;
}

void
ligma_image_get_visible_array (LigmaImage *image,
                              gboolean  *components)
{
  LigmaImagePrivate *private;
  gint              i;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (components != NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  for (i = 0; i < MAX_CHANNELS; i++)
    components[i] = private->visible[i];
}

LigmaComponentMask
ligma_image_get_visible_mask (LigmaImage *image)
{
  LigmaImagePrivate  *private;
  LigmaComponentMask  mask = 0;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), 0);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  switch (ligma_image_get_base_type (image))
    {
    case LIGMA_RGB:
      mask |= (private->visible[RED])   ? LIGMA_COMPONENT_MASK_RED   : 0;
      mask |= (private->visible[GREEN]) ? LIGMA_COMPONENT_MASK_GREEN : 0;
      mask |= (private->visible[BLUE])  ? LIGMA_COMPONENT_MASK_BLUE  : 0;
      mask |= (private->visible[ALPHA]) ? LIGMA_COMPONENT_MASK_ALPHA : 0;
      break;

    case LIGMA_GRAY:
    case LIGMA_INDEXED:
      mask |= (private->visible[GRAY])  ? LIGMA_COMPONENT_MASK_RED   : 0;
      mask |= (private->visible[GRAY])  ? LIGMA_COMPONENT_MASK_GREEN : 0;
      mask |= (private->visible[GRAY])  ? LIGMA_COMPONENT_MASK_BLUE  : 0;
      mask |= (private->visible[ALPHA]) ? LIGMA_COMPONENT_MASK_ALPHA : 0;
      break;
    }

  return mask;
}


/*  emitting image signals  */

void
ligma_image_mode_changed (LigmaImage *image)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  g_signal_emit (image, ligma_image_signals[MODE_CHANGED], 0);
}

void
ligma_image_precision_changed (LigmaImage *image)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  g_signal_emit (image, ligma_image_signals[PRECISION_CHANGED], 0);
}

void
ligma_image_alpha_changed (LigmaImage *image)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  g_signal_emit (image, ligma_image_signals[ALPHA_CHANGED], 0);
}

void
ligma_image_invalidate (LigmaImage *image,
                       gint       x,
                       gint       y,
                       gint       width,
                       gint       height)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  ligma_projectable_invalidate (LIGMA_PROJECTABLE (image),
                               x, y, width, height);

  LIGMA_IMAGE_GET_PRIVATE (image)->flush_accum.preview_invalidated = TRUE;
}

void
ligma_image_invalidate_all (LigmaImage *image)
{
  const GeglRectangle *bounding_box;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  bounding_box = &LIGMA_IMAGE_GET_PRIVATE (image)->bounding_box;

  ligma_image_invalidate (image,
                         bounding_box->x,     bounding_box->y,
                         bounding_box->width, bounding_box->height);
}

void
ligma_image_guide_added (LigmaImage *image,
                        LigmaGuide *guide)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_GUIDE (guide));

  g_signal_emit (image, ligma_image_signals[GUIDE_ADDED], 0,
                 guide);
}

void
ligma_image_guide_removed (LigmaImage *image,
                          LigmaGuide *guide)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_GUIDE (guide));

  g_signal_emit (image, ligma_image_signals[GUIDE_REMOVED], 0,
                 guide);
}

void
ligma_image_guide_moved (LigmaImage *image,
                        LigmaGuide *guide)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_GUIDE (guide));

  g_signal_emit (image, ligma_image_signals[GUIDE_MOVED], 0,
                 guide);
}

void
ligma_image_sample_point_added (LigmaImage       *image,
                               LigmaSamplePoint *sample_point)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_SAMPLE_POINT (sample_point));

  g_signal_emit (image, ligma_image_signals[SAMPLE_POINT_ADDED], 0,
                 sample_point);
}

void
ligma_image_sample_point_removed (LigmaImage       *image,
                                 LigmaSamplePoint *sample_point)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_SAMPLE_POINT (sample_point));

  g_signal_emit (image, ligma_image_signals[SAMPLE_POINT_REMOVED], 0,
                 sample_point);
}

void
ligma_image_sample_point_moved (LigmaImage       *image,
                               LigmaSamplePoint *sample_point)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_SAMPLE_POINT (sample_point));

  g_signal_emit (image, ligma_image_signals[SAMPLE_POINT_MOVED], 0,
                 sample_point);
}

/**
 * ligma_image_size_changed_detailed:
 * @image:
 * @previous_origin_x:
 * @previous_origin_y:
 *
 * Emits the size-changed-detailed signal that is typically used to adjust the
 * position of the image in the display shell on various operations,
 * e.g. crop.
 *
 * This function makes sure that LigmaViewable::size-changed is also emitted.
 **/
void
ligma_image_size_changed_detailed (LigmaImage *image,
                                  gint       previous_origin_x,
                                  gint       previous_origin_y,
                                  gint       previous_width,
                                  gint       previous_height)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  g_signal_emit (image, ligma_image_signals[SIZE_CHANGED_DETAILED], 0,
                 previous_origin_x,
                 previous_origin_y,
                 previous_width,
                 previous_height);
}

void
ligma_image_colormap_changed (LigmaImage *image,
                             gint       color_index)
{
  LigmaPalette *palette;
  gint         n_colors;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  palette = LIGMA_IMAGE_GET_PRIVATE (image)->palette;
  n_colors = palette ? ligma_palette_get_n_colors (palette) : 0;
  g_return_if_fail (color_index >= -1 && color_index < n_colors);

  g_signal_emit (image, ligma_image_signals[COLORMAP_CHANGED], 0,
                 color_index);
}

void
ligma_image_selection_invalidate (LigmaImage *image)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  g_signal_emit (image, ligma_image_signals[SELECTION_INVALIDATE], 0);
}

void
ligma_image_quick_mask_changed (LigmaImage *image)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  g_signal_emit (image, ligma_image_signals[QUICK_MASK_CHANGED], 0);
}

void
ligma_image_undo_event (LigmaImage     *image,
                       LigmaUndoEvent  event,
                       LigmaUndo      *undo)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (((event == LIGMA_UNDO_EVENT_UNDO_FREE   ||
                      event == LIGMA_UNDO_EVENT_UNDO_FREEZE ||
                      event == LIGMA_UNDO_EVENT_UNDO_THAW) && undo == NULL) ||
                    LIGMA_IS_UNDO (undo));

  g_signal_emit (image, ligma_image_signals[UNDO_EVENT], 0, event, undo);
}


/*  dirty counters  */

/* NOTE about the image->dirty counter:
 *   If 0, then the image is clean (ie, copy on disk is the same as the one
 *      in memory).
 *   If positive, then that's the number of dirtying operations done
 *       on the image since the last save.
 *   If negative, then user has hit undo and gone back in time prior
 *       to the saved copy.  Hitting redo will eventually come back to
 *       the saved copy.
 *
 *   The image is dirty (ie, needs saving) if counter is non-zero.
 *
 *   If the counter is around 100000, this is due to undo-ing back
 *   before a saved version, then changing the image (thus destroying
 *   the redo stack).  Once this has happened, it's impossible to get
 *   the image back to the state on disk, since the redo info has been
 *   freed.  See ligmaimage-undo.c for the gory details.
 */

/*
 * NEVER CALL ligma_image_dirty() directly!
 *
 * If your code has just dirtied the image, push an undo instead.
 * Failing that, push the trivial undo which tells the user the
 * command is not undoable: undo_push_cantundo() (But really, it would
 * be best to push a proper undo).  If you just dirty the image
 * without pushing an undo then the dirty count is increased, but
 * popping that many undo actions won't lead to a clean image.
 */

gint
ligma_image_dirty (LigmaImage     *image,
                  LigmaDirtyMask  dirty_mask)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  private->dirty++;
  private->export_dirty++;

  if (! private->dirty_time)
    private->dirty_time = time (NULL);

  g_signal_emit (image, ligma_image_signals[DIRTY], 0, dirty_mask);

  TRC (("dirty %d -> %d\n", private->dirty - 1, private->dirty));

  return private->dirty;
}

gint
ligma_image_clean (LigmaImage     *image,
                  LigmaDirtyMask  dirty_mask)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  private->dirty--;
  private->export_dirty--;

  g_signal_emit (image, ligma_image_signals[CLEAN], 0, dirty_mask);

  TRC (("clean %d -> %d\n", private->dirty + 1, private->dirty));

  return private->dirty;
}

void
ligma_image_clean_all (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  private->dirty      = 0;
  private->dirty_time = 0;

  g_signal_emit (image, ligma_image_signals[CLEAN], 0, LIGMA_DIRTY_ALL);

  ligma_object_name_changed (LIGMA_OBJECT (image));
}

void
ligma_image_export_clean_all (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  private->export_dirty = 0;

  g_signal_emit (image, ligma_image_signals[CLEAN], 0, LIGMA_DIRTY_ALL);

  ligma_object_name_changed (LIGMA_OBJECT (image));
}

/**
 * ligma_image_is_dirty:
 * @image:
 *
 * Returns: True if the image is dirty, false otherwise.
 **/
gint
ligma_image_is_dirty (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  return LIGMA_IMAGE_GET_PRIVATE (image)->dirty != 0;
}

/**
 * ligma_image_is_export_dirty:
 * @image:
 *
 * Returns: True if the image export is dirty, false otherwise.
 **/
gboolean
ligma_image_is_export_dirty (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  return LIGMA_IMAGE_GET_PRIVATE (image)->export_dirty != 0;
}

gint64
ligma_image_get_dirty_time (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), 0);

  return LIGMA_IMAGE_GET_PRIVATE (image)->dirty_time;
}

/**
 * ligma_image_saving:
 * @image:
 *
 * Emits the "saving" signal, indicating that @image is about to be saved,
 * or exported.
 */
void
ligma_image_saving (LigmaImage *image)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  g_signal_emit (image, ligma_image_signals[SAVING], 0);
}

/**
 * ligma_image_saved:
 * @image:
 * @file:
 *
 * Emits the "saved" signal, indicating that @image was saved to the
 * location specified by @file.
 */
void
ligma_image_saved (LigmaImage *image,
                  GFile     *file)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (G_IS_FILE (file));

  g_signal_emit (image, ligma_image_signals[SAVED], 0, file);
}

/**
 * ligma_image_exported:
 * @image:
 * @file:
 *
 * Emits the "exported" signal, indicating that @image was exported to the
 * location specified by @file.
 */
void
ligma_image_exported (LigmaImage *image,
                     GFile     *file)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (G_IS_FILE (file));

  g_signal_emit (image, ligma_image_signals[EXPORTED], 0, file);
}


/*  flush this image's displays  */

void
ligma_image_flush (LigmaImage *image)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  ligma_projectable_flush (LIGMA_PROJECTABLE (image),
                          LIGMA_IMAGE_GET_PRIVATE (image)->flush_accum.preview_invalidated);
}


/*  display / instance counters  */

gint
ligma_image_get_display_count (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), 0);

  return LIGMA_IMAGE_GET_PRIVATE (image)->disp_count;
}

void
ligma_image_inc_display_count (LigmaImage *image)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  LIGMA_IMAGE_GET_PRIVATE (image)->disp_count++;
}

void
ligma_image_dec_display_count (LigmaImage *image)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  LIGMA_IMAGE_GET_PRIVATE (image)->disp_count--;
}

gint
ligma_image_get_instance_count (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), 0);

  return LIGMA_IMAGE_GET_PRIVATE (image)->instance_count;
}

void
ligma_image_inc_instance_count (LigmaImage *image)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  LIGMA_IMAGE_GET_PRIVATE (image)->instance_count++;
}

void
ligma_image_inc_show_all_count (LigmaImage *image)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  LIGMA_IMAGE_GET_PRIVATE (image)->show_all++;

  if (LIGMA_IMAGE_GET_PRIVATE (image)->show_all == 1)
    {
      g_clear_object (&LIGMA_IMAGE_GET_PRIVATE (image)->pickable_buffer);

      ligma_image_update_bounding_box (image);
    }
}

void
ligma_image_dec_show_all_count (LigmaImage *image)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  LIGMA_IMAGE_GET_PRIVATE (image)->show_all--;

  if (LIGMA_IMAGE_GET_PRIVATE (image)->show_all == 0)
    {
      g_clear_object (&LIGMA_IMAGE_GET_PRIVATE (image)->pickable_buffer);

      ligma_image_update_bounding_box (image);
    }
}


/*  parasites  */

const LigmaParasite *
ligma_image_parasite_find (LigmaImage   *image,
                          const gchar *name)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return ligma_parasite_list_find (LIGMA_IMAGE_GET_PRIVATE (image)->parasites,
                                  name);
}

static void
list_func (gchar          *key,
           LigmaParasite   *p,
           gchar        ***cur)
{
  *(*cur)++ = (gchar *) g_strdup (key);
}

gchar **
ligma_image_parasite_list (LigmaImage *image)
{
  LigmaImagePrivate  *private;
  gint               count;
  gchar            **list;
  gchar            **cur;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  count = ligma_parasite_list_length (private->parasites);
  cur = list = g_new0 (gchar *, count + 1);

  ligma_parasite_list_foreach (private->parasites, (GHFunc) list_func, &cur);

  return list;
}

gboolean
ligma_image_parasite_validate (LigmaImage           *image,
                              const LigmaParasite  *parasite,
                              GError             **error)
{
  const gchar *name;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (parasite != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  name = ligma_parasite_get_name (parasite);

  if (strcmp (name, LIGMA_ICC_PROFILE_PARASITE_NAME) == 0 ||
      strcmp (name, LIGMA_SIMULATION_ICC_PROFILE_PARASITE_NAME) == 0)
    {
      return ligma_image_validate_icc_parasite (image, parasite,
                                               name, NULL, error);
    }
  else if (strcmp (name, "ligma-comment") == 0)
    {
      const gchar *data;
      guint32      length;
      gboolean     valid  = FALSE;

      data = ligma_parasite_get_data (parasite, &length);
      if (length > 0)
        {
          if (data[length - 1] == '\0')
            valid = g_utf8_validate (data, -1, NULL);
          else
            valid = g_utf8_validate (data, length, NULL);
        }

      if (! valid)
        {
          g_set_error (error, LIGMA_ERROR, LIGMA_FAILED,
                       _("'ligma-comment' parasite validation failed: "
                         "comment contains invalid UTF-8"));
          return FALSE;
        }
    }

  return TRUE;
}

void
ligma_image_parasite_attach (LigmaImage          *image,
                            const LigmaParasite *parasite,
                            gboolean            push_undo)
{
  LigmaImagePrivate *private;
  LigmaParasite      copy;
  const gchar      *name;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (parasite != NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  name = ligma_parasite_get_name (parasite);

  /*  this is so ugly and is only for the PDB  */
  if (strcmp (name, LIGMA_ICC_PROFILE_PARASITE_NAME) == 0 ||
      strcmp (name, LIGMA_SIMULATION_ICC_PROFILE_PARASITE_NAME) == 0)
    {
      LigmaColorProfile *profile;
      LigmaColorProfile *builtin;
      const guint8     *parasite_data;
      guint32           parasite_length;

      parasite_data = ligma_parasite_get_data (parasite, &parasite_length);
      profile =
        ligma_color_profile_new_from_icc_profile (parasite_data, parasite_length,
                                                 NULL);
      builtin = ligma_image_get_builtin_color_profile (image);

      if (ligma_color_profile_is_equal (profile, builtin))
        {
          /* setting the builtin profile is equal to removing the profile */
          ligma_image_parasite_detach (image, LIGMA_ICC_PROFILE_PARASITE_NAME,
                                      push_undo);
          g_object_unref (profile);
          return;
        }

      g_object_unref (profile);
    }

  /*  make a temporary copy of the LigmaParasite struct because
   *  ligma_parasite_shift_parent() changes it
   */
  copy = *parasite;

  /*  only set the dirty bit manually if we can be saved and the new
   *  parasite differs from the current one and we aren't undoable
   */
  if (push_undo && ligma_parasite_is_undoable (&copy))
    ligma_image_undo_push_image_parasite (image,
                                         C_("undo-type", "Attach Parasite to Image"),
                                         &copy);

  /*  We used to push a cantundo on the stack here. This made the undo stack
   *  unusable (NULL on the stack) and prevented people from undoing after a
   *  save (since most save plug-ins attach an undoable comment parasite).
   *  Now we simply attach the parasite without pushing an undo. That way
   *  it's undoable but does not block the undo system.   --Sven
   */
  ligma_parasite_list_add (private->parasites, &copy);

  if (push_undo && ligma_parasite_has_flag (&copy, LIGMA_PARASITE_ATTACH_PARENT))
    {
      ligma_parasite_shift_parent (&copy);
      ligma_parasite_attach (image->ligma, &copy);
    }

  if (strcmp (name, LIGMA_ICC_PROFILE_PARASITE_NAME) == 0)
    _ligma_image_update_color_profile (image, parasite);

  if (strcmp (name, LIGMA_SIMULATION_ICC_PROFILE_PARASITE_NAME) == 0)
    _ligma_image_update_simulation_profile (image, parasite);

  g_signal_emit (image, ligma_image_signals[PARASITE_ATTACHED], 0,
                 name);
}

void
ligma_image_parasite_detach (LigmaImage   *image,
                            const gchar *name,
                            gboolean     push_undo)
{
  LigmaImagePrivate   *private;
  const LigmaParasite *parasite;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (name != NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (! (parasite = ligma_parasite_list_find (private->parasites, name)))
    return;

  if (push_undo && ligma_parasite_is_undoable (parasite))
    ligma_image_undo_push_image_parasite_remove (image,
                                                C_("undo-type", "Remove Parasite from Image"),
                                                name);

  ligma_parasite_list_remove (private->parasites, name);

  if (strcmp (name, LIGMA_ICC_PROFILE_PARASITE_NAME) == 0)
    _ligma_image_update_color_profile (image, NULL);

  if (strcmp (name, LIGMA_SIMULATION_ICC_PROFILE_PARASITE_NAME) == 0)
    _ligma_image_update_simulation_profile (image, NULL);

  g_signal_emit (image, ligma_image_signals[PARASITE_DETACHED], 0,
                 name);
}


/*  tattoos  */

LigmaTattoo
ligma_image_get_new_tattoo (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), 0);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  private->tattoo_state++;

  if (G_UNLIKELY (private->tattoo_state == 0))
    g_warning ("%s: Tattoo state corrupted (integer overflow).", G_STRFUNC);

  return private->tattoo_state;
}

LigmaTattoo
ligma_image_get_tattoo_state (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), 0);

  return LIGMA_IMAGE_GET_PRIVATE (image)->tattoo_state;
}

gboolean
ligma_image_set_tattoo_state (LigmaImage  *image,
                             LigmaTattoo  val)
{
  GList      *all_items;
  GList      *list;
  gboolean    retval = TRUE;
  LigmaTattoo  maxval = 0;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  /* Check that the layer tattoos don't overlap with channel or vector ones */
  all_items = ligma_image_get_layer_list (image);

  for (list = all_items; list; list = g_list_next (list))
    {
      LigmaTattoo ltattoo;

      ltattoo = ligma_item_get_tattoo (LIGMA_ITEM (list->data));
      if (ltattoo > maxval)
        maxval = ltattoo;

      if (ligma_image_get_channel_by_tattoo (image, ltattoo))
        retval = FALSE; /* Oopps duplicated tattoo in channel */

      if (ligma_image_get_vectors_by_tattoo (image, ltattoo))
        retval = FALSE; /* Oopps duplicated tattoo in vectors */
    }

  g_list_free (all_items);

  /* Now check that the channel and vectors tattoos don't overlap */
  all_items = ligma_image_get_channel_list (image);

  for (list = all_items; list; list = g_list_next (list))
    {
      LigmaTattoo ctattoo;

      ctattoo = ligma_item_get_tattoo (LIGMA_ITEM (list->data));
      if (ctattoo > maxval)
        maxval = ctattoo;

      if (ligma_image_get_vectors_by_tattoo (image, ctattoo))
        retval = FALSE; /* Oopps duplicated tattoo in vectors */
    }

  g_list_free (all_items);

  /* Find the max tattoo value in the vectors */
  all_items = ligma_image_get_vectors_list (image);

  for (list = all_items; list; list = g_list_next (list))
    {
      LigmaTattoo vtattoo;

      vtattoo = ligma_item_get_tattoo (LIGMA_ITEM (list->data));
      if (vtattoo > maxval)
        maxval = vtattoo;
    }

  g_list_free (all_items);

  if (val < maxval)
    retval = FALSE;

  /* Must check if the state is valid */
  if (retval == TRUE)
    LIGMA_IMAGE_GET_PRIVATE (image)->tattoo_state = val;

  return retval;
}


/*  projection  */

LigmaProjection *
ligma_image_get_projection (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->projection;
}


/*  layers / channels / vectors  */

LigmaItemTree *
ligma_image_get_layer_tree (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->layers;
}

LigmaItemTree *
ligma_image_get_channel_tree (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->channels;
}

LigmaItemTree *
ligma_image_get_vectors_tree (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->vectors;
}

LigmaContainer *
ligma_image_get_layers (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->layers->container;
}

LigmaContainer *
ligma_image_get_channels (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->channels->container;
}

LigmaContainer *
ligma_image_get_vectors (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->vectors->container;
}

gint
ligma_image_get_n_layers (LigmaImage *image)
{
  LigmaItemStack *stack;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), 0);

  stack = LIGMA_ITEM_STACK (ligma_image_get_layers (image));

  return ligma_item_stack_get_n_items (stack);
}

gint
ligma_image_get_n_channels (LigmaImage *image)
{
  LigmaItemStack *stack;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), 0);

  stack = LIGMA_ITEM_STACK (ligma_image_get_channels (image));

  return ligma_item_stack_get_n_items (stack);
}

gint
ligma_image_get_n_vectors (LigmaImage *image)
{
  LigmaItemStack *stack;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), 0);

  stack = LIGMA_ITEM_STACK (ligma_image_get_vectors (image));

  return ligma_item_stack_get_n_items (stack);
}

GList *
ligma_image_get_layer_iter (LigmaImage *image)
{
  LigmaItemStack *stack;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  stack = LIGMA_ITEM_STACK (ligma_image_get_layers (image));

  return ligma_item_stack_get_item_iter (stack);
}

GList *
ligma_image_get_channel_iter (LigmaImage *image)
{
  LigmaItemStack *stack;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  stack = LIGMA_ITEM_STACK (ligma_image_get_channels (image));

  return ligma_item_stack_get_item_iter (stack);
}

GList *
ligma_image_get_vectors_iter (LigmaImage *image)
{
  LigmaItemStack *stack;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  stack = LIGMA_ITEM_STACK (ligma_image_get_vectors (image));

  return ligma_item_stack_get_item_iter (stack);
}

GList *
ligma_image_get_layer_list (LigmaImage *image)
{
  LigmaItemStack *stack;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  stack = LIGMA_ITEM_STACK (ligma_image_get_layers (image));

  return ligma_item_stack_get_item_list (stack);
}

GList *
ligma_image_get_channel_list (LigmaImage *image)
{
  LigmaItemStack *stack;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  stack = LIGMA_ITEM_STACK (ligma_image_get_channels (image));

  return ligma_item_stack_get_item_list (stack);
}

GList *
ligma_image_get_vectors_list (LigmaImage *image)
{
  LigmaItemStack *stack;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  stack = LIGMA_ITEM_STACK (ligma_image_get_vectors (image));

  return ligma_item_stack_get_item_list (stack);
}


/*  active drawable, layer, channel, vectors  */

LigmaLayer *
ligma_image_get_active_layer (LigmaImage *image)
{
  GList *layers;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  layers = ligma_image_get_selected_layers (image);
  if (g_list_length (layers) == 1)
    return layers->data;
  else
    return NULL;
}

LigmaChannel *
ligma_image_get_active_channel (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  return LIGMA_CHANNEL (ligma_item_tree_get_active_item (private->channels));
}

LigmaVectors *
ligma_image_get_active_vectors (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  return LIGMA_VECTORS (ligma_item_tree_get_active_item (private->vectors));
}

LigmaLayer *
ligma_image_set_active_layer (LigmaImage *image,
                             LigmaLayer *layer)
{
  GList     *layers = NULL;
  LigmaLayer *active_layer;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (layer == NULL || LIGMA_IS_LAYER (layer), NULL);
  g_return_val_if_fail (layer == NULL ||
                        (ligma_item_is_attached (LIGMA_ITEM (layer)) &&
                         ligma_item_get_image (LIGMA_ITEM (layer)) == image),
                        NULL);
  if (layer)
    layers = g_list_prepend (NULL, layer);

  ligma_image_set_selected_layers (image, layers);
  g_list_free (layers);

  layers = ligma_image_get_selected_layers (image);
  active_layer = g_list_length (layers) == 1 ? layers->data : NULL;

  return active_layer;
}

LigmaChannel *
ligma_image_set_active_channel (LigmaImage   *image,
                               LigmaChannel *channel)
{
  LigmaChannel *active_channel;
  GList       *channels = NULL;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (channel == NULL || LIGMA_IS_CHANNEL (channel), NULL);
  g_return_val_if_fail (channel == NULL ||
                        (ligma_item_is_attached (LIGMA_ITEM (channel)) &&
                         ligma_item_get_image (LIGMA_ITEM (channel)) == image),
                        NULL);

  /*  Not if there is a floating selection  */
  if (channel && ligma_image_get_floating_selection (image))
    return NULL;

  if (channel)
    channels = g_list_prepend (NULL, channel);

  ligma_image_set_selected_channels (image, channels);
  g_list_free (channels);

  channels = ligma_image_get_selected_channels (image);
  active_channel = g_list_length (channels) == 1 ? channels->data : NULL;

  return active_channel;
}

void
ligma_image_unset_selected_channels (LigmaImage *image)
{
  LigmaImagePrivate *private;
  GList            *channels;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  channels = ligma_image_get_selected_channels (image);

  if (channels)
    {
      ligma_image_set_selected_channels (image, NULL);

      if (private->layer_stack)
        ligma_image_set_selected_layers (image, private->layer_stack->data);
    }
}

LigmaVectors *
ligma_image_set_active_vectors (LigmaImage   *image,
                               LigmaVectors *vectors)
{
  GList       *all_vectors = NULL;
  LigmaVectors *active_vectors;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (vectors == NULL || LIGMA_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (vectors == NULL ||
                        (ligma_item_is_attached (LIGMA_ITEM (vectors)) &&
                         ligma_item_get_image (LIGMA_ITEM (vectors)) == image),
                        NULL);
  if (vectors)
    all_vectors = g_list_prepend (NULL, vectors);

  ligma_image_set_selected_vectors (image, all_vectors);
  g_list_free (all_vectors);

  all_vectors = ligma_image_get_selected_vectors (image);
  active_vectors = (g_list_length (all_vectors) == 1 ? all_vectors->data : NULL);

  return active_vectors;
}

/**
 * ligma_image_is_selected_drawable:
 * @image:
 * @drawable:
 *
 * Checks if @drawable belong to the list of currently selected
 * drawables. It doesn't mean this is the only selected drawable (if
 * this is what you want to check, use
 * ligma_image_equal_selected_drawables() with a list containing only
 * this drawable).
 *
 * Returns: %TRUE is @drawable is one of the selected drawables.
 */
gboolean
ligma_image_is_selected_drawable (LigmaImage    *image,
                                 LigmaDrawable *drawable)
{
  GList    *selected_drawables;
  gboolean  found;

  selected_drawables = ligma_image_get_selected_drawables (image);
  found = (g_list_find (selected_drawables, drawable) != NULL);
  g_list_free (selected_drawables);

  return found;
}

/**
 * ligma_image_equal_selected_drawables:
 * @image:
 * @drawables:
 *
 * Compare the list of @drawables with the selected drawables in @image
 * (i.e. the result of ligma_image_equal_selected_drawables()).
 * The order of the @drawables does not matter, only if the size and
 * contents of the list is the same.
 */
gboolean
ligma_image_equal_selected_drawables (LigmaImage *image,
                                     GList     *drawables)
{
  GList    *selected_drawables;
  GList    *iter;
  gboolean  equal = FALSE;

  selected_drawables = ligma_image_get_selected_drawables (image);

  if (g_list_length (drawables) == g_list_length (selected_drawables))
    {
      equal = TRUE;

      for (iter = drawables; iter; iter = iter->next)
        if (! g_list_find (selected_drawables, iter->data))
          {
            equal = FALSE;
            break;
          }
    }
  g_list_free (selected_drawables);

  return equal;
}

GList *
ligma_image_get_selected_drawables (LigmaImage *image)
{
  LigmaImagePrivate *private;
  GList            *selected_channels;
  GList            *selected_layers;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  selected_channels = ligma_item_tree_get_selected_items (private->channels);
  selected_layers   = ligma_item_tree_get_selected_items (private->layers);

  /*  If there is an active channel (a saved selection, etc.),
   *  we ignore the active layer
   */
  if (selected_channels)
    {
      return g_list_copy (selected_channels);
    }
  else if (selected_layers)
    {
      selected_layers = g_list_copy (selected_layers);
      if (g_list_length (selected_layers) == 1)
        {
          /* As a special case, if only one layer is selected and mask
           * edit is in progress, we return the mask as selected
           * drawable instead of the layer.
           */
          LigmaLayer     *layer = LIGMA_LAYER (selected_layers->data);
          LigmaLayerMask *mask  = ligma_layer_get_mask (layer);

          if (mask && ligma_layer_get_edit_mask (layer))
            selected_layers->data = mask;
        }
      return selected_layers;
    }

  return NULL;
}

GList *
ligma_image_get_selected_layers (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  return ligma_item_tree_get_selected_items (private->layers);
}

GList *
ligma_image_get_selected_channels (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  return ligma_item_tree_get_selected_items (private->channels);
}

GList *
ligma_image_get_selected_vectors (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  return ligma_item_tree_get_selected_items (private->vectors);
}

void
ligma_image_set_selected_layers (LigmaImage *image,
                                GList     *layers)
{
  LigmaImagePrivate *private;
  LigmaLayer        *floating_sel;
  GList            *selected_layers;
  GList            *layers2;
  GList            *iter;
  gboolean          selection_changed = TRUE;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  layers2 = g_list_copy (layers);
  for (iter = layers; iter; iter = iter->next)
    {
      g_return_if_fail (LIGMA_IS_LAYER (iter->data));
      g_return_if_fail (ligma_item_get_image (LIGMA_ITEM (iter->data)) == image);

      /* Silently remove non-attached layers from selection. Do not
       * error out on it as it may happen for instance when selection
       * changes while in the process of removing a layer group.
       */
      if (! ligma_item_is_attached (LIGMA_ITEM (iter->data)))
        layers2 = g_list_remove (layers2, iter->data);
    }

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  floating_sel = ligma_image_get_floating_selection (image);

  /*  Make sure the floating_sel always is the active layer  */
  if (floating_sel && (g_list_length (layers2) != 1 || layers2->data != floating_sel))
    return;

  selected_layers = ligma_image_get_selected_layers (image);

  if (g_list_length (layers2) == g_list_length (selected_layers))
    {
      selection_changed = FALSE;
      for (iter = layers2; iter; iter = iter->next)
        {
          if (g_list_find (selected_layers, iter->data) == NULL)
            {
              selection_changed = TRUE;
              break;
            }
        }
    }

  if (selection_changed)
    {
      /*  Don't cache selection info for the previous active layer  */
      if (selected_layers)
        ligma_drawable_invalidate_boundary (LIGMA_DRAWABLE (selected_layers->data));

      ligma_item_tree_set_selected_items (private->layers, layers2);

      /* We cannot edit masks with multiple selected layers. */
      if (g_list_length (layers2) > 1)
        {
          for (iter = layers2; iter; iter = iter->next)
            {
              if (ligma_layer_get_mask (iter->data))
                ligma_layer_set_edit_mask (iter->data, FALSE);
            }
        }
    }
  else
    {
      g_list_free (layers2);
    }
}

void
ligma_image_set_selected_channels (LigmaImage *image,
                                  GList     *channels)
{
  LigmaImagePrivate *private;
  GList            *iter;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  for (iter = channels; iter; iter = iter->next)
    {
      g_return_if_fail (LIGMA_IS_CHANNEL (iter->data));
      g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (iter->data)) &&
                        ligma_item_get_image (LIGMA_ITEM (iter->data)) == image);
    }
  private = LIGMA_IMAGE_GET_PRIVATE (image);

  /*  Not if there is a floating selection  */
  if (g_list_length (channels) > 0 && ligma_image_get_floating_selection (image))
    return;

  ligma_item_tree_set_selected_items (private->channels, g_list_copy (channels));
}

void
ligma_image_set_selected_vectors (LigmaImage *image,
                                 GList     *vectors)
{
  LigmaImagePrivate *private;
  GList            *iter;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  for (iter = vectors; iter; iter = iter->next)
    {
      g_return_if_fail (LIGMA_IS_VECTORS (iter->data));
      g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (iter->data)) &&
                        ligma_item_get_image (LIGMA_ITEM (iter->data)) == image);
    }

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  ligma_item_tree_set_selected_items (private->vectors, g_list_copy (vectors));
}


/*  layer, channel, vectors by tattoo  */

LigmaLayer *
ligma_image_get_layer_by_tattoo (LigmaImage  *image,
                                LigmaTattoo  tattoo)
{
  LigmaItemStack *stack;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  stack = LIGMA_ITEM_STACK (ligma_image_get_layers (image));

  return LIGMA_LAYER (ligma_item_stack_get_item_by_tattoo (stack, tattoo));
}

LigmaChannel *
ligma_image_get_channel_by_tattoo (LigmaImage  *image,
                                  LigmaTattoo  tattoo)
{
  LigmaItemStack *stack;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  stack = LIGMA_ITEM_STACK (ligma_image_get_channels (image));

  return LIGMA_CHANNEL (ligma_item_stack_get_item_by_tattoo (stack, tattoo));
}

LigmaVectors *
ligma_image_get_vectors_by_tattoo (LigmaImage  *image,
                                  LigmaTattoo  tattoo)
{
  LigmaItemStack *stack;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  stack = LIGMA_ITEM_STACK (ligma_image_get_vectors (image));

  return LIGMA_VECTORS (ligma_item_stack_get_item_by_tattoo (stack, tattoo));
}


/*  layer, channel, vectors by name  */

LigmaLayer *
ligma_image_get_layer_by_name (LigmaImage   *image,
                              const gchar *name)
{
  LigmaItemTree *tree;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  tree = ligma_image_get_layer_tree (image);

  return LIGMA_LAYER (ligma_item_tree_get_item_by_name (tree, name));
}

LigmaChannel *
ligma_image_get_channel_by_name (LigmaImage   *image,
                                const gchar *name)
{
  LigmaItemTree *tree;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  tree = ligma_image_get_channel_tree (image);

  return LIGMA_CHANNEL (ligma_item_tree_get_item_by_name (tree, name));
}

LigmaVectors *
ligma_image_get_vectors_by_name (LigmaImage   *image,
                                const gchar *name)
{
  LigmaItemTree *tree;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  tree = ligma_image_get_vectors_tree (image);

  return LIGMA_VECTORS (ligma_item_tree_get_item_by_name (tree, name));
}


/*  items  */

gboolean
ligma_image_reorder_item (LigmaImage   *image,
                         LigmaItem    *item,
                         LigmaItem    *new_parent,
                         gint         new_index,
                         gboolean     push_undo,
                         const gchar *undo_desc)
{
  LigmaItemTree *tree;
  gboolean      result;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (ligma_item_get_image (item) == image, FALSE);

  tree = ligma_item_get_tree (item);

  g_return_val_if_fail (tree != NULL, FALSE);

  if (push_undo)
    {
      if (! undo_desc)
        undo_desc = LIGMA_ITEM_GET_CLASS (item)->reorder_desc;

      ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_ITEM_REORDER,
                                   undo_desc);
    }

  ligma_image_freeze_bounding_box (image);

  ligma_item_start_move (item, push_undo);

  /*  item and new_parent are type-checked in LigmaItemTree
   */
  result = ligma_item_tree_reorder_item (tree, item,
                                        new_parent, new_index,
                                        push_undo, undo_desc);

  ligma_item_end_move (item, push_undo);

  ligma_image_thaw_bounding_box (image);

  if (push_undo)
    ligma_image_undo_group_end (image);

  return result;
}

gboolean
ligma_image_raise_item (LigmaImage *image,
                       LigmaItem  *item,
                       GError    **error)
{
  gint index;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  index = ligma_item_get_index (item);

  g_return_val_if_fail (index != -1, FALSE);

  if (index == 0)
    {
      g_set_error_literal (error,  LIGMA_ERROR, LIGMA_FAILED,
                           LIGMA_ITEM_GET_CLASS (item)->raise_failed);
      return FALSE;
    }

  return ligma_image_reorder_item (image, item,
                                  ligma_item_get_parent (item), index - 1,
                                  TRUE, LIGMA_ITEM_GET_CLASS (item)->raise_desc);
}

gboolean
ligma_image_raise_item_to_top (LigmaImage *image,
                              LigmaItem  *item)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  return ligma_image_reorder_item (image, item,
                                  ligma_item_get_parent (item), 0,
                                  TRUE, LIGMA_ITEM_GET_CLASS (item)->raise_to_top_desc);
}

gboolean
ligma_image_lower_item (LigmaImage *image,
                       LigmaItem  *item,
                       GError    **error)
{
  LigmaContainer *container;
  gint           index;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  container = ligma_item_get_container (item);

  g_return_val_if_fail (container != NULL, FALSE);

  index = ligma_item_get_index (item);

  if (index == ligma_container_get_n_children (container) - 1)
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           LIGMA_ITEM_GET_CLASS (item)->lower_failed);
      return FALSE;
    }

  return ligma_image_reorder_item (image, item,
                                  ligma_item_get_parent (item), index + 1,
                                  TRUE, LIGMA_ITEM_GET_CLASS (item)->lower_desc);
}

gboolean
ligma_image_lower_item_to_bottom (LigmaImage *image,
                                  LigmaItem *item)
{
  LigmaContainer *container;
  gint           length;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  container = ligma_item_get_container (item);

  g_return_val_if_fail (container != NULL, FALSE);

  length = ligma_container_get_n_children (container);

  return ligma_image_reorder_item (image, item,
                                  ligma_item_get_parent (item), length - 1,
                                  TRUE, LIGMA_ITEM_GET_CLASS (item)->lower_to_bottom_desc);
}


/*  layers  */

gboolean
ligma_image_add_layer (LigmaImage *image,
                      LigmaLayer *layer,
                      LigmaLayer *parent,
                      gint       position,
                      gboolean   push_undo)
{
  LigmaImagePrivate *private;
  GList            *selected_layers;
  gboolean          old_has_alpha;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  /*  item and parent are type-checked in LigmaItemTree
   */
  if (! ligma_item_tree_get_insert_pos (private->layers,
                                       (LigmaItem *) layer,
                                       (LigmaItem **) &parent,
                                       &position))
    return FALSE;

  ligma_image_unset_default_new_layer_mode (image);

  /*  If there is a floating selection (and this isn't it!),
   *  make sure the insert position is greater than 0
   */
  if (parent == NULL && position == 0 &&
      ligma_image_get_floating_selection (image))
    position = 1;

  old_has_alpha = ligma_image_has_alpha (image);

  if (push_undo)
    ligma_image_undo_push_layer_add (image, C_("undo-type", "Add Layer"),
                                    layer,
                                    ligma_image_get_selected_layers (image));

  ligma_item_tree_add_item (private->layers, LIGMA_ITEM (layer),
                           LIGMA_ITEM (parent), position);

  selected_layers = g_list_prepend (NULL, layer);
  ligma_image_set_selected_layers (image, selected_layers);
  g_list_free (selected_layers);

  /*  If the layer is a floating selection, attach it to the drawable  */
  if (ligma_layer_is_floating_sel (layer))
    ligma_drawable_attach_floating_sel (ligma_layer_get_floating_sel_drawable (layer),
                                       layer);

  if (old_has_alpha != ligma_image_has_alpha (image))
    private->flush_accum.alpha_changed = TRUE;

  return TRUE;
}

void
ligma_image_remove_layer (LigmaImage *image,
                         LigmaLayer *layer,
                         gboolean   push_undo,
                         GList     *new_selected)
{
  LigmaImagePrivate *private;
  GList            *selected_layers;
  gboolean          old_has_alpha;
  const gchar      *undo_desc;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_LAYER (layer));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (layer)));
  g_return_if_fail (ligma_item_get_image (LIGMA_ITEM (layer)) == image);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  ligma_image_unset_default_new_layer_mode (image);

  if (push_undo)
    ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                 C_("undo-type", "Remove Layer"));

  ligma_item_start_move (LIGMA_ITEM (layer), push_undo);

  if (ligma_drawable_get_floating_sel (LIGMA_DRAWABLE (layer)))
    {
      if (! push_undo)
        {
          g_warning ("%s() was called from an undo function while the layer "
                     "had a floating selection. Please report this at "
                     "https://www.ligma.org/bugs/", G_STRFUNC);
          return;
        }

      ligma_image_remove_layer (image,
                               ligma_drawable_get_floating_sel (LIGMA_DRAWABLE (layer)),
                               TRUE, NULL);
    }

  selected_layers = g_list_copy (ligma_image_get_selected_layers (image));

  old_has_alpha = ligma_image_has_alpha (image);

  if (ligma_layer_is_floating_sel (layer))
    {
      undo_desc = C_("undo-type", "Remove Floating Selection");

      ligma_drawable_detach_floating_sel (ligma_layer_get_floating_sel_drawable (layer));
    }
  else
    {
      undo_desc = C_("undo-type", "Remove Layer");
    }

  if (push_undo)
    ligma_image_undo_push_layer_remove (image, undo_desc, layer,
                                       ligma_layer_get_parent (layer),
                                       ligma_item_get_index (LIGMA_ITEM (layer)),
                                       selected_layers);

  g_object_ref (layer);

  /*  Make sure we're not caching any old selection info  */
  if (g_list_find (selected_layers, layer))
    ligma_drawable_invalidate_boundary (LIGMA_DRAWABLE (layer));

  /* Remove layer and its children from the MRU layer stack. */
  ligma_image_remove_from_layer_stack (image, layer);

  new_selected = ligma_item_tree_remove_item (private->layers,
                                             LIGMA_ITEM (layer),
                                             new_selected);

  if (ligma_layer_is_floating_sel (layer))
    {
      /*  If this was the floating selection, activate the underlying drawable
       */
      floating_sel_activate_drawable (layer);
    }
  else if (selected_layers &&
           (g_list_find (selected_layers, layer) ||
            g_list_find_custom (selected_layers, layer,
                                (GCompareFunc) ligma_image_selected_is_descendant)))
    {
      ligma_image_set_selected_layers (image, new_selected);
    }

  ligma_item_end_move (LIGMA_ITEM (layer), push_undo);

  g_object_unref (layer);
  g_list_free (selected_layers);
  if (new_selected)
    g_list_free (new_selected);

  if (old_has_alpha != ligma_image_has_alpha (image))
    private->flush_accum.alpha_changed = TRUE;

  if (push_undo)
    ligma_image_undo_group_end (image);
}

void
ligma_image_add_layers (LigmaImage   *image,
                       GList       *layers,
                       LigmaLayer   *parent,
                       gint         position,
                       gint         x,
                       gint         y,
                       gint         width,
                       gint         height,
                       const gchar *undo_desc)
{
  LigmaImagePrivate *private;
  GList            *list;
  gint              layers_x      = G_MAXINT;
  gint              layers_y      = G_MAXINT;
  gint              layers_width  = 0;
  gint              layers_height = 0;
  gint              offset_x;
  gint              offset_y;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (layers != NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  /*  item and parent are type-checked in LigmaItemTree
   */
  if (! ligma_item_tree_get_insert_pos (private->layers,
                                       (LigmaItem *) layers->data,
                                       (LigmaItem **) &parent,
                                       &position))
    return;

  for (list = layers; list; list = g_list_next (list))
    {
      LigmaItem *item = LIGMA_ITEM (list->data);
      gint      off_x, off_y;

      ligma_item_get_offset (item, &off_x, &off_y);

      layers_x = MIN (layers_x, off_x);
      layers_y = MIN (layers_y, off_y);

      layers_width  = MAX (layers_width,
                           off_x + ligma_item_get_width (item)  - layers_x);
      layers_height = MAX (layers_height,
                           off_y + ligma_item_get_height (item) - layers_y);
    }

  offset_x = x + (width  - layers_width)  / 2 - layers_x;
  offset_y = y + (height - layers_height) / 2 - layers_y;

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_LAYER_ADD, undo_desc);

  for (list = layers; list; list = g_list_next (list))
    {
      LigmaItem *new_item = LIGMA_ITEM (list->data);

      ligma_item_translate (new_item, offset_x, offset_y, FALSE);

      ligma_image_add_layer (image, LIGMA_LAYER (new_item),
                            parent, position, TRUE);
      position++;
    }

  if (layers)
    ligma_image_set_selected_layers (image, layers);

  ligma_image_undo_group_end (image);
}

/*
 * ligma_image_store_item_set:
 * @image:
 * @set: (transfer full): a set of items which @images takes ownership
 *                        of.
 *
 * Store a new set of @layers.
 * If a set with the same name and type existed, this call will silently
 * replace it with the new set of layers.
 */
void
ligma_image_store_item_set (LigmaImage    *image,
                           LigmaItemList *set)
{
  LigmaImagePrivate  *private;
  GList            **stored_sets;
  GList             *iter;
  guint              signal;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_ITEM_LIST (set));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (ligma_item_list_get_item_type (set) == LIGMA_TYPE_LAYER)
    {
      stored_sets = &private->stored_layer_sets;
      signal = ligma_image_signals[LAYER_SETS_CHANGED];
    }
  else if (ligma_item_list_get_item_type (set) == LIGMA_TYPE_CHANNEL)
    {
      stored_sets = &private->stored_channel_sets;
      signal = ligma_image_signals[CHANNEL_SETS_CHANGED];
    }
  else if (ligma_item_list_get_item_type (set) == LIGMA_TYPE_VECTORS)
    {
      stored_sets = &private->stored_vectors_sets;
      signal = ligma_image_signals[VECTORS_SETS_CHANGED];
    }
  else
    {
      g_return_if_reached ();
    }

  for (iter = *stored_sets; iter; iter = iter->next)
    {
      gboolean         is_pattern;
      gboolean         is_pattern2;
      LigmaSelectMethod pattern_syntax;
      LigmaSelectMethod pattern_syntax2;

      is_pattern  = ligma_item_list_is_pattern (iter->data, &pattern_syntax);
      is_pattern2 = ligma_item_list_is_pattern (set, &pattern_syntax2);

      /* Remove a previous item set of same type and name. */
      if (is_pattern == is_pattern2 && (! is_pattern || pattern_syntax == pattern_syntax2) &&
          g_strcmp0 (ligma_object_get_name (iter->data), ligma_object_get_name (set)) == 0)
        break;
    }
  if (iter)
    {
      g_object_unref (iter->data);
      *stored_sets = g_list_delete_link (*stored_sets, iter);
    }

  *stored_sets = g_list_prepend (*stored_sets, set);
  g_signal_emit (image, signal, 0);
}

/*
 * @ligma_image_unlink_item_set:
 * @image:
 * @link_name:
 *
 * Remove the set of layers named @link_name.
 *
 * Returns: %TRUE if the set was removed, %FALSE if no sets with this
 *          name existed.
 */
gboolean
ligma_image_unlink_item_set (LigmaImage    *image,
                            LigmaItemList *set)
{
  LigmaImagePrivate  *private;
  GList             *found;
  GList            **stored_sets;
  guint              signal;
  gboolean           success;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (ligma_item_list_get_item_type (set) == LIGMA_TYPE_LAYER)
    {
      stored_sets = &private->stored_layer_sets;
      signal = ligma_image_signals[LAYER_SETS_CHANGED];
    }
  else if (ligma_item_list_get_item_type (set) == LIGMA_TYPE_CHANNEL)
    {
      stored_sets = &private->stored_channel_sets;
      signal = ligma_image_signals[CHANNEL_SETS_CHANGED];
    }
  else if (ligma_item_list_get_item_type (set) == LIGMA_TYPE_VECTORS)
    {
      stored_sets = &private->stored_vectors_sets;
      signal = ligma_image_signals[VECTORS_SETS_CHANGED];
    }
  else
    {
      g_return_val_if_reached (FALSE);
    }

  found = g_list_find (*stored_sets, set);
  success = (found != NULL);
  if (success)
    {
      *stored_sets = g_list_delete_link (*stored_sets, found);
      g_object_unref (set);
      g_signal_emit (image, signal, 0);
    }

  return success;
}

/*
 * @ligma_image_get_stored_item_sets:
 * @image:
 * @item_type:
 *
 * Returns: (transfer none): the list of all the layer sets (which you
 *          should not modify). Order of items is relevant.
 */
GList *
ligma_image_get_stored_item_sets (LigmaImage *image,
                                 GType      item_type)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (item_type == LIGMA_TYPE_LAYER)
    return private->stored_layer_sets;
  else if (item_type == LIGMA_TYPE_CHANNEL)
    return private->stored_channel_sets;
  else if (item_type == LIGMA_TYPE_VECTORS)
    return private->stored_vectors_sets;

  g_return_val_if_reached (FALSE);
}

/*
 * @ligma_image_select_item_set:
 * @image:
 * @set:
 *
 * Replace currently selected layers in @image with the layers belonging
 * to @set.
 */
void
ligma_image_select_item_set (LigmaImage    *image,
                            LigmaItemList *set)
{
  GList  *items;
  GError *error = NULL;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_ITEM_LIST (set));

  items = ligma_item_list_get_items (set, &error);

  if (! error)
    {
      GType item_type = ligma_item_list_get_item_type (set);

      if (item_type == LIGMA_TYPE_LAYER)
        ligma_image_set_selected_layers (image, items);
      else if (item_type == LIGMA_TYPE_CHANNEL)
        ligma_image_set_selected_channels (image, items);
      else if (item_type == LIGMA_TYPE_VECTORS)
        ligma_image_set_selected_vectors (image, items);
      else
        g_return_if_reached ();
    }

  g_list_free (items);
  g_clear_error (&error);
}

/*
 * @ligma_image_add_item_set:
 * @image:
 * @set:
 *
 * Add the layers belonging to @set to the items currently selected in
 * @image.
 */
void
ligma_image_add_item_set (LigmaImage    *image,
                         LigmaItemList *set)
{
  GList  *items;
  GError *error = NULL;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_ITEM_LIST (set));

  items = ligma_item_list_get_items (set, &error);

  if (! error)
    {
      GList *selected;
      GList *iter;
      GType  item_type = ligma_item_list_get_item_type (set);

      if (item_type == LIGMA_TYPE_LAYER)
        selected = ligma_image_get_selected_layers (image);
      else if (item_type == LIGMA_TYPE_CHANNEL)
        selected = ligma_image_get_selected_channels (image);
      else if (item_type == LIGMA_TYPE_VECTORS)
        selected = ligma_image_get_selected_vectors (image);
      else
        g_return_if_reached ();

      selected = g_list_copy (selected);
      for (iter = items; iter; iter = iter->next)
        {
          if (! g_list_find (selected, iter->data))
            selected = g_list_prepend (selected, iter->data);
        }

      if (item_type == LIGMA_TYPE_LAYER)
        ligma_image_set_selected_layers (image, selected);
      else if (item_type == LIGMA_TYPE_CHANNEL)
        ligma_image_set_selected_channels (image, selected);
      else if (item_type == LIGMA_TYPE_VECTORS)
        ligma_image_set_selected_vectors (image, items);

      g_list_free (selected);
    }
  g_clear_error (&error);
}

/*
 * @ligma_image_remove_item_set:
 * @image:
 * @set:
 *
 * Remove the layers belonging to the set named @link_name (which must
 * exist) from the layers currently selected in @image.
 *
 * Returns: %TRUE if the selection change is done (even if it turned out
 *          selected layers stay the same), %FALSE if no sets with this
 *          name existed.
 */
void
ligma_image_remove_item_set (LigmaImage    *image,
                            LigmaItemList *set)
{
  GList  *items;
  GError *error = NULL;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_ITEM_LIST (set));

  items = ligma_item_list_get_items (set, &error);

  if (! error)
    {
      GList *selected;
      GList *iter;
      GType  item_type = ligma_item_list_get_item_type (set);

      if (item_type == LIGMA_TYPE_LAYER)
        selected = ligma_image_get_selected_layers (image);
      else if (item_type == LIGMA_TYPE_CHANNEL)
        selected = ligma_image_get_selected_channels (image);
      else if (item_type == LIGMA_TYPE_VECTORS)
        selected = ligma_image_get_selected_vectors (image);
      else
        g_return_if_reached ();

      selected = g_list_copy (selected);
      for (iter = items; iter; iter = iter->next)
        {
          GList *remove;

          if ((remove = g_list_find (selected, iter->data)))
            selected = g_list_delete_link (selected, remove);
        }

      if (item_type == LIGMA_TYPE_LAYER)
        ligma_image_set_selected_layers (image, selected);
      else if (item_type == LIGMA_TYPE_CHANNEL)
        ligma_image_set_selected_channels (image, selected);
      else if (item_type == LIGMA_TYPE_VECTORS)
        ligma_image_set_selected_vectors (image, items);

      g_list_free (selected);
    }
  g_clear_error (&error);
}

/*
 * @ligma_image_intersect_item_set:
 * @image:
 * @set:
 *
 * Remove any layers from the layers currently selected in @image if
 * they don't also belong to the set named @link_name (which must
 * exist).
 *
 * Returns: %TRUE if the selection change is done (even if it turned out
 *          selected layers stay the same), %FALSE if no sets with this
 *          name existed.
 */
void
ligma_image_intersect_item_set (LigmaImage    *image,
                               LigmaItemList *set)
{
  GList *items;
  GError *error = NULL;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_ITEM_LIST (set));

  items = ligma_item_list_get_items (set, &error);

  if (! error)
    {
      GList *selected;
      GList *remove = NULL;
      GList *iter;
      GType  item_type = ligma_item_list_get_item_type (set);

      if (item_type == LIGMA_TYPE_LAYER)
        selected = ligma_image_get_selected_layers (image);
      else if (item_type == LIGMA_TYPE_CHANNEL)
        selected = ligma_image_get_selected_channels (image);
      else if (item_type == LIGMA_TYPE_VECTORS)
        selected = ligma_image_get_selected_vectors (image);
      else
        g_return_if_reached ();

      selected = g_list_copy (selected);

      /* Remove items in selected but not in items. */
      for (iter = selected; iter; iter = iter->next)
        {
          if (! g_list_find (items, iter->data))
            remove = g_list_prepend (remove, iter);
        }
      for (iter = remove; iter; iter = iter->next)
        selected = g_list_delete_link (selected, iter->data);
      g_list_free (remove);

      /* Finally select the intersection. */
      if (item_type == LIGMA_TYPE_LAYER)
        ligma_image_set_selected_layers (image, selected);
      else if (item_type == LIGMA_TYPE_CHANNEL)
        ligma_image_set_selected_channels (image, selected);
      else if (item_type == LIGMA_TYPE_VECTORS)
        ligma_image_set_selected_vectors (image, items);

      g_list_free (selected);
    }
  g_clear_error (&error);
}


/*  channels  */

gboolean
ligma_image_add_channel (LigmaImage   *image,
                        LigmaChannel *channel,
                        LigmaChannel *parent,
                        gint         position,
                        gboolean     push_undo)
{
  LigmaImagePrivate *private;
  GList            *channels;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  /*  item and parent are type-checked in LigmaItemTree
   */
  if (! ligma_item_tree_get_insert_pos (private->channels,
                                       (LigmaItem *) channel,
                                       (LigmaItem **) &parent,
                                       &position))
    return FALSE;

  if (push_undo)
    ligma_image_undo_push_channel_add (image, C_("undo-type", "Add Channel"),
                                      channel,
                                      ligma_image_get_selected_channels (image));

  ligma_item_tree_add_item (private->channels, LIGMA_ITEM (channel),
                           LIGMA_ITEM (parent), position);

  channels = g_list_prepend (NULL, channel);
  ligma_image_set_selected_channels (image, channels);
  g_list_free (channels);

  return TRUE;
}

void
ligma_image_remove_channel (LigmaImage   *image,
                           LigmaChannel *channel,
                           gboolean     push_undo,
                           GList       *new_selected)
{
  LigmaImagePrivate *private;
  GList            *selected_channels;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (channel)));
  g_return_if_fail (ligma_item_get_image (LIGMA_ITEM (channel)) == image);

  if (push_undo)
    ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                 C_("undo-type", "Remove Channel"));

  ligma_item_start_move (LIGMA_ITEM (channel), push_undo);

  if (ligma_drawable_get_floating_sel (LIGMA_DRAWABLE (channel)))
    {
      if (! push_undo)
        {
          g_warning ("%s() was called from an undo function while the channel "
                     "had a floating selection. Please report this at "
                     "https://www.ligma.org/bugs/", G_STRFUNC);
          return;
        }

      ligma_image_remove_layer (image,
                               ligma_drawable_get_floating_sel (LIGMA_DRAWABLE (channel)),
                               TRUE, NULL);
    }

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  selected_channels = ligma_image_get_selected_channels (image);
  selected_channels = g_list_copy (selected_channels);

  if (push_undo)
    ligma_image_undo_push_channel_remove (image, C_("undo-type", "Remove Channel"), channel,
                                         ligma_channel_get_parent (channel),
                                         ligma_item_get_index (LIGMA_ITEM (channel)),
                                         selected_channels);

  g_object_ref (channel);

  new_selected = ligma_item_tree_remove_item (private->channels,
                                             LIGMA_ITEM (channel),
                                             new_selected);

  if (selected_channels &&
      (g_list_find (selected_channels, channel) ||
       g_list_find_custom (selected_channels, channel,
                           (GCompareFunc) ligma_image_selected_is_descendant)))
    {
      if (new_selected)
        ligma_image_set_selected_channels (image, new_selected);
      else
        ligma_image_unset_selected_channels (image);
    }

  g_list_free (selected_channels);

  ligma_item_end_move (LIGMA_ITEM (channel), push_undo);

  g_object_unref (channel);
  if (new_selected)
    g_list_free (new_selected);

  if (push_undo)
    ligma_image_undo_group_end (image);
}


/*  vectors  */

gboolean
ligma_image_add_vectors (LigmaImage   *image,
                        LigmaVectors *vectors,
                        LigmaVectors *parent,
                        gint         position,
                        gboolean     push_undo)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  /*  item and parent are type-checked in LigmaItemTree
   */
  if (! ligma_item_tree_get_insert_pos (private->vectors,
                                       (LigmaItem *) vectors,
                                       (LigmaItem **) &parent,
                                       &position))
    return FALSE;

  if (push_undo)
    ligma_image_undo_push_vectors_add (image, C_("undo-type", "Add Path"),
                                      vectors,
                                      ligma_image_get_selected_vectors (image));

  ligma_item_tree_add_item (private->vectors, LIGMA_ITEM (vectors),
                           LIGMA_ITEM (parent), position);

  ligma_image_set_active_vectors (image, vectors);

  return TRUE;
}

void
ligma_image_remove_vectors (LigmaImage   *image,
                           LigmaVectors *vectors,
                           gboolean     push_undo,
                           GList       *new_selected)
{
  LigmaImagePrivate *private;
  GList            *selected_vectors;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_VECTORS (vectors));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (vectors)));
  g_return_if_fail (ligma_item_get_image (LIGMA_ITEM (vectors)) == image);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (push_undo)
    ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                 C_("undo-type", "Remove Path"));

  ligma_item_start_move (LIGMA_ITEM (vectors), push_undo);

  selected_vectors = ligma_image_get_selected_vectors (image);
  selected_vectors = g_list_copy (selected_vectors);

  if (push_undo)
    ligma_image_undo_push_vectors_remove (image, C_("undo-type", "Remove Path"), vectors,
                                         ligma_vectors_get_parent (vectors),
                                         ligma_item_get_index (LIGMA_ITEM (vectors)),
                                         selected_vectors);

  g_object_ref (vectors);

  new_selected = ligma_item_tree_remove_item (private->vectors,
                                             LIGMA_ITEM (vectors),
                                             new_selected);

  if (selected_vectors &&
      (g_list_find (selected_vectors, vectors) ||
       g_list_find_custom (selected_vectors, vectors,
                           (GCompareFunc) ligma_image_selected_is_descendant)))
    {
      ligma_image_set_selected_vectors (image, new_selected);
    }

  g_list_free (selected_vectors);

  ligma_item_end_move (LIGMA_ITEM (vectors), push_undo);

  g_object_unref (vectors);
  if (new_selected)
    g_list_free (new_selected);

  if (push_undo)
    ligma_image_undo_group_end (image);
}

/*  hidden items  */

/* Sometimes you want to create a channel or other types of drawables to
 * work on them without adding them to the public trees and to be
 * visible in the GUI. This is when you create hidden items. No undo
 * steps will ever be created either for any processing on these items.
 *
 * Note that you are expected to manage the hidden items properly. In
 * particular, once you are done with them, remove them with
 * ligma_image_remove_hidden_item() and free them.
 * @image is not assuming ownership of @item.
 */
gboolean
ligma_image_add_hidden_item (LigmaImage *image,
                            LigmaItem  *item)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (! ligma_item_is_attached (item), FALSE);
  g_return_val_if_fail (ligma_item_get_image (item) == image, FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  private->hidden_items = g_list_prepend (private->hidden_items, item);

  return TRUE;
}

void
ligma_image_remove_hidden_item (LigmaImage *image,
                               LigmaItem  *item)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (ligma_item_get_image (item) == image);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (g_list_find (private->hidden_items, item) != NULL);

  private->hidden_items = g_list_remove (private->hidden_items, item);
}

gboolean
ligma_image_is_hidden_item (LigmaImage *image,
                           LigmaItem  *item)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (ligma_item_get_image (item) == image, FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  return (g_list_find (private->hidden_items, item) != NULL);
}

gboolean
ligma_image_coords_in_active_pickable (LigmaImage        *image,
                                      const LigmaCoords *coords,
                                      gboolean          show_all,
                                      gboolean          sample_merged,
                                      gboolean          selected_only)
{
  gint     x, y;
  gboolean in_pickable = FALSE;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  x = floor (coords->x);
  y = floor (coords->y);

  if (sample_merged)
    {
      if (show_all || (x >= 0 && x < ligma_image_get_width  (image) &&
                       y >= 0 && y < ligma_image_get_height (image)))
        {
          in_pickable = TRUE;
        }
    }
  else
    {
      GList *drawables = ligma_image_get_selected_drawables (image);
      GList *iter;

      for (iter = drawables; iter; iter = iter->next)
        {
          LigmaItem *item = iter->data;
          gint      off_x, off_y;
          gint      d_x, d_y;

          ligma_item_get_offset (item, &off_x, &off_y);

          d_x = x - off_x;
          d_y = y - off_y;

          if (d_x >= 0 && d_x < ligma_item_get_width  (item) &&
              d_y >= 0 && d_y < ligma_item_get_height (item))
            {
              in_pickable = TRUE;
              break;
            }
        }
      g_list_free (drawables);
    }

  if (in_pickable && selected_only)
    {
      LigmaChannel *selection = ligma_image_get_mask (image);

      if (! ligma_channel_is_empty (selection) &&
          ! ligma_pickable_get_opacity_at (LIGMA_PICKABLE (selection),
                                          x, y))
        {
          in_pickable = FALSE;
        }
    }

  return in_pickable;
}

void
ligma_image_invalidate_previews (LigmaImage *image)
{
  LigmaItemStack *layers;
  LigmaItemStack *channels;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  layers   = LIGMA_ITEM_STACK (ligma_image_get_layers (image));
  channels = LIGMA_ITEM_STACK (ligma_image_get_channels (image));

  ligma_item_stack_invalidate_previews (layers);
  ligma_item_stack_invalidate_previews (channels);
}
