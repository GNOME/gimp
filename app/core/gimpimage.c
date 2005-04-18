/* The GIMP -- an image manipulation program
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

#include "core-types.h"

#include "base/temp-buf.h"
#include "base/tile-manager.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-utils.h"
#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimp-parasites.h"
#include "gimp-utils.h"
#include "gimpcontext.h"
#include "gimpgrid.h"
#include "gimpimage.h"
#include "gimpimage-colorhash.h"
#include "gimpimage-colormap.h"
#include "gimpimage-guides.h"
#include "gimpimage-preview.h"
#include "gimpimage-qmask.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplayermask.h"
#include "gimplist.h"
#include "gimpmarshal.h"
#include "gimpparasitelist.h"
#include "gimpprojection.h"
#include "gimpselection.h"
#include "gimptemplate.h"
#include "gimpundostack.h"

#include "file/file-utils.h"

#include "vectors/gimpvectors.h"

#include "gimp-intl.h"


#ifdef DEBUG
#define TRC(x) printf x
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
  QMASK_CHANGED,
  SELECTION_CONTROL,
  CLEAN,
  DIRTY,
  UPDATE,
  UPDATE_GUIDE,
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

static void     gimp_image_class_init            (GimpImageClass *klass);
static void     gimp_image_init                  (GimpImage      *gimage);

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

static void     gimp_image_invalidate_preview    (GimpViewable   *viewable);
static void     gimp_image_size_changed          (GimpViewable   *viewable);
static gchar  * gimp_image_get_description       (GimpViewable   *viewable,
                                                  gchar         **tooltip);
static void     gimp_image_real_colormap_changed (GimpImage      *gimage,
                                                  gint            color_index);
static void     gimp_image_real_flush            (GimpImage      *gimage);

static void     gimp_image_mask_update           (GimpDrawable   *drawable,
                                                  gint            x,
                                                  gint            y,
                                                  gint            width,
                                                  gint            height,
                                                  GimpImage      *gimage);
static void     gimp_image_drawable_update       (GimpDrawable   *drawable,
                                                  gint            x,
                                                  gint            y,
                                                  gint            width,
                                                  gint            height,
                                                  GimpImage      *gimage);
static void     gimp_image_drawable_visibility   (GimpItem       *item,
                                                  GimpImage      *gimage);
static void     gimp_image_layer_alpha_changed   (GimpDrawable   *drawable,
                                                  GimpImage      *gimage);
static void     gimp_image_layer_add             (GimpContainer  *container,
                                                  GimpLayer      *layer,
                                                  GimpImage      *gimage);
static void     gimp_image_layer_remove          (GimpContainer  *container,
                                                  GimpLayer      *layer,
                                                  GimpImage      *gimage);
static void     gimp_image_channel_add           (GimpContainer  *container,
                                                  GimpChannel    *channel,
                                                  GimpImage      *gimage);
static void     gimp_image_channel_remove        (GimpContainer  *container,
                                                  GimpChannel    *channel,
                                                  GimpImage      *gimage);
static void     gimp_image_channel_name_changed  (GimpChannel    *channel,
                                                  GimpImage      *gimage);
static void     gimp_image_channel_color_changed (GimpChannel    *channel,
                                                  GimpImage      *gimage);


static gint valid_combinations[][MAX_CHANNELS + 1] =
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

static guint gimp_image_signals[LAST_SIGNAL] = { 0 };

static GimpViewableClass *parent_class = NULL;


GType
gimp_image_get_type (void)
{
  static GType image_type = 0;

  if (! image_type)
    {
      static const GTypeInfo image_info =
      {
        sizeof (GimpImageClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_image_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpImage),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_image_init,
      };

      image_type = g_type_register_static (GIMP_TYPE_VIEWABLE,
                                           "GimpImage",
                                           &image_info, 0);
    }

  return image_type;
}


/*  private functions  */

static void
gimp_image_class_init (GimpImageClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gimp_image_signals[MODE_CHANGED] =
    g_signal_new ("mode_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, mode_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[ALPHA_CHANGED] =
    g_signal_new ("alpha_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, alpha_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[FLOATING_SELECTION_CHANGED] =
    g_signal_new ("floating_selection_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, floating_selection_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[ACTIVE_LAYER_CHANGED] =
    g_signal_new ("active_layer_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, active_layer_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[ACTIVE_CHANNEL_CHANGED] =
    g_signal_new ("active_channel_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, active_channel_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[ACTIVE_VECTORS_CHANGED] =
    g_signal_new ("active_vectors_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, active_vectors_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[COMPONENT_VISIBILITY_CHANGED] =
    g_signal_new ("component_visibility_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, component_visibility_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_CHANNEL_TYPE);

  gimp_image_signals[COMPONENT_ACTIVE_CHANGED] =
    g_signal_new ("component_active_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, component_active_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_CHANNEL_TYPE);

  gimp_image_signals[MASK_CHANGED] =
    g_signal_new ("mask_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, mask_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[RESOLUTION_CHANGED] =
    g_signal_new ("resolution_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, resolution_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[UNIT_CHANGED] =
    g_signal_new ("unit_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, unit_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[QMASK_CHANGED] =
    g_signal_new ("qmask_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, qmask_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_image_signals[SELECTION_CONTROL] =
    g_signal_new ("selection_control",
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
    g_signal_new ("update_guide",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, update_guide),
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  gimp_image_signals[COLORMAP_CHANGED] =
    g_signal_new ("colormap_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, colormap_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__INT,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  gimp_image_signals[UNDO_EVENT] =
    g_signal_new ("undo_event",
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
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->constructor           = gimp_image_constructor;
  object_class->set_property          = gimp_image_set_property;
  object_class->get_property          = gimp_image_get_property;
  object_class->dispose               = gimp_image_dispose;
  object_class->finalize              = gimp_image_finalize;

  gimp_object_class->name_changed     = gimp_image_name_changed;
  gimp_object_class->get_memsize      = gimp_image_get_memsize;

  viewable_class->default_stock_id    = "gimp-image";
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
  klass->update                       = NULL;
  klass->update_guide                 = NULL;
  klass->colormap_changed             = gimp_image_real_colormap_changed;
  klass->undo_event                   = NULL;
  klass->flush                        = gimp_image_real_flush;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp", NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_int ("id", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_WIDTH,
                                   g_param_spec_int ("width", NULL, NULL,
                                                     1, GIMP_MAX_IMAGE_SIZE, 1,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", NULL, NULL,
                                                     1, GIMP_MAX_IMAGE_SIZE, 1,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_BASE_TYPE,
                                   g_param_spec_enum ("base-type", NULL, NULL,
                                                      GIMP_TYPE_IMAGE_BASE_TYPE,
                                                      GIMP_RGB,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  gimp_image_color_hash_init ();
}

static void
gimp_image_init (GimpImage *gimage)
{
  gint i;

  gimage->ID                    = 0;

  gimage->save_proc             = NULL;

  gimage->width                 = 0;
  gimage->height                = 0;
  gimage->xresolution           = 1.0;
  gimage->yresolution           = 1.0;
  gimage->resolution_unit       = GIMP_UNIT_INCH;
  gimage->base_type             = GIMP_RGB;

  gimage->cmap                  = NULL;
  gimage->num_cols              = 0;

  gimage->dirty                 = 1;
  gimage->dirty_time            = 0;
  gimage->undo_freeze_count     = 0;

  gimage->instance_count        = 0;
  gimage->disp_count            = 0;

  gimage->tattoo_state          = 0;

  gimage->shadow                = NULL;

  gimage->projection            = gimp_projection_new (gimage);

  gimage->guides                = NULL;

  gimage->grid                  = NULL;

  gimage->layers                = gimp_list_new (GIMP_TYPE_LAYER,   TRUE);
  gimage->channels              = gimp_list_new (GIMP_TYPE_CHANNEL, TRUE);
  gimage->vectors               = gimp_list_new (GIMP_TYPE_VECTORS, TRUE);
  gimage->layer_stack           = NULL;

  gimage->layer_update_handler =
    gimp_container_add_handler (gimage->layers, "update",
                                G_CALLBACK (gimp_image_drawable_update),
                                gimage);
  gimage->layer_visible_handler =
    gimp_container_add_handler (gimage->layers, "visibility_changed",
                                G_CALLBACK (gimp_image_drawable_visibility),
                                gimage);
  gimage->layer_alpha_handler =
    gimp_container_add_handler (gimage->layers, "alpha_changed",
                                G_CALLBACK (gimp_image_layer_alpha_changed),
                                gimage);

  gimage->channel_update_handler =
    gimp_container_add_handler (gimage->channels, "update",
                                G_CALLBACK (gimp_image_drawable_update),
                                gimage);
  gimage->channel_visible_handler =
    gimp_container_add_handler (gimage->channels, "visibility_changed",
                                G_CALLBACK (gimp_image_drawable_visibility),
                                gimage);
  gimage->channel_name_changed_handler =
    gimp_container_add_handler (gimage->channels, "name_changed",
                                G_CALLBACK (gimp_image_channel_name_changed),
                                gimage);
  gimage->channel_color_changed_handler =
    gimp_container_add_handler (gimage->channels, "color_changed",
                                G_CALLBACK (gimp_image_channel_color_changed),
                                gimage);

  g_signal_connect (gimage->layers, "add",
                    G_CALLBACK (gimp_image_layer_add),
                    gimage);
  g_signal_connect (gimage->layers, "remove",
                    G_CALLBACK (gimp_image_layer_remove),
                    gimage);

  g_signal_connect (gimage->channels, "add",
                    G_CALLBACK (gimp_image_channel_add),
                    gimage);
  g_signal_connect (gimage->channels, "remove",
                    G_CALLBACK (gimp_image_channel_remove),
                    gimage);

  gimage->active_layer          = NULL;
  gimage->active_channel        = NULL;
  gimage->active_vectors        = NULL;

  gimage->floating_sel          = NULL;
  gimage->selection_mask        = NULL;

  gimage->parasites             = gimp_parasite_list_new ();

  for (i = 0; i < MAX_CHANNELS; i++)
    {
      gimage->visible[i] = TRUE;
      gimage->active[i]  = TRUE;
    }

  gimage->qmask_state           = FALSE;
  gimage->qmask_inverted        = FALSE;
  gimp_rgba_set (&gimage->qmask_color, 1.0, 0.0, 0.0, 0.5);

  gimage->undo_stack            = gimp_undo_stack_new (gimage);
  gimage->redo_stack            = gimp_undo_stack_new (gimage);
  gimage->group_count           = 0;
  gimage->pushing_undo_group    = GIMP_UNDO_GROUP_NONE;

  gimage->comp_preview          = NULL;
  gimage->comp_preview_valid    = FALSE;

  gimage->flush_accum.alpha_changed = FALSE;
  gimage->flush_accum.mask_changed  = FALSE;
}

static GObject *
gimp_image_constructor (GType                  type,
                        guint                  n_params,
                        GObjectConstructParam *params)
{
  GObject        *object;
  GimpImage      *gimage;
  GimpCoreConfig *config;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  gimage = GIMP_IMAGE (object);

  g_assert (GIMP_IS_GIMP (gimage->gimp));

  config = gimage->gimp->config;

  gimage->ID = gimage->gimp->next_image_ID++;

  g_hash_table_insert (gimage->gimp->image_table,
                       GINT_TO_POINTER (gimage->ID),
                       gimage);

  gimage->xresolution     = config->default_image->xresolution;
  gimage->yresolution     = config->default_image->yresolution;
  gimage->resolution_unit = config->default_image->resolution_unit;

  gimage->grid = gimp_config_duplicate (GIMP_CONFIG (config->default_grid));

  switch (gimage->base_type)
    {
    case GIMP_RGB:
    case GIMP_GRAY:
      break;
    case GIMP_INDEXED:
      /* always allocate 256 colors for the colormap */
      gimage->num_cols = 0;
      gimage->cmap     = g_new0 (guchar, GIMP_IMAGE_COLORMAP_SIZE);
      break;
    default:
      break;
    }

  /* create the selection mask */
  gimage->selection_mask = gimp_selection_new (gimage,
                                               gimage->width,
                                               gimage->height);
  g_object_ref (gimage->selection_mask);
  gimp_item_sink (GIMP_ITEM (gimage->selection_mask));

  g_signal_connect (gimage->selection_mask, "update",
                    G_CALLBACK (gimp_image_mask_update),
                    gimage);

  g_signal_connect_object (config, "notify::transparency-type",
                           G_CALLBACK (gimp_image_invalidate_layer_previews),
                           gimage, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::transparency-size",
                           G_CALLBACK (gimp_image_invalidate_layer_previews),
                           gimage, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::layer-previews",
                           G_CALLBACK (gimp_viewable_size_changed),
                           gimage, G_CONNECT_SWAPPED);

  return object;
}

static void
gimp_image_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  GimpImage *gimage = GIMP_IMAGE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      gimage->gimp = g_value_get_object (value);
      break;
    case PROP_ID:
      g_assert_not_reached ();
      break;
    case PROP_WIDTH:
      gimage->width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      gimage->height = g_value_get_int (value);
      break;
    case PROP_BASE_TYPE:
      gimage->base_type = g_value_get_enum (value);
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
  GimpImage *gimage = GIMP_IMAGE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, gimage->gimp);
      break;
    case PROP_ID:
      g_value_set_int (value, gimage->ID);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, gimage->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, gimage->height);
      break;
    case PROP_BASE_TYPE:
      g_value_set_enum (value, gimage->base_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_image_dispose (GObject *object)
{
  GimpImage *gimage = GIMP_IMAGE (object);

  gimp_image_undo_free (gimage);

  gimp_container_remove_handler (gimage->layers,
                                 gimage->layer_update_handler);
  gimp_container_remove_handler (gimage->layers,
                                 gimage->layer_visible_handler);
  gimp_container_remove_handler (gimage->layers,
                                 gimage->layer_alpha_handler);

  gimp_container_remove_handler (gimage->channels,
                                 gimage->channel_update_handler);
  gimp_container_remove_handler (gimage->channels,
                                 gimage->channel_visible_handler);
  gimp_container_remove_handler (gimage->channels,
                                 gimage->channel_name_changed_handler);
  gimp_container_remove_handler (gimage->channels,
                                 gimage->channel_color_changed_handler);

  g_signal_handlers_disconnect_by_func (gimage->layers,
                                        gimp_image_layer_add,
                                        gimage);
  g_signal_handlers_disconnect_by_func (gimage->layers,
                                        gimp_image_layer_remove,
                                        gimage);

  g_signal_handlers_disconnect_by_func (gimage->channels,
                                        gimp_image_channel_add,
                                        gimage);
  g_signal_handlers_disconnect_by_func (gimage->channels,
                                        gimp_image_channel_remove,
                                        gimage);

  gimp_container_foreach (gimage->layers,   (GFunc) gimp_item_removed, NULL);
  gimp_container_foreach (gimage->channels, (GFunc) gimp_item_removed, NULL);
  gimp_container_foreach (gimage->vectors,  (GFunc) gimp_item_removed, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_image_finalize (GObject *object)
{
  GimpImage *gimage = GIMP_IMAGE (object);

  if (gimage->projection)
    {
      g_object_unref (gimage->projection);
      gimage->projection = NULL;
    }

  if (gimage->shadow)
    gimp_image_free_shadow (gimage);

  if (gimage->cmap)
    {
      g_free (gimage->cmap);
      gimage->cmap = NULL;
    }

  if (gimage->layers)
    {
      g_object_unref (gimage->layers);
      gimage->layers = NULL;
    }
  if (gimage->channels)
    {
      g_object_unref (gimage->channels);
      gimage->channels = NULL;
    }
  if (gimage->vectors)
    {
      g_object_unref (gimage->vectors);
      gimage->vectors = NULL;
    }
  if (gimage->layer_stack)
    {
      g_slist_free (gimage->layer_stack);
      gimage->layer_stack = NULL;
    }

  if (gimage->selection_mask)
    {
      g_object_unref (gimage->selection_mask);
      gimage->selection_mask = NULL;
    }

  if (gimage->comp_preview)
    {
      temp_buf_free (gimage->comp_preview);
      gimage->comp_preview = NULL;
    }

  if (gimage->parasites)
    {
      g_object_unref (gimage->parasites);
      gimage->parasites = NULL;
    }

  if (gimage->guides)
    {
      g_list_foreach (gimage->guides, (GFunc) gimp_image_guide_unref, NULL);
      g_list_free (gimage->guides);
      gimage->guides = NULL;
    }

  if (gimage->grid)
    {
      g_object_unref (gimage->grid);
      gimage->grid = NULL;
    }

  if (gimage->undo_stack)
    {
      g_object_unref (gimage->undo_stack);
      gimage->undo_stack = NULL;
    }
  if (gimage->redo_stack)
    {
      g_object_unref (gimage->redo_stack);
      gimage->redo_stack = NULL;
    }

  if (gimage->gimp && gimage->gimp->image_table)
    {
      g_hash_table_remove (gimage->gimp->image_table,
                           GINT_TO_POINTER (gimage->ID));
      gimage->gimp = NULL;
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
  GimpImage *gimage  = GIMP_IMAGE (object);
  gint64     memsize = 0;

  if (gimage->cmap)
    memsize += GIMP_IMAGE_COLORMAP_SIZE;

  if (gimage->shadow)
    memsize += tile_manager_get_memsize (gimage->shadow, FALSE);

  if (gimage->projection)
    memsize += gimp_object_get_memsize (GIMP_OBJECT (gimage->projection),
                                        gui_size);

  memsize += gimp_g_list_get_memsize (gimage->guides, sizeof (GimpGuide));

  if (gimage->grid)
    memsize += gimp_object_get_memsize (GIMP_OBJECT (gimage->grid), gui_size);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimage->layers),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimage->channels),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimage->vectors),
                                      gui_size);

  memsize += gimp_g_slist_get_memsize (gimage->layer_stack, 0);

  if (gimage->selection_mask)
    memsize += gimp_object_get_memsize (GIMP_OBJECT (gimage->selection_mask),
                                        gui_size);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimage->parasites),
                                      gui_size);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimage->undo_stack),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimage->redo_stack),
                                      gui_size);

  if (gimage->comp_preview)
    *gui_size += temp_buf_get_memsize (gimage->comp_preview);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_image_invalidate_preview (GimpViewable *viewable)
{
  GimpImage *gimage = GIMP_IMAGE (viewable);

  if (GIMP_VIEWABLE_CLASS (parent_class)->invalidate_preview)
    GIMP_VIEWABLE_CLASS (parent_class)->invalidate_preview (viewable);

  gimage->comp_preview_valid = FALSE;

  if (gimage->comp_preview)
    {
      temp_buf_free (gimage->comp_preview);
      gimage->comp_preview = NULL;
    }
}

static void
gimp_image_size_changed (GimpViewable *viewable)
{
  GimpImage *gimage = GIMP_IMAGE (viewable);
  GList     *list;

  if (GIMP_VIEWABLE_CLASS (parent_class)->size_changed)
    GIMP_VIEWABLE_CLASS (parent_class)->size_changed (viewable);

  gimp_container_foreach (gimage->layers,
                          (GFunc) gimp_viewable_size_changed,
                          NULL);
  gimp_container_foreach (gimage->channels,
                          (GFunc) gimp_viewable_size_changed,
                          NULL);
  gimp_container_foreach (gimage->vectors,
                          (GFunc) gimp_viewable_size_changed,
                          NULL);

  for (list = GIMP_LIST (gimage->layers)->list; list; list = g_list_next (list))
    {
      GimpLayerMask *mask = gimp_layer_get_mask (GIMP_LAYER (list->data));

      if (mask)
        gimp_viewable_size_changed (GIMP_VIEWABLE (mask));
    }

  gimp_viewable_size_changed (GIMP_VIEWABLE (gimp_image_get_mask (gimage)));
}

static gchar *
gimp_image_get_description (GimpViewable  *viewable,
                            gchar        **tooltip)
{
  GimpImage   *gimage = GIMP_IMAGE (viewable);
  const gchar *uri;
  gchar       *basename;
  gchar       *retval;

  uri = gimp_image_get_uri (GIMP_IMAGE (gimage));

  basename = file_utils_uri_to_utf8_basename (uri);

  if (tooltip)
    *tooltip = file_utils_uri_to_utf8_filename (uri);

  retval = g_strdup_printf ("%s-%d", basename, gimp_image_get_ID (gimage));

  g_free (basename);

  return retval;
}

static void
gimp_image_real_colormap_changed (GimpImage *gimage,
                                  gint       color_index)
{
  if (gimp_image_base_type (gimage) == GIMP_INDEXED)
    {
      gimp_image_color_hash_invalidate (gimage, color_index);

      /* A colormap alteration affects the whole image */
      gimp_image_update (gimage, 0, 0, gimage->width, gimage->height);

      gimp_image_invalidate_layer_previews (gimage);
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimage));
    }
}

void
gimp_image_real_flush (GimpImage *gimage)
{
  if (gimage->flush_accum.alpha_changed)
    {
      gimp_image_alpha_changed (gimage);
      gimage->flush_accum.alpha_changed = FALSE;
    }

  if (gimage->flush_accum.mask_changed)
    {
      gimp_image_mask_changed (gimage);
      gimage->flush_accum.mask_changed = FALSE;
    }
}

static void
gimp_image_mask_update (GimpDrawable *drawable,
                        gint          x,
                        gint          y,
                        gint          width,
                        gint          height,
                        GimpImage    *gimage)
{
  gimage->flush_accum.mask_changed = TRUE;
}

static void
gimp_image_drawable_update (GimpDrawable *drawable,
                            gint          x,
                            gint          y,
                            gint          width,
                            gint          height,
                            GimpImage    *gimage)
{
  GimpItem *item = GIMP_ITEM (drawable);

  if (gimp_item_get_visible (item))
    {
      gint offset_x;
      gint offset_y;

      gimp_item_offsets (item, &offset_x, &offset_y);
      x += offset_x;
      y += offset_y;

      gimp_image_update (gimage, x, y, width, height);
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimage));
    }
}

static void
gimp_image_drawable_visibility (GimpItem  *item,
                                GimpImage *gimage)
{
  gint offset_x;
  gint offset_y;

  gimp_item_offsets (item, &offset_x, &offset_y);

  gimp_image_update (gimage,
                     offset_x, offset_y,
                     gimp_item_width (item),
                     gimp_item_height (item));
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimage));
}

static void
gimp_image_layer_alpha_changed (GimpDrawable *drawable,
                                GimpImage    *gimage)
{
  if (gimp_container_num_children (gimage->layers) == 1)
    gimage->flush_accum.alpha_changed = TRUE;
}

static void
gimp_image_layer_add (GimpContainer *container,
                      GimpLayer     *layer,
                      GimpImage     *gimage)
{
  GimpItem *item = GIMP_ITEM (layer);

  if (gimp_item_get_visible (item))
    gimp_image_drawable_visibility (item, gimage);
}

static void
gimp_image_layer_remove (GimpContainer *container,
                         GimpLayer     *layer,
                         GimpImage     *gimage)
{
  GimpItem *item = GIMP_ITEM (layer);

  if (gimp_item_get_visible (item))
    gimp_image_drawable_visibility (item, gimage);
}

static void
gimp_image_channel_add (GimpContainer *container,
                        GimpChannel   *channel,
                        GimpImage     *gimage)
{
  GimpItem *item = GIMP_ITEM (channel);

  if (gimp_item_get_visible (item))
    gimp_image_drawable_visibility (item, gimage);

  if (! strcmp (GIMP_IMAGE_QMASK_NAME,
                gimp_object_get_name (GIMP_OBJECT (channel))))
    {
      gimp_image_set_qmask_state (gimage, TRUE);
    }
}

static void
gimp_image_channel_remove (GimpContainer *container,
                           GimpChannel   *channel,
                           GimpImage     *gimage)
{
  GimpItem *item = GIMP_ITEM (channel);

  if (gimp_item_get_visible (item))
    gimp_image_drawable_visibility (item, gimage);

  if (! strcmp (GIMP_IMAGE_QMASK_NAME,
                gimp_object_get_name (GIMP_OBJECT (channel))))
    {
      gimp_image_set_qmask_state (gimage, FALSE);
    }
}

static void
gimp_image_channel_name_changed (GimpChannel *channel,
                                 GimpImage   *gimage)
{
  if (! strcmp (GIMP_IMAGE_QMASK_NAME,
                gimp_object_get_name (GIMP_OBJECT (channel))))
    {
      gimp_image_set_qmask_state (gimage, TRUE);
    }
  else if (gimp_image_get_qmask_state (gimage) &&
           ! gimp_image_get_qmask (gimage))
    {
      gimp_image_set_qmask_state (gimage, FALSE);
    }
}

static void
gimp_image_channel_color_changed (GimpChannel *channel,
                                  GimpImage   *gimage)
{
  if (! strcmp (GIMP_IMAGE_QMASK_NAME,
                gimp_object_get_name (GIMP_OBJECT (channel))))
    {
      gimage->qmask_color = channel->color;
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
gimp_image_base_type (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), -1);

  return gimage->base_type;
}

GimpImageType
gimp_image_base_type_with_alpha (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), -1);

  switch (gimage->base_type)
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
gimp_image_get_ID (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), -1);

  return gimage->ID;
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
gimp_image_set_uri (GimpImage   *gimage,
                    const gchar *uri)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  gimp_object_set_name (GIMP_OBJECT (gimage), uri);
}

const gchar *
gimp_image_get_uri (const GimpImage *gimage)
{
  const gchar *uri;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  uri = gimp_object_get_name (GIMP_OBJECT (gimage));

  return uri ? uri : _("Untitled");
}

void
gimp_image_set_filename (GimpImage   *gimage,
                         const gchar *filename)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  if (filename && strlen (filename))
    {
      gchar *uri;

      uri = file_utils_filename_to_uri (gimage->gimp->load_procs, filename,
                                        NULL);

      gimp_image_set_uri (gimage, uri);

      g_free (uri);
    }
  else
    {
      gimp_image_set_uri (gimage, NULL);
    }
}

gchar *
gimp_image_get_filename (const GimpImage *gimage)
{
  const gchar *uri;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  uri = gimp_object_get_name (GIMP_OBJECT (gimage));

  if (! uri)
    return NULL;

  return g_filename_from_uri (uri, NULL, NULL);
}

void
gimp_image_set_save_proc (GimpImage     *gimage,
                          PlugInProcDef *proc)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  gimage->save_proc = proc;
}

PlugInProcDef *
gimp_image_get_save_proc (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimage->save_proc;
}

void
gimp_image_set_resolution (GimpImage *gimage,
                           gdouble    xresolution,
                           gdouble    yresolution)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  /* don't allow to set the resolution out of bounds */
  if (xresolution < GIMP_MIN_RESOLUTION || xresolution > GIMP_MAX_RESOLUTION ||
      yresolution < GIMP_MIN_RESOLUTION || yresolution > GIMP_MAX_RESOLUTION)
    return;

  if ((ABS (gimage->xresolution - xresolution) >= 1e-5) ||
      (ABS (gimage->yresolution - yresolution) >= 1e-5))
    {
      gimp_image_undo_push_image_resolution (gimage,
                                             _("Change Image Resolution"));

      gimage->xresolution = xresolution;
      gimage->yresolution = yresolution;

      gimp_image_resolution_changed (gimage);
      gimp_viewable_size_changed (GIMP_VIEWABLE (gimage));
    }
}

void
gimp_image_get_resolution (const GimpImage *gimage,
                           gdouble         *xresolution,
                           gdouble         *yresolution)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (xresolution && yresolution);

  *xresolution = gimage->xresolution;
  *yresolution = gimage->yresolution;
}

void
gimp_image_resolution_changed (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (gimage, gimp_image_signals[RESOLUTION_CHANGED], 0);
}

void
gimp_image_set_unit (GimpImage *gimage,
                     GimpUnit   unit)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (unit > GIMP_UNIT_PIXEL);

  if (gimage->resolution_unit != unit)
    {
      gimp_image_undo_push_image_resolution (gimage,
                                             _("Change Image Unit"));

      gimage->resolution_unit = unit;
      gimp_image_unit_changed (gimage);
    }
}

GimpUnit
gimp_image_get_unit (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), GIMP_UNIT_INCH);

  return gimage->resolution_unit;
}

void
gimp_image_unit_changed (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (gimage, gimp_image_signals[UNIT_CHANGED], 0);
}

gint
gimp_image_get_width (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), 0);

  return gimage->width;
}

gint
gimp_image_get_height (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), 0);

  return gimage->height;
}

gboolean
gimp_image_has_alpha (const GimpImage *gimage)
{
  GimpLayer *layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), TRUE);

  layer = (GimpLayer *) gimp_container_get_child_by_index (gimage->layers, 0);

  return ((gimp_container_num_children (gimage->layers) > 1) ||
          (layer && gimp_drawable_has_alpha (GIMP_DRAWABLE (layer))));
}

gboolean
gimp_image_is_empty (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), TRUE);

  return (gimp_container_num_children (gimage->layers) == 0);
}

GimpLayer *
gimp_image_floating_sel (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimage->floating_sel;
}

void
gimp_image_floating_selection_changed (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (gimage, gimp_image_signals[FLOATING_SELECTION_CHANGED], 0);
}

GimpChannel *
gimp_image_get_mask (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimage->selection_mask;
}

void
gimp_image_mask_changed (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (gimage, gimp_image_signals[MASK_CHANGED], 0);
}

gint
gimp_image_get_component_index (const GimpImage *gimage,
                                GimpChannelType  channel)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), -1);

  switch (channel)
    {
    case GIMP_RED_CHANNEL:     return RED_PIX;
    case GIMP_GREEN_CHANNEL:   return GREEN_PIX;
    case GIMP_BLUE_CHANNEL:    return BLUE_PIX;
    case GIMP_GRAY_CHANNEL:    return GRAY_PIX;
    case GIMP_INDEXED_CHANNEL: return INDEXED_PIX;
    case GIMP_ALPHA_CHANNEL:
      switch (gimp_image_base_type (gimage))
        {
        case GIMP_RGB:     return ALPHA_PIX;
        case GIMP_GRAY:    return ALPHA_G_PIX;
        case GIMP_INDEXED: return ALPHA_I_PIX;
        }
    }

  return -1;
}

void
gimp_image_set_component_active (GimpImage       *gimage,
                                 GimpChannelType  channel,
                                 gboolean         active)
{
  gint index = -1;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  index = gimp_image_get_component_index (gimage, channel);

  if (index != -1 && active != gimage->active[index])
    {
      gimage->active[index] = active ? TRUE : FALSE;

      /*  If there is an active channel and we mess with the components,
       *  the active channel gets unset...
       */
      gimp_image_unset_active_channel (gimage);

      g_signal_emit (gimage,
                     gimp_image_signals[COMPONENT_ACTIVE_CHANGED], 0,
                     channel);
    }
}

gboolean
gimp_image_get_component_active (const GimpImage *gimage,
                                 GimpChannelType  channel)
{
  gint index = -1;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  index = gimp_image_get_component_index (gimage, channel);

  if (index != -1)
    return gimage->active[index];

  return FALSE;
}

void
gimp_image_set_component_visible (GimpImage       *gimage,
                                  GimpChannelType  channel,
                                  gboolean         visible)
{
  gint index = -1;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  index = gimp_image_get_component_index (gimage, channel);

  if (index != -1 && visible != gimage->visible[index])
    {
      gimage->visible[index] = visible ? TRUE : FALSE;

      g_signal_emit (gimage,
                     gimp_image_signals[COMPONENT_VISIBILITY_CHANGED], 0,
                     channel);

      gimp_image_update (gimage, 0, 0, gimage->width, gimage->height);
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimage));
    }
}

gboolean
gimp_image_get_component_visible (const GimpImage *gimage,
                                  GimpChannelType  channel)
{
  gint index = -1;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  index = gimp_image_get_component_index (gimage, channel);

  if (index != -1)
    return gimage->visible[index];

  return FALSE;
}

void
gimp_image_mode_changed (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (gimage, gimp_image_signals[MODE_CHANGED], 0);
}

void
gimp_image_alpha_changed (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (gimage, gimp_image_signals[ALPHA_CHANGED], 0);
}

void
gimp_image_update (GimpImage *gimage,
                   gint       x,
                   gint       y,
                   gint       width,
                   gint       height)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (gimage, gimp_image_signals[UPDATE], 0,
                 x, y, width, height);
}

void
gimp_image_update_guide (GimpImage *gimage,
                         GimpGuide *guide)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (guide != NULL);

  g_signal_emit (gimage, gimp_image_signals[UPDATE_GUIDE], 0, guide);
}

void
gimp_image_colormap_changed (GimpImage *gimage,
                             gint       color_index)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (color_index >= -1 && color_index < gimage->num_cols);

  g_signal_emit (gimage, gimp_image_signals[COLORMAP_CHANGED], 0,
                 color_index);
}

void
gimp_image_selection_control (GimpImage            *gimage,
                              GimpSelectionControl  control)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (gimage, gimp_image_signals[SELECTION_CONTROL], 0, control);
}

void
gimp_image_qmask_changed (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (gimage, gimp_image_signals[QMASK_CHANGED], 0);
}


/*  undo  */

gboolean
gimp_image_undo_is_enabled (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  return (gimage->undo_freeze_count == 0);
}

gboolean
gimp_image_undo_enable (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  /*  Free all undo steps as they are now invalidated  */
  gimp_image_undo_free (gimage);

  return gimp_image_undo_thaw (gimage);
}

gboolean
gimp_image_undo_disable (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  return gimp_image_undo_freeze (gimage);
}

gboolean
gimp_image_undo_freeze (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  gimage->undo_freeze_count++;

  if (gimage->undo_freeze_count == 1)
    gimp_image_undo_event (gimage, GIMP_UNDO_EVENT_UNDO_FREEZE, NULL);

  return TRUE;
}

gboolean
gimp_image_undo_thaw (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (gimage->undo_freeze_count > 0, FALSE);

  gimage->undo_freeze_count--;

  if (gimage->undo_freeze_count == 0)
    gimp_image_undo_event (gimage, GIMP_UNDO_EVENT_UNDO_THAW, NULL);

  return TRUE;
}

void
gimp_image_undo_event (GimpImage     *gimage,
                       GimpUndoEvent  event,
                       GimpUndo      *undo)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (((event == GIMP_UNDO_EVENT_UNDO_FREE   ||
                      event == GIMP_UNDO_EVENT_UNDO_FREEZE ||
                      event == GIMP_UNDO_EVENT_UNDO_THAW) && undo == NULL) ||
                    GIMP_IS_UNDO (undo));

  g_signal_emit (gimage, gimp_image_signals[UNDO_EVENT], 0, event, undo);
}


/* NOTE about the gimage->dirty counter:
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
gimp_image_dirty (GimpImage     *gimage,
                  GimpDirtyMask  dirty_mask)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  gimage->dirty++;

  if (! gimage->dirty_time)
    gimage->dirty_time = time (NULL);

  g_signal_emit (gimage, gimp_image_signals[DIRTY], 0, dirty_mask);

  TRC (("dirty %d -> %d\n", gimage->dirty - 1, gimage->dirty));

  return gimage->dirty;
}

gint
gimp_image_clean (GimpImage     *gimage,
                  GimpDirtyMask  dirty_mask)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  gimage->dirty--;

  g_signal_emit (gimage, gimp_image_signals[CLEAN], 0, dirty_mask);

  TRC (("clean %d -> %d\n", gimage->dirty + 1, gimage->dirty));

  return gimage->dirty;
}

void
gimp_image_clean_all (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  gimage->dirty      = 0;
  gimage->dirty_time = 0;

  g_signal_emit (gimage, gimp_image_signals[CLEAN], 0);
}


/*  flush this image's displays  */

void
gimp_image_flush (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (gimage, gimp_image_signals[FLUSH], 0);
}


/*  color transforms / utilities  */

void
gimp_image_get_foreground (const GimpImage    *gimage,
                           const GimpDrawable *drawable,
                           GimpContext        *context,
                           guchar             *fg)
{
  GimpRGB  color;
  guchar   pfg[3];

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (! drawable || GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (fg != NULL);

  gimp_context_get_foreground (context, &color);

  gimp_rgb_get_uchar (&color, &pfg[0], &pfg[1], &pfg[2]);

  gimp_image_transform_color (gimage, drawable, fg, GIMP_RGB, pfg);
}

void
gimp_image_get_background (const GimpImage    *gimage,
                           const GimpDrawable *drawable,
                           GimpContext        *context,
                           guchar             *bg)
{
  GimpRGB  color;
  guchar   pbg[3];

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (! drawable || GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (bg != NULL);

  gimp_context_get_background (context, &color);

  gimp_rgb_get_uchar (&color, &pbg[0], &pbg[1], &pbg[2]);

  gimp_image_transform_color (gimage, drawable, bg, GIMP_RGB, pbg);
}

void
gimp_image_get_color (const GimpImage *src_gimage,
                      GimpImageType    src_type,
                      const guchar    *src,
                      guchar          *rgba)
{
  gboolean has_alpha = FALSE;

  g_return_if_fail (GIMP_IS_IMAGE (src_gimage));

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

        *rgba++ = src_gimage->cmap[index++];
        *rgba++ = src_gimage->cmap[index++];
        *rgba++ = src_gimage->cmap[index++];
      }
      break;
    }

  if (has_alpha)
    *rgba = *src;
  else
    *rgba = OPAQUE_OPACITY;
}

void
gimp_image_transform_rgb (const GimpImage    *dest_gimage,
                          const GimpDrawable *dest_drawable,
                          const GimpRGB      *rgb,
                          guchar             *color)
{
  guchar col[3];

  g_return_if_fail (GIMP_IS_IMAGE (dest_gimage));
  g_return_if_fail (! dest_drawable || GIMP_IS_DRAWABLE (dest_drawable));
  g_return_if_fail (rgb != NULL);
  g_return_if_fail (color != NULL);

  gimp_rgb_get_uchar (rgb, &col[0], &col[1], &col[2]);

  gimp_image_transform_color (dest_gimage, dest_drawable, color, GIMP_RGB, col);
}

void
gimp_image_transform_color (const GimpImage    *dest_gimage,
                            const GimpDrawable *dest_drawable,
                            guchar             *dest,
                            GimpImageBaseType   src_type,
                            const guchar       *src)
{
  GimpImageType dest_type;

  g_return_if_fail (GIMP_IS_IMAGE (dest_gimage));
  g_return_if_fail (src_type != GIMP_INDEXED);

  dest_type = (dest_drawable ?
               gimp_drawable_type (dest_drawable) :
               gimp_image_base_type_with_alpha (dest_gimage));

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
          *dest = GIMP_RGB_INTENSITY (src[RED_PIX],
                                      src[GREEN_PIX],
                                      src[BLUE_PIX]) + 0.5;
          break;

        case GIMP_INDEXED_IMAGE:
        case GIMP_INDEXEDA_IMAGE:
          /*  Least squares method  */
          *dest = gimp_image_color_hash_rgb_to_indexed (dest_gimage,
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
          *dest = gimp_image_color_hash_rgb_to_indexed (dest_gimage,
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
gimp_image_transform_temp_buf (const GimpImage    *dest_image,
                               const GimpDrawable *dest_drawable,
                               TempBuf            *temp_buf,
                               gboolean           *new_buf)
{
  TempBuf       *ret_buf;
  GimpImageType  ret_buf_type;
  gboolean       has_alpha;
  gboolean       is_rgb;
  gint           in_bytes;
  gint           out_bytes;

  g_return_val_if_fail (GIMP_IMAGE (dest_image), NULL);
  g_return_val_if_fail (GIMP_DRAWABLE (dest_drawable), NULL);
  g_return_val_if_fail (temp_buf != NULL, NULL);
  g_return_val_if_fail (new_buf != NULL, NULL);

  in_bytes  = temp_buf->bytes;

  has_alpha = (in_bytes == 2 || in_bytes == 4);
  is_rgb    = (in_bytes == 3 || in_bytes == 4);

  if (has_alpha)
    ret_buf_type = gimp_drawable_type_with_alpha (dest_drawable);
  else
    ret_buf_type = gimp_drawable_type_without_alpha (dest_drawable);

  out_bytes = GIMP_IMAGE_TYPE_BYTES (ret_buf_type);

  /*  If the pattern doesn't match the image in terms of color type,
   *  transform it.  (ie  pattern is RGB, image is indexed)
   */
  if (in_bytes != out_bytes || gimp_drawable_is_indexed (dest_drawable))
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
          gimp_image_transform_color (dest_image, dest_drawable, dest,
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
gimp_image_shadow (GimpImage *gimage,
                   gint       width,
                   gint       height,
                   gint       bpp)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  if (gimage->shadow)
    {
      if ((width  != tile_manager_width  (gimage->shadow)) ||
          (height != tile_manager_height (gimage->shadow)) ||
          (bpp    != tile_manager_bpp    (gimage->shadow)))
        {
          gimp_image_free_shadow (gimage);
        }
      else
        {
          return gimage->shadow;
        }
    }

  gimage->shadow = tile_manager_new (width, height, bpp);

  return gimage->shadow;
}

void
gimp_image_free_shadow (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  if (gimage->shadow)
    {
      tile_manager_unref (gimage->shadow);
      gimage->shadow = NULL;
    }
}


/*  parasites  */

GimpParasite *
gimp_image_parasite_find (const GimpImage *gimage,
                          const gchar     *name)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimp_parasite_list_find (gimage->parasites, name);
}

static void
list_func (gchar          *key,
           GimpParasite   *p,
           gchar        ***cur)
{
  *(*cur)++ = (gchar *) g_strdup (key);
}

gchar **
gimp_image_parasite_list (const GimpImage *gimage,
                          gint            *count)
{
  gchar **list;
  gchar **cur;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  *count = gimp_parasite_list_length (gimage->parasites);
  cur = list = g_new (gchar*, *count);

  gimp_parasite_list_foreach (gimage->parasites, (GHFunc) list_func, &cur);

  return list;
}

void
gimp_image_parasite_attach (GimpImage    *gimage,
                            GimpParasite *parasite)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage) && parasite != NULL);

  /* only set the dirty bit manually if we can be saved and the new
     parasite differs from the current one and we aren't undoable */
  if (gimp_parasite_is_undoable (parasite))
    gimp_image_undo_push_image_parasite (gimage,
                                         _("Attach Parasite to Image"),
                                         parasite);

  /*  We used to push an cantundo on te stack here. This made the undo stack
      unusable (NULL on the stack) and prevented people from undoing after a
      save (since most save plug-ins attach an undoable comment parasite).
      Now we simply attach the parasite without pushing an undo. That way it's
      undoable but does not block the undo system.   --Sven
   */

  gimp_parasite_list_add (gimage->parasites, parasite);

  if (gimp_parasite_has_flag (parasite, GIMP_PARASITE_ATTACH_PARENT))
    {
      gimp_parasite_shift_parent (parasite);
      gimp_parasite_attach (gimage->gimp, parasite);
    }
}

void
gimp_image_parasite_detach (GimpImage   *gimage,
                            const gchar *parasite)
{
  GimpParasite *p;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (parasite != NULL);

  if (!(p = gimp_parasite_list_find (gimage->parasites, parasite)))
    return;

  if (gimp_parasite_is_undoable (p))
    gimp_image_undo_push_image_parasite_remove (gimage,
                                                _("Remove Parasite from Image"),
                                                gimp_parasite_name (p));

  gimp_parasite_list_remove (gimage->parasites, parasite);
}


/*  tattoos  */

GimpTattoo
gimp_image_get_new_tattoo (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), 0);

  gimage->tattoo_state++;

  if (gimage->tattoo_state <= 0)
    g_warning ("%s: Tattoo state corrupted (integer overflow).", G_STRFUNC);

  return gimage->tattoo_state;
}

GimpTattoo
gimp_image_get_tattoo_state (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), 0);

  return gimage->tattoo_state;
}

gboolean
gimp_image_set_tattoo_state (GimpImage  *gimage,
                             GimpTattoo  val)
{
  GList      *list;
  gboolean    retval = TRUE;
  GimpTattoo  maxval = 0;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  /* Check that the layer tatoos don't overlap with channel or vector ones */
  for (list = GIMP_LIST (gimage->layers)->list;
       list;
       list = g_list_next (list))
    {
      GimpTattoo ltattoo;

      ltattoo = gimp_item_get_tattoo (GIMP_ITEM (list->data));
      if (ltattoo > maxval)
        maxval = ltattoo;

      if (gimp_image_get_channel_by_tattoo (gimage, ltattoo))
        retval = FALSE; /* Oopps duplicated tattoo in channel */

      if (gimp_image_get_vectors_by_tattoo (gimage, ltattoo))
        retval = FALSE; /* Oopps duplicated tattoo in vectors */
    }

  /* Now check that the channel and vectors tattoos don't overlap */
  for (list = GIMP_LIST (gimage->channels)->list;
       list;
       list = g_list_next (list))
    {
      GimpTattoo ctattoo;

      ctattoo = gimp_item_get_tattoo (GIMP_ITEM (list->data));
      if (ctattoo > maxval)
        maxval = ctattoo;

      if (gimp_image_get_vectors_by_tattoo (gimage, ctattoo))
        retval = FALSE; /* Oopps duplicated tattoo in vectors */
    }

  /* Find the max tattoo value in the vectors */
  for (list = GIMP_LIST (gimage->vectors)->list;
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
    gimage->tattoo_state = val;

  return retval;
}


/*  layers / channels / vectors  */

GimpContainer *
gimp_image_get_layers (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimage->layers;
}

GimpContainer *
gimp_image_get_channels (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimage->channels;
}

GimpContainer *
gimp_image_get_vectors (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimage->vectors;
}

GimpDrawable *
gimp_image_active_drawable (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  /*  If there is an active channel (a saved selection, etc.),
   *  we ignore the active layer
   */
  if (gimage->active_channel)
    {
      return GIMP_DRAWABLE (gimage->active_channel);
    }
  else if (gimage->active_layer)
    {
      GimpLayer *layer = gimage->active_layer;

      if (layer->mask && layer->mask->edit_mask)
        return GIMP_DRAWABLE (layer->mask);
      else
        return GIMP_DRAWABLE (layer);
    }

  return NULL;
}

GimpLayer *
gimp_image_get_active_layer (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimage->active_layer;
}

GimpChannel *
gimp_image_get_active_channel (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimage->active_channel;
}

GimpVectors *
gimp_image_get_active_vectors (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return gimage->active_vectors;
}

GimpLayer *
gimp_image_set_active_layer (GimpImage *gimage,
                             GimpLayer *layer)
{
  GimpLayer *floating_sel;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (layer == NULL || GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (layer == NULL ||
                        gimp_container_have (gimage->layers,
                                             GIMP_OBJECT (layer)), NULL);

  floating_sel = gimp_image_floating_sel (gimage);

  /*  Make sure the floating_sel always is the active layer  */
  if (floating_sel && layer != floating_sel)
    return floating_sel;

  if (layer != gimage->active_layer)
    {
      if (layer)
        {
          /*  Configure the layer stack to reflect this change  */
          gimage->layer_stack = g_slist_remove (gimage->layer_stack, layer);
          gimage->layer_stack = g_slist_prepend (gimage->layer_stack, layer);
        }

      /*  Don't cache selection info for the previous active layer  */
      if (gimage->active_layer)
        gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (gimage->active_layer));

      gimage->active_layer = layer;

      g_signal_emit (gimage, gimp_image_signals[ACTIVE_LAYER_CHANGED], 0);

      if (layer && gimage->active_channel)
        gimp_image_set_active_channel (gimage, NULL);
    }

  return gimage->active_layer;
}

GimpChannel *
gimp_image_set_active_channel (GimpImage   *gimage,
                               GimpChannel *channel)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (channel == NULL || GIMP_IS_CHANNEL (channel), NULL);
  g_return_val_if_fail (channel == NULL ||
                        gimp_container_have (gimage->channels,
                                             GIMP_OBJECT (channel)), NULL);

  /*  Not if there is a floating selection  */
  if (channel && gimp_image_floating_sel (gimage))
    return NULL;

  if (channel != gimage->active_channel)
    {
      gimage->active_channel = channel;

      g_signal_emit (gimage, gimp_image_signals[ACTIVE_CHANNEL_CHANGED], 0);

      if (channel && gimage->active_layer)
        gimp_image_set_active_layer (gimage, NULL);
    }

  return gimage->active_channel;
}

GimpChannel *
gimp_image_unset_active_channel (GimpImage *gimage)
{
  GimpChannel *channel;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  channel = gimp_image_get_active_channel (gimage);

  if (channel)
    {
      gimp_image_set_active_channel (gimage, NULL);

      if (gimage->layer_stack)
        gimp_image_set_active_layer (gimage, gimage->layer_stack->data);
    }

  return channel;
}

GimpVectors *
gimp_image_set_active_vectors (GimpImage   *gimage,
                               GimpVectors *vectors)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (vectors == NULL || GIMP_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (vectors == NULL ||
                        gimp_container_have (gimage->vectors,
                                             GIMP_OBJECT (vectors)), NULL);

  if (vectors != gimage->active_vectors)
    {
      gimage->active_vectors = vectors;

      g_signal_emit (gimage, gimp_image_signals[ACTIVE_VECTORS_CHANGED], 0);
    }

  return gimage->active_vectors;
}

void
gimp_image_active_layer_changed (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (gimage, gimp_image_signals[ACTIVE_LAYER_CHANGED], 0);
}

void
gimp_image_active_channel_changed (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (gimage, gimp_image_signals[ACTIVE_CHANNEL_CHANGED], 0);
}

void
gimp_image_active_vectors_changed (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (gimage, gimp_image_signals[ACTIVE_VECTORS_CHANGED], 0);
}

gint
gimp_image_get_layer_index (const GimpImage   *gimage,
                            const GimpLayer   *layer)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), -1);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), -1);

  return gimp_container_get_child_index (gimage->layers,
                                         GIMP_OBJECT (layer));
}

gint
gimp_image_get_channel_index (const GimpImage   *gimage,
                              const GimpChannel *channel)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), -1);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), -1);

  return gimp_container_get_child_index (gimage->channels,
                                         GIMP_OBJECT (channel));
}

gint
gimp_image_get_vectors_index (const GimpImage   *gimage,
                              const GimpVectors *vectors)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), -1);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), -1);

  return gimp_container_get_child_index (gimage->vectors,
                                         GIMP_OBJECT (vectors));
}

GimpLayer *
gimp_image_get_layer_by_tattoo (const GimpImage *gimage,
                                GimpTattoo       tattoo)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  for (list = GIMP_LIST (gimage->layers)->list;
       list;
       list = g_list_next (list))
    {
      GimpLayer *layer = list->data;

      if (gimp_item_get_tattoo (GIMP_ITEM (layer)) == tattoo)
        return layer;
    }

  return NULL;
}

GimpChannel *
gimp_image_get_channel_by_tattoo (const GimpImage *gimage,
                                  GimpTattoo       tattoo)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  for (list = GIMP_LIST (gimage->channels)->list;
       list;
       list = g_list_next (list))
    {
      GimpChannel *channel = list->data;

      if (gimp_item_get_tattoo (GIMP_ITEM (channel)) == tattoo)
        return channel;
    }

  return NULL;
}

GimpVectors *
gimp_image_get_vectors_by_tattoo (const GimpImage *gimage,
                                  GimpTattoo       tattoo)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  for (list = GIMP_LIST (gimage->vectors)->list;
       list;
       list = g_list_next (list))
    {
      GimpVectors *vectors = list->data;

      if (gimp_item_get_tattoo (GIMP_ITEM (vectors)) == tattoo)
        return vectors;
    }

  return NULL;
}

GimpLayer *
gimp_image_get_layer_by_name (const GimpImage *gimage,
                              const gchar     *name)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return (GimpLayer *) gimp_container_get_child_by_name (gimage->layers,
                                                         name);
}

GimpChannel *
gimp_image_get_channel_by_name (const GimpImage *gimage,
                                const gchar     *name)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return (GimpChannel *) gimp_container_get_child_by_name (gimage->channels,
                                                           name);
}

GimpVectors *
gimp_image_get_vectors_by_name (const GimpImage *gimage,
                                const gchar     *name)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  return (GimpVectors *) gimp_container_get_child_by_name (gimage->vectors,
                                                           name);
}

gboolean
gimp_image_add_layer (GimpImage *gimage,
                      GimpLayer *layer,
                      gint       position)
{
  GimpLayer *active_layer;
  GimpLayer *floating_sel;
  gboolean   old_has_alpha;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  if (GIMP_ITEM (layer)->gimage != NULL &&
      GIMP_ITEM (layer)->gimage != gimage)
    {
      g_warning ("%s: attempting to add layer to wrong image.", G_STRFUNC);
      return FALSE;
    }

  if (gimp_container_have (gimage->layers, GIMP_OBJECT (layer)))
    {
      g_warning ("%s: trying to add layer to image twice.", G_STRFUNC);
      return FALSE;
    }

  floating_sel = gimp_image_floating_sel (gimage);

  if (floating_sel && gimp_layer_is_floating_sel (layer))
    {
      g_warning ("%s: trying to add floating layer to image which alyready "
                 "has a floating selection.", G_STRFUNC);
      return FALSE;
    }

  active_layer = gimp_image_get_active_layer (gimage);

  old_has_alpha = gimp_image_has_alpha (gimage);

  gimp_image_undo_push_layer_add (gimage, _("Add Layer"),
                                  layer, 0, active_layer);

  gimp_item_set_image (GIMP_ITEM (layer), gimage);

  if (layer->mask)
    gimp_item_set_image (GIMP_ITEM (layer->mask), gimage);

  /*  If the layer is a floating selection, set the ID  */
  if (gimp_layer_is_floating_sel (layer))
    gimage->floating_sel = layer;

  /*  add the layer to the list at the specified position  */
  if (position == -1)
    {
      if (active_layer)
        position = gimp_container_get_child_index (gimage->layers,
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
  if (position > gimp_container_num_children (gimage->layers))
    position = gimp_container_num_children (gimage->layers);

  gimp_container_insert (gimage->layers, GIMP_OBJECT (layer), position);
  gimp_item_sink (GIMP_ITEM (layer));

  /*  notify the layers dialog of the currently active layer  */
  gimp_image_set_active_layer (gimage, layer);

  if (old_has_alpha != gimp_image_has_alpha (gimage))
    gimp_image_alpha_changed (gimage);

  if (gimp_layer_is_floating_sel (layer))
    gimp_image_floating_selection_changed (gimage);

  return TRUE;
}

void
gimp_image_remove_layer (GimpImage *gimage,
                         GimpLayer *layer)
{
  GimpLayer *active_layer;
  gint       index;
  gboolean   old_has_alpha;
  gboolean   undo_group = FALSE;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_container_have (gimage->layers,
                                         GIMP_OBJECT (layer)));

  if (gimp_drawable_has_floating_sel (GIMP_DRAWABLE (layer)))
    {
      gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                   _("Remove Layer"));
      undo_group = TRUE;

      floating_sel_remove (gimp_image_floating_sel (gimage));
    }

  active_layer = gimp_image_get_active_layer (gimage);

  index = gimp_container_get_child_index (gimage->layers,
                                          GIMP_OBJECT (layer));

  old_has_alpha = gimp_image_has_alpha (gimage);

  gimp_image_undo_push_layer_remove (gimage, _("Remove Layer"),
                                     layer, index, active_layer);

  g_object_ref (layer);

  /*  Make sure we're not caching any old selection info  */
  if (layer == active_layer)
    gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (layer));

  gimp_container_remove (gimage->layers, GIMP_OBJECT (layer));
  gimage->layer_stack = g_slist_remove (gimage->layer_stack, layer);

  if (gimage->floating_sel == layer)
    {
      /*  If this was the floating selection, reset the fs pointer
       *  and activate the underlying drawable
       */
      gimage->floating_sel = NULL;

      floating_sel_activate_drawable (layer);

      gimp_image_floating_selection_changed (gimage);
    }
  else if (layer == active_layer)
    {
      if (gimage->layer_stack)
        {
          active_layer = gimage->layer_stack->data;
        }
      else
        {
          gint n_children = gimp_container_num_children (gimage->layers);

          if (n_children > 0)
            {
              index = CLAMP (index, 0, n_children - 1);

              active_layer = (GimpLayer *)
                gimp_container_get_child_by_index (gimage->layers, index);
            }
          else
            {
              active_layer = NULL;
            }
        }

      gimp_image_set_active_layer (gimage, active_layer);
    }

  gimp_item_removed (GIMP_ITEM (layer));

  g_object_unref (layer);

  if (old_has_alpha != gimp_image_has_alpha (gimage))
    gimp_image_alpha_changed (gimage);

  if (undo_group)
    gimp_image_undo_group_end (gimage);
}

gboolean
gimp_image_raise_layer (GimpImage *gimage,
                        GimpLayer *layer)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  index = gimp_container_get_child_index (gimage->layers,
                                          GIMP_OBJECT (layer));

  if (index == 0)
    {
      g_message (_("Layer cannot be raised higher."));
      return FALSE;
    }

  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    {
      g_message (_("Cannot raise a layer without alpha."));
      return FALSE;
    }

  return gimp_image_position_layer (gimage, layer, index - 1,
                                    TRUE, _("Raise Layer"));
}

gboolean
gimp_image_lower_layer (GimpImage *gimage,
                        GimpLayer *layer)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  index = gimp_container_get_child_index (gimage->layers,
                                          GIMP_OBJECT (layer));

  if (index == gimp_container_num_children (gimage->layers) - 1)
    {
      g_message (_("Layer cannot be lowered more."));
      return FALSE;
    }

  return gimp_image_position_layer (gimage, layer, index + 1,
                                    TRUE, _("Lower Layer"));
}

gboolean
gimp_image_raise_layer_to_top (GimpImage *gimage,
                               GimpLayer *layer)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  index = gimp_container_get_child_index (gimage->layers,
                                          GIMP_OBJECT (layer));

  if (index == 0)
    {
      g_message (_("Layer is already on top."));
      return FALSE;
    }

  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    {
      g_message (_("Cannot raise a layer without alpha."));
      return FALSE;
    }

  return gimp_image_position_layer (gimage, layer, 0,
                                    TRUE, _("Raise Layer to Top"));
}

gboolean
gimp_image_lower_layer_to_bottom (GimpImage *gimage,
                                  GimpLayer *layer)
{
  gint index;
  gint length;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  index = gimp_container_get_child_index (gimage->layers,
                                          GIMP_OBJECT (layer));

  length = gimp_container_num_children (gimage->layers);

  if (index == length - 1)
    {
      g_message (_("Layer is already on the bottom."));
      return FALSE;
    }

  return gimp_image_position_layer (gimage, layer, length - 1,
                                    TRUE, _("Lower Layer to Bottom"));
}

gboolean
gimp_image_position_layer (GimpImage   *gimage,
                           GimpLayer   *layer,
                           gint         new_index,
                           gboolean     push_undo,
                           const gchar *undo_desc)
{
  gint index;
  gint num_layers;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  index = gimp_container_get_child_index (gimage->layers,
                                          GIMP_OBJECT (layer));
  if (index < 0)
    return FALSE;

  num_layers = gimp_container_num_children (gimage->layers);

  new_index = CLAMP (new_index, 0, num_layers - 1);

  if (new_index == index)
    return TRUE;

  /* check if we want to move it below a bottom layer without alpha */
  if (new_index == num_layers - 1)
    {
      GimpLayer *tmp;

      tmp = (GimpLayer *) gimp_container_get_child_by_index (gimage->layers,
                                                             num_layers - 1);

      if (new_index == num_layers - 1 &&
          ! gimp_drawable_has_alpha (GIMP_DRAWABLE (tmp)))
        {
          g_message (_("Layer '%s' has no alpha. Layer was placed above it."),
                     GIMP_OBJECT (tmp)->name);
          new_index--;
        }
    }

  if (push_undo)
    gimp_image_undo_push_layer_reposition (gimage, undo_desc, layer);

  gimp_container_reorder (gimage->layers, GIMP_OBJECT (layer), new_index);

  if (gimp_item_get_visible (GIMP_ITEM (layer)))
    {
      gint off_x, off_y;

      gimp_item_offsets (GIMP_ITEM (layer), &off_x, &off_y);

      gimp_image_update (gimage,
                         off_x, off_y,
                         gimp_item_width  (GIMP_ITEM (layer)),
                         gimp_item_height (GIMP_ITEM (layer)));
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimage));
    }

  return TRUE;
}

gboolean
gimp_image_add_channel (GimpImage   *gimage,
                        GimpChannel *channel,
                        gint         position)
{
  GimpChannel *active_channel;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  if (GIMP_ITEM (channel)->gimage != NULL &&
      GIMP_ITEM (channel)->gimage != gimage)
    {
      g_warning ("%s: attempting to add channel to wrong image.", G_STRFUNC);
      return FALSE;
    }

  if (gimp_container_have (gimage->channels, GIMP_OBJECT (channel)))
    {
      g_warning ("%s: trying to add channel to image twice.", G_STRFUNC);
      return FALSE;
    }

  active_channel = gimp_image_get_active_channel (gimage);

  gimp_image_undo_push_channel_add (gimage, _("Add Channel"),
                                    channel, 0, active_channel);

  gimp_item_set_image (GIMP_ITEM (channel), gimage);

  /*  add the layer to the list at the specified position  */
  if (position == -1)
    {
      if (active_channel)
        position = gimp_container_get_child_index (gimage->channels,
                                                   GIMP_OBJECT (active_channel));
      else
        position = 0;
    }

  /*  Don't add at a non-existing index  */
  if (position > gimp_container_num_children (gimage->channels))
    position = gimp_container_num_children (gimage->channels);

  gimp_container_insert (gimage->channels, GIMP_OBJECT (channel), position);
  gimp_item_sink (GIMP_ITEM (channel));

  /*  notify this gimage of the currently active channel  */
  gimp_image_set_active_channel (gimage, channel);

  return TRUE;
}

void
gimp_image_remove_channel (GimpImage   *gimage,
                           GimpChannel *channel)
{
  GimpChannel *active_channel;
  gint         index;
  gboolean     undo_group = FALSE;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (gimp_container_have (gimage->channels,
                                         GIMP_OBJECT (channel)));

  if (gimp_drawable_has_floating_sel (GIMP_DRAWABLE (channel)))
    {
      gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                   _("Remove Channel"));
      undo_group = TRUE;

      floating_sel_remove (gimp_image_floating_sel (gimage));
    }

  active_channel = gimp_image_get_active_channel (gimage);

  index = gimp_container_get_child_index (gimage->channels,
                                          GIMP_OBJECT (channel));

  gimp_image_undo_push_channel_remove (gimage, _("Remove Channel"),
                                       channel, index, active_channel);

  g_object_ref (channel);

  gimp_container_remove (gimage->channels, GIMP_OBJECT (channel));
  gimp_item_removed (GIMP_ITEM (channel));

  if (channel == active_channel)
    {
      gint n_children = gimp_container_num_children (gimage->channels);

      if (n_children > 0)
        {
          index = CLAMP (index, 0, n_children - 1);

          active_channel = (GimpChannel *)
            gimp_container_get_child_by_index (gimage->channels, index);

          gimp_image_set_active_channel (gimage, active_channel);
        }
      else
        {
          gimp_image_unset_active_channel (gimage);
        }
    }

  g_object_unref (channel);

  if (undo_group)
    gimp_image_undo_group_end (gimage);
}

gboolean
gimp_image_raise_channel (GimpImage   *gimage,
                          GimpChannel *channel)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  index = gimp_container_get_child_index (gimage->channels,
                                          GIMP_OBJECT (channel));

  if (index == 0)
    {
      g_message (_("Channel cannot be raised higher."));
      return FALSE;
    }

  return gimp_image_position_channel (gimage, channel, index - 1,
                                      TRUE, _("Raise Channel"));
}

gboolean
gimp_image_raise_channel_to_top (GimpImage   *gimage,
                                 GimpChannel *channel)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  index = gimp_container_get_child_index (gimage->channels,
                                          GIMP_OBJECT (channel));

  if (index == 0)
    {
      g_message (_("Channel is already on top."));
      return FALSE;
    }

  return gimp_image_position_channel (gimage, channel, 0,
                                      TRUE, _("Raise Channel to Top"));
}

gboolean
gimp_image_lower_channel (GimpImage   *gimage,
                          GimpChannel *channel)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  index = gimp_container_get_child_index (gimage->channels,
                                          GIMP_OBJECT (channel));

  if (index == gimp_container_num_children (gimage->channels) - 1)
    {
      g_message (_("Channel cannot be lowered more."));
      return FALSE;
    }

  return gimp_image_position_channel (gimage, channel, index + 1,
                                      TRUE, _("Lower Channel"));
}

gboolean
gimp_image_lower_channel_to_bottom (GimpImage   *gimage,
                                    GimpChannel *channel)
{
  gint index;
  gint length;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  index = gimp_container_get_child_index (gimage->channels,
                                          GIMP_OBJECT (channel));

  length = gimp_container_num_children (gimage->channels);

  if (index == length - 1)
    {
      g_message (_("Channel is already on the bottom."));
      return FALSE;
    }

  return gimp_image_position_channel (gimage, channel, length - 1,
                                      TRUE, _("Lower Channel to Bottom"));
}

gboolean
gimp_image_position_channel (GimpImage   *gimage,
                             GimpChannel *channel,
                             gint         new_index,
                             gboolean     push_undo,
                             const gchar *undo_desc)
{
  gint index;
  gint num_channels;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  index = gimp_container_get_child_index (gimage->channels,
                                          GIMP_OBJECT (channel));
  if (index < 0)
    return FALSE;

  num_channels = gimp_container_num_children (gimage->channels);

  new_index = CLAMP (new_index, 0, num_channels - 1);

  if (new_index == index)
    return TRUE;

  if (push_undo)
    gimp_image_undo_push_channel_reposition (gimage, undo_desc, channel);

  gimp_container_reorder (gimage->channels,
                          GIMP_OBJECT (channel), new_index);

  if (gimp_item_get_visible (GIMP_ITEM (channel)))
    {
      gint off_x, off_y;

      gimp_item_offsets (GIMP_ITEM (channel), &off_x, &off_y);

      gimp_image_update (gimage,
                         off_x, off_y,
                         gimp_item_width  (GIMP_ITEM (channel)),
                         gimp_item_height (GIMP_ITEM (channel)));
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimage));
    }

  return TRUE;
}

gboolean
gimp_image_add_vectors (GimpImage   *gimage,
                        GimpVectors *vectors,
                        gint         position)
{
  GimpVectors *active_vectors;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);

  if (GIMP_ITEM (vectors)->gimage != NULL &&
      GIMP_ITEM (vectors)->gimage != gimage)
    {
      g_warning ("%s: attempting to add vectors to wrong image.", G_STRFUNC);
      return FALSE;
    }

  if (gimp_container_have (gimage->vectors, GIMP_OBJECT (vectors)))
    {
      g_warning ("%s: trying to add vectors to image twice.", G_STRFUNC);
      return FALSE;
    }

  active_vectors = gimp_image_get_active_vectors (gimage);

  gimp_image_undo_push_vectors_add (gimage, _("Add Path"),
                                    vectors, 0, active_vectors);

  gimp_item_set_image (GIMP_ITEM (vectors), gimage);

  /*  add the layer to the list at the specified position  */
  if (position == -1)
    {
      if (active_vectors)
        position = gimp_container_get_child_index (gimage->vectors,
                                                   GIMP_OBJECT (active_vectors));
      else
        position = 0;
    }

  /*  Don't add at a non-existing index  */
  if (position > gimp_container_num_children (gimage->vectors))
    position = gimp_container_num_children (gimage->vectors);

  gimp_container_insert (gimage->vectors, GIMP_OBJECT (vectors), position);
  gimp_item_sink (GIMP_ITEM (vectors));

  /*  notify this gimage of the currently active vectors  */
  gimp_image_set_active_vectors (gimage, vectors);

  return TRUE;
}

void
gimp_image_remove_vectors (GimpImage   *gimage,
                           GimpVectors *vectors)
{
  GimpVectors *active_vectors;
  gint         index;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_VECTORS (vectors));
  g_return_if_fail (gimp_container_have (gimage->vectors,
                                         GIMP_OBJECT (vectors)));

  active_vectors = gimp_image_get_active_vectors (gimage);

  index = gimp_container_get_child_index (gimage->vectors,
                                          GIMP_OBJECT (vectors));

  gimp_image_undo_push_vectors_remove (gimage, _("Remove Path"),
                                       vectors, index, active_vectors);

  g_object_ref (vectors);

  gimp_container_remove (gimage->vectors, GIMP_OBJECT (vectors));
  gimp_item_removed (GIMP_ITEM (vectors));

  if (vectors == active_vectors)
    {
      gint n_children = gimp_container_num_children (gimage->vectors);

      if (n_children > 0)
        {
          index = CLAMP (index, 0, n_children - 1);

          active_vectors = (GimpVectors *)
            gimp_container_get_child_by_index (gimage->vectors, index);
        }
      else
        {
          active_vectors = NULL;
        }

      gimp_image_set_active_vectors (gimage, active_vectors);
    }

  g_object_unref (vectors);
}

gboolean
gimp_image_raise_vectors (GimpImage   *gimage,
                          GimpVectors *vectors)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);

  index = gimp_container_get_child_index (gimage->vectors,
                                          GIMP_OBJECT (vectors));

  if (index == 0)
    {
      g_message (_("Path cannot be raised higher."));
      return FALSE;
    }

  return gimp_image_position_vectors (gimage, vectors, index - 1,
                                      TRUE, _("Raise Path"));
}

gboolean
gimp_image_raise_vectors_to_top (GimpImage   *gimage,
                                 GimpVectors *vectors)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);

  index = gimp_container_get_child_index (gimage->vectors,
                                          GIMP_OBJECT (vectors));

  if (index == 0)
    {
      g_message (_("Path is already on top."));
      return FALSE;
    }

  return gimp_image_position_vectors (gimage, vectors, 0,
                                      TRUE, _("Raise Path to Top"));
}

gboolean
gimp_image_lower_vectors (GimpImage   *gimage,
                          GimpVectors *vectors)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);

  index = gimp_container_get_child_index (gimage->vectors,
                                          GIMP_OBJECT (vectors));

  if (index == gimp_container_num_children (gimage->vectors) - 1)
    {
      g_message (_("Path cannot be lowered more."));
      return FALSE;
    }

  return gimp_image_position_vectors (gimage, vectors, index + 1,
                                      TRUE, _("Lower Path"));
}

gboolean
gimp_image_lower_vectors_to_bottom (GimpImage   *gimage,
                                    GimpVectors *vectors)
{
  gint index;
  gint length;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);

  index = gimp_container_get_child_index (gimage->vectors,
                                          GIMP_OBJECT (vectors));

  length = gimp_container_num_children (gimage->vectors);

  if (index == length - 1)
    {
      g_message (_("Path is already on the bottom."));
      return FALSE;
    }

  return gimp_image_position_vectors (gimage, vectors, length - 1,
                                      TRUE, _("Lower Path to Bottom"));
}

gboolean
gimp_image_position_vectors (GimpImage   *gimage,
                             GimpVectors *vectors,
                             gint         new_index,
                             gboolean     push_undo,
                             const gchar *undo_desc)
{
  gint index;
  gint num_vectors;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);

  index = gimp_container_get_child_index (gimage->vectors,
                                          GIMP_OBJECT (vectors));
  if (index < 0)
    return FALSE;

  num_vectors = gimp_container_num_children (gimage->vectors);

  new_index = CLAMP (new_index, 0, num_vectors - 1);

  if (new_index == index)
    return TRUE;

  if (push_undo)
    gimp_image_undo_push_vectors_reposition (gimage, undo_desc, vectors);

  gimp_container_reorder (gimage->vectors,
                          GIMP_OBJECT (vectors), new_index);

  return TRUE;
}

gboolean
gimp_image_layer_boundary (const GimpImage  *gimage,
                           BoundSeg        **segs,
                           gint             *n_segs)
{
  GimpLayer *layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (segs != NULL, FALSE);
  g_return_val_if_fail (n_segs != NULL, FALSE);

  /*  The second boundary corresponds to the active layer's
   *  perimeter...
   */
  layer = gimp_image_get_active_layer (gimage);

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
gimp_image_pick_correlate_layer (const GimpImage *gimage,
                                 gint             x,
                                 gint             y)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  for (list = GIMP_LIST (gimage->layers)->list;
       list;
       list = g_list_next (list))
    {
      GimpLayer *layer = list->data;

      if (gimp_layer_pick_correlate (layer, x, y))
        return layer;
    }

  return NULL;
}

gboolean
gimp_image_coords_in_active_drawable (GimpImage        *gimage,
                                      const GimpCoords *coords)
{
  GimpDrawable *drawable;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  drawable = gimp_image_active_drawable (gimage);

  if (drawable)
    {
      GimpItem *item = GIMP_ITEM (drawable);
      gint      x, y;

      gimp_item_offsets (item, &x, &y);

      x = ROUND (coords->x) - x;
      y = ROUND (coords->y) - y;

      if (x < 0 || x > gimp_item_width (item))
        return FALSE;

      if (y < 0 || y > gimp_item_height (item))
        return FALSE;

      return TRUE;
    }

  return FALSE;
}

void
gimp_image_invalidate_layer_previews (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  gimp_container_foreach (gimage->layers,
                          (GFunc) gimp_viewable_invalidate_preview,
                          NULL);
}

void
gimp_image_invalidate_channel_previews (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  gimp_container_foreach (gimage->channels,
                          (GFunc) gimp_viewable_invalidate_preview,
                          NULL);
}
