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
#include <time.h>

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "base/temp-buf.h"
#include "base/tile-manager.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimp-parasites.h"
#include "gimp-utils.h"
#include "gimpcontext.h"
#include "gimpgrid.h"
#include "gimpguide.h"
#include "gimpimage.h"
#include "gimpimage-colorhash.h"
#include "gimpimage-colormap.h"
#include "gimpimage-guides.h"
#include "gimpimage-sample-points.h"
#include "gimpimage-preview.h"
#include "gimpimage-quick-mask.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplayermask.h"
#include "gimplist.h"
#include "gimpmarshal.h"
#include "gimpparasitelist.h"
#include "gimppickable.h"
#include "gimpprojection.h"
#include "gimpsamplepoint.h"
#include "gimpselection.h"
#include "gimptemplate.h"
#include "gimpundostack.h"

#include "file/file-utils.h"

#include "vectors/gimpvectors.h"

#include "gimp-intl.h"


#ifdef DEBUG
#define TRC(x) g_printerr x
#else
#define TRC(x)
#endif


enum
{
  MODE_CHANGED,
  ALPHA_CHANGED,
  FLOATING_SELECTION_CHANGED,
  ACTIVE_LAYER_CHANGED,
  ACTIVE_CHANNEL_CHANGED,
  ACTIVE_VECTORS_CHANGED,
  COMPONENT_VISIBILITY_CHANGED,
  COMPONENT_ACTIVE_CHANGED,
  MASK_CHANGED,
  RESOLUTION_CHANGED,
  UNIT_CHANGED,
  QUICK_MASK_CHANGED,
  SELECTION_CONTROL,
  CLEAN,
  DIRTY,
  SAVED,
  UPDATE,
  UPDATE_GUIDE,
  UPDATE_SAMPLE_POINT,
  SAMPLE_POINT_ADDED,
  SAMPLE_POINT_REMOVED,
  PARASITE_ATTACHED,
  PARASITE_DETACHED,
  COLORMAP_CHANGED,
  UNDO_EVENT,
  FLUSH,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_GIMP,
  PROP_ID,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_BASE_TYPE
};


/*  local function prototypes  */

static void     gimp_color_managed_iface_init    (GimpColorManagedInterface *iface);

static GObject *gimp_image_constructor           (GType           type,
                                                  guint           n_params,
                                                  GObjectConstructParam *params);
static void     gimp_image_set_property          (GObject        *object,
                                                  guint           property_id,
                                                  const GValue   *value,
                                                  GParamSpec     *pspec);
static void     gimp_image_get_property          (GObject        *object,
                                                  guint           property_id,
                                                  GValue         *value,
                                                  GParamSpec     *pspec);
static void     gimp_image_dispose               (GObject        *object);
static void     gimp_image_finalize              (GObject        *object);

static void     gimp_image_name_changed          (GimpObject     *object);
static gint64   gimp_image_get_memsize           (GimpObject     *object,
                                                  gint64         *gui_size);

static gboolean gimp_image_get_size              (GimpViewable   *viewable,
                                                  gint           *width,
                                                  gint           *height);
static void     gimp_image_invalidate_preview    (GimpViewable   *viewable);
static void     gimp_image_size_changed          (GimpViewable   *viewable);
static gchar  * gimp_image_get_description       (GimpViewable   *viewable,
                                                  gchar         **tooltip);
static void     gimp_image_real_colormap_changed (GimpImage      *image,
                                                  gint            color_index);
static void     gimp_image_real_flush            (GimpImage      *image,
                                                  gboolean        invalidate_preview);

static void     gimp_image_mask_update           (GimpDrawable   *drawable,
                                                  gint            x,
                                                  gint            y,
                                                  gint            width,
                                                  gint            height,
                                                  GimpImage      *image);
static void     gimp_image_drawable_update       (GimpDrawable   *drawable,
                                                  gint            x,
                                                  gint            y,
                                                  gint            width,
                                                  gint            height,
                                                  GimpImage      *image);
static void     gimp_image_drawable_visibility   (GimpItem       *item,
                                                  GimpImage      *image);
static void     gimp_image_layer_alpha_changed   (GimpDrawable   *drawable,
                                                  GimpImage      *image);
static void     gimp_image_layer_add             (GimpContainer  *container,
                                                  GimpLayer      *layer,
                                                  GimpImage      *image);
static void     gimp_image_layer_remove          (GimpContainer  *container,
                                                  GimpLayer      *layer,
                                                  GimpImage      *image);
static void     gimp_image_channel_add           (GimpContainer  *container,
                                                  GimpChannel    *channel,
                                                  GimpImage      *image);
static void     gimp_image_channel_remove        (GimpContainer  *container,
                                                  GimpChannel    *channel,
                                                  GimpImage      *image);
static void     gimp_image_channel_name_changed  (GimpChannel    *channel,
                                                  GimpImage      *image);
static void     gimp_image_channel_color_changed (GimpChannel    *channel,
                                                  GimpImage      *image);

const guint8 *  gimp_image_get_icc_profile       (GimpColorManaged *managed,
                                                  gsize            *len);


static const gint valid_combinations[][MAX_CHANNELS + 1] =
{
  /* GIMP_RGB_IMAGE */
  { -1, -1, -1, COMBINE_INTEN_INTEN, COMBINE_INTEN_INTEN_A },
  /* GIMP_RGBA_IMAGE */
  { -1, -1, -1, COMBINE_INTEN_A_INTEN, COMBINE_INTEN_A_INTEN_A },
  /* GIMP_GRAY_IMAGE */
  { -1, COMBINE_INTEN_INTEN, COMBINE_INTEN_INTEN_A, -1, -1 },
  /* GIMP_GRAYA_IMAGE */
  { -1, COMBINE_INTEN_A_INTEN, COMBINE_INTEN_A_INTEN_A, -1, -1 },
  /* GIMP_INDEXED_IMAGE */
  { -1, COMBINE_INDEXED_INDEXED, COMBINE_INDEXED_INDEXED_A, -1, -1 },
  /* GIMP_INDEXEDA_IMAGE */
  { -1, -1, COMBINE_INDEXED_A_INDEXED_A, -1, -1 },
};


G_DEFINE_TYPE_WITH_CODE (GimpImage, gimp_image, GIMP_TYPE_VIEWABLE,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_COLOR_MANAGED,
                                                gimp_color_managed_iface_init))

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

  gimp_image_signals[SELECTION_CONTROL] =
    g_signal_new ("selection-control",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, selection_control),
                  NULL, NULL,
                  gimp_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_SELECTION_CONTROL);

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
                  gimp_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  gimp_image_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, update),
                  NULL, NULL,
                  gimp_marshal_VOID__INT_INT_INT_INT,
                  G_TYPE_NONE, 4,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  gimp_image_signals[UPDATE_GUIDE] =
    g_signal_new ("update-guide",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, update_guide),
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  gimp_image_signals[UPDATE_SAMPLE_POINT] =
    g_signal_new ("update-sample-point",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, update_sample_point),
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

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

  gimp_image_signals[FLUSH] =
    g_signal_new ("flush",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, flush),
                  NULL, NULL,
                  gimp_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE, 1,
                  G_TYPE_BOOLEAN);

  object_class->constructor           = gimp_image_constructor;
  object_class->set_property          = gimp_image_set_property;
  object_class->get_property          = gimp_image_get_property;
  object_class->dispose               = gimp_image_dispose;
  object_class->finalize              = gimp_image_finalize;

  gimp_object_class->name_changed     = gimp_image_name_changed;
  gimp_object_class->get_memsize      = gimp_image_get_memsize;

  viewable_class->default_stock_id    = "gimp-image";
  viewable_class->get_size            = gimp_image_get_size;
  viewable_class->invalidate_preview  = gimp_image_invalidate_preview;
  viewable_class->size_changed        = gimp_image_size_changed;
  viewable_class->get_preview_size    = gimp_image_get_preview_size;
  viewable_class->get_popup_size      = gimp_image_get_popup_size;
  viewable_class->get_preview         = gimp_image_get_preview;
  viewable_class->get_new_preview     = gimp_image_get_new_preview;
  viewable_class->get_description     = gimp_image_get_description;

  klass->mode_changed                 = NULL;
  klass->alpha_changed                = NULL;
  klass->floating_selection_changed   = NULL;
  klass->active_layer_changed         = NULL;
  klass->active_channel_changed       = NULL;
  klass->active_vectors_changed       = NULL;
  klass->component_visibility_changed = NULL;
  klass->component_active_changed     = NULL;
  klass->mask_changed                 = NULL;

  klass->clean                        = NULL;
  klass->dirty                        = NULL;
  klass->saved                        = NULL;
  klass->update                       = NULL;
  klass->update_guide                 = NULL;
  klass->update_sample_point          = NULL;
  klass->sample_point_added           = NULL;
  klass->sample_point_removed         = NULL;
  klass->parasite_attached            = NULL;
  klass->parasite_detached            = NULL;
  klass->colormap_changed             = gimp_image_real_colormap_changed;
  klass->undo_event                   = NULL;
  klass->flush                        = gimp_image_real_flush;

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

  gimp_image_color_hash_init ();
}

static void
gimp_color_managed_iface_init (GimpColorManagedInterface *iface)
{
  iface->get_icc_profile = gimp_image_get_icc_profile;
}

static void
gimp_image_init (GimpImage *image)
{
  gint i;

  image->ID                    = 0;

  image->save_proc             = NULL;

  image->width                 = 0;
  image->height                = 0;
  image->xresolution           = 1.0;
  image->yresolution           = 1.0;
  image->resolution_unit       = GIMP_UNIT_INCH;
  image->base_type             = GIMP_RGB;

  image->cmap                  = NULL;
  image->num_cols              = 0;

  image->dirty                 = 1;
  image->dirty_time            = 0;
  image->undo_freeze_count     = 0;

  image->instance_count        = 0;
  image->disp_count            = 0;

  image->tattoo_state          = 0;

  image->shadow                = NULL;

  image->projection            = gimp_projection_new (image);

  image->guides                = NULL;
  image->grid                  = NULL;
  image->sample_points         = NULL;

  image->layers                = gimp_list_new (GIMP_TYPE_LAYER,   TRUE);
  image->channels              = gimp_list_new (GIMP_TYPE_CHANNEL, TRUE);
  image->vectors               = gimp_list_new (GIMP_TYPE_VECTORS, TRUE);
  image->layer_stack           = NULL;

  image->layer_update_handler =
    gimp_container_add_handler (image->layers, "update",
                                G_CALLBACK (gimp_image_drawable_update),
                                image);
  image->layer_visible_handler =
    gimp_container_add_handler (image->layers, "visibility-changed",
                                G_CALLBACK (gimp_image_drawable_visibility),
                                image);
  image->layer_alpha_handler =
    gimp_container_add_handler (image->layers, "alpha-changed",
                                G_CALLBACK (gimp_image_layer_alpha_changed),
                                image);

  image->channel_update_handler =
    gimp_container_add_handler (image->channels, "update",
                                G_CALLBACK (gimp_image_drawable_update),
                                image);
  image->channel_visible_handler =
    gimp_container_add_handler (image->channels, "visibility-changed",
                                G_CALLBACK (gimp_image_drawable_visibility),
                                image);
  image->channel_name_changed_handler =
    gimp_container_add_handler (image->channels, "name-changed",
                                G_CALLBACK (gimp_image_channel_name_changed),
                                image);
  image->channel_color_changed_handler =
    gimp_container_add_handler (image->channels, "color-changed",
                                G_CALLBACK (gimp_image_channel_color_changed),
                                image);

  g_signal_connect (image->layers, "add",
                    G_CALLBACK (gimp_image_layer_add),
                    image);
  g_signal_connect (image->layers, "remove",
                    G_CALLBACK (gimp_image_layer_remove),
                    image);

  g_signal_connect (image->channels, "add",
                    G_CALLBACK (gimp_image_channel_add),
                    image);
  g_signal_connect (image->channels, "remove",
                    G_CALLBACK (gimp_image_channel_remove),
                    image);

  image->active_layer          = NULL;
  image->active_channel        = NULL;
  image->active_vectors        = NULL;

  image->floating_sel          = NULL;
  image->selection_mask        = NULL;

  image->parasites             = gimp_parasite_list_new ();

  for (i = 0; i < MAX_CHANNELS; i++)
    {
      image->visible[i] = TRUE;
      image->active[i]  = TRUE;
    }

  image->quick_mask_state      = FALSE;
  image->quick_mask_inverted   = FALSE;
  gimp_rgba_set (&image->quick_mask_color, 1.0, 0.0, 0.0, 0.5);

  image->undo_stack            = gimp_undo_stack_new (image);
  image->redo_stack            = gimp_undo_stack_new (image);
  image->group_count           = 0;
  image->pushing_undo_group    = GIMP_UNDO_GROUP_NONE;

  image->preview               = NULL;

  image->flush_accum.alpha_changed       = FALSE;
  image->flush_accum.mask_changed        = FALSE;
  image->flush_accum.preview_invalidated = FALSE;
}

static GObject *
gimp_image_constructor (GType                  type,
                        guint                  n_params,
                        GObjectConstructParam *params)
{
  GObject        *object;
  GimpImage      *image;
  GimpCoreConfig *config;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  image = GIMP_IMAGE (object);

  g_assert (GIMP_IS_GIMP (image->gimp));

  config = image->gimp->config;

  do
    {
      image->ID = image->gimp->next_image_ID++;

      if (image->gimp->next_image_ID == G_MAXINT)
        image->gimp->next_image_ID = 1;
    }
  while (g_hash_table_lookup (image->gimp->image_table,
                              GINT_TO_POINTER (image->ID)));

  g_hash_table_insert (image->gimp->image_table,
                       GINT_TO_POINTER (image->ID),
                       image);

  image->xresolution     = config->default_image->xresolution;
  image->yresolution     = config->default_image->yresolution;
  image->resolution_unit = config->default_image->resolution_unit;

  image->grid = gimp_config_duplicate (GIMP_CONFIG (config->default_grid));

  switch (image->base_type)
    {
    case GIMP_RGB:
    case GIMP_GRAY:
      break;
    case GIMP_INDEXED:
      /* always allocate 256 colors for the colormap */
      image->num_cols = 0;
      image->cmap     = g_new0 (guchar, GIMP_IMAGE_COLORMAP_SIZE);
      break;
    default:
      break;
    }

  /* create the selection mask */
  image->selection_mask = gimp_selection_new (image,
                                              image->width,
                                              image->height);
  g_object_ref_sink (image->selection_mask);

  g_signal_connect (image->selection_mask, "update",
                    G_CALLBACK (gimp_image_mask_update),
                    image);

  g_signal_connect_object (config, "notify::transparency-type",
                           G_CALLBACK (gimp_image_invalidate_layer_previews),
                           image, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::transparency-size",
                           G_CALLBACK (gimp_image_invalidate_layer_previews),
                           image, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::layer-previews",
                           G_CALLBACK (gimp_viewable_size_changed),
                           image, G_CONNECT_SWAPPED);

  gimp_container_add (image->gimp->images, GIMP_OBJECT (image));

  return object;
}

static void
gimp_image_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  GimpImage *image = GIMP_IMAGE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      image->gimp = g_value_get_object (value);
      break;
    case PROP_ID:
      g_assert_not_reached ();
      break;
    case PROP_WIDTH:
      image->width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      image->height = g_value_get_int (value);
      break;
    case PROP_BASE_TYPE:
      image->base_type = g_value_get_enum (value);
      break;
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
  GimpImage *image = GIMP_IMAGE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, image->gimp);
      break;
    case PROP_ID:
      g_value_set_int (value, image->ID);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, image->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, image->height);
      break;
    case PROP_BASE_TYPE:
      g_value_set_enum (value, image->base_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_image_dispose (GObject *object)
{
  GimpImage *image = GIMP_IMAGE (object);

  gimp_image_undo_free (image);

  gimp_container_remove_handler (image->layers,
                                 image->layer_update_handler);
  gimp_container_remove_handler (image->layers,
                                 image->layer_visible_handler);
  gimp_container_remove_handler (image->layers,
                                 image->layer_alpha_handler);

  gimp_container_remove_handler (image->channels,
                                 image->channel_update_handler);
  gimp_container_remove_handler (image->channels,
                                 image->channel_visible_handler);
  gimp_container_remove_handler (image->channels,
                                 image->channel_name_changed_handler);
  gimp_container_remove_handler (image->channels,
                                 image->channel_color_changed_handler);

  g_signal_handlers_disconnect_by_func (image->layers,
                                        gimp_image_layer_add,
                                        image);
  g_signal_handlers_disconnect_by_func (image->layers,
                                        gimp_image_layer_remove,
                                        image);

  g_signal_handlers_disconnect_by_func (image->channels,
                                        gimp_image_channel_add,
                                        image);
  g_signal_handlers_disconnect_by_func (image->channels,
                                        gimp_image_channel_remove,
                                        image);

  gimp_container_foreach (image->layers,   (GFunc) gimp_item_removed, NULL);
  gimp_container_foreach (image->channels, (GFunc) gimp_item_removed, NULL);
  gimp_container_foreach (image->vectors,  (GFunc) gimp_item_removed, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_image_finalize (GObject *object)
{
  GimpImage *image = GIMP_IMAGE (object);

  if (image->projection)
    {
      g_object_unref (image->projection);
      image->projection = NULL;
    }

  if (image->shadow)
    gimp_image_free_shadow_tiles (image);

  if (image->cmap)
    {
      g_free (image->cmap);
      image->cmap = NULL;
    }

  if (image->layers)
    {
      g_object_unref (image->layers);
      image->layers = NULL;
    }
  if (image->channels)
    {
      g_object_unref (image->channels);
      image->channels = NULL;
    }
  if (image->vectors)
    {
      g_object_unref (image->vectors);
      image->vectors = NULL;
    }
  if (image->layer_stack)
    {
      g_slist_free (image->layer_stack);
      image->layer_stack = NULL;
    }

  if (image->selection_mask)
    {
      g_object_unref (image->selection_mask);
      image->selection_mask = NULL;
    }

  if (image->preview)
    {
      temp_buf_free (image->preview);
      image->preview = NULL;
    }

  if (image->parasites)
    {
      g_object_unref (image->parasites);
      image->parasites = NULL;
    }

  if (image->guides)
    {
      g_list_foreach (image->guides, (GFunc) g_object_unref, NULL);
      g_list_free (image->guides);
      image->guides = NULL;
    }

  if (image->grid)
    {
      g_object_unref (image->grid);
      image->grid = NULL;
    }

  if (image->sample_points)
    {
      g_list_foreach (image->sample_points,
                      (GFunc) gimp_sample_point_unref, NULL);
      g_list_free (image->sample_points);
      image->sample_points = NULL;
    }

  if (image->undo_stack)
    {
      g_object_unref (image->undo_stack);
      image->undo_stack = NULL;
    }
  if (image->redo_stack)
    {
      g_object_unref (image->redo_stack);
      image->redo_stack = NULL;
    }

  if (image->gimp && image->gimp->image_table)
    {
      g_hash_table_remove (image->gimp->image_table,
                           GINT_TO_POINTER (image->ID));
      image->gimp = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_image_name_changed (GimpObject *object)
{
  const gchar *name;

  if (GIMP_OBJECT_CLASS (parent_class)->name_changed)
    GIMP_OBJECT_CLASS (parent_class)->name_changed (object);

  name = gimp_object_get_name (object);

  if (! (name && strlen (name)))
    {
      g_free (object->name);
      object->name = NULL;
    }
}

static gint64
gimp_image_get_memsize (GimpObject *object,
                        gint64     *gui_size)
{
  GimpImage *image   = GIMP_IMAGE (object);
  gint64     memsize = 0;

  if (image->cmap)
    memsize += GIMP_IMAGE_COLORMAP_SIZE;

  if (image->shadow)
    memsize += tile_manager_get_memsize (image->shadow, FALSE);

  if (image->projection)
    memsize += gimp_object_get_memsize (GIMP_OBJECT (image->projection),
                                        gui_size);

  memsize += gimp_g_list_get_memsize (image->guides, sizeof (GimpGuide));

  if (image->grid)
    memsize += gimp_object_get_memsize (GIMP_OBJECT (image->grid), gui_size);

  memsize += gimp_g_list_get_memsize (image->sample_points,
                                      sizeof (GimpSamplePoint));

  memsize += gimp_object_get_memsize (GIMP_OBJECT (image->layers),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (image->channels),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (image->vectors),
                                      gui_size);

  memsize += gimp_g_slist_get_memsize (image->layer_stack, 0);

  if (image->selection_mask)
    memsize += gimp_object_get_memsize (GIMP_OBJECT (image->selection_mask),
                                        gui_size);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (image->parasites),
                                      gui_size);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (image->undo_stack),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (image->redo_stack),
                                      gui_size);

  if (image->preview)
    *gui_size += temp_buf_get_memsize (image->preview);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_image_get_size (GimpViewable *viewable,
                     gint         *width,
                     gint         *height)
{
  GimpImage *image = GIMP_IMAGE (viewable);

  *width  = image->width;
  *height = image->height;

  return TRUE;
}

static void
gimp_image_invalidate_preview (GimpViewable *viewable)
{
  GimpImage *image = GIMP_IMAGE (viewable);

  if (GIMP_VIEWABLE_CLASS (parent_class)->invalidate_preview)
    GIMP_VIEWABLE_CLASS (parent_class)->invalidate_preview (viewable);

  if (image->preview)
    {
      temp_buf_free (image->preview);
      image->preview = NULL;
    }
}

static void
gimp_image_size_changed (GimpViewable *viewable)
{
  GimpImage *image = GIMP_IMAGE (viewable);
  GList     *list;

  if (GIMP_VIEWABLE_CLASS (parent_class)->size_changed)
    GIMP_VIEWABLE_CLASS (parent_class)->size_changed (viewable);

  gimp_container_foreach (image->layers,
                          (GFunc) gimp_viewable_size_changed,
                          NULL);
  gimp_container_foreach (image->channels,
                          (GFunc) gimp_viewable_size_changed,
                          NULL);
  gimp_container_foreach (image->vectors,
                          (GFunc) gimp_viewable_size_changed,
                          NULL);

  for (list = GIMP_LIST (image->layers)->list; list; list = g_list_next (list))
    {
      GimpLayerMask *mask = gimp_layer_get_mask (GIMP_LAYER (list->data));

      if (mask)
        gimp_viewable_size_changed (GIMP_VIEWABLE (mask));
    }

  gimp_viewable_size_changed (GIMP_VIEWABLE (gimp_image_get_mask (image)));
}

static gchar *
gimp_image_get_description (GimpViewable  *viewable,
                            gchar        **tooltip)
{
  GimpImage   *image = GIMP_IMAGE (viewable);
  const gchar *uri;
  gchar       *basename;
  gchar       *retval;

  uri = gimp_image_get_uri (GIMP_IMAGE (image));

  basename = file_utils_uri_display_basename (uri);

  if (tooltip)
    *tooltip = file_utils_uri_display_name (uri);

  retval = g_strdup_printf ("%s-%d", basename, gimp_image_get_ID (image));

  g_free (basename);

  return retval;
}

static void
gimp_image_real_colormap_changed (GimpImage *image,
                                  gint       color_index)
{
  if (gimp_image_base_type (image) == GIMP_INDEXED)
    {
      gimp_image_color_hash_invalidate (image, color_index);

      /* A colormap alteration affects the whole image */
      gimp_image_update (image, 0, 0, image->width, image->height);

      gimp_image_invalidate_layer_previews (image);
    }
}

static void
gimp_image_real_flush (GimpImage *image,
                       gboolean   invalidate_preview)
{
  if (image->flush_accum.alpha_changed)
    {
      gimp_image_alpha_changed (image);
      image->flush_accum.alpha_changed = FALSE;
    }

  if (image->flush_accum.mask_changed)
    {
      gimp_image_mask_changed (image);
      image->flush_accum.mask_changed = FALSE;
    }

  if (image->flush_accum.preview_invalidated)
    {
      /*  don't invalidate the preview here, the projection does this when
       *  it is completely constructed.
       */
      image->flush_accum.preview_invalidated = FALSE;
    }
}

static void
gimp_image_mask_update (GimpDrawable *drawable,
                        gint          x,
                        gint          y,
                        gint          width,
                        gint          height,
                        GimpImage    *image)
{
  image->flush_accum.mask_changed = TRUE;
}

static void
gimp_image_drawable_update (GimpDrawable *drawable,
                            gint          x,
                            gint          y,
                            gint          width,
                            gint          height,
                            GimpImage    *image)
{
  GimpItem *item = GIMP_ITEM (drawable);

  if (gimp_item_get_visible (item))
    {
      gint offset_x;
      gint offset_y;

      gimp_item_offsets (item, &offset_x, &offset_y);

      gimp_image_update (image, x + offset_x, y + offset_y, width, height);
    }
}

static void
gimp_image_drawable_visibility (GimpItem  *item,
                                GimpImage *image)
{
  gint offset_x;
  gint offset_y;

  gimp_item_offsets (item, &offset_x, &offset_y);

  gimp_image_update (image,
                     offset_x, offset_y,
                     gimp_item_width (item),
                     gimp_item_height (item));
}

static void
gimp_image_layer_alpha_changed (GimpDrawable *drawable,
                                GimpImage    *image)
{
  if (gimp_container_num_children (image->layers) == 1)
    image->flush_accum.alpha_changed = TRUE;
}

static void
gimp_image_layer_add (GimpContainer *container,
                      GimpLayer     *layer,
                      GimpImage     *image)
{
  GimpItem *item = GIMP_ITEM (layer);

  if (gimp_item_get_visible (item))
    gimp_image_drawable_visibility (item, image);
}

static void
gimp_image_layer_remove (GimpContainer *container,
                         GimpLayer     *layer,
                         GimpImage     *image)
{
  GimpItem *item = GIMP_ITEM (layer);

  if (gimp_item_get_visible (item))
    gimp_image_drawable_visibility (item, image);
}

static void
gimp_image_channel_add (GimpContainer *container,
                        GimpChannel   *channel,
                        GimpImage     *image)
{
  GimpItem *item = GIMP_ITEM (channel);

  if (gimp_item_get_visible (item))
    gimp_image_drawable_visibility (item, image);

  if (! strcmp (GIMP_IMAGE_QUICK_MASK_NAME,
                gimp_object_get_name (GIMP_OBJECT (channel))))
    {
      gimp_image_set_quick_mask_state (image, TRUE);
    }
}

static void
gimp_image_channel_remove (GimpContainer *container,
                           GimpChannel   *channel,
                           GimpImage     *image)
{
  GimpItem *item = GIMP_ITEM (channel);

  if (gimp_item_get_visible (item))
    gimp_image_drawable_visibility (item, image);

  if (! strcmp (GIMP_IMAGE_QUICK_MASK_NAME,
                gimp_object_get_name (GIMP_OBJECT (channel))))
    {
      gimp_image_set_quick_mask_state (image, FALSE);
    }
}

static void
gimp_image_channel_name_changed (GimpChannel *channel,
                                 GimpImage   *image)
{
  if (! strcmp (GIMP_IMAGE_QUICK_MASK_NAME,
                gimp_object_get_name (GIMP_OBJECT (channel))))
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
                gimp_object_get_name (GIMP_OBJECT (channel))))
    {
      image->quick_mask_color = channel->color;
    }
}


/*  public functions  */

GimpImage *
gimp_image_new (Gimp              *gimp,
                gint               width,
                gint               height,
                GimpImageBaseType  base_type)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_new (GIMP_TYPE_IMAGE,
                       "gimp",      gimp,
                       "width",     width,
                       "height",    height,
                       "base-type", base_type,
                       NULL);
}

GimpImageBaseType
gimp_image_base_type (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), -1);

  return image->base_type;
}

GimpImageType
gimp_image_base_type_with_alpha (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), -1);

  switch (image->base_type)
    {
    case GIMP_RGB:
      return GIMP_RGBA_IMAGE;
    case GIMP_GRAY:
      return GIMP_GRAYA_IMAGE;
    case GIMP_INDEXED:
      return GIMP_INDEXEDA_IMAGE;
    }

  return GIMP_RGB_IMAGE;
}

CombinationMode
gimp_image_get_combination_mode (GimpImageType dest_type,
                                 gint          src_bytes)
{
  return valid_combinations[dest_type][src_bytes];
}

gint
gimp_image_get_ID (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), -1);

  return image->ID;
}

GimpImage *
gimp_image_get_by_ID (Gimp *gimp,
                      gint  image_id)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (gimp->image_table == NULL)
    return NULL;

  return (GimpImage *) g_hash_table_lookup (gimp->image_table,
                                            GINT_TO_POINTER (image_id));
}

void
gimp_image_set_uri (GimpImage   *image,
                    const gchar *uri)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  gimp_object_set_name (GIMP_OBJECT (image), uri);
}

static void
gimp_image_take_uri (GimpImage *image,
                     gchar     *uri)
{
  gimp_object_take_name (GIMP_OBJECT (image), uri);
}

const gchar *
gimp_image_get_uri (const GimpImage *image)
{
  const gchar *uri;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  uri = gimp_object_get_name (GIMP_OBJECT (image));

  return uri ? uri : _("Untitled");
}

void
gimp_image_set_filename (GimpImage   *image,
                         const gchar *filename)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  if (filename && strlen (filename))
    {
      gimp_image_take_uri (image,
                           file_utils_filename_to_uri (image->gimp,
                                                       filename,
                                                       NULL));
    }
  else
    {
      gimp_image_set_uri (image, NULL);
    }
}

gchar *
gimp_image_get_filename (const GimpImage *image)
{
  const gchar *uri;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  uri = gimp_object_get_name (GIMP_OBJECT (image));

  if (! uri)
    return NULL;

  return g_filename_from_uri (uri, NULL, NULL);
}

void
gimp_image_set_save_proc (GimpImage           *image,
                          GimpPlugInProcedure *proc)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  image->save_proc = proc;
}

GimpPlugInProcedure *
gimp_image_get_save_proc (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return image->save_proc;
}

void
gimp_image_set_resolution (GimpImage *image,
                           gdouble    xresolution,
                           gdouble    yresolution)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  /* don't allow to set the resolution out of bounds */
  if (xresolution < GIMP_MIN_RESOLUTION || xresolution > GIMP_MAX_RESOLUTION ||
      yresolution < GIMP_MIN_RESOLUTION || yresolution > GIMP_MAX_RESOLUTION)
    return;

  if ((ABS (image->xresolution - xresolution) >= 1e-5) ||
      (ABS (image->yresolution - yresolution) >= 1e-5))
    {
      gimp_image_undo_push_image_resolution (image,
                                             _("Change Image Resolution"));

      image->xresolution = xresolution;
      image->yresolution = yresolution;

      gimp_image_resolution_changed (image);
      gimp_viewable_size_changed (GIMP_VIEWABLE (image));
    }
}

void
gimp_image_get_resolution (const GimpImage *image,
                           gdouble         *xresolution,
                           gdouble         *yresolution)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (xresolution && yresolution);

  *xresolution = image->xresolution;
  *yresolution = image->yresolution;
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
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (unit > GIMP_UNIT_PIXEL);

  if (image->resolution_unit != unit)
    {
      gimp_image_undo_push_image_resolution (image,
                                             _("Change Image Unit"));

      image->resolution_unit = unit;
      gimp_image_unit_changed (image);
    }
}

GimpUnit
gimp_image_get_unit (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), GIMP_UNIT_INCH);

  return image->resolution_unit;
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

  return image->width;
}

gint
gimp_image_get_height (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  return image->height;
}

gboolean
gimp_image_has_alpha (const GimpImage *image)
{
  GimpLayer *layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), TRUE);

  layer = (GimpLayer *) gimp_container_get_child_by_index (image->layers, 0);

  return ((gimp_container_num_children (image->layers) > 1) ||
          (layer && gimp_drawable_has_alpha (GIMP_DRAWABLE (layer))));
}

gboolean
gimp_image_is_empty (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), TRUE);

  return gimp_container_is_empty (image->layers);
}

GimpLayer *
gimp_image_floating_sel (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return image->floating_sel;
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

  return image->selection_mask;
}

void
gimp_image_mask_changed (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[MASK_CHANGED], 0);
}

gint
gimp_image_get_component_index (const GimpImage *image,
                                GimpChannelType  channel)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), -1);

  switch (channel)
    {
    case GIMP_RED_CHANNEL:     return RED_PIX;
    case GIMP_GREEN_CHANNEL:   return GREEN_PIX;
    case GIMP_BLUE_CHANNEL:    return BLUE_PIX;
    case GIMP_GRAY_CHANNEL:    return GRAY_PIX;
    case GIMP_INDEXED_CHANNEL: return INDEXED_PIX;
    case GIMP_ALPHA_CHANNEL:
      switch (gimp_image_base_type (image))
        {
        case GIMP_RGB:     return ALPHA_PIX;
        case GIMP_GRAY:    return ALPHA_G_PIX;
        case GIMP_INDEXED: return ALPHA_I_PIX;
        }
    }

  return -1;
}

void
gimp_image_set_component_active (GimpImage       *image,
                                 GimpChannelType  channel,
                                 gboolean         active)
{
  gint index = -1;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  index = gimp_image_get_component_index (image, channel);

  if (index != -1 && active != image->active[index])
    {
      GimpLayer *floating_sel = gimp_image_floating_sel (image);

      if (floating_sel)
        floating_sel_relax (floating_sel, FALSE);

      image->active[index] = active ? TRUE : FALSE;

      if (floating_sel)
        {
          floating_sel_rigor (floating_sel, FALSE);
          gimp_drawable_update (GIMP_DRAWABLE (floating_sel),
                                0, 0,
                                GIMP_ITEM (floating_sel)->width,
                                GIMP_ITEM (floating_sel)->height);
        }

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
    return image->active[index];

  return FALSE;
}

void
gimp_image_set_component_visible (GimpImage       *image,
                                  GimpChannelType  channel,
                                  gboolean         visible)
{
  gint index = -1;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  index = gimp_image_get_component_index (image, channel);

  if (index != -1 && visible != image->visible[index])
    {
      image->visible[index] = visible ? TRUE : FALSE;

      g_signal_emit (image,
                     gimp_image_signals[COMPONENT_VISIBILITY_CHANGED], 0,
                     channel);

      gimp_image_update (image, 0, 0, image->width, image->height);
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
    return image->visible[index];

  return FALSE;
}

void
gimp_image_mode_changed (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[MODE_CHANGED], 0);
}

void
gimp_image_alpha_changed (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[ALPHA_CHANGED], 0);
}

void
gimp_image_update (GimpImage *image,
                   gint       x,
                   gint       y,
                   gint       width,
                   gint       height)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[UPDATE], 0,
                 x, y, width, height);

  image->flush_accum.preview_invalidated = TRUE;
}

void
gimp_image_update_guide (GimpImage *image,
                         GimpGuide *guide)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (guide != NULL);

  g_signal_emit (image, gimp_image_signals[UPDATE_GUIDE], 0, guide);
}

void
gimp_image_update_sample_point (GimpImage       *image,
                                GimpSamplePoint *sample_point)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (sample_point != NULL);

  g_signal_emit (image, gimp_image_signals[UPDATE_SAMPLE_POINT], 0,
                 sample_point);
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
gimp_image_colormap_changed (GimpImage *image,
                             gint       color_index)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (color_index >= -1 && color_index < image->num_cols);

  g_signal_emit (image, gimp_image_signals[COLORMAP_CHANGED], 0,
                 color_index);
}

void
gimp_image_selection_control (GimpImage            *image,
                              GimpSelectionControl  control)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[SELECTION_CONTROL], 0, control);
}

void
gimp_image_quick_mask_changed (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[QUICK_MASK_CHANGED], 0);
}


/*  undo  */

gboolean
gimp_image_undo_is_enabled (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  return (image->undo_freeze_count == 0);
}

gboolean
gimp_image_undo_enable (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  /*  Free all undo steps as they are now invalidated  */
  gimp_image_undo_free (image);

  return gimp_image_undo_thaw (image);
}

gboolean
gimp_image_undo_disable (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  return gimp_image_undo_freeze (image);
}

gboolean
gimp_image_undo_freeze (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  image->undo_freeze_count++;

  if (image->undo_freeze_count == 1)
    gimp_image_undo_event (image, GIMP_UNDO_EVENT_UNDO_FREEZE, NULL);

  return TRUE;
}

gboolean
gimp_image_undo_thaw (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (image->undo_freeze_count > 0, FALSE);

  image->undo_freeze_count--;

  if (image->undo_freeze_count == 0)
    gimp_image_undo_event (image, GIMP_UNDO_EVENT_UNDO_THAW, NULL);

  return TRUE;
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
 *   If the counter is around 10000, this is due to undo-ing back
 *   before a saved version, then mutating the image (thus destroying
 *   the redo stack).  Once this has happened, it's impossible to get
 *   the image back to the state on disk, since the redo info has been
 *   freed.  See undo.c for the gorey details.
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
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  image->dirty++;

  if (! image->dirty_time)
    image->dirty_time = time (NULL);

  g_signal_emit (image, gimp_image_signals[DIRTY], 0, dirty_mask);

  TRC (("dirty %d -> %d\n", image->dirty - 1, image->dirty));

  return image->dirty;
}

gint
gimp_image_clean (GimpImage     *image,
                  GimpDirtyMask  dirty_mask)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  image->dirty--;

  g_signal_emit (image, gimp_image_signals[CLEAN], 0, dirty_mask);

  TRC (("clean %d -> %d\n", image->dirty + 1, image->dirty));

  return image->dirty;
}

void
gimp_image_clean_all (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  image->dirty      = 0;
  image->dirty_time = 0;

  g_signal_emit (image, gimp_image_signals[CLEAN], 0);
}

/**
 * gimp_image_saved:
 * @image:
 * @uri:
 *
 * Emits the "saved" signal, indicating that @image was saved to the
 * location specified by @uri.
 */
void
gimp_image_saved (GimpImage   *image,
                  const gchar *uri)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (uri != NULL);

  g_signal_emit (image, gimp_image_signals[SAVED], 0, uri);
}

/*  flush this image's displays  */

void
gimp_image_flush (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[FLUSH], 0,
                 image->flush_accum.preview_invalidated);
}


/*  color transforms / utilities  */

void
gimp_image_get_foreground (const GimpImage *image,
                           GimpContext     *context,
                           GimpImageType    dest_type,
                           guchar          *fg)
{
  GimpRGB  color;
  guchar   pfg[3];

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (fg != NULL);

  gimp_context_get_foreground (context, &color);

  gimp_rgb_get_uchar (&color, &pfg[0], &pfg[1], &pfg[2]);

  gimp_image_transform_color (image, dest_type, fg, GIMP_RGB, pfg);
}

void
gimp_image_get_background (const GimpImage *image,
                           GimpContext     *context,
                           GimpImageType    dest_type,
                           guchar          *bg)
{
  GimpRGB  color;
  guchar   pbg[3];

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (bg != NULL);

  gimp_context_get_background (context, &color);

  gimp_rgb_get_uchar (&color, &pbg[0], &pbg[1], &pbg[2]);

  gimp_image_transform_color (image, dest_type, bg, GIMP_RGB, pbg);
}

void
gimp_image_get_color (const GimpImage *src_image,
                      GimpImageType    src_type,
                      const guchar    *src,
                      guchar          *rgba)
{
  gboolean has_alpha = FALSE;

  g_return_if_fail (GIMP_IS_IMAGE (src_image));

  switch (src_type)
    {
    case GIMP_RGBA_IMAGE:
      has_alpha = TRUE;

    case GIMP_RGB_IMAGE:
      /*  Straight copy  */
      *rgba++ = *src++;
      *rgba++ = *src++;
      *rgba++ = *src++;
      break;

    case GIMP_GRAYA_IMAGE:
      has_alpha = TRUE;

    case GIMP_GRAY_IMAGE:
      /*  Gray to RG&B */
      *rgba++ = *src;
      *rgba++ = *src;
      *rgba++ = *src++;
      break;

    case GIMP_INDEXEDA_IMAGE:
      has_alpha = TRUE;

    case GIMP_INDEXED_IMAGE:
      /*  Indexed palette lookup  */
      {
        gint index = *src++ * 3;

        *rgba++ = src_image->cmap[index++];
        *rgba++ = src_image->cmap[index++];
        *rgba++ = src_image->cmap[index++];
      }
      break;
    }

  if (has_alpha)
    *rgba = *src;
  else
    *rgba = OPAQUE_OPACITY;
}

void
gimp_image_transform_rgb (const GimpImage    *dest_image,
                          GimpImageType       dest_type,
                          const GimpRGB      *rgb,
                          guchar             *color)
{
  guchar col[3];

  g_return_if_fail (GIMP_IS_IMAGE (dest_image));
  g_return_if_fail (rgb != NULL);
  g_return_if_fail (color != NULL);

  gimp_rgb_get_uchar (rgb, &col[0], &col[1], &col[2]);

  gimp_image_transform_color (dest_image, dest_type, color, GIMP_RGB, col);
}

void
gimp_image_transform_color (const GimpImage    *dest_image,
                            GimpImageType       dest_type,
                            guchar             *dest,
                            GimpImageBaseType   src_type,
                            const guchar       *src)
{
  g_return_if_fail (GIMP_IS_IMAGE (dest_image));
  g_return_if_fail (src_type != GIMP_INDEXED);

  switch (src_type)
    {
    case GIMP_RGB:
      switch (dest_type)
        {
        case GIMP_RGB_IMAGE:
        case GIMP_RGBA_IMAGE:
          /*  Straight copy  */
          *dest++ = *src++;
          *dest++ = *src++;
          *dest++ = *src++;
          break;

        case GIMP_GRAY_IMAGE:
        case GIMP_GRAYA_IMAGE:
          /*  NTSC conversion  */
          *dest = GIMP_RGB_LUMINANCE (src[RED_PIX],
                                      src[GREEN_PIX],
                                      src[BLUE_PIX]) + 0.5;
          break;

        case GIMP_INDEXED_IMAGE:
        case GIMP_INDEXEDA_IMAGE:
          /*  Least squares method  */
          *dest = gimp_image_color_hash_rgb_to_indexed (dest_image,
                                                        src[RED_PIX],
                                                        src[GREEN_PIX],
                                                        src[BLUE_PIX]);
          break;
        }
      break;

    case GIMP_GRAY:
      switch (dest_type)
        {
        case GIMP_RGB_IMAGE:
        case GIMP_RGBA_IMAGE:
          /*  Gray to RG&B */
          *dest++ = *src;
          *dest++ = *src;
          *dest++ = *src;
          break;

        case GIMP_GRAY_IMAGE:
        case GIMP_GRAYA_IMAGE:
          /*  Straight copy  */
          *dest = *src;
          break;

        case GIMP_INDEXED_IMAGE:
        case GIMP_INDEXEDA_IMAGE:
          /*  Least squares method  */
          *dest = gimp_image_color_hash_rgb_to_indexed (dest_image,
                                                        src[GRAY_PIX],
                                                        src[GRAY_PIX],
                                                        src[GRAY_PIX]);
          break;
        }
      break;

    default:
      break;
    }
}

TempBuf *
gimp_image_transform_temp_buf (const GimpImage *dest_image,
                               GimpImageType    dest_type,
                               TempBuf         *temp_buf,
                               gboolean        *new_buf)
{
  TempBuf       *ret_buf;
  GimpImageType  ret_buf_type;
  gboolean       has_alpha;
  gboolean       is_rgb;
  gint           in_bytes;
  gint           out_bytes;

  g_return_val_if_fail (GIMP_IMAGE (dest_image), NULL);
  g_return_val_if_fail (temp_buf != NULL, NULL);
  g_return_val_if_fail (new_buf != NULL, NULL);

  in_bytes  = temp_buf->bytes;

  has_alpha = (in_bytes == 2 || in_bytes == 4);
  is_rgb    = (in_bytes == 3 || in_bytes == 4);

  if (has_alpha)
    ret_buf_type = GIMP_IMAGE_TYPE_WITH_ALPHA (dest_type);
  else
    ret_buf_type = GIMP_IMAGE_TYPE_WITHOUT_ALPHA (dest_type);

  out_bytes = GIMP_IMAGE_TYPE_BYTES (ret_buf_type);

  /*  If the pattern doesn't match the image in terms of color type,
   *  transform it.  (ie  pattern is RGB, image is indexed)
   */
  if (in_bytes != out_bytes || GIMP_IMAGE_TYPE_IS_INDEXED (dest_type))
    {
      guchar *src;
      guchar *dest;
      gint    size;

      ret_buf = temp_buf_new (temp_buf->width, temp_buf->height,
                              out_bytes, 0, 0, NULL);

      src  = temp_buf_data (temp_buf);
      dest = temp_buf_data (ret_buf);

      size = temp_buf->width * temp_buf->height;

      while (size--)
        {
          gimp_image_transform_color (dest_image, dest_type, dest,
                                      is_rgb ? GIMP_RGB : GIMP_GRAY, src);

          /* Handle alpha */
          if (has_alpha)
            dest[out_bytes - 1] = src[in_bytes - 1];

          src  += in_bytes;
          dest += out_bytes;
        }

      *new_buf = TRUE;
    }
  else
    {
      ret_buf = temp_buf;
      *new_buf = FALSE;
    }

  return ret_buf;
}


/*  shadow tiles  */

TileManager *
gimp_image_get_shadow_tiles (GimpImage *image,
                             gint       width,
                             gint       height,
                             gint       bpp)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  if (image->shadow)
    {
      if ((width  != tile_manager_width  (image->shadow)) ||
          (height != tile_manager_height (image->shadow)) ||
          (bpp    != tile_manager_bpp    (image->shadow)))
        {
          gimp_image_free_shadow_tiles (image);
        }
      else
        {
          return image->shadow;
        }
    }

  image->shadow = tile_manager_new (width, height, bpp);

  return image->shadow;
}

void
gimp_image_free_shadow_tiles (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  if (image->shadow)
    {
      tile_manager_unref (image->shadow);
      image->shadow = NULL;
    }
}


/*  parasites  */

const GimpParasite *
gimp_image_parasite_find (const GimpImage *image,
                          const gchar     *name)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return gimp_parasite_list_find (image->parasites, name);
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
  gchar **list;
  gchar **cur;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  *count = gimp_parasite_list_length (image->parasites);
  cur = list = g_new (gchar *, *count);

  gimp_parasite_list_foreach (image->parasites, (GHFunc) list_func, &cur);

  return list;
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
                                         _("Attach Parasite to Image"),
                                         &copy);

  /*  We used to push an cantundo on te stack here. This made the undo stack
   *  unusable (NULL on the stack) and prevented people from undoing after a
   *  save (since most save plug-ins attach an undoable comment parasite).
   *  Now we simply attach the parasite without pushing an undo. That way
   *  it's undoable but does not block the undo system.   --Sven
   */
  gimp_parasite_list_add (image->parasites, &copy);

  if (gimp_parasite_has_flag (&copy, GIMP_PARASITE_ATTACH_PARENT))
    {
      gimp_parasite_shift_parent (&copy);
      gimp_parasite_attach (image->gimp, &copy);
    }

  g_signal_emit (image, gimp_image_signals[PARASITE_ATTACHED], 0,
                 parasite->name);

  if (strcmp (parasite->name, "icc-profile") == 0)
    gimp_color_managed_profile_changed (GIMP_COLOR_MANAGED (image));
}

void
gimp_image_parasite_detach (GimpImage   *image,
                            const gchar *name)
{
  const GimpParasite *parasite;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (name != NULL);

  if (! (parasite = gimp_parasite_list_find (image->parasites, name)))
    return;

  if (gimp_parasite_is_undoable (parasite))
    gimp_image_undo_push_image_parasite_remove (image,
                                                _("Remove Parasite from Image"),
                                                name);

  gimp_parasite_list_remove (image->parasites, name);

  g_signal_emit (image, gimp_image_signals[PARASITE_DETACHED], 0,
                 name);

  if (strcmp (name, "icc-profile") == 0)
    gimp_color_managed_profile_changed (GIMP_COLOR_MANAGED (image));
}


/*  tattoos  */

GimpTattoo
gimp_image_get_new_tattoo (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  image->tattoo_state++;

  if (G_UNLIKELY (image->tattoo_state == 0))
    g_warning ("%s: Tattoo state corrupted (integer overflow).", G_STRFUNC);

  return image->tattoo_state;
}

GimpTattoo
gimp_image_get_tattoo_state (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), 0);

  return image->tattoo_state;
}

gboolean
gimp_image_set_tattoo_state (GimpImage  *image,
                             GimpTattoo  val)
{
  GList      *list;
  gboolean    retval = TRUE;
  GimpTattoo  maxval = 0;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  /* Check that the layer tattoos don't overlap with channel or vector ones */
  for (list = GIMP_LIST (image->layers)->list;
       list;
       list = g_list_next (list))
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

  /* Now check that the channel and vectors tattoos don't overlap */
  for (list = GIMP_LIST (image->channels)->list;
       list;
       list = g_list_next (list))
    {
      GimpTattoo ctattoo;

      ctattoo = gimp_item_get_tattoo (GIMP_ITEM (list->data));
      if (ctattoo > maxval)
        maxval = ctattoo;

      if (gimp_image_get_vectors_by_tattoo (image, ctattoo))
        retval = FALSE; /* Oopps duplicated tattoo in vectors */
    }

  /* Find the max tattoo value in the vectors */
  for (list = GIMP_LIST (image->vectors)->list;
       list;
       list = g_list_next (list))
    {
      GimpTattoo vtattoo;

      vtattoo = gimp_item_get_tattoo (GIMP_ITEM (list->data));
      if (vtattoo > maxval)
        maxval = vtattoo;
    }

  if (val < maxval)
    retval = FALSE;

  /* Must check if the state is valid */
  if (retval == TRUE)
    image->tattoo_state = val;

  return retval;
}


/*  layers / channels / vectors  */

GimpContainer *
gimp_image_get_layers (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return image->layers;
}

GimpContainer *
gimp_image_get_channels (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return image->channels;
}

GimpContainer *
gimp_image_get_vectors (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return image->vectors;
}

GimpDrawable *
gimp_image_get_active_drawable (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  /*  If there is an active channel (a saved selection, etc.),
   *  we ignore the active layer
   */
  if (image->active_channel)
    {
      return GIMP_DRAWABLE (image->active_channel);
    }
  else if (image->active_layer)
    {
      GimpLayer *layer = image->active_layer;

      if (layer->mask && layer->mask->edit_mask)
        return GIMP_DRAWABLE (layer->mask);
      else
        return GIMP_DRAWABLE (layer);
    }

  return NULL;
}

GimpLayer *
gimp_image_get_active_layer (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return image->active_layer;
}

GimpChannel *
gimp_image_get_active_channel (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return image->active_channel;
}

GimpVectors *
gimp_image_get_active_vectors (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return image->active_vectors;
}

GimpLayer *
gimp_image_set_active_layer (GimpImage *image,
                             GimpLayer *layer)
{
  GimpLayer *floating_sel;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (layer == NULL || GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (layer == NULL ||
                        gimp_container_have (image->layers,
                                             GIMP_OBJECT (layer)), NULL);

  floating_sel = gimp_image_floating_sel (image);

  /*  Make sure the floating_sel always is the active layer  */
  if (floating_sel && layer != floating_sel)
    return floating_sel;

  if (layer != image->active_layer)
    {
      if (layer)
        {
          /*  Configure the layer stack to reflect this change  */
          image->layer_stack = g_slist_remove (image->layer_stack, layer);
          image->layer_stack = g_slist_prepend (image->layer_stack, layer);
        }

      /*  Don't cache selection info for the previous active layer  */
      if (image->active_layer)
        gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (image->active_layer));

      image->active_layer = layer;

      g_signal_emit (image, gimp_image_signals[ACTIVE_LAYER_CHANGED], 0);

      if (layer && image->active_channel)
        gimp_image_set_active_channel (image, NULL);
    }

  return image->active_layer;
}

GimpChannel *
gimp_image_set_active_channel (GimpImage   *image,
                               GimpChannel *channel)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (channel == NULL || GIMP_IS_CHANNEL (channel), NULL);
  g_return_val_if_fail (channel == NULL ||
                        gimp_container_have (image->channels,
                                             GIMP_OBJECT (channel)), NULL);

  /*  Not if there is a floating selection  */
  if (channel && gimp_image_floating_sel (image))
    return NULL;

  if (channel != image->active_channel)
    {
      image->active_channel = channel;

      g_signal_emit (image, gimp_image_signals[ACTIVE_CHANNEL_CHANGED], 0);

      if (channel && image->active_layer)
        gimp_image_set_active_layer (image, NULL);
    }

  return image->active_channel;
}

GimpChannel *
gimp_image_unset_active_channel (GimpImage *image)
{
  GimpChannel *channel;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  channel = gimp_image_get_active_channel (image);

  if (channel)
    {
      gimp_image_set_active_channel (image, NULL);

      if (image->layer_stack)
        gimp_image_set_active_layer (image, image->layer_stack->data);
    }

  return channel;
}

GimpVectors *
gimp_image_set_active_vectors (GimpImage   *image,
                               GimpVectors *vectors)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (vectors == NULL || GIMP_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (vectors == NULL ||
                        gimp_container_have (image->vectors,
                                             GIMP_OBJECT (vectors)), NULL);

  if (vectors != image->active_vectors)
    {
      image->active_vectors = vectors;

      g_signal_emit (image, gimp_image_signals[ACTIVE_VECTORS_CHANGED], 0);
    }

  return image->active_vectors;
}

void
gimp_image_active_layer_changed (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[ACTIVE_LAYER_CHANGED], 0);
}

void
gimp_image_active_channel_changed (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[ACTIVE_CHANNEL_CHANGED], 0);
}

void
gimp_image_active_vectors_changed (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_signal_emit (image, gimp_image_signals[ACTIVE_VECTORS_CHANGED], 0);
}

gint
gimp_image_get_layer_index (const GimpImage   *image,
                            const GimpLayer   *layer)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), -1);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), -1);

  return gimp_container_get_child_index (image->layers,
                                         GIMP_OBJECT (layer));
}

gint
gimp_image_get_channel_index (const GimpImage   *image,
                              const GimpChannel *channel)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), -1);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), -1);

  return gimp_container_get_child_index (image->channels,
                                         GIMP_OBJECT (channel));
}

gint
gimp_image_get_vectors_index (const GimpImage   *image,
                              const GimpVectors *vectors)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), -1);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), -1);

  return gimp_container_get_child_index (image->vectors,
                                         GIMP_OBJECT (vectors));
}

static GimpItem *
gimp_image_get_item_by_tattoo (GimpContainer *items,
                               GimpTattoo     tattoo)
{
  GList *list;

  for (list = GIMP_LIST (items)->list; list; list = g_list_next (list))
    {
      GimpItem *item = list->data;

      if (gimp_item_get_tattoo (item) == tattoo)
        return item;
    }

  return NULL;
}

GimpLayer *
gimp_image_get_layer_by_tattoo (const GimpImage *image,
                                GimpTattoo       tattoo)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_LAYER (gimp_image_get_item_by_tattoo (image->layers,
                                                    tattoo));
}

GimpChannel *
gimp_image_get_channel_by_tattoo (const GimpImage *image,
                                  GimpTattoo       tattoo)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_CHANNEL (gimp_image_get_item_by_tattoo (image->channels,
                                                      tattoo));
}

GimpVectors *
gimp_image_get_vectors_by_tattoo (const GimpImage *image,
                                  GimpTattoo       tattoo)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_VECTORS (gimp_image_get_item_by_tattoo (image->vectors,
                                                      tattoo));
}

GimpLayer *
gimp_image_get_layer_by_name (const GimpImage *image,
                              const gchar     *name)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_LAYER (gimp_container_get_child_by_name (image->layers,
                                                       name));
}

GimpChannel *
gimp_image_get_channel_by_name (const GimpImage *image,
                                const gchar     *name)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_CHANNEL (gimp_container_get_child_by_name (image->channels,
                                                         name));
}

GimpVectors *
gimp_image_get_vectors_by_name (const GimpImage *image,
                                const gchar     *name)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_VECTORS (gimp_container_get_child_by_name (image->vectors,
                                                         name));
}

gboolean
gimp_image_add_layer (GimpImage *image,
                      GimpLayer *layer,
                      gint       position)
{
  GimpLayer *active_layer;
  GimpLayer *floating_sel;
  gboolean   old_has_alpha;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  if (GIMP_ITEM (layer)->image != NULL &&
      GIMP_ITEM (layer)->image != image)
    {
      g_warning ("%s: attempting to add layer to wrong image.", G_STRFUNC);
      return FALSE;
    }

  if (gimp_container_have (image->layers, GIMP_OBJECT (layer)))
    {
      g_warning ("%s: trying to add layer to image twice.", G_STRFUNC);
      return FALSE;
    }

  floating_sel = gimp_image_floating_sel (image);

  if (floating_sel && gimp_layer_is_floating_sel (layer))
    {
      g_warning ("%s: trying to add floating layer to image which alyready "
                 "has a floating selection.", G_STRFUNC);
      return FALSE;
    }

  active_layer = gimp_image_get_active_layer (image);

  old_has_alpha = gimp_image_has_alpha (image);

  gimp_image_undo_push_layer_add (image, _("Add Layer"),
                                  layer, active_layer);

  gimp_item_set_image (GIMP_ITEM (layer), image);

  if (layer->mask)
    gimp_item_set_image (GIMP_ITEM (layer->mask), image);

  /*  If the layer is a floating selection, set the ID  */
  if (gimp_layer_is_floating_sel (layer))
    image->floating_sel = layer;

  /*  add the layer to the list at the specified position  */
  if (position == -1)
    {
      if (active_layer)
        position = gimp_container_get_child_index (image->layers,
                                                   GIMP_OBJECT (active_layer));
      else
        position = 0;
    }

  /*  If there is a floating selection (and this isn't it!),
   *  make sure the insert position is greater than 0
   */
  if (position == 0 && floating_sel)
    position = 1;

  /*  Don't add at a non-existing index  */
  if (position > gimp_container_num_children (image->layers))
    position = gimp_container_num_children (image->layers);

  g_object_ref_sink (layer);
  gimp_container_insert (image->layers, GIMP_OBJECT (layer), position);
  g_object_unref (layer);

  /*  notify the layers dialog of the currently active layer  */
  gimp_image_set_active_layer (image, layer);

  if (old_has_alpha != gimp_image_has_alpha (image))
    gimp_image_alpha_changed (image);

  if (gimp_layer_is_floating_sel (layer))
    gimp_image_floating_selection_changed (image);

  return TRUE;
}

void
gimp_image_remove_layer (GimpImage *image,
                         GimpLayer *layer)
{
  GimpLayer *active_layer;
  gint       index;
  gboolean   old_has_alpha;
  gboolean   undo_group = FALSE;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_container_have (image->layers,
                                         GIMP_OBJECT (layer)));

  if (gimp_drawable_has_floating_sel (GIMP_DRAWABLE (layer)))
    {
      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                   _("Remove Layer"));
      undo_group = TRUE;

      floating_sel_remove (gimp_image_floating_sel (image));
    }

  active_layer = gimp_image_get_active_layer (image);

  index = gimp_container_get_child_index (image->layers,
                                          GIMP_OBJECT (layer));

  old_has_alpha = gimp_image_has_alpha (image);

  gimp_image_undo_push_layer_remove (image, _("Remove Layer"),
                                     layer, index, active_layer);

  g_object_ref (layer);

  /*  Make sure we're not caching any old selection info  */
  if (layer == active_layer)
    gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (layer));

  gimp_container_remove (image->layers, GIMP_OBJECT (layer));
  image->layer_stack = g_slist_remove (image->layer_stack, layer);

  if (image->floating_sel == layer)
    {
      /*  If this was the floating selection, reset the fs pointer
       *  and activate the underlying drawable
       */
      image->floating_sel = NULL;

      floating_sel_activate_drawable (layer);

      gimp_image_floating_selection_changed (image);
    }
  else if (layer == active_layer)
    {
      if (image->layer_stack)
        {
          active_layer = image->layer_stack->data;
        }
      else
        {
          gint n_children = gimp_container_num_children (image->layers);

          if (n_children > 0)
            {
              index = CLAMP (index, 0, n_children - 1);

              active_layer = (GimpLayer *)
                gimp_container_get_child_by_index (image->layers, index);
            }
          else
            {
              active_layer = NULL;
            }
        }

      gimp_image_set_active_layer (image, active_layer);
    }

  gimp_item_removed (GIMP_ITEM (layer));

  g_object_unref (layer);

  if (old_has_alpha != gimp_image_has_alpha (image))
    gimp_image_alpha_changed (image);

  if (undo_group)
    gimp_image_undo_group_end (image);
}

void
gimp_image_add_layers (GimpImage   *image,
                       GList       *layers,
                       gint         position,
                       gint         x,
                       gint         y,
                       gint         width,
                       gint         height,
                       const gchar *undo_desc)
{
  GList *list;
  gint   layers_x      = G_MAXINT;
  gint   layers_y      = G_MAXINT;
  gint   layers_width  = 0;
  gint   layers_height = 0;
  gint   offset_x;
  gint   offset_y;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (layers != NULL);

  if (position == -1)
    {
      GimpLayer *active_layer = gimp_image_get_active_layer (image);

      if (active_layer)
        position = gimp_image_get_layer_index (image, active_layer);
      else
        position = 0;
    }

  for (list = layers; list; list = g_list_next (list))
    {
      GimpItem *item = GIMP_ITEM (list->data);
      gint      off_x, off_y;

      gimp_item_offsets (item, &off_x, &off_y);

      layers_x = MIN (layers_x, off_x);
      layers_y = MIN (layers_y, off_y);

      layers_width  = MAX (layers_width,
                           off_x + gimp_item_width (item)  - layers_x);
      layers_height = MAX (layers_height,
                           off_y + gimp_item_height (item) - layers_y);
    }

  offset_x = x + (width  - layers_width)  / 2 - layers_x;
  offset_y = y + (height - layers_height) / 2 - layers_y;

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_LAYER_ADD, undo_desc);

  for (list = layers; list; list = g_list_next (list))
    {
      GimpItem *new_item = GIMP_ITEM (list->data);

      gimp_item_translate (new_item, offset_x, offset_y, FALSE);

      gimp_image_add_layer (image, GIMP_LAYER (new_item), position);
      position++;
    }

  gimp_image_undo_group_end (image);
}

gboolean
gimp_image_raise_layer (GimpImage *image,
                        GimpLayer *layer)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  index = gimp_container_get_child_index (image->layers,
                                          GIMP_OBJECT (layer));

  if (index == 0)
    {
      g_message (_("Layer cannot be raised higher."));
      return FALSE;
    }

  return gimp_image_position_layer (image, layer, index - 1,
                                    TRUE, _("Raise Layer"));
}

gboolean
gimp_image_lower_layer (GimpImage *image,
                        GimpLayer *layer)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  index = gimp_container_get_child_index (image->layers,
                                          GIMP_OBJECT (layer));

  if (index == gimp_container_num_children (image->layers) - 1)
    {
      g_message (_("Layer cannot be lowered more."));
      return FALSE;
    }

  return gimp_image_position_layer (image, layer, index + 1,
                                    TRUE, _("Lower Layer"));
}

gboolean
gimp_image_raise_layer_to_top (GimpImage *image,
                               GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  return gimp_image_position_layer (image, layer, 0,
                                    TRUE, _("Raise Layer to Top"));
}

gboolean
gimp_image_lower_layer_to_bottom (GimpImage *image,
                                  GimpLayer *layer)
{
  gint length;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  length = gimp_container_num_children (image->layers);

  return gimp_image_position_layer (image, layer, length - 1,
                                    TRUE, _("Lower Layer to Bottom"));
}

gboolean
gimp_image_position_layer (GimpImage   *image,
                           GimpLayer   *layer,
                           gint         new_index,
                           gboolean     push_undo,
                           const gchar *undo_desc)
{
  gint  index;
  gint  num_layers;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  index = gimp_container_get_child_index (image->layers, GIMP_OBJECT (layer));
  if (index < 0)
    return FALSE;

  num_layers = gimp_container_num_children (image->layers);

  new_index = CLAMP (new_index, 0, num_layers - 1);

  if (new_index == index)
    return TRUE;

  if (push_undo)
    gimp_image_undo_push_layer_reposition (image, undo_desc, layer);

  gimp_container_reorder (image->layers, GIMP_OBJECT (layer), new_index);

  if (gimp_item_get_visible (GIMP_ITEM (layer)))
    {
      gint off_x, off_y;

      gimp_item_offsets (GIMP_ITEM (layer), &off_x, &off_y);

      gimp_image_update (image,
                         off_x, off_y,
                         gimp_item_width  (GIMP_ITEM (layer)),
                         gimp_item_height (GIMP_ITEM (layer)));
    }

  return TRUE;
}

gboolean
gimp_image_add_channel (GimpImage   *image,
                        GimpChannel *channel,
                        gint         position)
{
  GimpChannel *active_channel;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  if (GIMP_ITEM (channel)->image != NULL &&
      GIMP_ITEM (channel)->image != image)
    {
      g_warning ("%s: attempting to add channel to wrong image.", G_STRFUNC);
      return FALSE;
    }

  if (gimp_container_have (image->channels, GIMP_OBJECT (channel)))
    {
      g_warning ("%s: trying to add channel to image twice.", G_STRFUNC);
      return FALSE;
    }

  active_channel = gimp_image_get_active_channel (image);

  gimp_image_undo_push_channel_add (image, _("Add Channel"),
                                    channel, active_channel);

  gimp_item_set_image (GIMP_ITEM (channel), image);

  /*  add the layer to the list at the specified position  */
  if (position == -1)
    {
      if (active_channel)
        position = gimp_container_get_child_index (image->channels,
                                                   GIMP_OBJECT (active_channel));
      else
        position = 0;
    }

  /*  Don't add at a non-existing index  */
  if (position > gimp_container_num_children (image->channels))
    position = gimp_container_num_children (image->channels);

  g_object_ref_sink (channel);
  gimp_container_insert (image->channels, GIMP_OBJECT (channel), position);
  g_object_unref (channel);

  /*  notify this image of the currently active channel  */
  gimp_image_set_active_channel (image, channel);

  return TRUE;
}

void
gimp_image_remove_channel (GimpImage   *image,
                           GimpChannel *channel)
{
  GimpChannel *active_channel;
  gint         index;
  gboolean     undo_group = FALSE;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (gimp_container_have (image->channels,
                                         GIMP_OBJECT (channel)));

  if (gimp_drawable_has_floating_sel (GIMP_DRAWABLE (channel)))
    {
      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                   _("Remove Channel"));
      undo_group = TRUE;

      floating_sel_remove (gimp_image_floating_sel (image));
    }

  active_channel = gimp_image_get_active_channel (image);

  index = gimp_container_get_child_index (image->channels,
                                          GIMP_OBJECT (channel));

  gimp_image_undo_push_channel_remove (image, _("Remove Channel"),
                                       channel, index, active_channel);

  g_object_ref (channel);

  gimp_container_remove (image->channels, GIMP_OBJECT (channel));
  gimp_item_removed (GIMP_ITEM (channel));

  if (channel == active_channel)
    {
      gint n_children = gimp_container_num_children (image->channels);

      if (n_children > 0)
        {
          index = CLAMP (index, 0, n_children - 1);

          active_channel = (GimpChannel *)
            gimp_container_get_child_by_index (image->channels, index);

          gimp_image_set_active_channel (image, active_channel);
        }
      else
        {
          gimp_image_unset_active_channel (image);
        }
    }

  g_object_unref (channel);

  if (undo_group)
    gimp_image_undo_group_end (image);
}

gboolean
gimp_image_raise_channel (GimpImage   *image,
                          GimpChannel *channel)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  index = gimp_container_get_child_index (image->channels,
                                          GIMP_OBJECT (channel));

  if (index == 0)
    {
      g_message (_("Channel cannot be raised higher."));
      return FALSE;
    }

  return gimp_image_position_channel (image, channel, index - 1,
                                      TRUE, _("Raise Channel"));
}

gboolean
gimp_image_raise_channel_to_top (GimpImage   *image,
                                 GimpChannel *channel)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  index = gimp_container_get_child_index (image->channels,
                                          GIMP_OBJECT (channel));

  if (index == 0)
    {
      g_message (_("Channel is already on top."));
      return FALSE;
    }

  return gimp_image_position_channel (image, channel, 0,
                                      TRUE, _("Raise Channel to Top"));
}

gboolean
gimp_image_lower_channel (GimpImage   *image,
                          GimpChannel *channel)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  index = gimp_container_get_child_index (image->channels,
                                          GIMP_OBJECT (channel));

  if (index == gimp_container_num_children (image->channels) - 1)
    {
      g_message (_("Channel cannot be lowered more."));
      return FALSE;
    }

  return gimp_image_position_channel (image, channel, index + 1,
                                      TRUE, _("Lower Channel"));
}

gboolean
gimp_image_lower_channel_to_bottom (GimpImage   *image,
                                    GimpChannel *channel)
{
  gint index;
  gint length;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  index = gimp_container_get_child_index (image->channels,
                                          GIMP_OBJECT (channel));

  length = gimp_container_num_children (image->channels);

  if (index == length - 1)
    {
      g_message (_("Channel is already on the bottom."));
      return FALSE;
    }

  return gimp_image_position_channel (image, channel, length - 1,
                                      TRUE, _("Lower Channel to Bottom"));
}

gboolean
gimp_image_position_channel (GimpImage   *image,
                             GimpChannel *channel,
                             gint         new_index,
                             gboolean     push_undo,
                             const gchar *undo_desc)
{
  gint index;
  gint num_channels;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  index = gimp_container_get_child_index (image->channels,
                                          GIMP_OBJECT (channel));
  if (index < 0)
    return FALSE;

  num_channels = gimp_container_num_children (image->channels);

  new_index = CLAMP (new_index, 0, num_channels - 1);

  if (new_index == index)
    return TRUE;

  if (push_undo)
    gimp_image_undo_push_channel_reposition (image, undo_desc, channel);

  gimp_container_reorder (image->channels,
                          GIMP_OBJECT (channel), new_index);

  if (gimp_item_get_visible (GIMP_ITEM (channel)))
    {
      gint off_x, off_y;

      gimp_item_offsets (GIMP_ITEM (channel), &off_x, &off_y);

      gimp_image_update (image,
                         off_x, off_y,
                         gimp_item_width  (GIMP_ITEM (channel)),
                         gimp_item_height (GIMP_ITEM (channel)));
    }

  return TRUE;
}

gboolean
gimp_image_add_vectors (GimpImage   *image,
                        GimpVectors *vectors,
                        gint         position)
{
  GimpVectors *active_vectors;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);

  if (GIMP_ITEM (vectors)->image != NULL &&
      GIMP_ITEM (vectors)->image != image)
    {
      g_warning ("%s: attempting to add vectors to wrong image.", G_STRFUNC);
      return FALSE;
    }

  if (gimp_container_have (image->vectors, GIMP_OBJECT (vectors)))
    {
      g_warning ("%s: trying to add vectors to image twice.", G_STRFUNC);
      return FALSE;
    }

  active_vectors = gimp_image_get_active_vectors (image);

  gimp_image_undo_push_vectors_add (image, _("Add Path"),
                                    vectors, active_vectors);

  gimp_item_set_image (GIMP_ITEM (vectors), image);

  /*  add the layer to the list at the specified position  */
  if (position == -1)
    {
      if (active_vectors)
        position = gimp_container_get_child_index (image->vectors,
                                                   GIMP_OBJECT (active_vectors));
      else
        position = 0;
    }

  /*  Don't add at a non-existing index  */
  if (position > gimp_container_num_children (image->vectors))
    position = gimp_container_num_children (image->vectors);

  g_object_ref_sink (vectors);
  gimp_container_insert (image->vectors, GIMP_OBJECT (vectors), position);
  g_object_unref (vectors);

  /*  notify this image of the currently active vectors  */
  gimp_image_set_active_vectors (image, vectors);

  return TRUE;
}

void
gimp_image_remove_vectors (GimpImage   *image,
                           GimpVectors *vectors)
{
  GimpVectors *active_vectors;
  gint         index;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_VECTORS (vectors));
  g_return_if_fail (gimp_container_have (image->vectors,
                                         GIMP_OBJECT (vectors)));

  active_vectors = gimp_image_get_active_vectors (image);

  index = gimp_container_get_child_index (image->vectors,
                                          GIMP_OBJECT (vectors));

  gimp_image_undo_push_vectors_remove (image, _("Remove Path"),
                                       vectors, index, active_vectors);

  g_object_ref (vectors);

  gimp_container_remove (image->vectors, GIMP_OBJECT (vectors));
  gimp_item_removed (GIMP_ITEM (vectors));

  if (vectors == active_vectors)
    {
      gint n_children = gimp_container_num_children (image->vectors);

      if (n_children > 0)
        {
          index = CLAMP (index, 0, n_children - 1);

          active_vectors = (GimpVectors *)
            gimp_container_get_child_by_index (image->vectors, index);
        }
      else
        {
          active_vectors = NULL;
        }

      gimp_image_set_active_vectors (image, active_vectors);
    }

  g_object_unref (vectors);
}

gboolean
gimp_image_raise_vectors (GimpImage   *image,
                          GimpVectors *vectors)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);

  index = gimp_container_get_child_index (image->vectors,
                                          GIMP_OBJECT (vectors));

  if (index == 0)
    {
      g_message (_("Path cannot be raised higher."));
      return FALSE;
    }

  return gimp_image_position_vectors (image, vectors, index - 1,
                                      TRUE, _("Raise Path"));
}

gboolean
gimp_image_raise_vectors_to_top (GimpImage   *image,
                                 GimpVectors *vectors)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);

  index = gimp_container_get_child_index (image->vectors,
                                          GIMP_OBJECT (vectors));

  if (index == 0)
    {
      g_message (_("Path is already on top."));
      return FALSE;
    }

  return gimp_image_position_vectors (image, vectors, 0,
                                      TRUE, _("Raise Path to Top"));
}

gboolean
gimp_image_lower_vectors (GimpImage   *image,
                          GimpVectors *vectors)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);

  index = gimp_container_get_child_index (image->vectors,
                                          GIMP_OBJECT (vectors));

  if (index == gimp_container_num_children (image->vectors) - 1)
    {
      g_message (_("Path cannot be lowered more."));
      return FALSE;
    }

  return gimp_image_position_vectors (image, vectors, index + 1,
                                      TRUE, _("Lower Path"));
}

gboolean
gimp_image_lower_vectors_to_bottom (GimpImage   *image,
                                    GimpVectors *vectors)
{
  gint index;
  gint length;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);

  index = gimp_container_get_child_index (image->vectors,
                                          GIMP_OBJECT (vectors));

  length = gimp_container_num_children (image->vectors);

  if (index == length - 1)
    {
      g_message (_("Path is already on the bottom."));
      return FALSE;
    }

  return gimp_image_position_vectors (image, vectors, length - 1,
                                      TRUE, _("Lower Path to Bottom"));
}

gboolean
gimp_image_position_vectors (GimpImage   *image,
                             GimpVectors *vectors,
                             gint         new_index,
                             gboolean     push_undo,
                             const gchar *undo_desc)
{
  gint index;
  gint num_vectors;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);

  index = gimp_container_get_child_index (image->vectors,
                                          GIMP_OBJECT (vectors));
  if (index < 0)
    return FALSE;

  num_vectors = gimp_container_num_children (image->vectors);

  new_index = CLAMP (new_index, 0, num_vectors - 1);

  if (new_index == index)
    return TRUE;

  if (push_undo)
    gimp_image_undo_push_vectors_reposition (image, undo_desc, vectors);

  gimp_container_reorder (image->vectors,
                          GIMP_OBJECT (vectors), new_index);

  return TRUE;
}

gboolean
gimp_image_layer_boundary (const GimpImage  *image,
                           BoundSeg        **segs,
                           gint             *n_segs)
{
  GimpLayer *layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (segs != NULL, FALSE);
  g_return_val_if_fail (n_segs != NULL, FALSE);

  /*  The second boundary corresponds to the active layer's
   *  perimeter...
   */
  layer = gimp_image_get_active_layer (image);

  if (layer)
    {
      *segs = gimp_layer_boundary (layer, n_segs);
      return TRUE;
    }
  else
    {
      *segs = NULL;
      *n_segs = 0;
      return FALSE;
    }
}

GimpLayer *
gimp_image_pick_correlate_layer (const GimpImage *image,
                                 gint             x,
                                 gint             y)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  for (list = GIMP_LIST (image->layers)->list;
       list;
       list = g_list_next (list))
    {
      GimpLayer *layer = list->data;
      gint       off_x, off_y;

      gimp_item_offsets (GIMP_ITEM (layer), &off_x, &off_y);

      if (gimp_pickable_get_opacity_at (GIMP_PICKABLE (layer),
                                        x - off_x, y - off_y) > 63)
        {
          return layer;
        }
    }

  return NULL;
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
      if (x >= 0 && x < image->width &&
          y >= 0 && y < image->height)
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

          gimp_item_offsets (item, &off_x, &off_y);

          d_x = x - off_x;
          d_y = y - off_y;

          if (d_x >= 0 && d_x < gimp_item_width (item) &&
              d_y >= 0 && d_y < gimp_item_height (item))
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
gimp_image_invalidate_layer_previews (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  gimp_container_foreach (image->layers,
                          (GFunc) gimp_viewable_invalidate_preview,
                          NULL);
}

void
gimp_image_invalidate_channel_previews (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  gimp_container_foreach (image->channels,
                          (GFunc) gimp_viewable_invalidate_preview,
                          NULL);
}

const guint8 *
gimp_image_get_icc_profile (GimpColorManaged *managed,
                            gsize            *len)
{
  const GimpParasite *parasite;

  parasite = gimp_image_parasite_find (GIMP_IMAGE (managed), "icc-profile");

  if (parasite)
    {
      *len = gimp_parasite_data_size (parasite);

      return gimp_parasite_data (parasite);
    }

  return NULL;
}
