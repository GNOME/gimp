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

#include "gegl/gimp-babl.h"

#include "gimp.h"
#include "gimp-memsize.h"
#include "gimp-parasites.h"
#include "gimpcontext.h"
#include "gimpdrawablestack.h"
#include "gimpgrid.h"
#include "gimperror.h"
#include "gimpguide.h"
#include "gimpidtable.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpimage-guides.h"
#include "gimpimage-item-list.h"
#include "gimpimage-metadata.h"
#include "gimpimage-sample-points.h"
#include "gimpimage-preview.h"
#include "gimpimage-private.h"
#include "gimpimage-profile.h"
#include "gimpimage-quick-mask.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimpitemtree.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplayermask.h"
#include "gimpmarshal.h"
#include "gimpparasitelist.h"
#include "gimppickable.h"
#include "gimpprojectable.h"
#include "gimpprojection.h"
#include "gimpsamplepoint.h"
#include "gimpselection.h"
#include "gimptempbuf.h"
#include "gimptemplate.h"
#include "gimpundostack.h"

#include "file/file-utils.h"

#include "vectors/gimpvectors.h"

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
  ACTIVE_LAYER_CHANGED,
  ACTIVE_CHANNEL_CHANGED,
  ACTIVE_VECTORS_CHANGED,
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
  PROP_BUFFER
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

static void        gimp_image_projectable_flush  (GimpProjectable   *projectable,
                                                  gboolean           invalidate_preview);
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

static void     gimp_image_mask_update           (GimpDrawable      *drawable,
                                                  gint               x,
                                                  gint               y,
                                                  gint               width,
                                                  gint               height,
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
static void     gimp_image_active_layer_notify   (GimpItemTree      *tree,
                                                  const GParamSpec  *pspec,
                                                  GimpImage         *image);
static void     gimp_image_active_channel_notify (GimpItemTree      *tree,
                                                  const GParamSpec  *pspec,
                                                  GimpImage         *image);
static void     gimp_image_active_vectors_notify (GimpItemTree      *tree,
                                                  const GParamSpec  *pspec,
                                                  GimpImage         *image);


G_DEFINE_TYPE_WITH_CODE (GimpImage, gimp_image, GIMP_TYPE_VIEWABLE,
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
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[PRECISION_CHANGED] =
    g_signal_new ("precision-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, precision_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[ALPHA_CHANGED] =
    g_signal_new ("alpha-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, alpha_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[FLOATING_SELECTION_CHANGED] =
    g_signal_new ("floating-selection-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, floating_selection_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[ACTIVE_LAYER_CHANGED] =
    g_signal_new ("active-layer-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, active_layer_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[ACTIVE_CHANNEL_CHANGED] =
    g_signal_new ("active-channel-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, active_channel_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[ACTIVE_VECTORS_CHANGED] =
    g_signal_new ("active-vectors-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, active_vectors_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[COMPONENT_VISIBILITY_CHANGED] =
    g_signal_new ("component-visibility-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, component_visibility_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_CHANNEL_TYPE);

  gimp_image_signals[COMPONENT_ACTIVE_CHANGED] =
    g_signal_new ("component-active-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, component_active_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_CHANNEL_TYPE);

  gimp_image_signals[MASK_CHANGED] =
    g_signal_new ("mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, mask_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[RESOLUTION_CHANGED] =
    g_signal_new ("resolution-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, resolution_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
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
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[QUICK_MASK_CHANGED] =
    g_signal_new ("quick-mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, quick_mask_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[SELECTION_INVALIDATE] =
    g_signal_new ("selection-invalidate",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, selection_invalidate),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[CLEAN] =
    g_signal_new ("clean",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, clean),
                  NULL, NULL,
                  gimp_marshal_VOID__FLAGS,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DIRTY_MASK);

  gimp_image_signals[DIRTY] =
    g_signal_new ("dirty",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, dirty),
                  NULL, NULL,
                  gimp_marshal_VOID__FLAGS,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DIRTY_MASK);

  gimp_image_signals[SAVED] =
    g_signal_new ("saved",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, saved),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  G_TYPE_FILE);

  gimp_image_signals[EXPORTED] =
    g_signal_new ("exported",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, exported),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  G_TYPE_FILE);

  gimp_image_signals[GUIDE_ADDED] =
    g_signal_new ("guide-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, guide_added),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_GUIDE);

  gimp_image_signals[GUIDE_REMOVED] =
    g_signal_new ("guide-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, guide_removed),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_GUIDE);

  gimp_image_signals[GUIDE_MOVED] =
    g_signal_new ("guide-moved",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, guide_moved),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_GUIDE);

  gimp_image_signals[SAMPLE_POINT_ADDED] =
    g_signal_new ("sample-point-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, sample_point_added),
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  gimp_image_signals[SAMPLE_POINT_REMOVED] =
    g_signal_new ("sample-point-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, sample_point_removed),
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  gimp_image_signals[SAMPLE_POINT_MOVED] =
    g_signal_new ("sample-point-moved",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, sample_point_moved),
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  gimp_image_signals[PARASITE_ATTACHED] =
    g_signal_new ("parasite-attached",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, parasite_attached),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  gimp_image_signals[PARASITE_DETACHED] =
    g_signal_new ("parasite-detached",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, parasite_detached),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  gimp_image_signals[COLORMAP_CHANGED] =
    g_signal_new ("colormap-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, colormap_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__INT,
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

  object_class->constructed           = gimp_image_constructed;
  object_class->set_property          = gimp_image_set_property;
  object_class->get_property          = gimp_image_get_property;
  object_class->dispose               = gimp_image_dispose;
  object_class->finalize              = gimp_image_finalize;

  gimp_object_class->name_changed     = gimp_image_name_changed;
  gimp_object_class->get_memsize      = gimp_image_get_memsize;

  viewable_class->default_icon_name   = "gimp-image";
  viewable_class->get_size            = gimp_image_get_size;
  viewable_class->size_changed        = gimp_image_size_changed;
  viewable_class->get_preview_size    = gimp_image_get_preview_size;
  viewable_class->get_popup_size      = gimp_image_get_popup_size;
  viewable_class->get_new_preview     = gimp_image_get_new_preview;
  viewable_class->get_description     = gimp_image_get_description;

  klass->mode_changed                 = gimp_image_real_mode_changed;
  klass->precision_changed            = gimp_image_real_precision_changed;
  klass->alpha_changed                = NULL;
  klass->floating_selection_changed   = NULL;
  klass->active_layer_changed         = NULL;
  klass->active_channel_changed       = NULL;
  klass->active_vectors_changed       = NULL;
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
                                                      GIMP_PRECISION_U8_GAMMA,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_METADATA,
                                   g_param_spec_object ("metadata", NULL, NULL,
                                                        GEXIV2_TYPE_METADATA,
                                                        GIMP_PARAM_READABLE));

  g_object_class_override_property (object_class, PROP_BUFFER, "buffer");

  g_type_class_add_private (klass, sizeof (GimpImagePrivate));
}

static void
gimp_color_managed_iface_init (GimpColorManagedInterface *iface)
{
  iface->get_icc_profile = gimp_image_color_managed_get_icc_profile;
}

static void
gimp_projectable_iface_init (GimpProjectableInterface *iface)
{
  iface->flush              = gimp_image_projectable_flush;
  iface->get_image          = gimp_image_get_image;
  iface->get_format         = gimp_image_get_proj_format;
  iface->get_size           = (void (*) (GimpProjectable*, gint*, gint*)) gimp_image_get_size;
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
}

static void
gimp_image_init (GimpImage *image)
{
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);
  gint              i;

  private->ID                  = 0;

  private->load_proc           = NULL;
  private->save_proc           = NULL;

  private->width               = 0;
  private->height              = 0;
  private->xresolution         = 1.0;
  private->yresolution         = 1.0;
  private->resolution_unit     = GIMP_UNIT_INCH;
  private->base_type           = GIMP_RGB;
  private->precision           = GIMP_PRECISION_U8_GAMMA;

  private->colormap            = NULL;
  private->n_colors            = 0;
  private->palette             = NULL;

  private->metadata            = NULL;

  private->dirty               = 1;
  private->dirty_time          = 0;
  private->undo_freeze_count   = 0;

  private->export_dirty        = 1;

  private->instance_count      = 0;
  private->disp_count          = 0;

  private->tattoo_state        = 0;

  private->projection          = gimp_projection_new (GIMP_PROJECTABLE (image));

  private->guides              = NULL;
  private->grid                = NULL;
  private->sample_points       = NULL;

  private->layers              = gimp_item_tree_new (image,
                                                     GIMP_TYPE_DRAWABLE_STACK,
                                                     GIMP_TYPE_LAYER);
  private->channels            = gimp_item_tree_new (image,
                                                     GIMP_TYPE_DRAWABLE_STACK,
                                                     GIMP_TYPE_CHANNEL);
  private->vectors             = gimp_item_tree_new (image,
                                                     GIMP_TYPE_ITEM_STACK,
                                                     GIMP_TYPE_VECTORS);
  private->layer_stack         = NULL;

  g_signal_connect (private->layers, "notify::active-item",
                    G_CALLBACK (gimp_image_active_layer_notify),
                    image);
  g_signal_connect (private->channels, "notify::active-item",
                    G_CALLBACK (gimp_image_active_channel_notify),
                    image);
  g_signal_connect (private->vectors, "notify::active-item",
                    G_CALLBACK (gimp_image_active_vectors_notify),
                    image);

  g_signal_connect_swapped (private->layers->container, "update",
                            G_CALLBACK (gimp_image_invalidate),
                            image);

  private->layer_alpha_handler =
    gimp_container_add_handler (private->layers->container, "alpha-changed",
                                G_CALLBACK (gimp_image_layer_alpha_changed),
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
  gimp_rgba_set (&private->quick_mask_color, 1.0, 0.0, 0.0, 0.5);

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

  g_assert (GIMP_IS_GIMP (image->gimp));

  config = image->gimp->config;

  private->ID = gimp_id_table_insert (image->gimp->image_table, image);

  template = config->default_image;

  private->xresolution     = gimp_template_get_resolution_x (template);
  private->yresolution     = gimp_template_get_resolution_y (template);
  private->resolution_unit = gimp_template_get_resolution_unit (template);

  private->grid = gimp_config_duplicate (GIMP_CONFIG (config->default_grid));

  private->quick_mask_color = config->quick_mask_color;

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
  g_signal_connect_object (config, "notify::layer-previews",
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
    case PROP_ID:
      g_assert_not_reached ();
      break;
    case PROP_WIDTH:
      private->width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      private->height = g_value_get_int (value);
      break;
    case PROP_BASE_TYPE:
      private->base_type = g_value_get_enum (value);
      break;
    case PROP_PRECISION:
      private->precision = g_value_get_enum (value);
      break;
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

  if (private->colormap)
    gimp_image_colormap_dispose (image);

  gimp_image_undo_free (image);

  g_signal_handlers_disconnect_by_func (private->layers->container,
                                        gimp_image_invalidate,
                                        image);

  gimp_container_remove_handler (private->layers->container,
                                 private->layer_alpha_handler);

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

  gimp_container_foreach (private->layers->container,
                          (GFunc) gimp_item_removed, NULL);
  gimp_container_foreach (private->channels->container,
                          (GFunc) gimp_item_removed, NULL);
  gimp_container_foreach (private->vectors->container,
                          (GFunc) gimp_item_removed, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_image_finalize (GObject *object)
{
  GimpImage        *image   = GIMP_IMAGE (object);
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  if (private->projection)
    {
      g_object_unref (private->projection);
      private->projection = NULL;
    }

  if (private->graph)
    {
      g_object_unref (private->graph);
      private->graph = NULL;
      private->visible_mask = NULL;
    }

  if (private->colormap)
    gimp_image_colormap_free (image);

  if (private->metadata)
    {
      g_object_unref (private->metadata);
      private->metadata = NULL;
    }

  if (private->file)
    {
      g_object_unref (private->file);
      private->file = NULL;
    }

  if (private->imported_file)
    {
      g_object_unref (private->imported_file);
      private->imported_file = NULL;
    }

  if (private->exported_file)
    {
      g_object_unref (private->exported_file);
      private->exported_file = NULL;
    }

  if (private->save_a_copy_file)
    {
      g_object_unref (private->save_a_copy_file);
      private->save_a_copy_file = NULL;
    }

  if (private->untitled_file)
    {
      g_object_unref (private->untitled_file);
      private->untitled_file = NULL;
    }

  if (private->layers)
    {
      g_object_unref (private->layers);
      private->layers = NULL;
    }
  if (private->channels)
    {
      g_object_unref (private->channels);
      private->channels = NULL;
    }
  if (private->vectors)
    {
      g_object_unref (private->vectors);
      private->vectors = NULL;
    }
  if (private->layer_stack)
    {
      g_slist_free (private->layer_stack);
      private->layer_stack = NULL;
    }

  if (private->selection_mask)
    {
      g_object_unref (private->selection_mask);
      private->selection_mask = NULL;
    }

  if (private->parasites)
    {
      g_object_unref (private->parasites);
      private->parasites = NULL;
    }

  if (private->guides)
    {
      g_list_free_full (private->guides, (GDestroyNotify) g_object_unref);
      private->guides = NULL;
    }

  if (private->grid)
    {
      g_object_unref (private->grid);
      private->grid = NULL;
    }

  if (private->sample_points)
    {
      g_list_free_full (private->sample_points,
                        (GDestroyNotify) gimp_sample_point_unref);
      private->sample_points = NULL;
    }

  if (private->undo_stack)
    {
      g_object_unref (private->undo_stack);
      private->undo_stack = NULL;
    }
  if (private->redo_stack)
    {
      g_object_unref (private->redo_stack);
      private->redo_stack = NULL;
    }

  if (image->gimp && image->gimp->image_table)
    {
      gimp_id_table_remove (image->gimp->image_table, private->ID);
      image->gimp = NULL;
    }

  if (private->display_name)
    {
      g_free (private->display_name);
      private->display_name = NULL;
    }

  if (private->display_path)
    {
      g_free (private->display_path);
      private->display_path = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_image_name_changed (GimpObject *object)
{
  GimpImage        *image   = GIMP_IMAGE (object);
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);
  const gchar      *name;

  if (GIMP_OBJECT_CLASS (parent_class)->name_changed)
    GIMP_OBJECT_CLASS (parent_class)->name_changed (object);

  if (private->display_name)
    {
      g_free (private->display_name);
      private->display_name = NULL;
    }

  if (private->display_path)
    {
      g_free (private->display_path);
      private->display_path = NULL;
    }

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

  if (private->file)
    {
      g_object_unref (private->file);
      private->file = NULL;
    }

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

  if (gimp_image_get_colormap (image))
    memsize += GIMP_IMAGE_COLORMAP_SIZE;

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
  memsize += gimp_object_get_memsize (GIMP_OBJECT (private->vectors),
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
  GimpImage    *image = GIMP_IMAGE (viewable);
  GimpMetadata *metadata;
  GList        *all_items;
  GList        *list;

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

  all_items = gimp_image_get_vectors_list (image);
  g_list_free_full (all_items, (GDestroyNotify) gimp_viewable_size_changed);

  gimp_viewable_size_changed (GIMP_VIEWABLE (gimp_image_get_mask (image)));

  metadata = gimp_image_get_metadata (image);
  if (metadata)
    gimp_metadata_set_pixel_size (metadata,
                                  gimp_image_get_width  (image),
                                  gimp_image_get_height (image));

  gimp_projectable_structure_changed (GIMP_PROJECTABLE (image));
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
			  gimp_image_get_ID (image));
}

static void
gimp_image_real_mode_changed (GimpImage *image)
{
  gimp_projectable_structure_changed (GIMP_PROJECTABLE (image));
}

static void
gimp_image_real_precision_changed (GimpImage *image)
{
  GimpMetadata *metadata;

  metadata = gimp_image_get_metadata (image);
  if (metadata)
    {
      switch (gimp_image_get_component_type (image))
        {
        case GIMP_COMPONENT_TYPE_U8:
          gimp_metadata_set_bits_per_sample (metadata, 8);
          break;

        case GIMP_COMPONENT_TYPE_U16:
        case GIMP_COMPONENT_TYPE_HALF:
          gimp_metadata_set_bits_per_sample (metadata, 16);
          break;

        case GIMP_COMPONENT_TYPE_U32:
        case GIMP_COMPONENT_TYPE_FLOAT:
          gimp_metadata_set_bits_per_sample (metadata, 32);
          break;

        case GIMP_COMPONENT_TYPE_DOUBLE:
          gimp_metadata_set_bits_per_sample (metadata, 64);
          break;
        }
    }

  gimp_projectable_structure_changed (GIMP_PROJECTABLE (image));
}

static void
gimp_image_real_resolution_changed (GimpImage *image)
{
  GimpMetadata *metadata;

  metadata = gimp_image_get_metadata (image);
  if (metadata)
    {
      gdouble xres, yres;

      gimp_image_get_resolution (image, &xres, &yres);
      gimp_metadata_set_resolution (metadata, xres, yres,
                                    gimp_image_get_unit (image));
    }
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
  GimpMetadata *metadata;

  metadata = gimp_image_get_metadata (image);
  if (metadata)
    {
      gdouble xres, yres;

      gimp_image_get_resolution (image, &xres, &yres);
      gimp_metadata_set_resolution (metadata, xres, yres,
                                    gimp_image_get_unit (image));
    }
}

static void
gimp_image_real_colormap_changed (GimpImage *image,
                                  gint       color_index)
{
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  if (private->colormap && private->n_colors > 0)
    {
      babl_palette_set_palette (private->babl_palette_rgb,
                                gimp_babl_format (GIMP_RGB,
                                                  private->precision, FALSE),
                                private->colormap,
                                private->n_colors);
      babl_palette_set_palette (private->babl_palette_rgba,
                                gimp_babl_format (GIMP_RGB,
                                                  private->precision, FALSE),
                                private->colormap,
                                private->n_colors);
    }

  if (gimp_image_get_base_type (image) == GIMP_INDEXED)
    {
      /* A colormap alteration affects the whole image */
      gimp_image_invalidate (image,
                             0, 0,
                             gimp_image_get_width  (image),
                             gimp_image_get_height (image));

      gimp_item_stack_invalidate_previews (GIMP_ITEM_STACK (private->layers->container));
    }
}

static const guint8 *
gimp_image_color_managed_get_icc_profile (GimpColorManaged *managed,
                                          gsize            *len)
{
  const GimpParasite *parasite;

  parasite = gimp_image_get_icc_profile (GIMP_IMAGE (managed));

  if (parasite)
    {
      gsize data_size = gimp_parasite_data_size (parasite);

      if (data_size > 0)
        {
          *len = data_size;

          return gimp_parasite_data (parasite);
        }
    }

  return NULL;
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
                                    gimp_image_get_precision (image), TRUE);

    case GIMP_GRAY:
      return gimp_image_get_format (image, GIMP_GRAY,
                                    gimp_image_get_precision (image), TRUE);
    }

  g_assert_not_reached ();

  return NULL;
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
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (pickable);

  return gimp_pickable_get_buffer (GIMP_PICKABLE (private->projection));
}

static gboolean
gimp_image_get_pixel_at (GimpPickable *pickable,
                         gint          x,
                         gint          y,
                         const Babl   *format,
                         gpointer      pixel)
{
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (pickable);

  return gimp_pickable_get_pixel_at (GIMP_PICKABLE (private->projection),
                                     x, y, format, pixel);
}

static gdouble
gimp_image_get_opacity_at (GimpPickable *pickable,
                           gint          x,
                           gint          y)
{
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (pickable);

  return gimp_pickable_get_opacity_at (GIMP_PICKABLE (private->projection),
                                       x, y);
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

  channels_node =
    gimp_filter_stack_get_graph (GIMP_FILTER_STACK (private->channels->container));

  gegl_node_add_child (private->graph, channels_node);

  gegl_node_connect_to (layers_node,   "output",
                        channels_node, "input");

  mask = ~gimp_image_get_visible_mask (image) & GIMP_COMPONENT_ALL;

  private->visible_mask =
    gegl_node_new_child (private->graph,
                         "operation", "gimp:mask-components",
                         "mask",      mask,
                         NULL);

  gegl_node_connect_to (channels_node,         "output",
                        private->visible_mask, "input");

  output = gegl_node_get_output_proxy (private->graph, "output");

  gegl_node_connect_to (private->visible_mask, "output",
                        output,                "input");

  return private->graph;
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
      GIMP_IMAGE_GET_PRIVATE (image)->quick_mask_color = channel->color;
    }
}

static void
gimp_image_active_layer_notify (GimpItemTree     *tree,
                                const GParamSpec *pspec,
                                GimpImage        *image)
{
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);
  GimpLayer        *layer   = gimp_image_get_active_layer (image);

  if (layer)
    {
      /*  Configure the layer stack to reflect this change  */
      private->layer_stack = g_slist_remove (private->layer_stack, layer);
      private->layer_stack = g_slist_prepend (private->layer_stack, layer);
    }

  g_signal_emit (image, gimp_image_signals[ACTIVE_LAYER_CHANGED], 0);

  if (layer && gimp_image_get_active_channel (image))
    gimp_image_set_active_channel (image, NULL);
}

static void
gimp_image_active_channel_notify (GimpItemTree     *tree,
                                  const GParamSpec *pspec,
                                  GimpImage        *image)
{
  GimpChannel *channel = gimp_image_get_active_channel (image);

  g_signal_emit (image, gimp_image_signals[ACTIVE_CHANNEL_CHANGED], 0);

  if (channel && gimp_image_get_active_layer (image))
    gimp_image_set_active_layer (image, NULL);
}

static void
gimp_image_active_vectors_notify (GimpItemTree     *tree,
                                  const GParamSpec *pspec,
                                  GimpImage        *image)
{
  g_signal_emit (image, gimp_image_signals[ACTIVE_VECTORS_CHANGED], 0);
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
  g_return_val_if_fail (base_type != GIMP_INDEXED ||
                        precision == GIMP_PRECISION_U8_GAMMA, NULL);

  return g_object_new (GIMP_TYPE_IMAGE,
                       "gimp",      gimp,
                       "width",     width,
                       "height",    height,
                       "base-type", base_type,
                       "precision", precision,
                       NULL);
}

gint64
gimp_image_estimate_memsize (const GimpImage   *image,
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
  drawables = gimp_image_item_list_get_list (image, NULL,
                                             GIMP_ITEM_TYPE_LAYERS |
                                             GIMP_ITEM_TYPE_CHANNELS,
                                             GIMP_ITEM_SET_ALL);

  gimp_image_item_list_filter (NULL, drawables, TRUE, FALSE);

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
gimp_image_get_base_type (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), -1);

  return GIMP_IMAGE_GET_PRIVATE (image)->base_type;
}

GimpComponentType
gimp_image_get_component_type (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), -1);

  return gimp_babl_component_type (GIMP_IMAGE_GET_PRIVATE (image)->precision);
}

GimpPrecision
gimp_image_get_precision (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), -1);

  return GIMP_IMAGE_GET_PRIVATE (image)->precision;
}

const Babl *
gimp_image_get_format (const GimpImage   *image,
                       GimpImageBaseType  base_type,
                       GimpPrecision      precision,
                       gboolean           with_alpha)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  switch (base_type)
    {
    case GIMP_RGB:
    case GIMP_GRAY:
      return gimp_babl_format (base_type, precision, with_alpha);

    case GIMP_INDEXED:
      if (precision == GIMP_PRECISION_U8_GAMMA)
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
gimp_image_get_layer_format (const GimpImage *image,
                             gboolean         with_alpha)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return gimp_image_get_format (image,
                                gimp_image_get_base_type (image),
                                gimp_image_get_precision (image),
                                with_alpha);
}

const Babl *
gimp_image_get_channel_format (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return gimp_image_get_format (image, GIMP_GRAY,
                                gimp_image_get_precision (image),
                                FALSE);
}

const Babl *
gimp_image_get_mask_format (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return gimp_babl_mask_format (gimp_image_get_precision (image));
}

gint
gimp_image_get_ID (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), -1);

  return GIMP_IMAGE_GET_PRIVATE (image)->ID;
}

GimpImage *
gimp_image_get_by_ID (Gimp *gimp,
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
gimp_image_get_untitled_file (const GimpImage *image)
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
gimp_image_get_file_or_untitled (const GimpImage *image)
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
 * Get the file of the XCF image, or NULL if there is no file.
 *
 * Returns: The file, or NULL.
 **/
GFile *
gimp_image_get_file (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->file;
}

void
gimp_image_set_filename (GimpImage   *image,
                         const gchar *filename)
{
  GFile *file = NULL;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  if (filename && strlen (filename))
    file = file_utils_filename_to_file (image->gimp, filename, NULL);

  gimp_image_set_file (image, file);

  if (file)
    g_object_unref (file);
}

/**
 * gimp_image_get_imported_file:
 * @image: A #GimpImage.
 *
 * Returns: The file of the imported image, or NULL if the image has
 * been saved as XCF after it was imported.
 **/
GFile *
gimp_image_get_imported_file (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->imported_file;
}

/**
 * gimp_image_get_exported_file:
 * @image: A #GimpImage.
 *
 * Returns: The file of the image last exported from this XCF file, or
 * NULL if the image has never been exported.
 **/
GFile *
gimp_image_get_exported_file (const GimpImage *image)
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
gimp_image_get_save_a_copy_file (const GimpImage *image)
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
gimp_image_get_any_file (const GimpImage *image)
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

  if (private->imported_file != file)
    {
      if (private->imported_file)
        g_object_unref (private->imported_file);

      private->imported_file = file;

      if (private->imported_file)
        g_object_ref (private->imported_file);

      gimp_object_name_changed (GIMP_OBJECT (image));
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

  if (private->exported_file != file)
    {
      if (private->exported_file)
        g_object_unref (private->exported_file);

      private->exported_file = file;

      if (private->exported_file)
        g_object_ref (private->exported_file);

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

  if (private->save_a_copy_file != file)
    {
      if (private->save_a_copy_file)
        g_object_unref (private->save_a_copy_file);

      private->save_a_copy_file = file;

      if (private->save_a_copy_file)
        g_object_ref (private->save_a_copy_file);
    }
}

gchar *
gimp_image_get_filename (const GimpImage *image)
{
  GFile *file;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  file = gimp_image_get_file (image);

  if (! file)
    return NULL;

  return g_file_get_path (file);
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
      display_file = file;
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
        display_file = file_utils_file_with_new_ext (display_file, NULL);

      uri_format = "[%s]";
    }

  if (! display_file)
    display_file = gimp_image_get_untitled_file (image);

  if (basename)
    display_uri = g_path_get_basename (gimp_file_get_utf8_name (display_file));
  else
    display_uri = g_strdup (gimp_file_get_utf8_name (display_file));

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
gimp_image_get_load_proc (const GimpImage *image)
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
gimp_image_get_save_proc (const GimpImage *image)
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
gimp_image_get_export_proc (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->export_proc;
}

gint
gimp_image_get_xcf_version (GimpImage    *image,
                            gboolean      zlib_compression,
                            gint         *gimp_version,
                            const gchar **version_string)
{
  GList *list;
  gint   version = 0;  /* default to oldest */

  /* need version 1 for colormaps */
  if (gimp_image_get_colormap (image))
    version = 1;

  for (list = gimp_image_get_layer_iter (image);
       list && version < 3;
       list = g_list_next (list))
    {
      GimpLayer *layer = GIMP_LAYER (list->data);

      switch (gimp_layer_get_mode (layer))
        {
          /* new layer modes not supported by gimp-1.2 */
        case GIMP_SOFTLIGHT_MODE:
        case GIMP_GRAIN_EXTRACT_MODE:
        case GIMP_GRAIN_MERGE_MODE:
        case GIMP_COLOR_ERASE_MODE:
          version = MAX (2, version);
          break;

        default:
          break;
        }

      /* need version 3 for layer trees */
      if (gimp_viewable_get_children (GIMP_VIEWABLE (layer)))
        version = MAX (3, version);
    }

  /* need version 6 for new metadata */
  if (gimp_image_get_metadata (image))
    version = MAX (6, version);

  /* need version 7 for high bit depth images */
  if (gimp_image_get_precision (image) != GIMP_PRECISION_U8_GAMMA)
    version = MAX (7, version);

  /* need version 8 for zlib compression */
  if (zlib_compression)
    version = MAX (8, version);

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
      if (gimp_version)   *gimp_version   = 210;
      if (version_string) *version_string = "GIMP 2.10";
      break;
    }

  return version;
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
gimp_image_get_resolution (const GimpImage *image,
                           gdouble         *xresolution,
                           gdouble         *yresolution)
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
                     GimpUnit   unit)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (unit > GIMP_UNIT_PIXEL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (private->resolution_unit != unit)
    {
      gimp_image_undo_push_image_resolution (image,
                                             C_("undo-type", "Change Image Unit"));

      private->resolution_unit = unit;
      gimp_image_unit_changed (image);
    }
}

GimpUnit
gimp_image_get_unit (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), GIMP_UNIT_INCH);

  return GIMP_IMAGE_GET_PRIVATE (image)->resolution_unit;
}

void
gimp_image_unit_changed (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[UNIT_CHANGED], 0);
}

gint
gimp_image_get_width (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  return GIMP_IMAGE_GET_PRIVATE (image)->width;
}

gint
gimp_image_get_height (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  return GIMP_IMAGE_GET_PRIVATE (image)->height;
}

gboolean
gimp_image_has_alpha (const GimpImage *image)
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
gimp_image_is_empty (const GimpImage *image)
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
gimp_image_get_floating_selection (const GimpImage *image)
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
gimp_image_get_mask (const GimpImage *image)
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
gimp_image_get_component_format (const GimpImage *image,
                                 GimpChannelType  channel)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  switch (channel)
    {
    case GIMP_RED_CHANNEL:
      return gimp_babl_component_format (GIMP_RGB,
                                         gimp_image_get_precision (image),
                                         RED);

    case GIMP_GREEN_CHANNEL:
      return gimp_babl_component_format (GIMP_RGB,
                                         gimp_image_get_precision (image),
                                         GREEN);

    case GIMP_BLUE_CHANNEL:
      return gimp_babl_component_format (GIMP_RGB,
                                         gimp_image_get_precision (image),
                                         BLUE);

    case GIMP_ALPHA_CHANNEL:
      return gimp_babl_component_format (GIMP_RGB,
                                         gimp_image_get_precision (image),
                                         ALPHA);

    case GIMP_GRAY_CHANNEL:
      return gimp_babl_component_format (GIMP_GRAY,
                                         gimp_image_get_precision (image),
                                         GRAY);

    case GIMP_INDEXED_CHANNEL:
      return babl_format ("Y u8"); /* will extract grayscale, the best
                                    * we can do here */
    }

  return NULL;
}

gint
gimp_image_get_component_index (const GimpImage *image,
                                GimpChannelType  channel)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), -1);

  switch (channel)
    {
    case GIMP_RED_CHANNEL:     return RED;
    case GIMP_GREEN_CHANNEL:   return GREEN;
    case GIMP_BLUE_CHANNEL:    return BLUE;
    case GIMP_GRAY_CHANNEL:    return GRAY;
    case GIMP_INDEXED_CHANNEL: return INDEXED;
    case GIMP_ALPHA_CHANNEL:
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
      gimp_image_unset_active_channel (image);

      g_signal_emit (image,
                     gimp_image_signals[COMPONENT_ACTIVE_CHANGED], 0,
                     channel);
    }
}

gboolean
gimp_image_get_component_active (const GimpImage *image,
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
gimp_image_get_active_array (const GimpImage *image,
                             gboolean        *components)
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
gimp_image_get_active_mask (const GimpImage *image)
{
  GimpImagePrivate  *private;
  GimpComponentMask  mask = 0;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  switch (gimp_image_get_base_type (image))
    {
    case GIMP_RGB:
      mask |= (private->active[RED])   ? GIMP_COMPONENT_RED   : 0;
      mask |= (private->active[GREEN]) ? GIMP_COMPONENT_GREEN : 0;
      mask |= (private->active[BLUE])  ? GIMP_COMPONENT_BLUE  : 0;
      mask |= (private->active[ALPHA]) ? GIMP_COMPONENT_ALPHA : 0;
      break;

    case GIMP_GRAY:
    case GIMP_INDEXED:
      mask |= (private->active[GRAY])    ? GIMP_COMPONENT_RED   : 0;
      mask |= (private->active[GRAY])    ? GIMP_COMPONENT_GREEN : 0;
      mask |= (private->active[GRAY])    ? GIMP_COMPONENT_BLUE  : 0;
      mask |= (private->active[ALPHA_G]) ? GIMP_COMPONENT_ALPHA : 0;
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

          mask = ~gimp_image_get_visible_mask (image) & GIMP_COMPONENT_ALL;

          gegl_node_set (private->visible_mask,
                         "mask", mask,
                         NULL);
        }

      g_signal_emit (image,
                     gimp_image_signals[COMPONENT_VISIBILITY_CHANGED], 0,
                     channel);

      gimp_image_invalidate (image,
                             0, 0,
                             gimp_image_get_width  (image),
                             gimp_image_get_height (image));
    }
}

gboolean
gimp_image_get_component_visible (const GimpImage *image,
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
gimp_image_get_visible_array (const GimpImage *image,
                              gboolean        *components)
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
gimp_image_get_visible_mask (const GimpImage *image)
{
  GimpImagePrivate  *private;
  GimpComponentMask  mask = 0;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  switch (gimp_image_get_base_type (image))
    {
    case GIMP_RGB:
      mask |= (private->visible[RED])   ? GIMP_COMPONENT_RED   : 0;
      mask |= (private->visible[GREEN]) ? GIMP_COMPONENT_GREEN : 0;
      mask |= (private->visible[BLUE])  ? GIMP_COMPONENT_BLUE  : 0;
      mask |= (private->visible[ALPHA]) ? GIMP_COMPONENT_ALPHA : 0;
      break;

    case GIMP_GRAY:
    case GIMP_INDEXED:
      mask |= (private->visible[GRAY])  ? GIMP_COMPONENT_RED   : 0;
      mask |= (private->visible[GRAY])  ? GIMP_COMPONENT_GREEN : 0;
      mask |= (private->visible[GRAY])  ? GIMP_COMPONENT_BLUE  : 0;
      mask |= (private->visible[ALPHA]) ? GIMP_COMPONENT_ALPHA : 0;
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
  g_return_if_fail (sample_point != NULL);

  g_signal_emit (image, gimp_image_signals[SAMPLE_POINT_ADDED], 0,
                 sample_point);
}

void
gimp_image_sample_point_removed (GimpImage       *image,
                                 GimpSamplePoint *sample_point)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (sample_point != NULL);

  g_signal_emit (image, gimp_image_signals[SAMPLE_POINT_REMOVED], 0,
                 sample_point);
}

void
gimp_image_sample_point_moved (GimpImage       *image,
                               GimpSamplePoint *sample_point)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (sample_point != NULL);

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
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (color_index >= -1 &&
                    color_index < GIMP_IMAGE_GET_PRIVATE (image)->n_colors);

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
}

void
gimp_image_export_clean_all (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  private->export_dirty = 0;

  g_signal_emit (image, gimp_image_signals[CLEAN], 0, GIMP_DIRTY_ALL);
}

/**
 * gimp_image_is_dirty:
 * @image:
 *
 * Returns: True if the image is dirty, false otherwise.
 **/
gint
gimp_image_is_dirty (const GimpImage *image)
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
gimp_image_is_export_dirty (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  return GIMP_IMAGE_GET_PRIVATE (image)->export_dirty != 0;
}

gint64
gimp_image_get_dirty_time (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  return GIMP_IMAGE_GET_PRIVATE (image)->dirty_time;
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
gimp_image_get_display_count (const GimpImage *image)
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
gimp_image_get_instance_count (const GimpImage *image)
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


/*  parasites  */

const GimpParasite *
gimp_image_parasite_find (const GimpImage *image,
                          const gchar     *name)
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
gimp_image_parasite_list (const GimpImage *image,
                          gint            *count)
{
  GimpImagePrivate  *private;
  gchar            **list;
  gchar            **cur;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  *count = gimp_parasite_list_length (private->parasites);
  cur = list = g_new (gchar *, *count);

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

  name = gimp_parasite_name (parasite);

  if (strcmp (name, GIMP_ICC_PROFILE_PARASITE_NAME) == 0)
    {
      return gimp_image_validate_icc_profile (image, parasite, error);
    }

  return TRUE;
}

void
gimp_image_parasite_attach (GimpImage          *image,
                            const GimpParasite *parasite)
{
  GimpParasite  copy;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (parasite != NULL);

  /*  make a temporary copy of the GimpParasite struct because
   *  gimp_parasite_shift_parent() changes it
   */
  copy = *parasite;

  /*  only set the dirty bit manually if we can be saved and the new
   *  parasite differs from the current one and we aren't undoable
   */
  if (gimp_parasite_is_undoable (&copy))
    gimp_image_undo_push_image_parasite (image,
                                         C_("undo-type", "Attach Parasite to Image"),
                                         &copy);

  /*  We used to push an cantundo on te stack here. This made the undo stack
   *  unusable (NULL on the stack) and prevented people from undoing after a
   *  save (since most save plug-ins attach an undoable comment parasite).
   *  Now we simply attach the parasite without pushing an undo. That way
   *  it's undoable but does not block the undo system.   --Sven
   */
  gimp_parasite_list_add (GIMP_IMAGE_GET_PRIVATE (image)->parasites, &copy);

  if (gimp_parasite_has_flag (&copy, GIMP_PARASITE_ATTACH_PARENT))
    {
      gimp_parasite_shift_parent (&copy);
      gimp_parasite_attach (image->gimp, &copy);
    }

  g_signal_emit (image, gimp_image_signals[PARASITE_ATTACHED], 0,
                 gimp_parasite_name (parasite));

  if (strcmp (gimp_parasite_name (parasite), GIMP_ICC_PROFILE_PARASITE_NAME) == 0)
    gimp_color_managed_profile_changed (GIMP_COLOR_MANAGED (image));
}

void
gimp_image_parasite_detach (GimpImage   *image,
                            const gchar *name)
{
  GimpImagePrivate   *private;
  const GimpParasite *parasite;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (name != NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (! (parasite = gimp_parasite_list_find (private->parasites, name)))
    return;

  if (gimp_parasite_is_undoable (parasite))
    gimp_image_undo_push_image_parasite_remove (image,
                                                C_("undo-type", "Remove Parasite from Image"),
                                                name);

  gimp_parasite_list_remove (private->parasites, name);

  g_signal_emit (image, gimp_image_signals[PARASITE_DETACHED], 0,
                 name);

  if (strcmp (name, GIMP_ICC_PROFILE_PARASITE_NAME) == 0)
    gimp_color_managed_profile_changed (GIMP_COLOR_MANAGED (image));
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

      if (gimp_image_get_vectors_by_tattoo (image, ltattoo))
        retval = FALSE; /* Oopps duplicated tattoo in vectors */
    }

  g_list_free (all_items);

  /* Now check that the channel and vectors tattoos don't overlap */
  all_items = gimp_image_get_channel_list (image);

  for (list = all_items; list; list = g_list_next (list))
    {
      GimpTattoo ctattoo;

      ctattoo = gimp_item_get_tattoo (GIMP_ITEM (list->data));
      if (ctattoo > maxval)
        maxval = ctattoo;

      if (gimp_image_get_vectors_by_tattoo (image, ctattoo))
        retval = FALSE; /* Oopps duplicated tattoo in vectors */
    }

  g_list_free (all_items);

  /* Find the max tattoo value in the vectors */
  all_items = gimp_image_get_vectors_list (image);

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
gimp_image_get_projection (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->projection;
}


/*  layers / channels / vectors  */

GimpItemTree *
gimp_image_get_layer_tree (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->layers;
}

GimpItemTree *
gimp_image_get_channel_tree (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->channels;
}

GimpItemTree *
gimp_image_get_vectors_tree (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->vectors;
}

GimpContainer *
gimp_image_get_layers (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->layers->container;
}

GimpContainer *
gimp_image_get_channels (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->channels->container;
}

GimpContainer *
gimp_image_get_vectors (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->vectors->container;
}

gint
gimp_image_get_n_layers (const GimpImage *image)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  stack = GIMP_ITEM_STACK (gimp_image_get_layers (image));

  return gimp_item_stack_get_n_items (stack);
}

gint
gimp_image_get_n_channels (const GimpImage *image)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  stack = GIMP_ITEM_STACK (gimp_image_get_channels (image));

  return gimp_item_stack_get_n_items (stack);
}

gint
gimp_image_get_n_vectors (const GimpImage *image)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  stack = GIMP_ITEM_STACK (gimp_image_get_vectors (image));

  return gimp_item_stack_get_n_items (stack);
}

GList *
gimp_image_get_layer_iter (const GimpImage *image)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  stack = GIMP_ITEM_STACK (gimp_image_get_layers (image));

  return gimp_item_stack_get_item_iter (stack);
}

GList *
gimp_image_get_channel_iter (const GimpImage *image)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  stack = GIMP_ITEM_STACK (gimp_image_get_channels (image));

  return gimp_item_stack_get_item_iter (stack);
}

GList *
gimp_image_get_vectors_iter (const GimpImage *image)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  stack = GIMP_ITEM_STACK (gimp_image_get_vectors (image));

  return gimp_item_stack_get_item_iter (stack);
}

GList *
gimp_image_get_layer_list (const GimpImage *image)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  stack = GIMP_ITEM_STACK (gimp_image_get_layers (image));

  return gimp_item_stack_get_item_list (stack);
}

GList *
gimp_image_get_channel_list (const GimpImage *image)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  stack = GIMP_ITEM_STACK (gimp_image_get_channels (image));

  return gimp_item_stack_get_item_list (stack);
}

GList *
gimp_image_get_vectors_list (const GimpImage *image)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  stack = GIMP_ITEM_STACK (gimp_image_get_vectors (image));

  return gimp_item_stack_get_item_list (stack);
}


/*  active drawable, layer, channel, vectors  */

GimpDrawable *
gimp_image_get_active_drawable (const GimpImage *image)
{
  GimpImagePrivate *private;
  GimpItem         *active_channel;
  GimpItem         *active_layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  active_channel = gimp_item_tree_get_active_item (private->channels);
  active_layer   = gimp_item_tree_get_active_item (private->layers);

  /*  If there is an active channel (a saved selection, etc.),
   *  we ignore the active layer
   */
  if (active_channel)
    {
      return GIMP_DRAWABLE (active_channel);
    }
  else if (active_layer)
    {
      GimpLayer     *layer = GIMP_LAYER (active_layer);
      GimpLayerMask *mask  = gimp_layer_get_mask (layer);

      if (mask && gimp_layer_get_edit_mask (layer))
        return GIMP_DRAWABLE (mask);
      else
        return GIMP_DRAWABLE (layer);
    }

  return NULL;
}

GimpLayer *
gimp_image_get_active_layer (const GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  return GIMP_LAYER (gimp_item_tree_get_active_item (private->layers));
}

GimpChannel *
gimp_image_get_active_channel (const GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  return GIMP_CHANNEL (gimp_item_tree_get_active_item (private->channels));
}

GimpVectors *
gimp_image_get_active_vectors (const GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  return GIMP_VECTORS (gimp_item_tree_get_active_item (private->vectors));
}

GimpLayer *
gimp_image_set_active_layer (GimpImage *image,
                             GimpLayer *layer)
{
  GimpImagePrivate *private;
  GimpLayer        *floating_sel;
  GimpLayer        *active_layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (layer == NULL || GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (layer == NULL ||
                        (gimp_item_is_attached (GIMP_ITEM (layer)) &&
                         gimp_item_get_image (GIMP_ITEM (layer)) == image),
                        NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  floating_sel = gimp_image_get_floating_selection (image);

  /*  Make sure the floating_sel always is the active layer  */
  if (floating_sel && layer != floating_sel)
    return floating_sel;

  active_layer = gimp_image_get_active_layer (image);

  if (layer != active_layer)
    {
      /*  Don't cache selection info for the previous active layer  */
      if (active_layer)
        gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (active_layer));

      gimp_item_tree_set_active_item (private->layers, GIMP_ITEM (layer));
    }

  return gimp_image_get_active_layer (image);
}

GimpChannel *
gimp_image_set_active_channel (GimpImage   *image,
                               GimpChannel *channel)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (channel == NULL || GIMP_IS_CHANNEL (channel), NULL);
  g_return_val_if_fail (channel == NULL ||
                        (gimp_item_is_attached (GIMP_ITEM (channel)) &&
                         gimp_item_get_image (GIMP_ITEM (channel)) == image),
                        NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  /*  Not if there is a floating selection  */
  if (channel && gimp_image_get_floating_selection (image))
    return NULL;

  if (channel != gimp_image_get_active_channel (image))
    {
      gimp_item_tree_set_active_item (private->channels, GIMP_ITEM (channel));
    }

  return gimp_image_get_active_channel (image);
}

GimpChannel *
gimp_image_unset_active_channel (GimpImage *image)
{
  GimpImagePrivate *private;
  GimpChannel      *channel;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  channel = gimp_image_get_active_channel (image);

  if (channel)
    {
      gimp_image_set_active_channel (image, NULL);

      if (private->layer_stack)
        gimp_image_set_active_layer (image, private->layer_stack->data);
    }

  return channel;
}

GimpVectors *
gimp_image_set_active_vectors (GimpImage   *image,
                               GimpVectors *vectors)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (vectors == NULL || GIMP_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (vectors == NULL ||
                        (gimp_item_is_attached (GIMP_ITEM (vectors)) &&
                         gimp_item_get_image (GIMP_ITEM (vectors)) == image),
                        NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (vectors != gimp_image_get_active_vectors (image))
    {
      gimp_item_tree_set_active_item (private->vectors, GIMP_ITEM (vectors));
    }

  return gimp_image_get_active_vectors (image);
}


/*  layer, channel, vectors by tattoo  */

GimpLayer *
gimp_image_get_layer_by_tattoo (const GimpImage *image,
                                GimpTattoo       tattoo)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  stack = GIMP_ITEM_STACK (gimp_image_get_layers (image));

  return GIMP_LAYER (gimp_item_stack_get_item_by_tattoo (stack, tattoo));
}

GimpChannel *
gimp_image_get_channel_by_tattoo (const GimpImage *image,
                                  GimpTattoo       tattoo)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  stack = GIMP_ITEM_STACK (gimp_image_get_channels (image));

  return GIMP_CHANNEL (gimp_item_stack_get_item_by_tattoo (stack, tattoo));
}

GimpVectors *
gimp_image_get_vectors_by_tattoo (const GimpImage *image,
                                  GimpTattoo       tattoo)
{
  GimpItemStack *stack;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  stack = GIMP_ITEM_STACK (gimp_image_get_vectors (image));

  return GIMP_VECTORS (gimp_item_stack_get_item_by_tattoo (stack, tattoo));
}


/*  layer, channel, vectors by name  */

GimpLayer *
gimp_image_get_layer_by_name (const GimpImage *image,
                              const gchar     *name)
{
  GimpItemTree *tree;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  tree = gimp_image_get_layer_tree (image);

  return GIMP_LAYER (gimp_item_tree_get_item_by_name (tree, name));
}

GimpChannel *
gimp_image_get_channel_by_name (const GimpImage *image,
                                const gchar     *name)
{
  GimpItemTree *tree;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  tree = gimp_image_get_channel_tree (image);

  return GIMP_CHANNEL (gimp_item_tree_get_item_by_name (tree, name));
}

GimpVectors *
gimp_image_get_vectors_by_name (const GimpImage *image,
                                const gchar     *name)
{
  GimpItemTree *tree;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  tree = gimp_image_get_vectors_tree (image);

  return GIMP_VECTORS (gimp_item_tree_get_item_by_name (tree, name));
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

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (gimp_item_get_image (item) == image, FALSE);

  tree = gimp_item_get_tree (item);

  g_return_val_if_fail (tree != NULL, FALSE);

  if (push_undo && ! undo_desc)
    undo_desc = GIMP_ITEM_GET_CLASS (item)->reorder_desc;

  /*  item and new_parent are type-checked in GimpItemTree
   */
  return gimp_item_tree_reorder_item (tree, item,
                                      new_parent, new_index,
                                      push_undo, undo_desc);
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


/*  layers  */

gboolean
gimp_image_add_layer (GimpImage *image,
                      GimpLayer *layer,
                      GimpLayer *parent,
                      gint       position,
                      gboolean   push_undo)
{
  GimpImagePrivate *private;
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
                                    gimp_image_get_active_layer (image));

  gimp_item_tree_add_item (private->layers, GIMP_ITEM (layer),
                           GIMP_ITEM (parent), position);

  gimp_image_set_active_layer (image, layer);

  /*  If the layer is a floating selection, attach it to the drawable  */
  if (gimp_layer_is_floating_sel (layer))
    gimp_drawable_attach_floating_sel (gimp_layer_get_floating_sel_drawable (layer),
                                       layer);

  if (old_has_alpha != gimp_image_has_alpha (image))
    private->flush_accum.alpha_changed = TRUE;

  return TRUE;
}

void
gimp_image_remove_layer (GimpImage *image,
                         GimpLayer *layer,
                         gboolean   push_undo,
                         GimpLayer *new_active)
{
  GimpImagePrivate *private;
  GimpLayer        *active_layer;
  gboolean          old_has_alpha;
  gboolean          undo_group = FALSE;
  const gchar      *undo_desc;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)));
  g_return_if_fail (gimp_item_get_image (GIMP_ITEM (layer)) == image);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (gimp_drawable_get_floating_sel (GIMP_DRAWABLE (layer)))
    {
      if (! push_undo)
        {
          g_warning ("%s() was called from an undo function while the layer "
                     "had a floating selection. Please report this at "
                     "http://www.gimp.org/bugs/", G_STRFUNC);
          return;
        }

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                   C_("undo-type", "Remove Layer"));
      undo_group = TRUE;

      gimp_image_remove_layer (image,
                               gimp_drawable_get_floating_sel (GIMP_DRAWABLE (layer)),
                               TRUE, NULL);
    }

  active_layer = gimp_image_get_active_layer (image);

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
    gimp_image_undo_push_layer_remove (image, undo_desc, layer,
                                       gimp_layer_get_parent (layer),
                                       gimp_item_get_index (GIMP_ITEM (layer)),
                                       active_layer);

  g_object_ref (layer);

  /*  Make sure we're not caching any old selection info  */
  if (layer == active_layer)
    gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (layer));

  private->layer_stack = g_slist_remove (private->layer_stack, layer);

  /*  Also remove all children of a group layer from the layer_stack  */
  if (gimp_viewable_get_children (GIMP_VIEWABLE (layer)))
    {
      GimpContainer *stack = gimp_viewable_get_children (GIMP_VIEWABLE (layer));
      GList         *children;
      GList         *list;

      children = gimp_item_stack_get_item_list (GIMP_ITEM_STACK (stack));

      for (list = children; list; list = g_list_next (list))
        {
          private->layer_stack = g_slist_remove (private->layer_stack,
                                                 list->data);
        }

      g_list_free (children);
    }

  new_active =
    GIMP_LAYER (gimp_item_tree_remove_item (private->layers,
                                            GIMP_ITEM (layer),
                                            GIMP_ITEM (new_active)));

  if (gimp_layer_is_floating_sel (layer))
    {
      /*  If this was the floating selection, activate the underlying drawable
       */
      floating_sel_activate_drawable (layer);
    }
  else if (active_layer &&
           (layer == active_layer ||
            gimp_viewable_is_ancestor (GIMP_VIEWABLE (layer),
                                       GIMP_VIEWABLE (active_layer))))
    {
      gimp_image_set_active_layer (image, new_active);
    }

  g_object_unref (layer);

  if (old_has_alpha != gimp_image_has_alpha (image))
    private->flush_accum.alpha_changed = TRUE;

  if (undo_group)
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
      position++;
    }

  if (layers)
    gimp_image_set_active_layer (image, layers->data);

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
                                      gimp_image_get_active_channel (image));

  gimp_item_tree_add_item (private->channels, GIMP_ITEM (channel),
                           GIMP_ITEM (parent), position);

  gimp_image_set_active_channel (image, channel);

  return TRUE;
}

void
gimp_image_remove_channel (GimpImage   *image,
                           GimpChannel *channel,
                           gboolean     push_undo,
                           GimpChannel *new_active)
{
  GimpImagePrivate *private;
  GimpChannel      *active_channel;
  gboolean          undo_group = FALSE;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)));
  g_return_if_fail (gimp_item_get_image (GIMP_ITEM (channel)) == image);

  if (gimp_drawable_get_floating_sel (GIMP_DRAWABLE (channel)))
    {
      if (! push_undo)
        {
          g_warning ("%s() was called from an undo function while the channel "
                     "had a floating selection. Please report this at "
                     "http://www.gimp.org/bugs/", G_STRFUNC);
          return;
        }

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                   C_("undo-type", "Remove Channel"));
      undo_group = TRUE;

      gimp_image_remove_layer (image,
                               gimp_drawable_get_floating_sel (GIMP_DRAWABLE (channel)),
                               TRUE, NULL);
    }

  private = GIMP_IMAGE_GET_PRIVATE (image);

  active_channel = gimp_image_get_active_channel (image);

  if (push_undo)
    gimp_image_undo_push_channel_remove (image, C_("undo-type", "Remove Channel"), channel,
                                         gimp_channel_get_parent (channel),
                                         gimp_item_get_index (GIMP_ITEM (channel)),
                                         active_channel);

  g_object_ref (channel);

  new_active =
    GIMP_CHANNEL (gimp_item_tree_remove_item (private->channels,
                                              GIMP_ITEM (channel),
                                              GIMP_ITEM (new_active)));

  if (active_channel &&
      (channel == active_channel ||
       gimp_viewable_is_ancestor (GIMP_VIEWABLE (channel),
                                  GIMP_VIEWABLE (active_channel))))
    {
      if (new_active)
        gimp_image_set_active_channel (image, new_active);
      else
        gimp_image_unset_active_channel (image);
    }

  g_object_unref (channel);

  if (undo_group)
    gimp_image_undo_group_end (image);
}


/*  vectors  */

gboolean
gimp_image_add_vectors (GimpImage   *image,
                        GimpVectors *vectors,
                        GimpVectors *parent,
                        gint         position,
                        gboolean     push_undo)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  /*  item and parent are type-checked in GimpItemTree
   */
  if (! gimp_item_tree_get_insert_pos (private->vectors,
                                       (GimpItem *) vectors,
                                       (GimpItem **) &parent,
                                       &position))
    return FALSE;

  if (push_undo)
    gimp_image_undo_push_vectors_add (image, C_("undo-type", "Add Path"),
                                      vectors,
                                      gimp_image_get_active_vectors (image));

  gimp_item_tree_add_item (private->vectors, GIMP_ITEM (vectors),
                           GIMP_ITEM (parent), position);

  gimp_image_set_active_vectors (image, vectors);

  return TRUE;
}

void
gimp_image_remove_vectors (GimpImage   *image,
                           GimpVectors *vectors,
                           gboolean     push_undo,
                           GimpVectors *new_active)
{
  GimpImagePrivate *private;
  GimpVectors      *active_vectors;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_VECTORS (vectors));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (vectors)));
  g_return_if_fail (gimp_item_get_image (GIMP_ITEM (vectors)) == image);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  active_vectors = gimp_image_get_active_vectors (image);

  if (push_undo)
    gimp_image_undo_push_vectors_remove (image, C_("undo-type", "Remove Path"), vectors,
                                         gimp_vectors_get_parent (vectors),
                                         gimp_item_get_index (GIMP_ITEM (vectors)),
                                         active_vectors);

  g_object_ref (vectors);

  new_active =
    GIMP_VECTORS (gimp_item_tree_remove_item (private->vectors,
                                              GIMP_ITEM (vectors),
                                              GIMP_ITEM (new_active)));

  if (active_vectors &&
      (vectors == active_vectors ||
       gimp_viewable_is_ancestor (GIMP_VIEWABLE (vectors),
                                  GIMP_VIEWABLE (active_vectors))))
    {
      gimp_image_set_active_vectors (image, new_active);
    }

  g_object_unref (vectors);
}

gboolean
gimp_image_coords_in_active_pickable (GimpImage        *image,
                                      const GimpCoords *coords,
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
      if (x >= 0 && x < gimp_image_get_width  (image) &&
          y >= 0 && y < gimp_image_get_height (image))
        in_pickable = TRUE;
    }
  else
    {
      GimpDrawable *drawable = gimp_image_get_active_drawable (image);

      if (drawable)
        {
          GimpItem *item = GIMP_ITEM (drawable);
          gint      off_x, off_y;
          gint      d_x, d_y;

          gimp_item_get_offset (item, &off_x, &off_y);

          d_x = x - off_x;
          d_y = y - off_y;

          if (d_x >= 0 && d_x < gimp_item_get_width  (item) &&
              d_y >= 0 && d_y < gimp_item_get_height (item))
            in_pickable = TRUE;
        }
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
