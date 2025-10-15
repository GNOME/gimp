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

#include <string.h>
#include <time.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <gexiv2/gexiv2.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-loops.h"

#include "gimp.h"
#include "gimp-memsize.h"
#include "gimp-parasites.h"
#include "gimp-utils.h"
#include "gimpcontext.h"
#include "gimpdrawable-filters.h"
#include "gimpdrawable-floating-selection.h"
#include "gimpdrawablefilter.h"
#include "gimpdrawablestack.h"
#include "gimpgrid.h"
#include "gimperror.h"
#include "gimpguide.h"
#include "gimpidtable.h"
#include "gimpimage.h"
#include "gimpimage-color-profile.h"
#include "gimpimage-colormap.h"
#include "gimpimage-guides.h"
#include "gimpimage-item-list.h"
#include "gimpimage-metadata.h"
#include "gimpimage-sample-points.h"
#include "gimpimage-preview.h"
#include "gimpimage-private.h"
#include "gimpimage-quick-mask.h"
#include "gimpimage-symmetry.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimpitemlist.h"
#include "gimpitemtree.h"
#include "gimplayer.h"
#include "gimplayer-floating-selection.h"
#include "gimplayermask.h"
#include "gimplayerstack.h"
#include "gimplinklayer.h"
#include "gimpmarshal.h"
#include "gimppalette.h"
#include "gimpparasitelist.h"
#include "gimppickable.h"
#include "gimpprojectable.h"
#include "gimpprojection.h"
#include "gimpsamplepoint.h"
#include "gimpselection.h"
#include "gimpsymmetry.h"
#include "gimptempbuf.h"
#include "gimptemplate.h"
#include "gimpundostack.h"

#include "text/gimptextlayer.h"

#include "path/gimppath.h"
#include "path/gimpvectorlayer.h"

#include "gimp-log.h"
#include "gimp-intl.h"


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
  SELECTED_PATHS_CHANGED,
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
  ITEM_SETS_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_GIMP,
  PROP_ID,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_BASE_TYPE,
  PROP_PRECISION,
  PROP_METADATA,
  PROP_BUFFER,
  PROP_SYMMETRY,
  PROP_CONVERTING,
};


/*  local function prototypes  */

static void     gimp_color_managed_iface_init    (GimpColorManagedInterface *iface);
static void     gimp_projectable_iface_init      (GimpProjectableInterface  *iface);
static void     gimp_pickable_iface_init         (GimpPickableInterface     *iface);

static void     gimp_image_constructed           (GObject           *object);
static void     gimp_image_set_property          (GObject           *object,
                                                  guint              property_id,
                                                  const GValue      *value,
                                                  GParamSpec        *pspec);
static void     gimp_image_get_property          (GObject           *object,
                                                  guint              property_id,
                                                  GValue            *value,
                                                  GParamSpec        *pspec);
static void     gimp_image_dispose               (GObject           *object);
static void     gimp_image_finalize              (GObject           *object);

static void     gimp_image_name_changed          (GimpObject        *object);
static gint64   gimp_image_get_memsize           (GimpObject        *object,
                                                  gint64            *gui_size);

static gboolean gimp_image_get_size              (GimpViewable      *viewable,
                                                  gint              *width,
                                                  gint              *height);
static void     gimp_image_size_changed          (GimpViewable      *viewable);
static gchar  * gimp_image_get_description       (GimpViewable      *viewable,
                                                  gchar            **tooltip);

static void     gimp_image_real_mode_changed     (GimpImage         *image);
static void     gimp_image_real_precision_changed(GimpImage         *image);
static void     gimp_image_real_resolution_changed(GimpImage        *image);
static void     gimp_image_real_size_changed_detailed
                                                 (GimpImage         *image,
                                                  gint               previous_origin_x,
                                                  gint               previous_origin_y,
                                                  gint               previous_width,
                                                  gint               previous_height);
static void     gimp_image_real_unit_changed     (GimpImage         *image);
static void     gimp_image_real_colormap_changed (GimpImage         *image,
                                                  gint               color_index);

static const guint8 *
        gimp_image_color_managed_get_icc_profile (GimpColorManaged  *managed,
                                                  gsize             *len);
static GimpColorProfile *
      gimp_image_color_managed_get_color_profile (GimpColorManaged  *managed);
static void
        gimp_image_color_managed_profile_changed (GimpColorManaged  *managed);

static GimpColorProfile *
      gimp_image_color_managed_get_simulation_profile     (GimpColorManaged  *managed);
static void
      gimp_image_color_managed_simulation_profile_changed (GimpColorManaged  *managed);

static GimpColorRenderingIntent
      gimp_image_color_managed_get_simulation_intent      (GimpColorManaged  *managed);
static void
      gimp_image_color_managed_simulation_intent_changed  (GimpColorManaged  *managed);

static gboolean
      gimp_image_color_managed_get_simulation_bpc         (GimpColorManaged  *managed);
static void
      gimp_image_color_managed_simulation_bpc_changed     (GimpColorManaged  *managed);

static void        gimp_image_projectable_flush  (GimpProjectable   *projectable,
                                                  gboolean           invalidate_preview);
static GeglRectangle gimp_image_get_bounding_box (GimpProjectable   *projectable);
static GeglNode   * gimp_image_get_graph         (GimpProjectable   *projectable);
static GimpImage  * gimp_image_get_image         (GimpProjectable   *projectable);
static const Babl * gimp_image_get_proj_format   (GimpProjectable   *projectable);

static void         gimp_image_pickable_flush    (GimpPickable      *pickable);
static GeglBuffer * gimp_image_get_buffer        (GimpPickable      *pickable);
static gboolean     gimp_image_get_pixel_at      (GimpPickable      *pickable,
                                                  gint               x,
                                                  gint               y,
                                                  const Babl        *format,
                                                  gpointer           pixel);
static gdouble      gimp_image_get_opacity_at    (GimpPickable      *pickable,
                                                  gint               x,
                                                  gint               y);
static void         gimp_image_get_pixel_average (GimpPickable      *pickable,
                                                  const GeglRectangle *rect,
                                                  const Babl        *format,
                                                  gpointer           pixel);

static void     gimp_image_projection_buffer_notify
                                                 (GimpProjection    *projection,
                                                  const GParamSpec  *pspec,
                                                  GimpImage         *image);
static void     gimp_image_mask_update           (GimpDrawable      *drawable,
                                                  gint               x,
                                                  gint               y,
                                                  gint               width,
                                                  gint               height,
                                                  GimpImage         *image);
static void     gimp_image_layers_changed        (GimpContainer     *container,
                                                  GimpChannel       *channel,
                                                  GimpImage         *image);
static void     gimp_image_layer_offset_changed  (GimpDrawable      *drawable,
                                                  const GParamSpec  *pspec,
                                                  GimpImage         *image);
static void     gimp_image_layer_bounding_box_changed
                                                 (GimpDrawable      *drawable,
                                                  GimpImage         *image);
static void     gimp_image_layer_alpha_changed   (GimpDrawable      *drawable,
                                                  GimpImage         *image);
static void     gimp_image_channel_add           (GimpContainer     *container,
                                                  GimpChannel       *channel,
                                                  GimpImage         *image);
static void     gimp_image_channel_remove        (GimpContainer     *container,
                                                  GimpChannel       *channel,
                                                  GimpImage         *image);
static void     gimp_image_channel_name_changed  (GimpChannel       *channel,
                                                  GimpImage         *image);
static void     gimp_image_channel_color_changed (GimpChannel       *channel,
                                                  GimpImage         *image);

static void     gimp_image_selected_layers_notify   (GimpItemTree      *tree,
                                                     const GParamSpec  *pspec,
                                                     GimpImage         *image);
static void     gimp_image_selected_channels_notify (GimpItemTree      *tree,
                                                     const GParamSpec  *pspec,
                                                     GimpImage         *image);
static void     gimp_image_selected_paths_notify    (GimpItemTree      *tree,
                                                     const GParamSpec  *pspec,
                                                     GimpImage         *image);

static void     gimp_image_freeze_bounding_box   (GimpImage         *image);
static void     gimp_image_thaw_bounding_box     (GimpImage         *image);
static void     gimp_image_update_bounding_box   (GimpImage         *image);

static gint     gimp_image_layer_stack_cmp         (GList           *layers1,
                                                    GList           *layers2);
static void gimp_image_rec_remove_layer_stack_dups (GimpImage       *image,
                                                    GSList          *start);
static void     gimp_image_clean_layer_stack       (GimpImage       *image);
static void     gimp_image_rec_filter_remove_undo  (GimpImage       *image,
                                                    GimpLayer       *layer);
static void     gimp_image_remove_from_layer_stack (GimpImage       *image,
                                                    GimpLayer       *layer);
static gint     gimp_image_selected_is_descendant  (GimpViewable    *selected,
                                                    GimpViewable    *viewable);

G_DEFINE_TYPE_WITH_CODE (GimpImage, gimp_image, GIMP_TYPE_VIEWABLE,
                         G_ADD_PRIVATE (GimpImage)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_COLOR_MANAGED,
                                                gimp_color_managed_iface_init)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PROJECTABLE,
                                                gimp_projectable_iface_init)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PICKABLE,
                                                gimp_pickable_iface_init))

#define parent_class gimp_image_parent_class

static guint gimp_image_signals[LAST_SIGNAL] = { 0 };


static void
gimp_image_class_init (GimpImageClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);

  gimp_image_signals[MODE_CHANGED] =
    g_signal_new ("mode-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, mode_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_image_signals[PRECISION_CHANGED] =
    g_signal_new ("precision-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, precision_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_image_signals[ALPHA_CHANGED] =
    g_signal_new ("alpha-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, alpha_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_image_signals[FLOATING_SELECTION_CHANGED] =
    g_signal_new ("floating-selection-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, floating_selection_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_image_signals[SELECTED_LAYERS_CHANGED] =
    g_signal_new ("selected-layers-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, selected_layers_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_image_signals[SELECTED_CHANNELS_CHANGED] =
    g_signal_new ("selected-channels-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, selected_channels_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_image_signals[SELECTED_PATHS_CHANGED] =
    g_signal_new ("selected-paths-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, selected_paths_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_image_signals[COMPONENT_VISIBILITY_CHANGED] =
    g_signal_new ("component-visibility-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, component_visibility_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_CHANNEL_TYPE);

  gimp_image_signals[COMPONENT_ACTIVE_CHANGED] =
    g_signal_new ("component-active-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, component_active_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_CHANNEL_TYPE);

  gimp_image_signals[MASK_CHANGED] =
    g_signal_new ("mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, mask_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_image_signals[RESOLUTION_CHANGED] =
    g_signal_new ("resolution-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, resolution_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_image_signals[SIZE_CHANGED_DETAILED] =
    g_signal_new ("size-changed-detailed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, size_changed_detailed),
                  NULL, NULL,
                  gimp_marshal_VOID__INT_INT_INT_INT,
                  G_TYPE_NONE, 4,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  gimp_image_signals[UNIT_CHANGED] =
    g_signal_new ("unit-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, unit_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_image_signals[QUICK_MASK_CHANGED] =
    g_signal_new ("quick-mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, quick_mask_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_image_signals[SELECTION_INVALIDATE] =
    g_signal_new ("selection-invalidate",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, selection_invalidate),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_image_signals[CLEAN] =
    g_signal_new ("clean",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, clean),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DIRTY_MASK);

  gimp_image_signals[DIRTY] =
    g_signal_new ("dirty",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, dirty),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DIRTY_MASK);

  gimp_image_signals[SAVING] =
    g_signal_new ("saving",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, saving),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_image_signals[SAVED] =
    g_signal_new ("saved",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, saved),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_FILE);

  gimp_image_signals[EXPORTED] =
    g_signal_new ("exported",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, exported),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_FILE);

  gimp_image_signals[GUIDE_ADDED] =
    g_signal_new ("guide-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, guide_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_GUIDE);

  gimp_image_signals[GUIDE_REMOVED] =
    g_signal_new ("guide-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, guide_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_GUIDE);

  gimp_image_signals[GUIDE_MOVED] =
    g_signal_new ("guide-moved",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, guide_moved),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_GUIDE);

  gimp_image_signals[SAMPLE_POINT_ADDED] =
    g_signal_new ("sample-point-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, sample_point_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_SAMPLE_POINT);

  gimp_image_signals[SAMPLE_POINT_REMOVED] =
    g_signal_new ("sample-point-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, sample_point_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_SAMPLE_POINT);

  gimp_image_signals[SAMPLE_POINT_MOVED] =
    g_signal_new ("sample-point-moved",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, sample_point_moved),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_SAMPLE_POINT);

  gimp_image_signals[PARASITE_ATTACHED] =
    g_signal_new ("parasite-attached",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, parasite_attached),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  gimp_image_signals[PARASITE_DETACHED] =
    g_signal_new ("parasite-detached",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, parasite_detached),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  gimp_image_signals[COLORMAP_CHANGED] =
    g_signal_new ("colormap-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, colormap_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  gimp_image_signals[UNDO_EVENT] =
    g_signal_new ("undo-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, undo_event),
                  NULL, NULL,
                  gimp_marshal_VOID__ENUM_OBJECT,
                  G_TYPE_NONE, 2,
                  GIMP_TYPE_UNDO_EVENT,
                  GIMP_TYPE_UNDO);

  gimp_image_signals[ITEM_SETS_CHANGED] =
    g_signal_new ("item-sets-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, item_sets_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_GTYPE);

  object_class->constructed           = gimp_image_constructed;
  object_class->set_property          = gimp_image_set_property;
  object_class->get_property          = gimp_image_get_property;
  object_class->dispose               = gimp_image_dispose;
  object_class->finalize              = gimp_image_finalize;

  gimp_object_class->name_changed     = gimp_image_name_changed;
  gimp_object_class->get_memsize      = gimp_image_get_memsize;

  viewable_class->default_icon_name   = "gimp-image";
  viewable_class->default_name        = _("Image");
  viewable_class->get_size            = gimp_image_get_size;
  viewable_class->size_changed        = gimp_image_size_changed;
  viewable_class->get_preview_size    = gimp_image_get_preview_size;
  viewable_class->get_popup_size      = gimp_image_get_popup_size;
  viewable_class->get_new_preview     = gimp_image_get_new_preview;
  viewable_class->get_new_pixbuf      = gimp_image_get_new_pixbuf;
  viewable_class->get_description     = gimp_image_get_description;

  klass->mode_changed                 = gimp_image_real_mode_changed;
  klass->precision_changed            = gimp_image_real_precision_changed;
  klass->alpha_changed                = NULL;
  klass->floating_selection_changed   = NULL;
  klass->selected_layers_changed      = NULL;
  klass->selected_channels_changed    = NULL;
  klass->selected_paths_changed       = NULL;
  klass->component_visibility_changed = NULL;
  klass->component_active_changed     = NULL;
  klass->mask_changed                 = NULL;
  klass->resolution_changed           = gimp_image_real_resolution_changed;
  klass->size_changed_detailed        = gimp_image_real_size_changed_detailed;
  klass->unit_changed                 = gimp_image_real_unit_changed;
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
  klass->colormap_changed             = gimp_image_real_colormap_changed;
  klass->undo_event                   = NULL;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp", NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_int ("id", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_WIDTH,
                                   g_param_spec_int ("width", NULL, NULL,
                                                     1, GIMP_MAX_IMAGE_SIZE, 1,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", NULL, NULL,
                                                     1, GIMP_MAX_IMAGE_SIZE, 1,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_BASE_TYPE,
                                   g_param_spec_enum ("base-type", NULL, NULL,
                                                      GIMP_TYPE_IMAGE_BASE_TYPE,
                                                      GIMP_RGB,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PRECISION,
                                   g_param_spec_enum ("precision", NULL, NULL,
                                                      GIMP_TYPE_PRECISION,
                                                      GIMP_PRECISION_U8_NON_LINEAR,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_METADATA,
                                   g_param_spec_object ("metadata", NULL, NULL,
                                                        GEXIV2_TYPE_METADATA,
                                                        GIMP_PARAM_READABLE));

  g_object_class_override_property (object_class, PROP_BUFFER, "buffer");

  g_object_class_install_property (object_class, PROP_SYMMETRY,
                                   g_param_spec_gtype ("symmetry",
                                                       NULL, _("Symmetry"),
                                                       GIMP_TYPE_SYMMETRY,
                                                       GIMP_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CONVERTING,
                                   g_param_spec_boolean ("converting",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));
}

static void
gimp_color_managed_iface_init (GimpColorManagedInterface *iface)
{
  iface->get_icc_profile            = gimp_image_color_managed_get_icc_profile;
  iface->get_color_profile          = gimp_image_color_managed_get_color_profile;
  iface->profile_changed            = gimp_image_color_managed_profile_changed;
  iface->get_simulation_profile     = gimp_image_color_managed_get_simulation_profile;
  iface->simulation_profile_changed = gimp_image_color_managed_simulation_profile_changed;
  iface->get_simulation_intent      = gimp_image_color_managed_get_simulation_intent;
  iface->simulation_intent_changed  = gimp_image_color_managed_simulation_intent_changed;
  iface->get_simulation_bpc         = gimp_image_color_managed_get_simulation_bpc;
  iface->simulation_bpc_changed     = gimp_image_color_managed_simulation_bpc_changed;
}

static void
gimp_projectable_iface_init (GimpProjectableInterface *iface)
{
  iface->flush              = gimp_image_projectable_flush;
  iface->get_image          = gimp_image_get_image;
  iface->get_format         = gimp_image_get_proj_format;
  iface->get_bounding_box   = gimp_image_get_bounding_box;
  iface->get_graph          = gimp_image_get_graph;
  iface->invalidate_preview = (void (*) (GimpProjectable*)) gimp_viewable_invalidate_preview;
}

static void
gimp_pickable_iface_init (GimpPickableInterface *iface)
{
  iface->flush                 = gimp_image_pickable_flush;
  iface->get_image             = (GimpImage  * (*) (GimpPickable *pickable)) gimp_image_get_image;
  iface->get_format            = (const Babl * (*) (GimpPickable *pickable)) gimp_image_get_proj_format;
  iface->get_format_with_alpha = (const Babl * (*) (GimpPickable *pickable)) gimp_image_get_proj_format;
  iface->get_buffer            = gimp_image_get_buffer;
  iface->get_pixel_at          = gimp_image_get_pixel_at;
  iface->get_opacity_at        = gimp_image_get_opacity_at;
  iface->get_pixel_average     = gimp_image_get_pixel_average;
}

static void
gimp_image_init (GimpImage *image)
{
  GimpImagePrivate *private = gimp_image_get_instance_private (image);
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
  private->resolution_unit     = gimp_unit_inch ();
  private->base_type           = GIMP_RGB;
  private->precision           = GIMP_PRECISION_U8_NON_LINEAR;
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
  private->simulation_intent   = GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
  private->simulation_bpc      = FALSE;

  private->dirty               = 1;
  private->dirty_time          = 0;
  private->undo_freeze_count   = 0;

  private->export_dirty        = 1;

  private->instance_count      = 0;
  private->disp_count          = 0;

  private->tattoo_state        = 0;

  private->projection          = gimp_projection_new (GIMP_PROJECTABLE (image));

  private->symmetries          = NULL;
  private->active_symmetry     = NULL;

  private->guides              = NULL;
  private->grid                = NULL;
  private->sample_points       = NULL;

  private->layers              = gimp_item_tree_new (image,
                                                     GIMP_TYPE_LAYER_STACK,
                                                     GIMP_TYPE_LAYER);
  private->channels            = gimp_item_tree_new (image,
                                                     GIMP_TYPE_DRAWABLE_STACK,
                                                     GIMP_TYPE_CHANNEL);
  private->paths               = gimp_item_tree_new (image,
                                                     GIMP_TYPE_ITEM_STACK,
                                                     GIMP_TYPE_PATH);
  private->layer_stack         = NULL;

  private->stored_layer_sets   = NULL;
  private->stored_channel_sets = NULL;
  private->stored_path_sets    = NULL;

  g_signal_connect (private->projection, "notify::buffer",
                    G_CALLBACK (gimp_image_projection_buffer_notify),
                    image);

  g_signal_connect (private->layers, "notify::selected-items",
                    G_CALLBACK (gimp_image_selected_layers_notify),
                    image);
  g_signal_connect (private->channels, "notify::selected-items",
                    G_CALLBACK (gimp_image_selected_channels_notify),
                    image);
  g_signal_connect (private->paths, "notify::selected-items",
                    G_CALLBACK (gimp_image_selected_paths_notify),
                    image);

  g_signal_connect_swapped (private->layers->container, "update",
                            G_CALLBACK (gimp_image_invalidate),
                            image);

  private->layer_offset_x_handler =
    gimp_container_add_handler (private->layers->container, "notify::offset-x",
                                G_CALLBACK (gimp_image_layer_offset_changed),
                                image);
  private->layer_offset_y_handler =
    gimp_container_add_handler (private->layers->container, "notify::offset-y",
                                G_CALLBACK (gimp_image_layer_offset_changed),
                                image);
  private->layer_bounding_box_handler =
    gimp_container_add_handler (private->layers->container, "bounding-box-changed",
                                G_CALLBACK (gimp_image_layer_bounding_box_changed),
                                image);
  private->layer_alpha_handler =
    gimp_container_add_handler (private->layers->container, "alpha-changed",
                                G_CALLBACK (gimp_image_layer_alpha_changed),
                                image);

  g_signal_connect (private->layers->container, "add",
                    G_CALLBACK (gimp_image_layers_changed),
                    image);
  g_signal_connect (private->layers->container, "remove",
                    G_CALLBACK (gimp_image_layers_changed),
                    image);

  g_signal_connect_swapped (private->channels->container, "update",
                            G_CALLBACK (gimp_image_invalidate),
                            image);

  private->channel_name_changed_handler =
    gimp_container_add_handler (private->channels->container, "name-changed",
                                G_CALLBACK (gimp_image_channel_name_changed),
                                image);
  private->channel_color_changed_handler =
    gimp_container_add_handler (private->channels->container, "color-changed",
                                G_CALLBACK (gimp_image_channel_color_changed),
                                image);

  g_signal_connect (private->channels->container, "add",
                    G_CALLBACK (gimp_image_channel_add),
                    image);
  g_signal_connect (private->channels->container, "remove",
                    G_CALLBACK (gimp_image_channel_remove),
                    image);

  private->floating_sel        = NULL;
  private->selection_mask      = NULL;

  private->parasites           = gimp_parasite_list_new ();

  for (i = 0; i < MAX_CHANNELS; i++)
    {
      private->visible[i] = TRUE;
      private->active[i]  = TRUE;
    }

  private->quick_mask_state    = FALSE;
  private->quick_mask_inverted = FALSE;
  /* Quick mask color set to sRGB red with half opacity by default. */
  private->quick_mask_color    = gegl_color_new ("red");
  gimp_color_set_alpha (private->quick_mask_color, 0.5);

  private->undo_stack          = gimp_undo_stack_new (image);
  private->redo_stack          = gimp_undo_stack_new (image);
  private->group_count         = 0;
  private->pushing_undo_group  = GIMP_UNDO_GROUP_NONE;

  private->flush_accum.alpha_changed              = FALSE;
  private->flush_accum.mask_changed               = FALSE;
  private->flush_accum.floating_selection_changed = FALSE;
  private->flush_accum.preview_invalidated        = FALSE;
}

static void
gimp_image_constructed (GObject *object)
{
  GimpImage        *image   = GIMP_IMAGE (object);
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);
  GimpChannel      *selection;
  GimpCoreConfig   *config;
  GimpTemplate     *template;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_GIMP (image->gimp));

  config = image->gimp->config;

  private->ID = gimp_id_table_insert (image->gimp->image_table, image);

  template = config->default_image;

  private->xresolution     = gimp_template_get_resolution_x (template);
  private->yresolution     = gimp_template_get_resolution_y (template);
  private->resolution_unit = gimp_template_get_resolution_unit (template);

  private->grid = gimp_config_duplicate (GIMP_CONFIG (config->default_grid));

  g_clear_object (&private->quick_mask_color);
  private->quick_mask_color = gegl_color_duplicate (config->quick_mask_color);

  gimp_image_update_bounding_box (image);

  if (private->base_type == GIMP_INDEXED)
    gimp_image_colormap_init (image);

  selection = gimp_selection_new (image,
                                  gimp_image_get_width  (image),
                                  gimp_image_get_height (image));
  gimp_image_take_mask (image, selection);

  g_signal_connect_object (config, "notify::transparency-type",
                           G_CALLBACK (gimp_item_stack_invalidate_previews),
                           private->layers->container, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::transparency-size",
                           G_CALLBACK (gimp_item_stack_invalidate_previews),
                           private->layers->container, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::transparency-custom-color1",
                           G_CALLBACK (gimp_item_stack_invalidate_previews),
                           private->layers->container, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::transparency-custom-color2",
                           G_CALLBACK (gimp_item_stack_invalidate_previews),
                           private->layers->container, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::layer-previews",
                           G_CALLBACK (gimp_viewable_size_changed),
                           image, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::group-layer-previews",
                           G_CALLBACK (gimp_viewable_size_changed),
                           image, G_CONNECT_SWAPPED);

  gimp_container_add (image->gimp->images, GIMP_OBJECT (image));
}

static void
gimp_image_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  GimpImage        *image   = GIMP_IMAGE (object);
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  switch (property_id)
    {
    case PROP_GIMP:
      image->gimp = g_value_get_object (value);
      break;

    case PROP_WIDTH:
      private->width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      private->height = g_value_get_int (value);
      break;

    case PROP_BASE_TYPE:
      private->base_type = g_value_get_enum (value);
      _gimp_image_free_color_transforms (image);
      break;

    case PROP_PRECISION:
      private->precision = g_value_get_enum (value);
      _gimp_image_free_color_transforms (image);
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
            GimpSymmetry *sym = iter->data;

            if (type == G_TYPE_FROM_INSTANCE (sym))
              private->active_symmetry = iter->data;
          }

        if (! private->active_symmetry &&
            g_type_is_a (type, GIMP_TYPE_SYMMETRY))
          {
            GimpSymmetry *sym = gimp_image_symmetry_new (image, type);

            gimp_image_symmetry_add (image, sym);
            g_object_unref (sym);

            private->active_symmetry = sym;
          }

        if (private->active_symmetry)
          g_object_set (private->active_symmetry,
                        "active", TRUE,
                        NULL);
      }
      break;

    case PROP_CONVERTING:
      private->converting = g_value_get_boolean (value);
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
gimp_image_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  GimpImage        *image   = GIMP_IMAGE (object);
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, image->gimp);
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
      g_value_set_object (value, gimp_image_get_metadata (image));
      break;
    case PROP_BUFFER:
      g_value_set_object (value, gimp_image_get_buffer (GIMP_PICKABLE (image)));
      break;
    case PROP_SYMMETRY:
      g_value_set_gtype (value,
                         private->active_symmetry ?
                         G_TYPE_FROM_INSTANCE (private->active_symmetry) :
                         G_TYPE_NONE);
      break;
    case PROP_CONVERTING:
      g_value_set_boolean (value, private->converting);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_image_dispose (GObject *object)
{
  GimpImage        *image   = GIMP_IMAGE (object);
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  if (private->palette)
    gimp_image_colormap_dispose (image);

  gimp_image_undo_free (image);

  g_list_free_full (private->stored_layer_sets,   g_object_unref);
  g_list_free_full (private->stored_channel_sets, g_object_unref);
  g_list_free_full (private->stored_path_sets,    g_object_unref);

  g_signal_handlers_disconnect_by_func (private->layers->container,
                                        gimp_image_invalidate,
                                        image);

  gimp_container_remove_handler (private->layers->container,
                                 private->layer_offset_x_handler);
  gimp_container_remove_handler (private->layers->container,
                                 private->layer_offset_y_handler);
  gimp_container_remove_handler (private->layers->container,
                                 private->layer_bounding_box_handler);
  gimp_container_remove_handler (private->layers->container,
                                 private->layer_alpha_handler);

  g_signal_handlers_disconnect_by_func (private->layers->container,
                                        gimp_image_layers_changed,
                                        image);

  g_signal_handlers_disconnect_by_func (private->channels->container,
                                        gimp_image_invalidate,
                                        image);

  gimp_container_remove_handler (private->channels->container,
                                 private->channel_name_changed_handler);
  gimp_container_remove_handler (private->channels->container,
                                 private->channel_color_changed_handler);

  g_signal_handlers_disconnect_by_func (private->channels->container,
                                        gimp_image_channel_add,
                                        image);
  g_signal_handlers_disconnect_by_func (private->channels->container,
                                        gimp_image_channel_remove,
                                        image);

  g_object_run_dispose (G_OBJECT (private->layers));
  g_object_run_dispose (G_OBJECT (private->channels));
  g_object_run_dispose (G_OBJECT (private->paths));

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_image_finalize (GObject *object)
{
  GimpImage        *image   = GIMP_IMAGE (object);
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  g_clear_object (&private->projection);
  g_clear_object (&private->graph);
  private->visible_mask = NULL;

  if (private->palette)
    gimp_image_colormap_free (image);

  _gimp_image_free_color_profile (image);

  g_clear_object (&private->pickable_buffer);
  g_clear_object (&private->metadata);
  g_clear_object (&private->file);
  g_clear_object (&private->imported_file);
  g_clear_object (&private->exported_file);
  g_clear_object (&private->save_a_copy_file);
  g_clear_object (&private->untitled_file);
  g_clear_object (&private->layers);
  g_clear_object (&private->channels);
  g_clear_object (&private->paths);
  g_clear_object (&private->quick_mask_color);
  g_clear_pointer (&private->quick_mask_selected, g_list_free);

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

  if (image->gimp && image->gimp->image_table)
    {
      gimp_id_table_remove (image->gimp->image_table, private->ID);
      image->gimp = NULL;
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
gimp_image_name_changed (GimpObject *object)
{
  GimpImage        *image   = GIMP_IMAGE (object);
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);
  const gchar      *name;

  if (GIMP_OBJECT_CLASS (parent_class)->name_changed)
    GIMP_OBJECT_CLASS (parent_class)->name_changed (object);

  g_clear_pointer (&private->display_name, g_free);
  g_clear_pointer (&private->display_path, g_free);

  /* We never want the empty string as a name, so change empty strings
   * to NULL strings (without emitting the "name-changed" signal
   * again)
   */
  name = gimp_object_get_name (object);
  if (name && strlen (name) == 0)
    {
      gimp_object_name_free (object);
      name = NULL;
    }

  g_clear_object (&private->file);

  if (name)
    private->file = g_file_new_for_uri (name);
}

static gint64
gimp_image_get_memsize (GimpObject *object,
                        gint64     *gui_size)
{
  GimpImage        *image   = GIMP_IMAGE (object);
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);
  gint64            memsize = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (private->palette),
                                      gui_size);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (private->projection),
                                      gui_size);

  memsize += gimp_g_list_get_memsize (gimp_image_get_guides (image),
                                      sizeof (GimpGuide));

  memsize += gimp_object_get_memsize (GIMP_OBJECT (private->grid), gui_size);

  memsize += gimp_g_list_get_memsize (gimp_image_get_sample_points (image),
                                      sizeof (GimpSamplePoint));

  memsize += gimp_object_get_memsize (GIMP_OBJECT (private->layers),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (private->channels),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (private->paths),
                                      gui_size);

  memsize += gimp_g_slist_get_memsize (private->layer_stack, 0);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (private->selection_mask),
                                      gui_size);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (private->parasites),
                                      gui_size);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (private->undo_stack),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (private->redo_stack),
                                      gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_image_get_size (GimpViewable *viewable,
                     gint         *width,
                     gint         *height)
{
  GimpImage *image = GIMP_IMAGE (viewable);

  *width  = gimp_image_get_width  (image);
  *height = gimp_image_get_height (image);

  return TRUE;
}

static void
gimp_image_size_changed (GimpViewable *viewable)
{
  GimpImage *image = GIMP_IMAGE (viewable);
  GList     *all_items;
  GList     *list;

  if (GIMP_VIEWABLE_CLASS (parent_class)->size_changed)
    GIMP_VIEWABLE_CLASS (parent_class)->size_changed (viewable);

  all_items = gimp_image_get_layer_list (image);
  for (list = all_items; list; list = g_list_next (list))
    {
      GimpLayerMask *mask = gimp_layer_get_mask (GIMP_LAYER (list->data));

      gimp_viewable_size_changed (GIMP_VIEWABLE (list->data));

      if (mask)
        gimp_viewable_size_changed (GIMP_VIEWABLE (mask));
    }
  g_list_free (all_items);

  all_items = gimp_image_get_channel_list (image);
  g_list_free_full (all_items, (GDestroyNotify) gimp_viewable_size_changed);

  all_items = gimp_image_get_path_list (image);
  g_list_free_full (all_items, (GDestroyNotify) gimp_viewable_size_changed);

  gimp_viewable_size_changed (GIMP_VIEWABLE (gimp_image_get_mask (image)));

  gimp_image_metadata_update_pixel_size (image);

  g_clear_object (&GIMP_IMAGE_GET_PRIVATE (image)->pickable_buffer);

  gimp_image_update_bounding_box (image);
}

static gchar *
gimp_image_get_description (GimpViewable  *viewable,
                            gchar        **tooltip)
{
  GimpImage *image = GIMP_IMAGE (viewable);

  if (tooltip)
    *tooltip = g_strdup (gimp_image_get_display_path (image));

  return g_strdup_printf ("%s-%d",
                          gimp_image_get_display_name (image),
                          gimp_image_get_id (image));
}

static void
gimp_image_real_mode_changed (GimpImage *image)
{
  gimp_projectable_structure_changed (GIMP_PROJECTABLE (image));
}

static void
gimp_image_real_precision_changed (GimpImage *image)
{
  gimp_image_metadata_update_bits_per_sample (image);

  gimp_projectable_structure_changed (GIMP_PROJECTABLE (image));
}

static void
gimp_image_real_resolution_changed (GimpImage *image)
{
  gimp_image_metadata_update_resolution (image);
}

static void
gimp_image_real_size_changed_detailed (GimpImage *image,
                                       gint       previous_origin_x,
                                       gint       previous_origin_y,
                                       gint       previous_width,
                                       gint       previous_height)
{
  /* Whenever GimpImage::size-changed-detailed is emitted, so is
   * GimpViewable::size-changed. Clients choose what signal to listen
   * to depending on how much info they need.
   */
  gimp_viewable_size_changed (GIMP_VIEWABLE (image));
}

static void
gimp_image_real_unit_changed (GimpImage *image)
{
  gimp_image_metadata_update_resolution (image);
}

static void
gimp_image_real_colormap_changed (GimpImage *image,
                                  gint       color_index)
{
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  gimp_image_colormap_update_formats (image);

  if (gimp_image_get_base_type (image) == GIMP_INDEXED)
    {
      /* A colormap alteration affects the whole image */
      gimp_image_invalidate_all (image);

      gimp_item_stack_invalidate_previews (GIMP_ITEM_STACK (private->layers->container));
    }
}

static const guint8 *
gimp_image_color_managed_get_icc_profile (GimpColorManaged *managed,
                                          gsize            *len)
{
  return gimp_image_get_icc_profile (GIMP_IMAGE (managed), len);
}

static GimpColorProfile *
gimp_image_color_managed_get_color_profile (GimpColorManaged *managed)
{
  GimpImage        *image = GIMP_IMAGE (managed);
  GimpColorProfile *profile;

  profile = gimp_image_get_color_profile (image);

  if (! profile)
    profile = gimp_image_get_builtin_color_profile (image);

  return profile;
}

static void
gimp_image_color_managed_profile_changed (GimpColorManaged *managed)
{
  GimpImage     *image  = GIMP_IMAGE (managed);
  GimpItemStack *layers = GIMP_ITEM_STACK (gimp_image_get_layers (image));

  gimp_image_metadata_update_colorspace (image);

  gimp_projectable_structure_changed (GIMP_PROJECTABLE (image));
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (image));
  gimp_item_stack_profile_changed (layers);
}

static GimpColorProfile *
gimp_image_color_managed_get_simulation_profile (GimpColorManaged *managed)
{
  GimpImage        *image = GIMP_IMAGE (managed);
  GimpColorProfile *profile;

  profile = gimp_image_get_simulation_profile (image);

  return profile;
}

static void
gimp_image_color_managed_simulation_profile_changed (GimpColorManaged *managed)
{
  GimpImage *image = GIMP_IMAGE (managed);

  gimp_projectable_structure_changed (GIMP_PROJECTABLE (image));
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (image));
}

static GimpColorRenderingIntent
gimp_image_color_managed_get_simulation_intent (GimpColorManaged *managed)
{
  GimpImage *image = GIMP_IMAGE (managed);

  return gimp_image_get_simulation_intent (image);
}

static void
gimp_image_color_managed_simulation_intent_changed (GimpColorManaged *managed)
{
  GimpImage *image = GIMP_IMAGE (managed);

  gimp_projectable_structure_changed (GIMP_PROJECTABLE (image));
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (image));
}

static gboolean
gimp_image_color_managed_get_simulation_bpc (GimpColorManaged *managed)
{
  GimpImage *image = GIMP_IMAGE (managed);

  return gimp_image_get_simulation_bpc (image);
}

static void
gimp_image_color_managed_simulation_bpc_changed (GimpColorManaged *managed)
{
  GimpImage *image = GIMP_IMAGE (managed);

  gimp_projectable_structure_changed (GIMP_PROJECTABLE (image));
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (image));
}

static void
gimp_image_projectable_flush (GimpProjectable *projectable,
                              gboolean         invalidate_preview)
{
  GimpImage        *image   = GIMP_IMAGE (projectable);
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  if (private->flush_accum.alpha_changed)
    {
      gimp_image_alpha_changed (image);
      private->flush_accum.alpha_changed = FALSE;
    }

  if (private->flush_accum.mask_changed)
    {
      gimp_image_mask_changed (image);
      private->flush_accum.mask_changed = FALSE;
    }

  if (private->flush_accum.floating_selection_changed)
    {
      gimp_image_floating_selection_changed (image);
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

static GimpImage *
gimp_image_get_image (GimpProjectable *projectable)
{
  return GIMP_IMAGE (projectable);
}

static const Babl *
gimp_image_get_proj_format (GimpProjectable *projectable)
{
  GimpImage        *image   = GIMP_IMAGE (projectable);
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  switch (private->base_type)
    {
    case GIMP_RGB:
    case GIMP_INDEXED:
      return gimp_image_get_format (image, GIMP_RGB,
                                    gimp_image_get_precision (image), TRUE,
                                    gimp_image_get_layer_space (image));

    case GIMP_GRAY:
      return gimp_image_get_format (image, GIMP_GRAY,
                                    gimp_image_get_precision (image), TRUE,
                                    gimp_image_get_layer_space (image));
    }

  g_return_val_if_reached (NULL);
}

static void
gimp_image_pickable_flush (GimpPickable *pickable)
{
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (pickable);

  return gimp_pickable_flush (GIMP_PICKABLE (private->projection));
}

static GeglBuffer *
gimp_image_get_buffer (GimpPickable *pickable)
{
  GimpImage        *image   = GIMP_IMAGE (pickable);
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  if (! private->pickable_buffer)
    {
      GeglBuffer *buffer;

      buffer = gimp_pickable_get_buffer (GIMP_PICKABLE (private->projection));

      if (! private->show_all)
        {
          private->pickable_buffer = g_object_ref (buffer);
        }
      else
        {
          private->pickable_buffer = gegl_buffer_create_sub_buffer (
            buffer,
            GEGL_RECTANGLE (0, 0,
                            gimp_image_get_width  (image),
                            gimp_image_get_height (image)));
        }
    }

  return private->pickable_buffer;
}

static gboolean
gimp_image_get_pixel_at (GimpPickable *pickable,
                         gint          x,
                         gint          y,
                         const Babl   *format,
                         gpointer      pixel)
{
  GimpImage        *image   = GIMP_IMAGE (pickable);
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  if (x >= 0                            &&
      y >= 0                            &&
      x < gimp_image_get_width  (image) &&
      y < gimp_image_get_height (image))
    {
      return gimp_pickable_get_pixel_at (GIMP_PICKABLE (private->projection),
                                         x, y, format, pixel);
    }

  return FALSE;
}

static gdouble
gimp_image_get_opacity_at (GimpPickable *pickable,
                           gint          x,
                           gint          y)
{
  GimpImage        *image   = GIMP_IMAGE (pickable);
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  if (x >= 0                            &&
      y >= 0                            &&
      x < gimp_image_get_width  (image) &&
      y < gimp_image_get_height (image))
    {
      return gimp_pickable_get_opacity_at (GIMP_PICKABLE (private->projection),
                                           x, y);
    }

  return FALSE;
}

static void
gimp_image_get_pixel_average (GimpPickable        *pickable,
                              const GeglRectangle *rect,
                              const Babl          *format,
                              gpointer             pixel)
{
  GeglBuffer *buffer = gimp_pickable_get_buffer (pickable);

  return gimp_gegl_average_color (buffer, rect, TRUE, GEGL_ABYSS_NONE, format,
                                  pixel);
}

static GeglRectangle
gimp_image_get_bounding_box (GimpProjectable *projectable)
{
  GimpImage *image = GIMP_IMAGE (projectable);

  return GIMP_IMAGE_GET_PRIVATE (image)->bounding_box;
}

static GeglNode *
gimp_image_get_graph (GimpProjectable *projectable)
{
  GimpImage         *image   = GIMP_IMAGE (projectable);
  GimpImagePrivate  *private = GIMP_IMAGE_GET_PRIVATE (image);
  GeglNode          *layers_node;
  GeglNode          *channels_node;
  GeglNode          *output;
  GimpComponentMask  mask;

  if (private->graph)
    return private->graph;

  private->graph = gegl_node_new ();

  layers_node =
    gimp_filter_stack_get_graph (GIMP_FILTER_STACK (private->layers->container));

  gegl_node_add_child (private->graph, layers_node);

  mask = ~gimp_image_get_visible_mask (image) & GIMP_COMPONENT_MASK_ALL;

  private->visible_mask =
    gegl_node_new_child (private->graph,
                         "operation", "gimp:mask-components",
                         "mask",      mask,
                         "alpha",     1.0,
                         NULL);

  gegl_node_link (layers_node, private->visible_mask);

  channels_node =
    gimp_filter_stack_get_graph (GIMP_FILTER_STACK (private->channels->container));

  gegl_node_add_child (private->graph, channels_node);

  gegl_node_link (private->visible_mask, channels_node);

  output = gegl_node_get_output_proxy (private->graph, "output");

  gegl_node_link (channels_node, output);

  return private->graph;
}

static void
gimp_image_projection_buffer_notify (GimpProjection   *projection,
                                     const GParamSpec *pspec,
                                     GimpImage        *image)
{
  g_clear_object (&GIMP_IMAGE_GET_PRIVATE (image)->pickable_buffer);
}

static void
gimp_image_mask_update (GimpDrawable *drawable,
                        gint          x,
                        gint          y,
                        gint          width,
                        gint          height,
                        GimpImage    *image)
{
  GIMP_IMAGE_GET_PRIVATE (image)->flush_accum.mask_changed = TRUE;
}

static void
gimp_image_layers_changed (GimpContainer *container,
                           GimpChannel   *channel,
                           GimpImage     *image)
{
  gimp_image_update_bounding_box (image);
}

static void
gimp_image_layer_offset_changed (GimpDrawable     *drawable,
                                 const GParamSpec *pspec,
                                 GimpImage        *image)
{
  gimp_image_update_bounding_box (image);
}

static void
gimp_image_layer_bounding_box_changed (GimpDrawable *drawable,
                                       GimpImage    *image)
{
  gimp_image_update_bounding_box (image);
}

static void
gimp_image_layer_alpha_changed (GimpDrawable *drawable,
                                GimpImage    *image)
{
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  if (gimp_container_get_n_children (private->layers->container) == 1)
    private->flush_accum.alpha_changed = TRUE;
}

static void
gimp_image_channel_add (GimpContainer *container,
                        GimpChannel   *channel,
                        GimpImage     *image)
{
  if (! strcmp (GIMP_IMAGE_QUICK_MASK_NAME,
                gimp_object_get_name (channel)))
    {
      gimp_image_set_quick_mask_state (image, TRUE);
    }
}

static void
gimp_image_channel_remove (GimpContainer *container,
                           GimpChannel   *channel,
                           GimpImage     *image)
{
  if (! strcmp (GIMP_IMAGE_QUICK_MASK_NAME,
                gimp_object_get_name (channel)))
    {
      gimp_image_set_quick_mask_state (image, FALSE);
    }
}

static void
gimp_image_channel_name_changed (GimpChannel *channel,
                                 GimpImage   *image)
{
  if (! strcmp (GIMP_IMAGE_QUICK_MASK_NAME,
                gimp_object_get_name (channel)))
    {
      gimp_image_set_quick_mask_state (image, TRUE);
    }
  else if (gimp_image_get_quick_mask_state (image) &&
           ! gimp_image_get_quick_mask (image))
    {
      gimp_image_set_quick_mask_state (image, FALSE);
    }
}

static void
gimp_image_channel_color_changed (GimpChannel *channel,
                                  GimpImage   *image)
{
  if (! strcmp (GIMP_IMAGE_QUICK_MASK_NAME,
                gimp_object_get_name (channel)))
    {
      g_clear_object (&(GIMP_IMAGE_GET_PRIVATE (image)->quick_mask_color));
      GIMP_IMAGE_GET_PRIVATE (image)->quick_mask_color = gegl_color_duplicate (channel->color);
    }
}

static void
gimp_image_selected_layers_notify (GimpItemTree     *tree,
                                   const GParamSpec *pspec,
                                   GimpImage        *image)
{
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);
  GList            *layers  = gimp_image_get_selected_layers (image);

  if (layers)
    {
      /*  Configure the layer stack to reflect this change  */
      GSList *prev_layers;

      while ((prev_layers = g_slist_find_custom (private->layer_stack, layers,
                                                (GCompareFunc) gimp_image_layer_stack_cmp)))
        {
          g_list_free (prev_layers->data);
          private->layer_stack = g_slist_delete_link (private->layer_stack,
                                                      prev_layers);
        }
      private->layer_stack = g_slist_prepend (private->layer_stack, g_list_copy (layers));
    }

  if (layers && gimp_image_get_selected_channels (image))
    gimp_image_set_selected_channels (image, NULL);

  g_signal_emit (image, gimp_image_signals[SELECTED_LAYERS_CHANGED], 0);
}

static void
gimp_image_selected_channels_notify (GimpItemTree     *tree,
                                     const GParamSpec *pspec,
                                     GimpImage        *image)
{
  GList *channels = gimp_image_get_selected_channels (image);

  g_signal_emit (image, gimp_image_signals[SELECTED_CHANNELS_CHANGED], 0);

  if (channels && gimp_image_get_selected_layers (image))
    gimp_image_set_selected_layers (image, NULL);
}

static void
gimp_image_selected_paths_notify (GimpItemTree     *tree,
                                  const GParamSpec *pspec,
                                  GimpImage        *image)
{
  g_signal_emit (image, gimp_image_signals[SELECTED_PATHS_CHANGED], 0);
}

static void
gimp_image_freeze_bounding_box (GimpImage *image)
{
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  private->bounding_box_freeze_count++;
}

static void
gimp_image_thaw_bounding_box (GimpImage *image)
{
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  private->bounding_box_freeze_count--;

  if (private->bounding_box_freeze_count == 0 &&
      private->bounding_box_update_pending)
    {
      private->bounding_box_update_pending = FALSE;

      gimp_image_update_bounding_box (image);
    }
}

static void
gimp_image_update_bounding_box (GimpImage *image)
{
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);
  GeglRectangle     bounding_box;

  if (private->bounding_box_freeze_count > 0)
    {
      private->bounding_box_update_pending = TRUE;

      return;
    }

  bounding_box.x      = 0;
  bounding_box.y      = 0;
  bounding_box.width  = gimp_image_get_width  (image);
  bounding_box.height = gimp_image_get_height (image);

  if (private->show_all)
    {
      GList *iter;

      for (iter = gimp_image_get_layer_iter (image);
           iter;
           iter = g_list_next (iter))
        {
          GimpLayer     *layer = iter->data;
          GeglRectangle  layer_bounding_box;
          gint           offset_x;
          gint           offset_y;

          gimp_item_get_offset (GIMP_ITEM (layer), &offset_x, &offset_y);

          layer_bounding_box = gimp_drawable_get_bounding_box (
            GIMP_DRAWABLE (layer));

          layer_bounding_box.x += offset_x;
          layer_bounding_box.y += offset_y;

          gegl_rectangle_bounding_box (&bounding_box,
                                       &bounding_box, &layer_bounding_box);
        }
    }

  if (! gegl_rectangle_equal (&bounding_box, &private->bounding_box))
    {
      private->bounding_box = bounding_box;

      gimp_projectable_bounds_changed (GIMP_PROJECTABLE (image), 0, 0);
    }
}

static gint
gimp_image_layer_stack_cmp (GList *layers1,
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
 * gimp_image_rec_remove_layer_stack_dups:
 * @image:
 *
 * Recursively remove duplicates from the layer stack. You should not
 * call this directly, call gimp_image_clean_layer_stack() instead.
 */
static void
gimp_image_rec_remove_layer_stack_dups (GimpImage *image,
                                        GSList    *start)
{
  GimpImagePrivate *private;
  GSList           *dup_layers;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (start == NULL || start->next == NULL)
    return;

  while ((dup_layers = g_slist_find_custom (start->next, start->data,
                                            (GCompareFunc) gimp_image_layer_stack_cmp)))
    {
      g_list_free (dup_layers->data);
      /* We can safely remove the duplicate then search again because we
       * know that @start is never removed as we search after it.
       */
      private->layer_stack = g_slist_delete_link (private->layer_stack,
                                                  dup_layers);
    }

  gimp_image_rec_remove_layer_stack_dups (image, start->next);
}

/**
 * gimp_image_clean_layer_stack:
 * @image:
 *
 * Remove any duplicate and empty selections in the layer stack.
 */
static void
gimp_image_clean_layer_stack (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  /* First remove all empty layer lists. */
  private->layer_stack = g_slist_remove_all (private->layer_stack, NULL);
  /* Then remove all duplicates. */
  gimp_image_rec_remove_layer_stack_dups (image, private->layer_stack);
}

static void
gimp_image_rec_filter_remove_undo (GimpImage *image,
                                   GimpLayer *layer)
{
  GimpContainer *filters;

  if (gimp_viewable_get_children (GIMP_VIEWABLE (layer)))
    {
      GimpContainer *stack = gimp_viewable_get_children (GIMP_VIEWABLE (layer));
      GList         *children;
      GList         *iter;

      children = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (stack));

      for (iter = children; iter; iter = iter->next)
        {
          GimpLayer *child = iter->data;

          gimp_image_rec_filter_remove_undo (image, child);
        }
    }

  filters = gimp_drawable_get_filters (GIMP_DRAWABLE (layer));

  if (gimp_container_get_n_children (filters) > 0)
    {
      GList *filter_list;

      for (filter_list = GIMP_LIST (filters)->queue->tail; filter_list;
           filter_list = g_list_previous (filter_list))
        {
          if (GIMP_IS_DRAWABLE_FILTER (filter_list->data) &&
              ! gimp_drawable_filter_get_temporary (filter_list->data))
            {
              GimpDrawableFilter *filter = filter_list->data;

              gimp_image_undo_push_filter_remove (image,
                                                  _("Remove filter"),
                                                  GIMP_DRAWABLE (layer),
                                                  filter);
            }
        }
    }
}

static void
gimp_image_remove_from_layer_stack (GimpImage *image,
                                    GimpLayer *layer)
{
  GimpImagePrivate *private;
  GSList           *slist;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_LAYER (layer));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  /* Remove layer itself from the MRU layer stack. */
  for (slist = private->layer_stack; slist; slist = slist->next)
    slist->data = g_list_remove (slist->data, layer);

  /*  Also remove all children of a group layer from the layer_stack  */
  if (gimp_viewable_get_children (GIMP_VIEWABLE (layer)))
    {
      GimpContainer *stack = gimp_viewable_get_children (GIMP_VIEWABLE (layer));
      GList         *children;
      GList         *list;

      children = gimp_item_stack_get_item_list (GIMP_ITEM_STACK (stack));

      for (list = children; list; list = g_list_next (list))
        {
          GimpLayer *child = list->data;

          for (slist = private->layer_stack; slist; slist = slist->next)
            slist->data = g_list_remove (slist->data, child);
        }

      g_list_free (children);
    }

  gimp_image_clean_layer_stack (image);
}

static gint
gimp_image_selected_is_descendant (GimpViewable *selected,
                                   GimpViewable *viewable)
{
  /* Used as a GCompareFunc to g_list_find_custom() in order to know if
   * one of the selected items is a descendant to @viewable.
   */
  if (gimp_viewable_is_ancestor (viewable, selected))
    return 0;
  else
    return 1;
}


/*  public functions  */

GimpImage *
gimp_image_new (Gimp              *gimp,
                gint               width,
                gint               height,
                GimpImageBaseType  base_type,
                GimpPrecision      precision)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (gimp_babl_is_valid (base_type, precision), NULL);

  return g_object_new (GIMP_TYPE_IMAGE,
                       "gimp",      gimp,
                       "width",     width,
                       "height",    height,
                       "base-type", base_type,
                       "precision", precision,
                       NULL);
}

gint64
gimp_image_estimate_memsize (GimpImage         *image,
                             GimpComponentType  component_type,
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

  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  current_width  = gimp_image_get_width (image);
  current_height = gimp_image_get_height (image);
  current_size   = gimp_object_get_memsize (GIMP_OBJECT (image), NULL);

  /*  the part of the image's memsize that scales linearly with the image  */
  drawables = gimp_image_item_list_get_list (image,
                                             GIMP_ITEM_TYPE_LAYERS |
                                             GIMP_ITEM_TYPE_CHANNELS,
                                             GIMP_ITEM_SET_ALL);

  gimp_image_item_list_filter (drawables);

  drawables = g_list_prepend (drawables, gimp_image_get_mask (image));

  for (list = drawables; list; list = g_list_next (list))
    {
      GimpDrawable *drawable = list->data;
      gdouble       drawable_width;
      gdouble       drawable_height;

      drawable_width  = gimp_item_get_width  (GIMP_ITEM (drawable));
      drawable_height = gimp_item_get_height (GIMP_ITEM (drawable));

      scalable_size += gimp_drawable_estimate_memsize (drawable,
                                                       gimp_drawable_get_component_type (drawable),
                                                       drawable_width,
                                                       drawable_height);

      scaled_size += gimp_drawable_estimate_memsize (drawable,
                                                     component_type,
                                                     drawable_width * width /
                                                     current_width,
                                                     drawable_height * height /
                                                     current_height);
    }

  g_list_free (drawables);

  scalable_size +=
    gimp_projection_estimate_memsize (gimp_image_get_base_type (image),
                                      gimp_image_get_component_type (image),
                                      gimp_image_get_width (image),
                                      gimp_image_get_height (image));

  scaled_size +=
    gimp_projection_estimate_memsize (gimp_image_get_base_type (image),
                                      component_type,
                                      width, height);

  GIMP_LOG (IMAGE_SCALE,
            "scalable_size = %"G_GINT64_FORMAT"  scaled_size = %"G_GINT64_FORMAT,
            scalable_size, scaled_size);

  new_size = current_size - scalable_size + scaled_size;

  return new_size;
}

GimpImageBaseType
gimp_image_get_base_type (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), -1);

  return GIMP_IMAGE_GET_PRIVATE (image)->base_type;
}

GimpComponentType
gimp_image_get_component_type (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), -1);

  return gimp_babl_component_type (GIMP_IMAGE_GET_PRIVATE (image)->precision);
}

GimpPrecision
gimp_image_get_precision (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), -1);

  return GIMP_IMAGE_GET_PRIVATE (image)->precision;
}

const Babl *
gimp_image_get_format (GimpImage         *image,
                       GimpImageBaseType  base_type,
                       GimpPrecision      precision,
                       gboolean           with_alpha,
                       const Babl        *space)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  switch (base_type)
    {
    case GIMP_RGB:
    case GIMP_GRAY:
      return gimp_babl_format (base_type, precision, with_alpha, space);

    case GIMP_INDEXED:
      if (precision == GIMP_PRECISION_U8_NON_LINEAR)
        {
          if (with_alpha)
            return gimp_image_colormap_get_rgba_format (image);
          else
            return gimp_image_colormap_get_rgb_format (image);
        }
    }

  g_return_val_if_reached (NULL);
}

const Babl *
gimp_image_get_layer_space (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->layer_space;
}

const Babl *
gimp_image_get_layer_format (GimpImage *image,
                             gboolean   with_alpha)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return gimp_image_get_format (image,
                                gimp_image_get_base_type (image),
                                gimp_image_get_precision (image),
                                with_alpha,
                                gimp_image_get_layer_space (image));
}

const Babl *
gimp_image_get_channel_format (GimpImage *image)
{
  GimpPrecision precision;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  precision = gimp_image_get_precision (image);

  if (precision == GIMP_PRECISION_U8_NON_LINEAR)
    return gimp_image_get_format (image, GIMP_GRAY,
                                  gimp_image_get_precision (image),
                                  FALSE, NULL);

  return gimp_babl_mask_format (precision);
}

const Babl *
gimp_image_get_mask_format (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return gimp_babl_mask_format (gimp_image_get_precision (image));
}

GimpLayerMode
gimp_image_get_default_new_layer_mode (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), GIMP_LAYER_MODE_NORMAL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (private->new_layer_mode == -1)
    {
      GList *layers = gimp_image_get_layer_list (image);

      if (layers)
        {
          GList *list;

          for (list = layers; list; list = g_list_next (list))
            {
              GimpLayer     *layer = list->data;
              GimpLayerMode  mode  = gimp_layer_get_mode (layer);

              if (! gimp_layer_mode_is_legacy (mode))
                {
                  /*  any non-legacy layer switches the mode to non-legacy
                   */
                  private->new_layer_mode = GIMP_LAYER_MODE_NORMAL;
                  break;
                }
            }

          /*  only if all layers are legacy, the mode is also legacy
           */
          if (! list)
            private->new_layer_mode = GIMP_LAYER_MODE_NORMAL_LEGACY;

          g_list_free (layers);
        }
      else
        {
          /*  empty images are never considered legacy
           */
          private->new_layer_mode = GIMP_LAYER_MODE_NORMAL;
        }
    }

  return private->new_layer_mode;
}

void
gimp_image_unset_default_new_layer_mode (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  GIMP_IMAGE_GET_PRIVATE (image)->new_layer_mode = -1;
}

gint
gimp_image_get_id (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), -1);

  return GIMP_IMAGE_GET_PRIVATE (image)->ID;
}

GimpImage *
gimp_image_get_by_id (Gimp *gimp,
                      gint  image_id)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (gimp->image_table == NULL)
    return NULL;

  return (GimpImage *) gimp_id_table_lookup (gimp->image_table, image_id);
}

void
gimp_image_set_file (GimpImage *image,
                     GFile     *file)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (private->file != file)
    {
      gimp_object_take_name (GIMP_OBJECT (image),
                             file ? g_file_get_uri (file) : NULL);
    }
}

/**
 * gimp_image_get_untitled_file:
 *
 * Returns: A #GFile saying "Untitled" for newly created images.
 **/
GFile *
gimp_image_get_untitled_file (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (! private->untitled_file)
    private->untitled_file = g_file_new_for_uri (_("Untitled"));

  return private->untitled_file;
}

/**
 * gimp_image_get_file_or_untitled:
 * @image: A #GimpImage.
 *
 * Get the file of the XCF image, or the "Untitled" file if there is no file.
 *
 * Returns: A #GFile.
 **/
GFile *
gimp_image_get_file_or_untitled (GimpImage *image)
{
  GFile *file;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  file = gimp_image_get_file (image);

  if (! file)
    file = gimp_image_get_untitled_file (image);

  return file;
}

/**
 * gimp_image_get_file:
 * @image: A #GimpImage.
 *
 * Get the file of the XCF image, or %NULL if there is no file.
 *
 * Returns: (nullable): The file, or %NULL.
 **/
GFile *
gimp_image_get_file (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->file;
}

/**
 * gimp_image_get_imported_file:
 * @image: A #GimpImage.
 *
 * Returns: (nullable): The file of the imported image, or %NULL if the image
 * has been saved as XCF after it was imported.
 **/
GFile *
gimp_image_get_imported_file (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->imported_file;
}

/**
 * gimp_image_get_exported_file:
 * @image: A #GimpImage.
 *
 * Returns: (nullable): The file of the image last exported from this XCF file,
 * or %NULL if the image has never been exported.
 **/
GFile *
gimp_image_get_exported_file (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->exported_file;
}

/**
 * gimp_image_get_save_a_copy_file:
 * @image: A #GimpImage.
 *
 * Returns: The URI of the last copy that was saved of this XCF file.
 **/
GFile *
gimp_image_get_save_a_copy_file (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->save_a_copy_file;
}

/**
 * gimp_image_get_any_file:
 * @image: A #GimpImage.
 *
 * Returns: The XCF file, the imported file, or the exported file, in
 * that order of precedence.
 **/
GFile *
gimp_image_get_any_file (GimpImage *image)
{
  GFile *file;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  file = gimp_image_get_file (image);
  if (! file)
    {
      file = gimp_image_get_imported_file (image);
      if (! file)
        {
          file = gimp_image_get_exported_file (image);
        }
    }

  return file;
}

/**
 * gimp_image_set_imported_uri:
 * @image: A #GimpImage.
 * @file:
 *
 * Sets the URI this file was imported from.
 **/
void
gimp_image_set_imported_file (GimpImage *image,
                              GFile     *file)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (g_set_object (&private->imported_file, file))
    {
      gimp_object_name_changed (GIMP_OBJECT (image));
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
      private->resolution_unit = gimp_unit_inch ();
    }
}

/**
 * gimp_image_set_exported_file:
 * @image: A #GimpImage.
 * @file:
 *
 * Sets the file this image was last exported to. Note that saving as
 * XCF is not "exporting".
 **/
void
gimp_image_set_exported_file (GimpImage *image,
                              GFile     *file)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (g_set_object (&private->exported_file, file))
    {
      gimp_object_name_changed (GIMP_OBJECT (image));
    }
}

/**
 * gimp_image_set_save_a_copy_file:
 * @image: A #GimpImage.
 * @uri:
 *
 * Set the URI to the last copy this XCF file was saved to through the
 * "save a copy" action.
 **/
void
gimp_image_set_save_a_copy_file (GimpImage *image,
                                 GFile     *file)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  g_set_object (&private->save_a_copy_file, file);
}

static gchar *
gimp_image_format_display_uri (GimpImage *image,
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

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  file   = gimp_image_get_file (image);
  source = gimp_image_get_imported_file (image);
  dest   = gimp_image_get_exported_file (image);

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
      if (! gimp_image_is_export_dirty (image))
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
        display_file = gimp_file_with_new_extension (display_file, NULL);

      uri_format = "[%s]";
    }

  if (! display_file)
    display_file = g_object_ref (gimp_image_get_untitled_file (image));

  if (basename)
    display_uri = g_path_get_basename (gimp_file_get_utf8_name (display_file));
  else
    display_uri = g_strdup (gimp_file_get_utf8_name (display_file));

  g_object_unref (display_file);

  format_string = g_strconcat (uri_format, export_status, NULL);

  tmp = g_strdup_printf (format_string, display_uri);
  g_free (display_uri);
  display_uri = tmp;

  g_free (format_string);

  return display_uri;
}

const gchar *
gimp_image_get_display_name (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (! private->display_name)
    private->display_name = gimp_image_format_display_uri (image, TRUE);

  return private->display_name;
}

const gchar *
gimp_image_get_display_path (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (! private->display_path)
    private->display_path = gimp_image_format_display_uri (image, FALSE);

  return private->display_path;
}

void
gimp_image_set_load_proc (GimpImage           *image,
                          GimpPlugInProcedure *proc)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  GIMP_IMAGE_GET_PRIVATE (image)->load_proc = proc;
}

GimpPlugInProcedure *
gimp_image_get_load_proc (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->load_proc;
}

void
gimp_image_set_save_proc (GimpImage           *image,
                          GimpPlugInProcedure *proc)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  GIMP_IMAGE_GET_PRIVATE (image)->save_proc = proc;
}

GimpPlugInProcedure *
gimp_image_get_save_proc (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->save_proc;
}

void
gimp_image_set_export_proc (GimpImage           *image,
                            GimpPlugInProcedure *proc)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  GIMP_IMAGE_GET_PRIVATE (image)->export_proc = proc;
}

GimpPlugInProcedure *
gimp_image_get_export_proc (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->export_proc;
}

gint
gimp_image_get_xcf_version (GimpImage    *image,
                            gboolean      zlib_compression,
                            gint         *gimp_version,
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
  if (gimp_image_get_colormap_palette (image))
    version = 1;

  items = gimp_image_get_layer_list (image);
  for (list = items; list; list = g_list_next (list))
    {
      GimpLayer     *layer = GIMP_LAYER (list->data);
      GimpContainer *filters;

      switch (gimp_layer_get_mode (layer))
        {
          /*  Modes that exist since ancient times  */
        case GIMP_LAYER_MODE_NORMAL_LEGACY:
        case GIMP_LAYER_MODE_DISSOLVE:
        case GIMP_LAYER_MODE_BEHIND_LEGACY:
        case GIMP_LAYER_MODE_MULTIPLY_LEGACY:
        case GIMP_LAYER_MODE_SCREEN_LEGACY:
        case GIMP_LAYER_MODE_OVERLAY_LEGACY:
        case GIMP_LAYER_MODE_DIFFERENCE_LEGACY:
        case GIMP_LAYER_MODE_ADDITION_LEGACY:
        case GIMP_LAYER_MODE_SUBTRACT_LEGACY:
        case GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY:
        case GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY:
        case GIMP_LAYER_MODE_HSV_HUE_LEGACY:
        case GIMP_LAYER_MODE_HSV_SATURATION_LEGACY:
        case GIMP_LAYER_MODE_HSL_COLOR_LEGACY:
        case GIMP_LAYER_MODE_HSV_VALUE_LEGACY:
        case GIMP_LAYER_MODE_DIVIDE_LEGACY:
        case GIMP_LAYER_MODE_DODGE_LEGACY:
        case GIMP_LAYER_MODE_BURN_LEGACY:
        case GIMP_LAYER_MODE_HARDLIGHT_LEGACY:
          break;

          /*  Since 2.6  */
        case GIMP_LAYER_MODE_SOFTLIGHT_LEGACY:
        case GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY:
        case GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY:
        case GIMP_LAYER_MODE_COLOR_ERASE_LEGACY:
          gimp_enum_get_value (GIMP_TYPE_LAYER_MODE,
                               gimp_layer_get_mode (layer),
                               NULL, NULL, &enum_desc, NULL);
          ADD_REASON (g_strdup_printf (_("Layer mode '%s' was added in %s"),
                                       enum_desc, "GIMP 2.6"));
          version = MAX (2, version);
          break;

          /*  Since 2.10  */
        case GIMP_LAYER_MODE_OVERLAY:
        case GIMP_LAYER_MODE_LCH_HUE:
        case GIMP_LAYER_MODE_LCH_CHROMA:
        case GIMP_LAYER_MODE_LCH_COLOR:
        case GIMP_LAYER_MODE_LCH_LIGHTNESS:
          gimp_enum_get_value (GIMP_TYPE_LAYER_MODE,
                               gimp_layer_get_mode (layer),
                               NULL, NULL, &enum_desc, NULL);
          ADD_REASON (g_strdup_printf (_("Layer mode '%s' was added in %s"),
                                       enum_desc, "GIMP 2.10"));
          version = MAX (9, version);
          break;

          /*  Since 2.10  */
        case GIMP_LAYER_MODE_NORMAL:
        case GIMP_LAYER_MODE_BEHIND:
        case GIMP_LAYER_MODE_MULTIPLY:
        case GIMP_LAYER_MODE_SCREEN:
        case GIMP_LAYER_MODE_DIFFERENCE:
        case GIMP_LAYER_MODE_ADDITION:
        case GIMP_LAYER_MODE_SUBTRACT:
        case GIMP_LAYER_MODE_DARKEN_ONLY:
        case GIMP_LAYER_MODE_LIGHTEN_ONLY:
        case GIMP_LAYER_MODE_HSV_HUE:
        case GIMP_LAYER_MODE_HSV_SATURATION:
        case GIMP_LAYER_MODE_HSL_COLOR:
        case GIMP_LAYER_MODE_HSV_VALUE:
        case GIMP_LAYER_MODE_DIVIDE:
        case GIMP_LAYER_MODE_DODGE:
        case GIMP_LAYER_MODE_BURN:
        case GIMP_LAYER_MODE_HARDLIGHT:
        case GIMP_LAYER_MODE_SOFTLIGHT:
        case GIMP_LAYER_MODE_GRAIN_EXTRACT:
        case GIMP_LAYER_MODE_GRAIN_MERGE:
        case GIMP_LAYER_MODE_VIVID_LIGHT:
        case GIMP_LAYER_MODE_PIN_LIGHT:
        case GIMP_LAYER_MODE_LINEAR_LIGHT:
        case GIMP_LAYER_MODE_HARD_MIX:
        case GIMP_LAYER_MODE_EXCLUSION:
        case GIMP_LAYER_MODE_LINEAR_BURN:
        case GIMP_LAYER_MODE_LUMA_DARKEN_ONLY:
        case GIMP_LAYER_MODE_LUMA_LIGHTEN_ONLY:
        case GIMP_LAYER_MODE_LUMINANCE:
        case GIMP_LAYER_MODE_COLOR_ERASE:
        case GIMP_LAYER_MODE_ERASE:
        case GIMP_LAYER_MODE_MERGE:
        case GIMP_LAYER_MODE_SPLIT:
        case GIMP_LAYER_MODE_PASS_THROUGH:
          gimp_enum_get_value (GIMP_TYPE_LAYER_MODE,
                               gimp_layer_get_mode (layer),
                               NULL, NULL, &enum_desc, NULL);
          ADD_REASON (g_strdup_printf (_("Layer mode '%s' was added in %s"),
                                       enum_desc, "GIMP 2.10"));
          version = MAX (10, version);
          break;

          /*  Just here instead of default so we get compiler warnings  */
        case GIMP_LAYER_MODE_REPLACE:
        case GIMP_LAYER_MODE_OVERWRITE:
        case GIMP_LAYER_MODE_ANTI_ERASE:
        case GIMP_LAYER_MODE_SEPARATOR:
          gimp_enum_get_value (GIMP_TYPE_LAYER_MODE,
                               gimp_layer_get_mode (layer),
                               NULL, NULL, &enum_desc, NULL);
          g_warning ("%s: layer mode '%s' should not be set on a layer.",
                     G_STRFUNC, enum_desc);
          break;
        }

      /* need version 3 for layer trees */
      if (gimp_viewable_get_children (GIMP_VIEWABLE (layer)))
        {
          ADD_REASON (g_strdup_printf (_("Layer groups were added in %s"),
                                       "GIMP 2.8"));
          version = MAX (3, version);

          /* need version 13 for group layers with masks */
          if (gimp_layer_get_mask (layer))
            {
              ADD_REASON (g_strdup_printf (_("Masks on layer groups were "
                                             "added in %s"), "GIMP 2.10"));
              version = MAX (13, version);
            }

          if (gimp_item_get_lock_position (GIMP_ITEM (layer)))
            {
              ADD_REASON (g_strdup_printf (_("Position locks on layer groups were added in %s"),
                                           "GIMP 3.0"));
              version = MAX (17, version);
            }

          if (gimp_layer_get_lock_alpha (layer))
            {
              ADD_REASON (g_strdup_printf (_("Alpha channel locks on layer groups were added in %s"),
                                           "GIMP 3.0"));
              version = MAX (17, version);
            }
        }

      if (gimp_item_get_lock_visibility (GIMP_ITEM (layer)))
        {
          ADD_REASON (g_strdup_printf (_("Visibility locks were added in %s"),
                                       "GIMP 3.0"));
          version = MAX (17, version);
        }

      if (GIMP_IS_TEXT_LAYER (layer) &&
          /* If we loaded an old XCF and didn't touch the text layers, then we
           * just resave the text layer as-is. No need to bump the XCF version.
           */
          ! GIMP_TEXT_LAYER (layer)->text_parasite_is_old)
        {
          ADD_REASON (g_strdup_printf (_("Format of font information in text layer was changed in %s"),
                                       "GIMP 3.0"));
          version = MAX (19, version);
        }

      filters = gimp_drawable_get_filters (GIMP_DRAWABLE (layer));
      if (gimp_container_get_n_children (filters) > 0)
        {
          ADD_REASON (g_strdup_printf (_("Layer effects were added in %s"),
                                       "GIMP 3.0"));
          /* Version 20 added layer effects; version 22 stored the effect
           * version as an additional field. Only ever store as XCF 22
           * when layer effects are used.
           */
          version = MAX (22, version);
        }

      if ((gimp_layer_get_real_blend_space (layer) == GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL)
        ||(gimp_layer_get_real_composite_space (layer) == GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL))
        {
          ADD_REASON (g_strdup_printf (_("A new perceptual blending space was added in %s"),
                                       "GIMP 3.0"));
          /* The blending space variant corresponding to SPACE_RGB_PERCEPTUAL in <3.0
           * corresponds to R'G'B'A which is NON_LINEAR in babl. Perceptual in babl is
           * R~G~B~A, >= 3.0 the code, comments and usage matches the existing enum value
           * as being NON_LINEAR and new layers created use the new integer value for
           * PERCEPTUAL.
           */
          version = MAX (23, version);
        }

      /* Need version 24 for vector layers. */
      if (GIMP_IS_VECTOR_LAYER (layer))
        {
          ADD_REASON (g_strdup_printf (_("Vector layers were added in %s"),
                                       "GIMP 3.2"));
          version = MAX (24, version);
        }

      /* Need version 25 for link layers. */
      if (GIMP_IS_LINK_LAYER (layer))
        {
          ADD_REASON (g_strdup_printf (_("Link layers were added in %s"),
                                       "GIMP 3.2"));
          version = MAX (25, version);
        }
    }
  g_list_free (items);

  items = gimp_image_get_channel_list (image);
  for (list = items; list; list = g_list_next (list))
    {
      GimpChannel *channel = GIMP_CHANNEL (list->data);

      if (gimp_item_get_lock_visibility (GIMP_ITEM (channel)))
        {
          ADD_REASON (g_strdup_printf (_("Visibility locks were added in %s"),
                                       "GIMP 3.0"));
          version = MAX (17, version);
        }
    }
  g_list_free (items);

  if (g_list_length (gimp_image_get_selected_paths (image)) > 1)
    {
      ADD_REASON (g_strdup_printf (_("Multiple path selection was "
                                     "added in %s"), "GIMP 3.0.0"));
      version = MAX (18, version);
    }

  items = gimp_image_get_path_list (image);
  for (list = items; list; list = g_list_next (list))
    {
      GimpPath *path = GIMP_PATH (list->data);

      if (gimp_item_get_color_tag (GIMP_ITEM (path)) != GIMP_COLOR_TAG_NONE)
        {
          ADD_REASON (g_strdup_printf (_("Storing color tags in path was "
                                         "added in %s"), "GIMP 3.0.0"));
          version = MAX (18, version);
        }
      if (gimp_item_get_lock_content (GIMP_ITEM (list->data)) ||
          gimp_item_get_lock_position (GIMP_ITEM (list->data)))
        {
          ADD_REASON (g_strdup_printf (_("Storing locks in path was "
                                         "added in %s"), "GIMP 3.0.0"));
          version = MAX (18, version);
        }
    }
  g_list_free (items);

  /* version 6 for new metadata has been dropped since they are
   * saved through parasites, which is compatible with older versions.
   */

  /* need version 7 for != 8-bit gamma images */
  if (gimp_image_get_precision (image) != GIMP_PRECISION_U8_NON_LINEAR)
    {
      ADD_REASON (g_strdup_printf (_("High bit-depth images were added "
                                     "in %s"), "GIMP 2.10"));
      version = MAX (7, version);
    }

  /* need version 12 for > 8-bit images for proper endian swapping */
  if (gimp_image_get_component_type (image) > GIMP_COMPONENT_TYPE_U8)
    {
      ADD_REASON (g_strdup_printf (_("Encoding of high bit-depth images was "
                                     "fixed in %s"), "GIMP 2.10"));
      version = MAX (12, version);
    }

  /* need version 8 for zlib compression */
  if (zlib_compression)
    {
      ADD_REASON (g_strdup_printf (_("Internal zlib compression was "
                                     "added in %s"), "GIMP 2.10"));
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
  if (gimp_object_get_memsize (GIMP_OBJECT (image), NULL) >= ((gint64) 1 << 32))
    {
      ADD_REASON (g_strdup_printf (_("Support for image files larger than "
                                     "4GB was added in %s"), "GIMP 2.10"));
      version = MAX (11, version);
    }

  if (g_list_length (gimp_image_get_selected_layers (image)) > 1)
    {
      ADD_REASON (g_strdup_printf (_("Multiple layer selection was "
                                     "added in %s"), "GIMP 3.0.0"));
      version = MAX (14, version);
    }

  if ((list = gimp_image_get_guides (image)))
    {
      for (; list; list = g_list_next (list))
        {
          gint32 position = gimp_guide_get_position (list->data);

          if (position < 0 ||
              (gimp_guide_get_orientation (list->data) == GIMP_ORIENTATION_HORIZONTAL &&
               position > gimp_image_get_height (image)) ||
              (gimp_guide_get_orientation (list->data) == GIMP_ORIENTATION_VERTICAL &&
               position > gimp_image_get_width (image)))
            {
              ADD_REASON (g_strdup_printf (_("Off-canvas guides "
                                             "added in %s"), "GIMP 3.0.0"));
              version = MAX (15, version);
              break;
            }
        }
    }

  if (gimp_image_get_stored_item_sets (image, GIMP_TYPE_LAYER) ||
      gimp_image_get_stored_item_sets (image, GIMP_TYPE_CHANNEL))
    {
      ADD_REASON (g_strdup_printf (_("Item set and pattern search in item's name were "
                                     "added in %s"), "GIMP 3.0.0"));
      version = MAX (16, version);
    }
  if (g_list_length (gimp_image_get_selected_channels (image)) > 1)
    {
      ADD_REASON (g_strdup_printf (_("Multiple channel selection was "
                                     "added in %s"), "GIMP 3.0.0"));
      version = MAX (16, version);
    }

  /* Note: user unit storage was changed in XCF 21, but we can still
   * easily save older XCF (we use the unit name for both singular and
   * plural forms). Therefore we don't bump the XCF version unnecessarily
   * and don't add any test.
   */

#undef ADD_REASON

  switch (version)
    {
    case 0:
    case 1:
    case 2:
      if (gimp_version)   *gimp_version   = 206;
      if (version_string) *version_string = "GIMP 2.6";
      break;

    case 3:
      if (gimp_version)   *gimp_version   = 208;
      if (version_string) *version_string = "GIMP 2.8";
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
      if (gimp_version)   *gimp_version   = 210;
      if (version_string) *version_string = "GIMP 2.10";
      break;
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
      if (gimp_version)   *gimp_version   = 300;
      if (version_string) *version_string = "GIMP 3.0";
      break;
    case 24:
    case 25:
      if (gimp_version)   *gimp_version   = 320;
      if (version_string) *version_string = "GIMP 3.2";
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
gimp_image_set_xcf_compression (GimpImage *image,
                                gboolean   compression)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  GIMP_IMAGE_GET_PRIVATE (image)->xcf_compression = compression;
}

gboolean
gimp_image_get_xcf_compression (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  return GIMP_IMAGE_GET_PRIVATE (image)->xcf_compression;
}

void
gimp_image_set_resolution (GimpImage *image,
                           gdouble    xresolution,
                           gdouble    yresolution)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  /* don't allow to set the resolution out of bounds */
  if (xresolution < GIMP_MIN_RESOLUTION || xresolution > GIMP_MAX_RESOLUTION ||
      yresolution < GIMP_MIN_RESOLUTION || yresolution > GIMP_MAX_RESOLUTION)
    return;

  private->resolution_set = TRUE;

  if ((ABS (private->xresolution - xresolution) >= 1e-5) ||
      (ABS (private->yresolution - yresolution) >= 1e-5))
    {
      gimp_image_undo_push_image_resolution (image,
                                             C_("undo-type", "Change Image Resolution"));

      private->xresolution = xresolution;
      private->yresolution = yresolution;

      gimp_image_resolution_changed (image);
      gimp_image_size_changed_detailed (image,
                                        0,
                                        0,
                                        gimp_image_get_width (image),
                                        gimp_image_get_height (image));
    }
}

void
gimp_image_get_resolution (GimpImage *image,
                           gdouble   *xresolution,
                           gdouble   *yresolution)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (xresolution != NULL && yresolution != NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  *xresolution = private->xresolution;
  *yresolution = private->yresolution;
}

void
gimp_image_resolution_changed (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[RESOLUTION_CHANGED], 0);
}

void
gimp_image_set_unit (GimpImage *image,
                     GimpUnit  *unit)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_UNIT (unit));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (private->resolution_unit != unit)
    {
      gimp_image_undo_push_image_resolution (image,
                                             C_("undo-type", "Change Image Unit"));

      private->resolution_unit = unit;
      gimp_image_unit_changed (image);
    }
}

GimpUnit *
gimp_image_get_unit (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->resolution_unit;
}

void
gimp_image_unit_changed (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[UNIT_CHANGED], 0);
}

gint
gimp_image_get_width (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  return GIMP_IMAGE_GET_PRIVATE (image)->width;
}

gint
gimp_image_get_height (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  return GIMP_IMAGE_GET_PRIVATE (image)->height;
}

gboolean
gimp_image_has_alpha (GimpImage *image)
{
  GimpImagePrivate *private;
  GimpLayer        *layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), TRUE);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  layer = GIMP_LAYER (gimp_container_get_first_child (private->layers->container));

  return ((gimp_image_get_n_layers (image) > 1) ||
          (layer && gimp_drawable_has_alpha (GIMP_DRAWABLE (layer))));
}

gboolean
gimp_image_is_empty (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), TRUE);

  return gimp_container_is_empty (GIMP_IMAGE_GET_PRIVATE (image)->layers->container);
}

void
gimp_image_set_floating_selection (GimpImage *image,
                                   GimpLayer *floating_sel)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (floating_sel == NULL || GIMP_IS_LAYER (floating_sel));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (private->floating_sel != floating_sel)
    {
      private->floating_sel = floating_sel;

      private->flush_accum.floating_selection_changed = TRUE;
    }
}

GimpLayer *
gimp_image_get_floating_selection (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->floating_sel;
}

void
gimp_image_floating_selection_changed (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[FLOATING_SELECTION_CHANGED], 0);
}

GimpChannel *
gimp_image_get_mask (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->selection_mask;
}

void
gimp_image_mask_changed (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[MASK_CHANGED], 0);
}

/**
 * gimp_image_mask_intersect:
 * @image:   the #GimpImage
 * @items:   a list of #GimpItem
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
gimp_image_mask_intersect (GimpImage *image,
                           GList     *items,
                           gint      *x,
                           gint      *y,
                           gint      *width,
                           gint      *height)
{
  GimpChannel *selection;
  GList       *iter;
  gint         sel_x, sel_y, sel_width, sel_height;
  gint         x1 = G_MAXINT;
  gint         y1 = G_MAXINT;
  gint         x2 = G_MININT;
  gint         y2 = G_MININT;
  gboolean     intersect = FALSE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  for (iter = items; iter; iter = iter->next)
    {
      g_return_val_if_fail (GIMP_IS_ITEM (iter->data), FALSE);
      g_return_val_if_fail (gimp_item_is_attached (iter->data), FALSE);
      g_return_val_if_fail (gimp_item_get_image (iter->data) == image, FALSE);
    }

  selection = gimp_image_get_mask (image);
  if (selection)
    gimp_item_bounds (GIMP_ITEM (selection),
                      &sel_x, &sel_y, &sel_width, &sel_height);

  for (iter = items; iter; iter = iter->next)
    {
      GimpItem *item = iter->data;
      gboolean  item_intersect;
      gint      tmp_x, tmp_y;
      gint      tmp_width, tmp_height;

      gimp_item_get_offset (item, &tmp_x, &tmp_y);

      /* check for is_empty() before intersecting so we ignore the
       * selection if it is suspended (like when stroking)
       */
      if (GIMP_ITEM (selection) != item       &&
          ! gimp_channel_is_empty (selection))
        {
          item_intersect = gimp_rectangle_intersect (sel_x, sel_y, sel_width, sel_height,
                                                     tmp_x, tmp_y,
                                                     gimp_item_get_width  (item),
                                                     gimp_item_get_height (item),
                                                     &tmp_x, &tmp_y,
                                                     &tmp_width, &tmp_height);
        }
      else
        {
          tmp_width  = gimp_item_get_width  (item);
          tmp_height = gimp_item_get_height (item);

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
gimp_image_take_mask (GimpImage   *image,
                      GimpChannel *mask)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_SELECTION (mask));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (private->selection_mask)
    g_object_unref (private->selection_mask);

  private->selection_mask = g_object_ref_sink (mask);

  g_signal_connect (private->selection_mask, "update",
                    G_CALLBACK (gimp_image_mask_update),
                    image);
}


/*  image components  */

const Babl *
gimp_image_get_component_format (GimpImage       *image,
                                 GimpChannelType  channel)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  switch (channel)
    {
    case GIMP_CHANNEL_RED:
      return gimp_babl_component_format (GIMP_RGB,
                                         gimp_image_get_precision (image),
                                         RED);

    case GIMP_CHANNEL_GREEN:
      return gimp_babl_component_format (GIMP_RGB,
                                         gimp_image_get_precision (image),
                                         GREEN);

    case GIMP_CHANNEL_BLUE:
      return gimp_babl_component_format (GIMP_RGB,
                                         gimp_image_get_precision (image),
                                         BLUE);

    case GIMP_CHANNEL_ALPHA:
      return gimp_babl_component_format (GIMP_RGB,
                                         gimp_image_get_precision (image),
                                         ALPHA);

    case GIMP_CHANNEL_GRAY:
      return gimp_babl_component_format (GIMP_GRAY,
                                         gimp_image_get_precision (image),
                                         GRAY);

    case GIMP_CHANNEL_INDEXED:
      return babl_format ("Y u8"); /* will extract grayscale, the best
                                    * we can do here */
    }

  return NULL;
}

gint
gimp_image_get_component_index (GimpImage       *image,
                                GimpChannelType  channel)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), -1);

  switch (channel)
    {
    case GIMP_CHANNEL_RED:     return RED;
    case GIMP_CHANNEL_GREEN:   return GREEN;
    case GIMP_CHANNEL_BLUE:    return BLUE;
    case GIMP_CHANNEL_GRAY:    return GRAY;
    case GIMP_CHANNEL_INDEXED: return INDEXED;
    case GIMP_CHANNEL_ALPHA:
      switch (gimp_image_get_base_type (image))
        {
        case GIMP_RGB:     return ALPHA;
        case GIMP_GRAY:    return ALPHA_G;
        case GIMP_INDEXED: return ALPHA_I;
        }
    }

  return -1;
}

void
gimp_image_set_component_active (GimpImage       *image,
                                 GimpChannelType  channel,
                                 gboolean         active)
{
  GimpImagePrivate *private;
  gint              index = -1;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  index = gimp_image_get_component_index (image, channel);

  if (index != -1 && active != private->active[index])
    {
      private->active[index] = active ? TRUE : FALSE;

      /*  If there is an active channel and we mess with the components,
       *  the active channel gets unset...
       */
      gimp_image_unset_selected_channels (image);

      g_signal_emit (image,
                     gimp_image_signals[COMPONENT_ACTIVE_CHANGED], 0,
                     channel);
    }
}

gboolean
gimp_image_get_component_active (GimpImage       *image,
                                 GimpChannelType  channel)
{
  gint index = -1;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  index = gimp_image_get_component_index (image, channel);

  if (index != -1)
    return GIMP_IMAGE_GET_PRIVATE (image)->active[index];

  return FALSE;
}

void
gimp_image_get_active_array (GimpImage *image,
                             gboolean  *components)
{
  GimpImagePrivate *private;
  gint              i;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (components != NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  for (i = 0; i < MAX_CHANNELS; i++)
    components[i] = private->active[i];
}

GimpComponentMask
gimp_image_get_active_mask (GimpImage *image)
{
  GimpImagePrivate  *private;
  GimpComponentMask  mask = 0;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  switch (gimp_image_get_base_type (image))
    {
    case GIMP_RGB:
      mask |= (private->active[RED])   ? GIMP_COMPONENT_MASK_RED   : 0;
      mask |= (private->active[GREEN]) ? GIMP_COMPONENT_MASK_GREEN : 0;
      mask |= (private->active[BLUE])  ? GIMP_COMPONENT_MASK_BLUE  : 0;
      mask |= (private->active[ALPHA]) ? GIMP_COMPONENT_MASK_ALPHA : 0;
      break;

    case GIMP_GRAY:
    case GIMP_INDEXED:
      mask |= (private->active[GRAY])    ? GIMP_COMPONENT_MASK_RED   : 0;
      mask |= (private->active[GRAY])    ? GIMP_COMPONENT_MASK_GREEN : 0;
      mask |= (private->active[GRAY])    ? GIMP_COMPONENT_MASK_BLUE  : 0;
      mask |= (private->active[ALPHA_G]) ? GIMP_COMPONENT_MASK_ALPHA : 0;
      break;
    }

  return mask;
}

void
gimp_image_set_component_visible (GimpImage       *image,
                                  GimpChannelType  channel,
                                  gboolean         visible)
{
  GimpImagePrivate *private;
  gint              index = -1;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  index = gimp_image_get_component_index (image, channel);

  if (index != -1 && visible != private->visible[index])
    {
      private->visible[index] = visible ? TRUE : FALSE;

      if (private->visible_mask)
        {
          GimpComponentMask mask;

          mask = ~gimp_image_get_visible_mask (image) & GIMP_COMPONENT_MASK_ALL;

          gegl_node_set (private->visible_mask,
                         "mask", mask,
                         NULL);
        }

      g_signal_emit (image,
                     gimp_image_signals[COMPONENT_VISIBILITY_CHANGED], 0,
                     channel);

      gimp_image_invalidate_all (image);
    }
}

gboolean
gimp_image_get_component_visible (GimpImage       *image,
                                  GimpChannelType  channel)
{
  gint index = -1;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  index = gimp_image_get_component_index (image, channel);

  if (index != -1)
    return GIMP_IMAGE_GET_PRIVATE (image)->visible[index];

  return FALSE;
}

void
gimp_image_get_visible_array (GimpImage *image,
                              gboolean  *components)
{
  GimpImagePrivate *private;
  gint              i;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (components != NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  for (i = 0; i < MAX_CHANNELS; i++)
    components[i] = private->visible[i];
}

GimpComponentMask
gimp_image_get_visible_mask (GimpImage *image)
{
  GimpImagePrivate  *private;
  GimpComponentMask  mask = 0;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  switch (gimp_image_get_base_type (image))
    {
    case GIMP_RGB:
      mask |= (private->visible[RED])   ? GIMP_COMPONENT_MASK_RED   : 0;
      mask |= (private->visible[GREEN]) ? GIMP_COMPONENT_MASK_GREEN : 0;
      mask |= (private->visible[BLUE])  ? GIMP_COMPONENT_MASK_BLUE  : 0;
      mask |= (private->visible[ALPHA]) ? GIMP_COMPONENT_MASK_ALPHA : 0;
      break;

    case GIMP_GRAY:
    case GIMP_INDEXED:
      mask |= (private->visible[GRAY])  ? GIMP_COMPONENT_MASK_RED   : 0;
      mask |= (private->visible[GRAY])  ? GIMP_COMPONENT_MASK_GREEN : 0;
      mask |= (private->visible[GRAY])  ? GIMP_COMPONENT_MASK_BLUE  : 0;
      mask |= (private->visible[ALPHA]) ? GIMP_COMPONENT_MASK_ALPHA : 0;
      break;
    }

  return mask;
}


/*  emitting image signals  */

void
gimp_image_mode_changed (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[MODE_CHANGED], 0);
}

void
gimp_image_precision_changed (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[PRECISION_CHANGED], 0);
}

void
gimp_image_alpha_changed (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[ALPHA_CHANGED], 0);
}

void
gimp_image_invalidate (GimpImage *image,
                       gint       x,
                       gint       y,
                       gint       width,
                       gint       height)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  gimp_projectable_invalidate (GIMP_PROJECTABLE (image),
                               x, y, width, height);

  GIMP_IMAGE_GET_PRIVATE (image)->flush_accum.preview_invalidated = TRUE;
}

void
gimp_image_invalidate_all (GimpImage *image)
{
  const GeglRectangle *bounding_box;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  bounding_box = &GIMP_IMAGE_GET_PRIVATE (image)->bounding_box;

  gimp_image_invalidate (image,
                         bounding_box->x,     bounding_box->y,
                         bounding_box->width, bounding_box->height);
}

void
gimp_image_guide_added (GimpImage *image,
                        GimpGuide *guide)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  g_signal_emit (image, gimp_image_signals[GUIDE_ADDED], 0,
                 guide);
}

void
gimp_image_guide_removed (GimpImage *image,
                          GimpGuide *guide)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  g_signal_emit (image, gimp_image_signals[GUIDE_REMOVED], 0,
                 guide);
}

void
gimp_image_guide_moved (GimpImage *image,
                        GimpGuide *guide)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  g_signal_emit (image, gimp_image_signals[GUIDE_MOVED], 0,
                 guide);
}

void
gimp_image_sample_point_added (GimpImage       *image,
                               GimpSamplePoint *sample_point)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_SAMPLE_POINT (sample_point));

  g_signal_emit (image, gimp_image_signals[SAMPLE_POINT_ADDED], 0,
                 sample_point);
}

void
gimp_image_sample_point_removed (GimpImage       *image,
                                 GimpSamplePoint *sample_point)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_SAMPLE_POINT (sample_point));

  g_signal_emit (image, gimp_image_signals[SAMPLE_POINT_REMOVED], 0,
                 sample_point);
}

void
gimp_image_sample_point_moved (GimpImage       *image,
                               GimpSamplePoint *sample_point)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_SAMPLE_POINT (sample_point));

  g_signal_emit (image, gimp_image_signals[SAMPLE_POINT_MOVED], 0,
                 sample_point);
}

/**
 * gimp_image_size_changed_detailed:
 * @image:
 * @previous_origin_x:
 * @previous_origin_y:
 *
 * Emits the size-changed-detailed signal that is typically used to adjust the
 * position of the image in the display shell on various operations,
 * e.g. crop.
 *
 * This function makes sure that GimpViewable::size-changed is also emitted.
 **/
void
gimp_image_size_changed_detailed (GimpImage *image,
                                  gint       previous_origin_x,
                                  gint       previous_origin_y,
                                  gint       previous_width,
                                  gint       previous_height)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[SIZE_CHANGED_DETAILED], 0,
                 previous_origin_x,
                 previous_origin_y,
                 previous_width,
                 previous_height);
}

void
gimp_image_colormap_changed (GimpImage *image,
                             gint       color_index)
{
  GimpPalette *palette;
  gint         n_colors;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  palette = GIMP_IMAGE_GET_PRIVATE (image)->palette;
  n_colors = palette ? gimp_palette_get_n_colors (palette) : 0;
  g_return_if_fail (color_index >= -1 && color_index < n_colors);

  g_signal_emit (image, gimp_image_signals[COLORMAP_CHANGED], 0,
                 color_index);
}

void
gimp_image_selection_invalidate (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[SELECTION_INVALIDATE], 0);
}

void
gimp_image_quick_mask_changed (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[QUICK_MASK_CHANGED], 0);
}

void
gimp_image_undo_event (GimpImage     *image,
                       GimpUndoEvent  event,
                       GimpUndo      *undo)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (((event == GIMP_UNDO_EVENT_UNDO_FREE   ||
                      event == GIMP_UNDO_EVENT_UNDO_FREEZE ||
                      event == GIMP_UNDO_EVENT_UNDO_THAW) && undo == NULL) ||
                    GIMP_IS_UNDO (undo));

  g_signal_emit (image, gimp_image_signals[UNDO_EVENT], 0, event, undo);
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
 *   freed.  See gimpimage-undo.c for the gory details.
 */

/*
 * NEVER CALL gimp_image_dirty() directly!
 *
 * If your code has just dirtied the image, push an undo instead.
 * Failing that, push the trivial undo which tells the user the
 * command is not undoable: undo_push_cantundo() (But really, it would
 * be best to push a proper undo).  If you just dirty the image
 * without pushing an undo then the dirty count is increased, but
 * popping that many undo actions won't lead to a clean image.
 */

gint
gimp_image_dirty (GimpImage     *image,
                  GimpDirtyMask  dirty_mask)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  private->dirty++;
  private->export_dirty++;

  if (! private->dirty_time)
    private->dirty_time = time (NULL);

  g_signal_emit (image, gimp_image_signals[DIRTY], 0, dirty_mask);

  TRC (("dirty %d -> %d\n", private->dirty - 1, private->dirty));

  return private->dirty;
}

gint
gimp_image_clean (GimpImage     *image,
                  GimpDirtyMask  dirty_mask)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  private->dirty--;
  private->export_dirty--;

  g_signal_emit (image, gimp_image_signals[CLEAN], 0, dirty_mask);

  TRC (("clean %d -> %d\n", private->dirty + 1, private->dirty));

  return private->dirty;
}

void
gimp_image_clean_all (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  private->dirty      = 0;
  private->dirty_time = 0;

  g_signal_emit (image, gimp_image_signals[CLEAN], 0, GIMP_DIRTY_ALL);

  gimp_object_name_changed (GIMP_OBJECT (image));
}

void
gimp_image_export_clean_all (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  private->export_dirty = 0;

  g_signal_emit (image, gimp_image_signals[CLEAN], 0, GIMP_DIRTY_ALL);

  gimp_object_name_changed (GIMP_OBJECT (image));
}

/**
 * gimp_image_is_dirty:
 * @image:
 *
 * Returns: True if the image is dirty, false otherwise.
 **/
gint
gimp_image_is_dirty (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  return GIMP_IMAGE_GET_PRIVATE (image)->dirty != 0;
}

/**
 * gimp_image_is_export_dirty:
 * @image:
 *
 * Returns: True if the image export is dirty, false otherwise.
 **/
gboolean
gimp_image_is_export_dirty (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  return GIMP_IMAGE_GET_PRIVATE (image)->export_dirty != 0;
}

gint64
gimp_image_get_dirty_time (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  return GIMP_IMAGE_GET_PRIVATE (image)->dirty_time;
}

/**
 * gimp_image_saving:
 * @image:
 *
 * Emits the "saving" signal, indicating that @image is about to be saved,
 * or exported.
 */
void
gimp_image_saving (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[SAVING], 0);
}

/**
 * gimp_image_saved:
 * @image:
 * @file:
 *
 * Emits the "saved" signal, indicating that @image was saved to the
 * location specified by @file.
 */
void
gimp_image_saved (GimpImage *image,
                  GFile     *file)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (G_IS_FILE (file));

  g_signal_emit (image, gimp_image_signals[SAVED], 0, file);
}

/**
 * gimp_image_exported:
 * @image:
 * @file:
 *
 * Emits the "exported" signal, indicating that @image was exported to the
 * location specified by @file.
 */
void
gimp_image_exported (GimpImage *image,
                     GFile     *file)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (G_IS_FILE (file));

  g_signal_emit (image, gimp_image_signals[EXPORTED], 0, file);
}


/*  flush this image's displays  */

void
gimp_image_flush (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  gimp_projectable_flush (GIMP_PROJECTABLE (image),
                          GIMP_IMAGE_GET_PRIVATE (image)->flush_accum.preview_invalidated);
}


/*  display / instance counters  */

gint
gimp_image_get_display_count (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  return GIMP_IMAGE_GET_PRIVATE (image)->disp_count;
}

void
gimp_image_inc_display_count (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  GIMP_IMAGE_GET_PRIVATE (image)->disp_count++;
}

void
gimp_image_dec_display_count (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  GIMP_IMAGE_GET_PRIVATE (image)->disp_count--;
}

gint
gimp_image_get_instance_count (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  return GIMP_IMAGE_GET_PRIVATE (image)->instance_count;
}

void
gimp_image_inc_instance_count (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  GIMP_IMAGE_GET_PRIVATE (image)->instance_count++;
}

void
gimp_image_inc_show_all_count (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  GIMP_IMAGE_GET_PRIVATE (image)->show_all++;

  if (GIMP_IMAGE_GET_PRIVATE (image)->show_all == 1)
    {
      g_clear_object (&GIMP_IMAGE_GET_PRIVATE (image)->pickable_buffer);

      gimp_image_update_bounding_box (image);
    }
}

void
gimp_image_dec_show_all_count (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  GIMP_IMAGE_GET_PRIVATE (image)->show_all--;

  if (GIMP_IMAGE_GET_PRIVATE (image)->show_all == 0)
    {
      g_clear_object (&GIMP_IMAGE_GET_PRIVATE (image)->pickable_buffer);

      gimp_image_update_bounding_box (image);
    }
}


/*  parasites  */

const GimpParasite *
gimp_image_parasite_find (GimpImage   *image,
                          const gchar *name)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return gimp_parasite_list_find (GIMP_IMAGE_GET_PRIVATE (image)->parasites,
                                  name);
}

static void
list_func (gchar          *key,
           GimpParasite   *p,
           gchar        ***cur)
{
  *(*cur)++ = (gchar *) g_strdup (key);
}

gchar **
gimp_image_parasite_list (GimpImage *image)
{
  GimpImagePrivate  *private;
  gint               count;
  gchar            **list;
  gchar            **cur;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  count = gimp_parasite_list_length (private->parasites);
  cur = list = g_new0 (gchar *, count + 1);

  gimp_parasite_list_foreach (private->parasites, (GHFunc) list_func, &cur);

  return list;
}

gboolean
gimp_image_parasite_validate (GimpImage           *image,
                              const GimpParasite  *parasite,
                              GError             **error)
{
  const gchar *name;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (parasite != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  name = gimp_parasite_get_name (parasite);

  if (strcmp (name, GIMP_ICC_PROFILE_PARASITE_NAME) == 0 ||
      strcmp (name, GIMP_SIMULATION_ICC_PROFILE_PARASITE_NAME) == 0)
    {
      return gimp_image_validate_icc_parasite (image, parasite,
                                               name, NULL, error);
    }
  else if (strcmp (name, "gimp-comment") == 0)
    {
      const gchar *data;
      guint32      length;
      gboolean     valid  = FALSE;

      data = gimp_parasite_get_data (parasite, &length);
      if (length > 0)
        {
          if (data[length - 1] == '\0')
            valid = g_utf8_validate (data, -1, NULL);
          else
            valid = g_utf8_validate (data, length, NULL);
        }

      if (! valid)
        {
          g_set_error (error, GIMP_ERROR, GIMP_FAILED,
                       _("'gimp-comment' parasite validation failed: "
                         "comment contains invalid UTF-8"));
          return FALSE;
        }
    }

  return TRUE;
}

void
gimp_image_parasite_attach (GimpImage          *image,
                            const GimpParasite *parasite,
                            gboolean            push_undo)
{
  GimpImagePrivate *private;
  GimpParasite      copy;
  const gchar      *name;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (parasite != NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  name = gimp_parasite_get_name (parasite);

  /*  this is so ugly and is only for the PDB  */
  if (strcmp (name, GIMP_ICC_PROFILE_PARASITE_NAME) == 0 ||
      strcmp (name, GIMP_SIMULATION_ICC_PROFILE_PARASITE_NAME) == 0)
    {
      GimpColorProfile *profile;
      GimpColorProfile *builtin;
      const guint8     *parasite_data;
      guint32           parasite_length;

      parasite_data = gimp_parasite_get_data (parasite, &parasite_length);
      profile =
        gimp_color_profile_new_from_icc_profile (parasite_data, parasite_length,
                                                 NULL);
      builtin = gimp_image_get_builtin_color_profile (image);

      if (gimp_color_profile_is_equal (profile, builtin))
        {
          /* setting the builtin profile is equal to removing the profile */
          gimp_image_parasite_detach (image, GIMP_ICC_PROFILE_PARASITE_NAME,
                                      push_undo);
          g_object_unref (profile);
          return;
        }

      g_object_unref (profile);
    }

  /*  make a temporary copy of the GimpParasite struct because
   *  gimp_parasite_shift_parent() changes it
   */
  copy = *parasite;

  /*  only set the dirty bit manually if we can be saved and the new
   *  parasite differs from the current one and we aren't undoable
   */
  if (push_undo && gimp_parasite_is_undoable (&copy))
    gimp_image_undo_push_image_parasite (image,
                                         C_("undo-type", "Attach Parasite to Image"),
                                         &copy);

  /*  We used to push a cantundo on the stack here. This made the undo stack
   *  unusable (NULL on the stack) and prevented people from undoing after a
   *  save (since most save plug-ins attach an undoable comment parasite).
   *  Now we simply attach the parasite without pushing an undo. That way
   *  it's undoable but does not block the undo system.   --Sven
   */
  gimp_parasite_list_add (private->parasites, &copy);

  if (push_undo && gimp_parasite_has_flag (&copy, GIMP_PARASITE_ATTACH_PARENT))
    {
      gimp_parasite_shift_parent (&copy);
      gimp_parasite_attach (image->gimp, &copy);
    }

  if (strcmp (name, GIMP_ICC_PROFILE_PARASITE_NAME) == 0)
    _gimp_image_update_color_profile (image, parasite);

  if (strcmp (name, GIMP_SIMULATION_ICC_PROFILE_PARASITE_NAME) == 0)
    _gimp_image_update_simulation_profile (image, parasite);

  g_signal_emit (image, gimp_image_signals[PARASITE_ATTACHED], 0,
                 name);
}

void
gimp_image_parasite_detach (GimpImage   *image,
                            const gchar *name,
                            gboolean     push_undo)
{
  GimpImagePrivate   *private;
  const GimpParasite *parasite;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (name != NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (! (parasite = gimp_parasite_list_find (private->parasites, name)))
    return;

  if (push_undo && gimp_parasite_is_undoable (parasite))
    gimp_image_undo_push_image_parasite_remove (image,
                                                C_("undo-type", "Remove Parasite from Image"),
                                                name);

  gimp_parasite_list_remove (private->parasites, name);

  if (strcmp (name, GIMP_ICC_PROFILE_PARASITE_NAME) == 0)
    _gimp_image_update_color_profile (image, NULL);

  if (strcmp (name, GIMP_SIMULATION_ICC_PROFILE_PARASITE_NAME) == 0)
    _gimp_image_update_simulation_profile (image, NULL);

  g_signal_emit (image, gimp_image_signals[PARASITE_DETACHED], 0,
                 name);
}


/*  tattoos  */

GimpTattoo
gimp_image_get_new_tattoo (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  private->tattoo_state++;

  if (G_UNLIKELY (private->tattoo_state == 0))
    g_warning ("%s: Tattoo state corrupted (integer overflow).", G_STRFUNC);

  return private->tattoo_state;
}

GimpTattoo
gimp_image_get_tattoo_state (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  return GIMP_IMAGE_GET_PRIVATE (image)->tattoo_state;
}

gboolean
gimp_image_set_tattoo_state (GimpImage  *image,
                             GimpTattoo  val)
{
  GList      *all_items;
  GList      *list;
  gboolean    retval = TRUE;
  GimpTattoo  maxval = 0;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  /* Check that the layer tattoos don't overlap with channel or vector ones */
  all_items = gimp_image_get_layer_list (image);

  for (list = all_items; list; list = g_list_next (list))
    {
      GimpTattoo ltattoo;

      ltattoo = gimp_item_get_tattoo (GIMP_ITEM (list->data));
      if (ltattoo > maxval)
        maxval = ltattoo;

      if (gimp_image_get_channel_by_tattoo (image, ltattoo))
        retval = FALSE; /* Oopps duplicated tattoo in channel */

      if (gimp_image_get_path_by_tattoo (image, ltattoo))
        retval = FALSE; /* Oopps duplicated tattoo in path */
    }

  g_list_free (all_items);

  /* Now check that the channel and path tattoos don't overlap */
  all_items = gimp_image_get_channel_list (image);

  for (list = all_items; list; list = g_list_next (list))
    {
      GimpTattoo ctattoo;

      ctattoo = gimp_item_get_tattoo (GIMP_ITEM (list->data));
      if (ctattoo > maxval)
        maxval = ctattoo;

      if (gimp_image_get_path_by_tattoo (image, ctattoo))
        retval = FALSE; /* Oopps duplicated tattoo in path */
    }

  g_list_free (all_items);

  /* Find the max tattoo value in the path */
  all_items = gimp_image_get_path_list (image);

  for (list = all_items; list; list = g_list_next (list))
    {
      GimpTattoo vtattoo;

      vtattoo = gimp_item_get_tattoo (GIMP_ITEM (list->data));
      if (vtattoo > maxval)
        maxval = vtattoo;
    }

  g_list_free (all_items);

  if (val < maxval)
    retval = FALSE;

  /* Must check if the state is valid */
  if (retval == TRUE)
    GIMP_IMAGE_GET_PRIVATE (image)->tattoo_state = val;

  return retval;
}


/*  projection  */

GimpProjection *
gimp_image_get_projection (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->projection;
}


/*  layers / channels / path  */

GimpItemTree *
gimp_image_get_layer_tree (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->layers;
}

GimpItemTree *
gimp_image_get_channel_tree (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->channels;
}

GimpItemTree *
gimp_image_get_path_tree (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->paths;
}

GimpContainer *
gimp_image_get_items (GimpImage *image,
                      GType      item_type)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  if (item_type == GIMP_TYPE_LAYER)
    {
      return gimp_image_get_layers (image);
    }
  else if (item_type == GIMP_TYPE_CHANNEL)
    {
      return gimp_image_get_channels (image);
    }
  else if (item_type == GIMP_TYPE_PATH)
    {
      return gimp_image_get_paths (image);
    }

  g_return_val_if_reached (NULL);
}

GimpContainer *
gimp_image_get_layers (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->layers->container;
}

GimpContainer *
gimp_image_get_channels (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->channels->container;
}

GimpContainer *
gimp_image_get_paths (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->paths->container;
}

gint
gimp_image_get_n_layers (GimpImage *image)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  stack = GIMP_ITEM_STACK (gimp_image_get_layers (image));

  return gimp_item_stack_get_n_items (stack);
}

gint
gimp_image_get_n_channels (GimpImage *image)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  stack = GIMP_ITEM_STACK (gimp_image_get_channels (image));

  return gimp_item_stack_get_n_items (stack);
}

gint
gimp_image_get_n_paths (GimpImage *image)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  stack = GIMP_ITEM_STACK (gimp_image_get_paths (image));

  return gimp_item_stack_get_n_items (stack);
}

GList *
gimp_image_get_layer_iter (GimpImage *image)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  stack = GIMP_ITEM_STACK (gimp_image_get_layers (image));

  return gimp_item_stack_get_item_iter (stack);
}

GList *
gimp_image_get_channel_iter (GimpImage *image)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  stack = GIMP_ITEM_STACK (gimp_image_get_channels (image));

  return gimp_item_stack_get_item_iter (stack);
}

GList *
gimp_image_get_path_iter (GimpImage *image)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  stack = GIMP_ITEM_STACK (gimp_image_get_paths (image));

  return gimp_item_stack_get_item_iter (stack);
}

GList *
gimp_image_get_layer_list (GimpImage *image)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  stack = GIMP_ITEM_STACK (gimp_image_get_layers (image));

  return gimp_item_stack_get_item_list (stack);
}

GList *
gimp_image_get_channel_list (GimpImage *image)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  stack = GIMP_ITEM_STACK (gimp_image_get_channels (image));

  return gimp_item_stack_get_item_list (stack);
}

GList *
gimp_image_get_path_list (GimpImage *image)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  stack = GIMP_ITEM_STACK (gimp_image_get_paths (image));

  return gimp_item_stack_get_item_list (stack);
}


/*  active drawable, layer, channel, path  */

void
gimp_image_unset_selected_channels (GimpImage *image)
{
  GimpImagePrivate *private;
  GList            *channels;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  channels = gimp_image_get_selected_channels (image);

  if (channels)
    {
      gimp_image_set_selected_channels (image, NULL);

      if (private->layer_stack)
        gimp_image_set_selected_layers (image, private->layer_stack->data);
    }
}

/**
 * gimp_image_is_selected_drawable:
 * @image:
 * @drawable:
 *
 * Checks if @drawable belong to the list of currently selected
 * drawables. It doesn't mean this is the only selected drawable (if
 * this is what you want to check, use
 * gimp_image_equal_selected_drawables() with a list containing only
 * this drawable).
 *
 * Returns: %TRUE is @drawable is one of the selected drawables.
 */
gboolean
gimp_image_is_selected_drawable (GimpImage    *image,
                                 GimpDrawable *drawable)
{
  GList    *selected_drawables;
  gboolean  found;

  selected_drawables = gimp_image_get_selected_drawables (image);
  found = (g_list_find (selected_drawables, drawable) != NULL);
  g_list_free (selected_drawables);

  return found;
}

/**
 * gimp_image_equal_selected_drawables:
 * @image:
 * @drawables:
 *
 * Compare the list of @drawables with the selected drawables in @image
 * (i.e. the result of gimp_image_equal_selected_drawables()).
 * The order of the @drawables does not matter, only if the size and
 * contents of the list is the same.
 */
gboolean
gimp_image_equal_selected_drawables (GimpImage *image,
                                     GList     *drawables)
{
  GList    *selected_drawables;
  GList    *iter;
  gboolean  equal = FALSE;

  selected_drawables = gimp_image_get_selected_drawables (image);

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
gimp_image_get_selected_drawables (GimpImage *image)
{
  GimpImagePrivate *private;
  GList            *selected_channels;
  GList            *selected_layers;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  selected_channels = gimp_item_tree_get_selected_items (private->channels);
  selected_layers   = gimp_item_tree_get_selected_items (private->layers);

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
          GimpLayer     *layer = GIMP_LAYER (selected_layers->data);
          GimpLayerMask *mask  = gimp_layer_get_mask (layer);

          if (mask && gimp_layer_get_edit_mask (layer))
            selected_layers->data = mask;
        }
      return selected_layers;
    }

  return NULL;
}

GList *
gimp_image_get_selected_items (GimpImage *image,
                               GType      item_type)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  if (item_type == GIMP_TYPE_LAYER)
    {
      return gimp_image_get_selected_layers (image);
    }
  else if (item_type == GIMP_TYPE_CHANNEL)
    {
      return gimp_image_get_selected_channels (image);
    }
  else if (item_type == GIMP_TYPE_PATH)
    {
      return gimp_image_get_selected_paths (image);
    }

  g_return_val_if_reached (NULL);
}

GList *
gimp_image_get_selected_layers (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  return gimp_item_tree_get_selected_items (private->layers);
}

GList *
gimp_image_get_selected_channels (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  return gimp_item_tree_get_selected_items (private->channels);
}

GList *
gimp_image_get_selected_paths (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  return gimp_item_tree_get_selected_items (private->paths);
}

void
gimp_image_set_selected_items (GimpImage *image,
                               GType      item_type,
                               GList     *items)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  if (g_type_is_a (item_type, GIMP_TYPE_LAYER))
    {
      gimp_image_set_selected_layers (image, items);
    }
  else if (g_type_is_a (item_type, GIMP_TYPE_CHANNEL))
    {
      gimp_image_set_selected_channels (image, items);
    }
  else if (g_type_is_a (item_type, GIMP_TYPE_PATH))
    {
      gimp_image_set_selected_paths (image, items);
    }
  else
    {
      g_return_if_reached ();
    }
}

void
gimp_image_set_selected_layers (GimpImage *image,
                                GList     *layers)
{
  GimpImagePrivate *private;
  GimpLayer        *floating_sel;
  GList            *selected_layers;
  GList            *layers2;
  GList            *iter;
  gboolean          selection_changed = TRUE;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  layers2 = g_list_copy (layers);
  for (iter = layers; iter; iter = iter->next)
    {
      g_return_if_fail (GIMP_IS_LAYER (iter->data));
      g_return_if_fail (gimp_item_get_image (GIMP_ITEM (iter->data)) == image);

      /* Silently remove non-attached layers from selection. Do not
       * error out on it as it may happen for instance when selection
       * changes while in the process of removing a layer group.
       */
      if (! gimp_item_is_attached (GIMP_ITEM (iter->data)))
        layers2 = g_list_remove (layers2, iter->data);
    }

  private = GIMP_IMAGE_GET_PRIVATE (image);

  floating_sel = gimp_image_get_floating_selection (image);

  /*  Make sure the floating_sel always is the active layer  */
  if (floating_sel && (g_list_length (layers2) != 1 || layers2->data != floating_sel))
    {
      g_list_free (layers2);
      return;
    }

  selected_layers = gimp_image_get_selected_layers (image);

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
      GList *layers3;

      /*  Don't cache selection info for the previous active layer  */
      if (selected_layers)
        gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (selected_layers->data));

      layers3 = g_list_copy (layers2);
      gimp_item_tree_set_selected_items (private->layers, layers2);

      /* We cannot edit masks with multiple selected layers. */
      if (g_list_length (layers3) > 1)
        {
          for (iter = layers3; iter; iter = iter->next)
            {
              if (gimp_layer_get_mask (iter->data))
                gimp_layer_set_edit_mask (iter->data, FALSE);
            }
        }

      g_list_free (layers3);
    }
  else
    {
      g_list_free (layers2);
    }
}

void
gimp_image_set_selected_channels (GimpImage *image,
                                  GList     *channels)
{
  GimpImagePrivate *private;
  GList            *iter;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  for (iter = channels; iter; iter = iter->next)
    {
      g_return_if_fail (GIMP_IS_CHANNEL (iter->data));
      g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (iter->data)) &&
                        gimp_item_get_image (GIMP_ITEM (iter->data)) == image);
    }
  private = GIMP_IMAGE_GET_PRIVATE (image);

  /*  Not if there is a floating selection  */
  if (g_list_length (channels) > 0 && gimp_image_get_floating_selection (image))
    return;

  gimp_item_tree_set_selected_items (private->channels, g_list_copy (channels));
}

void
gimp_image_set_selected_paths (GimpImage *image,
                               GList     *paths)
{
  GimpImagePrivate *private;
  GList            *iter;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  for (iter = paths; iter; iter = iter->next)
    {
      g_return_if_fail (GIMP_IS_PATH (iter->data));
      g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (iter->data)) &&
                        gimp_item_get_image (GIMP_ITEM (iter->data)) == image);
    }

  private = GIMP_IMAGE_GET_PRIVATE (image);

  gimp_item_tree_set_selected_items (private->paths, g_list_copy (paths));
}


/*  layer, channel, path by tattoo  */

GimpLayer *
gimp_image_get_layer_by_tattoo (GimpImage  *image,
                                GimpTattoo  tattoo)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  stack = GIMP_ITEM_STACK (gimp_image_get_layers (image));

  return GIMP_LAYER (gimp_item_stack_get_item_by_tattoo (stack, tattoo));
}

GimpChannel *
gimp_image_get_channel_by_tattoo (GimpImage  *image,
                                  GimpTattoo  tattoo)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  stack = GIMP_ITEM_STACK (gimp_image_get_channels (image));

  return GIMP_CHANNEL (gimp_item_stack_get_item_by_tattoo (stack, tattoo));
}

GimpPath *
gimp_image_get_path_by_tattoo (GimpImage  *image,
                               GimpTattoo  tattoo)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  stack = GIMP_ITEM_STACK (gimp_image_get_paths (image));

  return GIMP_PATH (gimp_item_stack_get_item_by_tattoo (stack, tattoo));
}


/*  layer, channel, path by name  */

GimpLayer *
gimp_image_get_layer_by_name (GimpImage   *image,
                              const gchar *name)
{
  GimpItemTree *tree;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  tree = gimp_image_get_layer_tree (image);

  return GIMP_LAYER (gimp_item_tree_get_item_by_name (tree, name));
}

GimpChannel *
gimp_image_get_channel_by_name (GimpImage   *image,
                                const gchar *name)
{
  GimpItemTree *tree;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  tree = gimp_image_get_channel_tree (image);

  return GIMP_CHANNEL (gimp_item_tree_get_item_by_name (tree, name));
}

GimpPath *
gimp_image_get_path_by_name (GimpImage   *image,
                             const gchar *name)
{
  GimpItemTree *tree;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  tree = gimp_image_get_path_tree (image);

  return GIMP_PATH (gimp_item_tree_get_item_by_name (tree, name));
}


/*  items  */

gboolean
gimp_image_reorder_item (GimpImage   *image,
                         GimpItem    *item,
                         GimpItem    *new_parent,
                         gint         new_index,
                         gboolean     push_undo,
                         const gchar *undo_desc)
{
  GimpItemTree *tree;
  gboolean      result;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (gimp_item_get_image (item) == image, FALSE);

  tree = gimp_item_get_tree (item);

  g_return_val_if_fail (tree != NULL, FALSE);

  if (push_undo)
    {
      if (! undo_desc)
        undo_desc = GIMP_ITEM_GET_CLASS (item)->reorder_desc;

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_ITEM_REORDER,
                                   undo_desc);
    }

  gimp_image_freeze_bounding_box (image);

  gimp_item_start_move (item, push_undo);

  /*  item and new_parent are type-checked in GimpItemTree
   */
  result = gimp_item_tree_reorder_item (tree, item,
                                        new_parent, new_index,
                                        push_undo, undo_desc);

  gimp_item_end_move (item, push_undo);

  gimp_image_thaw_bounding_box (image);

  if (push_undo)
    gimp_image_undo_group_end (image);

  return result;
}

gboolean
gimp_image_raise_item (GimpImage *image,
                       GimpItem  *item,
                       GError    **error)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  index = gimp_item_get_index (item);

  g_return_val_if_fail (index != -1, FALSE);

  if (index == 0)
    {
      g_set_error_literal (error,  GIMP_ERROR, GIMP_FAILED,
                           GIMP_ITEM_GET_CLASS (item)->raise_failed);
      return FALSE;
    }

  return gimp_image_reorder_item (image, item,
                                  gimp_item_get_parent (item), index - 1,
                                  TRUE, GIMP_ITEM_GET_CLASS (item)->raise_desc);
}

gboolean
gimp_image_raise_item_to_top (GimpImage *image,
                              GimpItem  *item)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  return gimp_image_reorder_item (image, item,
                                  gimp_item_get_parent (item), 0,
                                  TRUE, GIMP_ITEM_GET_CLASS (item)->raise_to_top_desc);
}

gboolean
gimp_image_lower_item (GimpImage *image,
                       GimpItem  *item,
                       GError    **error)
{
  GimpContainer *container;
  gint           index;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  container = gimp_item_get_container (item);

  g_return_val_if_fail (container != NULL, FALSE);

  index = gimp_item_get_index (item);

  if (index == gimp_container_get_n_children (container) - 1)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           GIMP_ITEM_GET_CLASS (item)->lower_failed);
      return FALSE;
    }

  return gimp_image_reorder_item (image, item,
                                  gimp_item_get_parent (item), index + 1,
                                  TRUE, GIMP_ITEM_GET_CLASS (item)->lower_desc);
}

gboolean
gimp_image_lower_item_to_bottom (GimpImage *image,
                                  GimpItem *item)
{
  GimpContainer *container;
  gint           length;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  container = gimp_item_get_container (item);

  g_return_val_if_fail (container != NULL, FALSE);

  length = gimp_container_get_n_children (container);

  return gimp_image_reorder_item (image, item,
                                  gimp_item_get_parent (item), length - 1,
                                  TRUE, GIMP_ITEM_GET_CLASS (item)->lower_to_bottom_desc);
}


gboolean
gimp_image_add_item (GimpImage *image,
                     GimpItem  *item,
                     GimpItem  *parent,
                     gint       position,
                     gboolean   push_undo)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  if (GIMP_IS_LAYER (item))
    {
      return gimp_image_add_layer (image, GIMP_LAYER (item),
                                   GIMP_LAYER (parent),
                                   position, push_undo);
    }
  else if (GIMP_IS_CHANNEL (item))
    {
      return gimp_image_add_channel (image, GIMP_CHANNEL (item),
                                     GIMP_CHANNEL (parent),
                                     position, push_undo);
    }
  else if (GIMP_IS_PATH (item))
    {
      return gimp_image_add_path (image, GIMP_PATH (item),
                                  GIMP_PATH (parent),
                                  position, push_undo);
    }

  g_return_val_if_reached (FALSE);
}

void
gimp_image_remove_item (GimpImage *image,
                        GimpItem  *item,
                        gboolean   push_undo,
                        GList     *new_selected)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  if (GIMP_IS_LAYER (item))
    {
      gimp_image_remove_layer (image, GIMP_LAYER (item),
                               push_undo, new_selected);
    }
  else if (GIMP_IS_CHANNEL (item))
    {
      gimp_image_remove_channel (image, GIMP_CHANNEL (item),
                                 push_undo, new_selected);
    }
  else if (GIMP_IS_PATH (item))
    {
      gimp_image_remove_path (image, GIMP_PATH (item),
                              push_undo, new_selected);
    }
  else
    {
      g_return_if_reached ();
    }
}


/*  layers  */

gboolean
gimp_image_add_layer (GimpImage *image,
                      GimpLayer *layer,
                      GimpLayer *parent,
                      gint       position,
                      gboolean   push_undo)
{
  GimpImagePrivate *private;
  GList            *selected_layers;
  gboolean          old_has_alpha;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  /*  item and parent are type-checked in GimpItemTree
   */
  if (! gimp_item_tree_get_insert_pos (private->layers,
                                       (GimpItem *) layer,
                                       (GimpItem **) &parent,
                                       &position))
    return FALSE;

  gimp_image_unset_default_new_layer_mode (image);

  /*  If there is a floating selection (and this isn't it!),
   *  make sure the insert position is greater than 0
   */
  if (parent == NULL && position == 0 &&
      gimp_image_get_floating_selection (image))
    position = 1;

  old_has_alpha = gimp_image_has_alpha (image);

  if (push_undo)
    gimp_image_undo_push_layer_add (image, C_("undo-type", "Add Layer"),
                                    layer,
                                    gimp_image_get_selected_layers (image));

  gimp_item_tree_add_item (private->layers, GIMP_ITEM (layer),
                           GIMP_ITEM (parent), position);

  selected_layers = g_list_prepend (NULL, layer);
  gimp_image_set_selected_layers (image, selected_layers);
  g_list_free (selected_layers);

  /*  If the layer is a floating selection, attach it to the drawable  */
  if (gimp_layer_is_floating_sel (layer))
    gimp_drawable_attach_floating_sel (gimp_layer_get_floating_sel_drawable (layer),
                                       layer);

  /* If the layer is a vector layer, also add its path to the image */
  if (gimp_item_is_vector_layer (GIMP_ITEM (layer)))
    {
      GimpPath *path = gimp_vector_layer_get_path (GIMP_VECTOR_LAYER (layer));

      if (path                                         &&
          (! gimp_item_is_attached (GIMP_ITEM (path))) &&
          gimp_item_get_image (GIMP_ITEM (path)) == image)
        gimp_image_add_path (image, path, NULL, -1, FALSE);
    }

  if (old_has_alpha != gimp_image_has_alpha (image))
    private->flush_accum.alpha_changed = TRUE;

  return TRUE;
}

void
gimp_image_remove_layer (GimpImage *image,
                         GimpLayer *layer,
                         gboolean   push_undo,
                         GList     *new_selected)
{
  GimpImagePrivate *private;
  GList            *selected_layers;
  gboolean          old_has_alpha;
  const gchar      *undo_desc;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)));
  g_return_if_fail (gimp_item_get_image (GIMP_ITEM (layer)) == image);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  gimp_image_unset_default_new_layer_mode (image);

  if (push_undo)
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                 C_("undo-type", "Remove Layer"));

  gimp_item_start_move (GIMP_ITEM (layer), push_undo);

  if (gimp_drawable_get_floating_sel (GIMP_DRAWABLE (layer)))
    {
      if (! push_undo)
        {
          g_warning ("%s() was called from an undo function while the layer "
                     "had a floating selection. Please report this at "
                     "https://www.gimp.org/bugs/", G_STRFUNC);
          return;
        }

      gimp_image_remove_layer (image,
                               gimp_drawable_get_floating_sel (GIMP_DRAWABLE (layer)),
                               TRUE, NULL);
    }

  selected_layers = g_list_copy (gimp_image_get_selected_layers (image));

  old_has_alpha = gimp_image_has_alpha (image);

  if (gimp_layer_is_floating_sel (layer))
    {
      undo_desc = C_("undo-type", "Remove Floating Selection");

      gimp_drawable_detach_floating_sel (gimp_layer_get_floating_sel_drawable (layer));
    }
  else
    {
      undo_desc = C_("undo-type", "Remove Layer");
    }

  if (push_undo)
    {
      gimp_image_rec_filter_remove_undo (image, layer);
      gimp_image_undo_push_layer_remove (image, undo_desc, layer,
                                         gimp_layer_get_parent (layer),
                                         gimp_item_get_index (GIMP_ITEM (layer)),
                                         selected_layers);
    }

  g_object_ref (layer);

  /*  Make sure we're not caching any old selection info  */
  if (g_list_find (selected_layers, layer))
    gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (layer));

  /* Remove layer and its children from the MRU layer stack. */
  gimp_image_remove_from_layer_stack (image, layer);

  new_selected = gimp_item_tree_remove_item (private->layers,
                                             GIMP_ITEM (layer),
                                             new_selected);

  if (gimp_layer_is_floating_sel (layer))
    {
      /*  If this was the floating selection, activate the underlying drawable
       */
      floating_sel_activate_drawable (layer);
    }
  else if (selected_layers &&
           (g_list_find (selected_layers, layer) ||
            g_list_find_custom (selected_layers, layer,
                                (GCompareFunc) gimp_image_selected_is_descendant)))
    {
      gimp_image_set_selected_layers (image, new_selected);
    }

  gimp_item_end_move (GIMP_ITEM (layer), push_undo);

  g_object_unref (layer);
  g_list_free (selected_layers);
  if (new_selected)
    g_list_free (new_selected);

  if (old_has_alpha != gimp_image_has_alpha (image))
    private->flush_accum.alpha_changed = TRUE;

  if (push_undo)
    gimp_image_undo_group_end (image);
}

void
gimp_image_add_layers (GimpImage   *image,
                       GList       *layers,
                       GimpLayer   *parent,
                       gint         position,
                       gint         x,
                       gint         y,
                       gint         width,
                       gint         height,
                       const gchar *undo_desc)
{
  GimpImagePrivate *private;
  GList            *list;
  gint              layers_x      = G_MAXINT;
  gint              layers_y      = G_MAXINT;
  gint              layers_width  = 0;
  gint              layers_height = 0;
  gint              offset_x;
  gint              offset_y;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (layers != NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  /*  item and parent are type-checked in GimpItemTree
   */
  if (! gimp_item_tree_get_insert_pos (private->layers,
                                       (GimpItem *) layers->data,
                                       (GimpItem **) &parent,
                                       &position))
    return;

  for (list = layers; list; list = g_list_next (list))
    {
      GimpItem *item = GIMP_ITEM (list->data);
      gint      off_x, off_y;

      gimp_item_get_offset (item, &off_x, &off_y);

      layers_x = MIN (layers_x, off_x);
      layers_y = MIN (layers_y, off_y);

      layers_width  = MAX (layers_width,
                           off_x + gimp_item_get_width (item)  - layers_x);
      layers_height = MAX (layers_height,
                           off_y + gimp_item_get_height (item) - layers_y);
    }

  offset_x = x + (width  - layers_width)  / 2 - layers_x;
  offset_y = y + (height - layers_height) / 2 - layers_y;

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_LAYER_ADD, undo_desc);

  for (list = layers; list; list = g_list_next (list))
    {
      GimpItem *new_item = GIMP_ITEM (list->data);

      gimp_item_translate (new_item, offset_x, offset_y, FALSE);

      gimp_image_add_layer (image, GIMP_LAYER (new_item),
                            parent, position, TRUE);
      gimp_drawable_enable_resize_undo (GIMP_DRAWABLE (new_item));
      position++;
    }

  if (layers)
    gimp_image_set_selected_layers (image, layers);

  gimp_image_undo_group_end (image);
}


/*  channels  */

gboolean
gimp_image_add_channel (GimpImage   *image,
                        GimpChannel *channel,
                        GimpChannel *parent,
                        gint         position,
                        gboolean     push_undo)
{
  GimpImagePrivate *private;
  GList            *channels;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  /*  item and parent are type-checked in GimpItemTree
   */
  if (! gimp_item_tree_get_insert_pos (private->channels,
                                       (GimpItem *) channel,
                                       (GimpItem **) &parent,
                                       &position))
    return FALSE;

  if (push_undo)
    gimp_image_undo_push_channel_add (image, C_("undo-type", "Add Channel"),
                                      channel,
                                      gimp_image_get_selected_channels (image));

  gimp_item_tree_add_item (private->channels, GIMP_ITEM (channel),
                           GIMP_ITEM (parent), position);

  channels = g_list_prepend (NULL, channel);
  gimp_image_set_selected_channels (image, channels);
  g_list_free (channels);

  return TRUE;
}

void
gimp_image_remove_channel (GimpImage   *image,
                           GimpChannel *channel,
                           gboolean     push_undo,
                           GList       *new_selected)
{
  GimpImagePrivate *private;
  GList            *selected_channels;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)));
  g_return_if_fail (gimp_item_get_image (GIMP_ITEM (channel)) == image);

  if (push_undo)
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                 C_("undo-type", "Remove Channel"));

  gimp_item_start_move (GIMP_ITEM (channel), push_undo);

  if (gimp_drawable_get_floating_sel (GIMP_DRAWABLE (channel)))
    {
      if (! push_undo)
        {
          g_warning ("%s() was called from an undo function while the channel "
                     "had a floating selection. Please report this at "
                     "https://www.gimp.org/bugs/", G_STRFUNC);
          return;
        }

      gimp_image_remove_layer (image,
                               gimp_drawable_get_floating_sel (GIMP_DRAWABLE (channel)),
                               TRUE, NULL);
    }

  private = GIMP_IMAGE_GET_PRIVATE (image);

  selected_channels = gimp_image_get_selected_channels (image);
  selected_channels = g_list_copy (selected_channels);

  if (push_undo)
    gimp_image_undo_push_channel_remove (image, C_("undo-type", "Remove Channel"), channel,
                                         gimp_channel_get_parent (channel),
                                         gimp_item_get_index (GIMP_ITEM (channel)),
                                         selected_channels);

  g_object_ref (channel);

  new_selected = gimp_item_tree_remove_item (private->channels,
                                             GIMP_ITEM (channel),
                                             new_selected);

  if (selected_channels &&
      (g_list_find (selected_channels, channel) ||
       g_list_find_custom (selected_channels, channel,
                           (GCompareFunc) gimp_image_selected_is_descendant)))
    {
      if (new_selected)
        gimp_image_set_selected_channels (image, new_selected);
      else
        gimp_image_unset_selected_channels (image);
    }

  g_list_free (selected_channels);

  gimp_item_end_move (GIMP_ITEM (channel), push_undo);

  g_object_unref (channel);
  if (new_selected)
    g_list_free (new_selected);

  if (push_undo)
    gimp_image_undo_group_end (image);
}


/*  paths  */

gboolean
gimp_image_add_path (GimpImage   *image,
                     GimpPath    *path,
                     GimpPath    *parent,
                     gint         position,
                     gboolean     push_undo)
{
  GimpImagePrivate *private;
  GList            *list = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  /*  item and parent are type-checked in GimpItemTree
   */
  if (! gimp_item_tree_get_insert_pos (private->paths,
                                       (GimpItem *) path,
                                       (GimpItem **) &parent,
                                       &position))
    return FALSE;

  if (push_undo)
    gimp_image_undo_push_path_add (image, C_("undo-type", "Add Path"),
                                   path,
                                   gimp_image_get_selected_paths (image));

  gimp_item_tree_add_item (private->paths, GIMP_ITEM (path),
                           GIMP_ITEM (parent), position);

  if (path != NULL)
    list = g_list_prepend (NULL, path);

  gimp_image_set_selected_paths (image, list);

  g_list_free (list);

  return TRUE;
}

void
gimp_image_remove_path (GimpImage   *image,
                        GimpPath    *path,
                        gboolean     push_undo,
                        GList       *new_selected)
{
  GimpImagePrivate *private;
  GList            *selected_path;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_PATH (path));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (path)));
  g_return_if_fail (gimp_item_get_image (GIMP_ITEM (path)) == image);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (push_undo)
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                 C_("undo-type", "Remove Path"));

  gimp_item_start_move (GIMP_ITEM (path), push_undo);

  selected_path = gimp_image_get_selected_paths (image);
  selected_path = g_list_copy (selected_path);

  if (push_undo)
    gimp_image_undo_push_path_remove (image, C_("undo-type", "Remove Path"), path,
                                      gimp_path_get_parent (path),
                                      gimp_item_get_index (GIMP_ITEM (path)),
                                      selected_path);

  g_object_ref (path);

  new_selected = gimp_item_tree_remove_item (private->paths,
                                             GIMP_ITEM (path),
                                             new_selected);

  if (selected_path &&
      (g_list_find (selected_path, path) ||
       g_list_find_custom (selected_path, path,
                           (GCompareFunc) gimp_image_selected_is_descendant)))
    {
      gimp_image_set_selected_paths (image, new_selected);
    }

  g_list_free (selected_path);

  gimp_item_end_move (GIMP_ITEM (path), push_undo);

  g_object_unref (path);
  if (new_selected)
    g_list_free (new_selected);

  if (push_undo)
    gimp_image_undo_group_end (image);
}


/*  item sets  */

/*
 * gimp_image_store_item_set:
 * @image:
 * @set: (transfer full): a set of items which @images takes ownership
 *                        of.
 *
 * Store a new set of @layers.
 * If a set with the same name and type existed, this call will silently
 * replace it with the new set of layers.
 */
void
gimp_image_store_item_set (GimpImage    *image,
                           GimpItemList *set)
{
  GimpImagePrivate  *private;
  GList            **stored_sets;
  GList             *iter;
  guint              signal;
  GType              item_type;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_ITEM_LIST (set));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  signal    = gimp_image_signals[ITEM_SETS_CHANGED];
  item_type = gimp_item_list_get_item_type (set);
  if (item_type == GIMP_TYPE_LAYER)
    stored_sets = &private->stored_layer_sets;
  else if (item_type == GIMP_TYPE_CHANNEL)
    stored_sets = &private->stored_channel_sets;
  else if (item_type == GIMP_TYPE_PATH)
    stored_sets = &private->stored_path_sets;
  else
    g_return_if_reached ();

  for (iter = *stored_sets; iter; iter = iter->next)
    {
      gboolean         is_pattern;
      gboolean         is_pattern2;
      GimpSelectMethod pattern_syntax;
      GimpSelectMethod pattern_syntax2;

      is_pattern  = gimp_item_list_is_pattern (iter->data, &pattern_syntax);
      is_pattern2 = gimp_item_list_is_pattern (set, &pattern_syntax2);

      /* Remove a previous item set of same type and name. */
      if (is_pattern == is_pattern2 && (! is_pattern || pattern_syntax == pattern_syntax2) &&
          g_strcmp0 (gimp_object_get_name (iter->data), gimp_object_get_name (set)) == 0)
        break;
    }
  if (iter)
    {
      g_object_unref (iter->data);
      *stored_sets = g_list_delete_link (*stored_sets, iter);
    }

  *stored_sets = g_list_prepend (*stored_sets, set);
  g_signal_emit (image, signal, 0, item_type);
}

/*
 * @gimp_image_unlink_item_set:
 * @image:
 * @link_name:
 *
 * Remove the set of layers named @link_name.
 *
 * Returns: %TRUE if the set was removed, %FALSE if no sets with this
 *          name existed.
 */
gboolean
gimp_image_unlink_item_set (GimpImage    *image,
                            GimpItemList *set)
{
  GimpImagePrivate  *private;
  GList             *found;
  GList            **stored_sets;
  guint              signal;
  GType              item_type;
  gboolean           success;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  signal    = gimp_image_signals[ITEM_SETS_CHANGED];
  item_type = gimp_item_list_get_item_type (set);
  if (item_type == GIMP_TYPE_LAYER)
    stored_sets = &private->stored_layer_sets;
  else if (item_type == GIMP_TYPE_CHANNEL)
    stored_sets = &private->stored_channel_sets;
  else if (item_type == GIMP_TYPE_PATH)
    stored_sets = &private->stored_path_sets;
  else
    g_return_val_if_reached (FALSE);

  found = g_list_find (*stored_sets, set);
  success = (found != NULL);
  if (success)
    {
      *stored_sets = g_list_delete_link (*stored_sets, found);
      g_object_unref (set);
      g_signal_emit (image, signal, 0, item_type);
    }

  return success;
}

/*
 * @gimp_image_get_stored_item_sets:
 * @image:
 * @item_type:
 *
 * Returns: (transfer none): the list of all the layer sets (which you
 *          should not modify). Order of items is relevant.
 */
GList *
gimp_image_get_stored_item_sets (GimpImage *image,
                                 GType      item_type)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (item_type == GIMP_TYPE_LAYER)
    return private->stored_layer_sets;
  else if (item_type == GIMP_TYPE_CHANNEL)
    return private->stored_channel_sets;
  else if (item_type == GIMP_TYPE_PATH)
    return private->stored_path_sets;

  g_return_val_if_reached (FALSE);
}

/*
 * @gimp_image_select_item_set:
 * @image:
 * @set:
 *
 * Replace currently selected layers in @image with the layers belonging
 * to @set.
 */
void
gimp_image_select_item_set (GimpImage    *image,
                            GimpItemList *set)
{
  GList  *items;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_ITEM_LIST (set));

  items = gimp_item_list_get_items (set, &error);

  if (! error)
    {
      GType item_type = gimp_item_list_get_item_type (set);

      if (item_type == GIMP_TYPE_LAYER)
        gimp_image_set_selected_layers (image, items);
      else if (item_type == GIMP_TYPE_CHANNEL)
        gimp_image_set_selected_channels (image, items);
      else if (item_type == GIMP_TYPE_PATH)
        gimp_image_set_selected_paths (image, items);
      else
        g_return_if_reached ();
    }

  g_list_free (items);
  g_clear_error (&error);
}

/*
 * @gimp_image_add_item_set:
 * @image:
 * @set:
 *
 * Add the layers belonging to @set to the items currently selected in
 * @image.
 */
void
gimp_image_add_item_set (GimpImage    *image,
                         GimpItemList *set)
{
  GList  *items;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_ITEM_LIST (set));

  items = gimp_item_list_get_items (set, &error);

  if (! error)
    {
      GList *selected;
      GList *iter;
      GType  item_type = gimp_item_list_get_item_type (set);

      if (item_type == GIMP_TYPE_LAYER)
        selected = gimp_image_get_selected_layers (image);
      else if (item_type == GIMP_TYPE_CHANNEL)
        selected = gimp_image_get_selected_channels (image);
      else if (item_type == GIMP_TYPE_PATH)
        selected = gimp_image_get_selected_paths (image);
      else
        g_return_if_reached ();

      selected = g_list_copy (selected);
      for (iter = items; iter; iter = iter->next)
        {
          if (! g_list_find (selected, iter->data))
            selected = g_list_prepend (selected, iter->data);
        }

      if (item_type == GIMP_TYPE_LAYER)
        gimp_image_set_selected_layers (image, selected);
      else if (item_type == GIMP_TYPE_CHANNEL)
        gimp_image_set_selected_channels (image, selected);
      else if (item_type == GIMP_TYPE_PATH)
        gimp_image_set_selected_paths (image, items);

      g_list_free (selected);
    }
  g_clear_error (&error);
}

/*
 * @gimp_image_remove_item_set:
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
gimp_image_remove_item_set (GimpImage    *image,
                            GimpItemList *set)
{
  GList  *items;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_ITEM_LIST (set));

  items = gimp_item_list_get_items (set, &error);

  if (! error)
    {
      GList *selected;
      GList *iter;
      GType  item_type = gimp_item_list_get_item_type (set);

      if (item_type == GIMP_TYPE_LAYER)
        selected = gimp_image_get_selected_layers (image);
      else if (item_type == GIMP_TYPE_CHANNEL)
        selected = gimp_image_get_selected_channels (image);
      else if (item_type == GIMP_TYPE_PATH)
        selected = gimp_image_get_selected_paths (image);
      else
        g_return_if_reached ();

      selected = g_list_copy (selected);
      for (iter = items; iter; iter = iter->next)
        {
          GList *remove;

          if ((remove = g_list_find (selected, iter->data)))
            selected = g_list_delete_link (selected, remove);
        }

      if (item_type == GIMP_TYPE_LAYER)
        gimp_image_set_selected_layers (image, selected);
      else if (item_type == GIMP_TYPE_CHANNEL)
        gimp_image_set_selected_channels (image, selected);
      else if (item_type == GIMP_TYPE_PATH)
        gimp_image_set_selected_paths (image, items);

      g_list_free (selected);
    }
  g_clear_error (&error);
}

/*
 * @gimp_image_intersect_item_set:
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
gimp_image_intersect_item_set (GimpImage    *image,
                               GimpItemList *set)
{
  GList *items;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_ITEM_LIST (set));

  items = gimp_item_list_get_items (set, &error);

  if (! error)
    {
      GList *selected;
      GList *remove = NULL;
      GList *iter;
      GType  item_type = gimp_item_list_get_item_type (set);

      if (item_type == GIMP_TYPE_LAYER)
        selected = gimp_image_get_selected_layers (image);
      else if (item_type == GIMP_TYPE_CHANNEL)
        selected = gimp_image_get_selected_channels (image);
      else if (item_type == GIMP_TYPE_PATH)
        selected = gimp_image_get_selected_paths (image);
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
      if (item_type == GIMP_TYPE_LAYER)
        gimp_image_set_selected_layers (image, selected);
      else if (item_type == GIMP_TYPE_CHANNEL)
        gimp_image_set_selected_channels (image, selected);
      else if (item_type == GIMP_TYPE_PATH)
        gimp_image_set_selected_paths (image, items);

      g_list_free (selected);
    }
  g_clear_error (&error);
}


/*  hidden items  */

/* Sometimes you want to create a channel or other types of drawables to
 * work on them without adding them to the public trees and to be
 * visible in the GUI. This is when you create hidden items. No undo
 * steps will ever be created either for any processing on these items.
 *
 * Note that you are expected to manage the hidden items properly. In
 * particular, once you are done with them, remove them with
 * gimp_image_remove_hidden_item() and free them.
 * @image is not assuming ownership of @item.
 */
gboolean
gimp_image_add_hidden_item (GimpImage *image,
                            GimpItem  *item)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (! gimp_item_is_attached (item), FALSE);
  g_return_val_if_fail (gimp_item_get_image (item) == image, FALSE);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  private->hidden_items = g_list_prepend (private->hidden_items, item);

  return TRUE;
}

void
gimp_image_remove_hidden_item (GimpImage *image,
                               GimpItem  *item)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_get_image (item) == image);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (g_list_find (private->hidden_items, item) != NULL);

  private->hidden_items = g_list_remove (private->hidden_items, item);
}

gboolean
gimp_image_is_hidden_item (GimpImage *image,
                           GimpItem  *item)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (gimp_item_get_image (item) == image, FALSE);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  return (g_list_find (private->hidden_items, item) != NULL);
}

gboolean
gimp_image_coords_in_active_pickable (GimpImage        *image,
                                      const GimpCoords *coords,
                                      gboolean          show_all,
                                      gboolean          sample_merged,
                                      gboolean          selected_only)
{
  gint     x, y;
  gboolean in_pickable = FALSE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  x = floor (coords->x);
  y = floor (coords->y);

  if (sample_merged)
    {
      if (show_all || (x >= 0 && x < gimp_image_get_width  (image) &&
                       y >= 0 && y < gimp_image_get_height (image)))
        {
          in_pickable = TRUE;
        }
    }
  else
    {
      GList *drawables = gimp_image_get_selected_drawables (image);
      GList *iter;

      for (iter = drawables; iter; iter = iter->next)
        {
          GimpItem *item = iter->data;
          gint      off_x, off_y;
          gint      d_x, d_y;

          gimp_item_get_offset (item, &off_x, &off_y);

          d_x = x - off_x;
          d_y = y - off_y;

          if (d_x >= 0 && d_x < gimp_item_get_width  (item) &&
              d_y >= 0 && d_y < gimp_item_get_height (item))
            {
              in_pickable = TRUE;
              break;
            }
        }
      g_list_free (drawables);
    }

  if (in_pickable && selected_only)
    {
      GimpChannel *selection = gimp_image_get_mask (image);

      if (! gimp_channel_is_empty (selection) &&
          ! gimp_pickable_get_opacity_at (GIMP_PICKABLE (selection),
                                          x, y))
        {
          in_pickable = FALSE;
        }
    }

  return in_pickable;
}

void
gimp_image_invalidate_previews (GimpImage *image)
{
  GimpItemStack *layers;
  GimpItemStack *channels;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  layers   = GIMP_ITEM_STACK (gimp_image_get_layers (image));
  channels = GIMP_ITEM_STACK (gimp_image_get_channels (image));

  gimp_item_stack_invalidate_previews (layers);
  gimp_item_stack_invalidate_previews (channels);
}

/* Sets the image into a "converting" state, which is there to warn other code
 * (such as shell render code) that the image properties might be in an
 * inconsistent state. For instance when converting to another precision with
 * gimp_image_convert_precision(), the babl format may be updated first, and the
 * profile later, after all drawables are converted. Rendering the image
 * in-between would at best render broken previews (at worst, crash, e.g.
 * because we depend on allocated data which might have become too small).
 */
void
gimp_image_set_converting (GimpImage *image,
                           gboolean   converting)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_object_set (image,
                "converting", converting,
                NULL);
}

gboolean
gimp_image_get_converting (GimpImage *image)
{
  GimpImagePrivate *private;
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  return private->converting;
}
