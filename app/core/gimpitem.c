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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "ligma.h"
#include "ligma-parasites.h"
#include "ligmachannel.h"
#include "ligmacontainer.h"
#include "ligmaidtable.h"
#include "ligmaimage.h"
#include "ligmaimage-undo.h"
#include "ligmaimage-undo-push.h"
#include "ligmaitem.h"
#include "ligmaitem-preview.h"
#include "ligmaitemtree.h"
#include "ligmalist.h"
#include "ligmaparasitelist.h"
#include "ligmaprogress.h"
#include "ligmastrokeoptions.h"

#include "paint/ligmapaintoptions.h"

#include "ligma-intl.h"


enum
{
  REMOVED,
  VISIBILITY_CHANGED,
  COLOR_TAG_CHANGED,
  LOCK_CONTENT_CHANGED,
  LOCK_POSITION_CHANGED,
  LOCK_VISIBILITY_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_ID,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_OFFSET_X,
  PROP_OFFSET_Y,
  PROP_VISIBLE,
  PROP_COLOR_TAG,
  PROP_LOCK_CONTENT,
  PROP_LOCK_POSITION,
  PROP_LOCK_VISIBILITY,
  N_PROPS
};


typedef struct _LigmaItemPrivate LigmaItemPrivate;

struct _LigmaItemPrivate
{
  gint              ID;                          /*  provides a unique ID        */
  guint32           tattoo;                      /*  provides a permanent ID     */

  LigmaImage        *image;                       /*  item owner                  */

  LigmaParasiteList *parasites;                   /*  Plug-in parasite data       */

  gint              width, height;               /*  size in pixels              */
  gint              offset_x, offset_y;          /*  pixel offset in image       */

  guint             visible                : 1;  /*  item visibility             */
  guint             bind_visible_to_active : 1;  /*  visibility bound to active  */

  guint             lock_content           : 1;  /*  content editability         */
  guint             lock_position          : 1;  /*  content movability          */
  guint             lock_visibility        : 1;  /*  automatic visibility change */

  guint             removed                : 1;  /*  removed from the image?     */

  LigmaColorTag      color_tag;                   /*  color tag                   */

  GList            *offset_nodes;                /*  offset nodes to manage      */
};

#define GET_PRIVATE(item) ((LigmaItemPrivate *) ligma_item_get_instance_private ((LigmaItem *) (item)))


/*  local function prototypes  */

static void       ligma_item_constructed             (GObject        *object);
static void       ligma_item_finalize                (GObject        *object);
static void       ligma_item_set_property            (GObject        *object,
                                                     guint           property_id,
                                                     const GValue   *value,
                                                     GParamSpec     *pspec);
static void       ligma_item_get_property            (GObject        *object,
                                                     guint           property_id,
                                                     GValue         *value,
                                                     GParamSpec     *pspec);

static gint64     ligma_item_get_memsize             (LigmaObject     *object,
                                                     gint64         *gui_size);

static gboolean   ligma_item_real_is_content_locked  (LigmaItem       *item,
                                                     LigmaItem      **locked_item);
static gboolean   ligma_item_real_is_position_locked (LigmaItem       *item,
                                                     LigmaItem      **locked_item,
                                                     gboolean        check_children);
static gboolean ligma_item_real_is_visibility_locked (LigmaItem       *item,
                                                     LigmaItem      **locked_item);
static gboolean   ligma_item_real_bounds             (LigmaItem       *item,
                                                     gdouble        *x,
                                                     gdouble        *y,
                                                     gdouble        *width,
                                                     gdouble        *height);
static LigmaItem * ligma_item_real_duplicate          (LigmaItem       *item,
                                                     GType           new_type);
static void       ligma_item_real_convert            (LigmaItem       *item,
                                                     LigmaImage      *dest_image,
                                                     GType           old_type);
static gboolean   ligma_item_real_rename             (LigmaItem       *item,
                                                     const gchar    *new_name,
                                                     const gchar    *undo_desc,
                                                     GError        **error);
static void       ligma_item_real_start_transform    (LigmaItem       *item,
                                                     gboolean        push_undo);
static void       ligma_item_real_end_transform      (LigmaItem       *item,
                                                     gboolean        push_undo);
static void       ligma_item_real_translate          (LigmaItem       *item,
                                                     gdouble         offset_x,
                                                     gdouble         offset_y,
                                                     gboolean        push_undo);
static void       ligma_item_real_scale              (LigmaItem       *item,
                                                     gint            new_width,
                                                     gint            new_height,
                                                     gint            new_offset_x,
                                                     gint            new_offset_y,
                                                     LigmaInterpolationType interpolation,
                                                     LigmaProgress   *progress);
static void       ligma_item_real_resize             (LigmaItem       *item,
                                                     LigmaContext    *context,
                                                     LigmaFillType    fill_type,
                                                     gint            new_width,
                                                     gint            new_height,
                                                     gint            offset_x,
                                                     gint            offset_y);
static LigmaTransformResize
                  ligma_item_real_get_clip           (LigmaItem       *item,
                                                     LigmaTransformResize clip_result);



G_DEFINE_TYPE_WITH_PRIVATE (LigmaItem, ligma_item, LIGMA_TYPE_FILTER)

#define parent_class ligma_item_parent_class

static guint ligma_item_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *ligma_item_props[N_PROPS] = { NULL, };


static void
ligma_item_class_init (LigmaItemClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass   *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);

  ligma_item_signals[REMOVED] =
    g_signal_new ("removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaItemClass, removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_item_signals[VISIBILITY_CHANGED] =
    g_signal_new ("visibility-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaItemClass, visibility_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_item_signals[COLOR_TAG_CHANGED] =
    g_signal_new ("color-tag-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaItemClass, color_tag_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_item_signals[LOCK_CONTENT_CHANGED] =
    g_signal_new ("lock-content-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaItemClass, lock_content_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_item_signals[LOCK_POSITION_CHANGED] =
    g_signal_new ("lock-position-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaItemClass, lock_position_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_item_signals[LOCK_VISIBILITY_CHANGED] =
    g_signal_new ("lock-visibility-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaItemClass, lock_visibility_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed        = ligma_item_constructed;
  object_class->finalize           = ligma_item_finalize;
  object_class->set_property       = ligma_item_set_property;
  object_class->get_property       = ligma_item_get_property;

  ligma_object_class->get_memsize   = ligma_item_get_memsize;

  viewable_class->name_editable    = TRUE;
  viewable_class->get_preview_size = ligma_item_get_preview_size;
  viewable_class->get_popup_size   = ligma_item_get_popup_size;

  klass->removed                   = NULL;
  klass->visibility_changed        = NULL;
  klass->color_tag_changed         = NULL;
  klass->lock_content_changed      = NULL;
  klass->lock_position_changed     = NULL;
  klass->lock_visibility_changed   = NULL;

  klass->unset_removed             = NULL;
  klass->is_attached               = NULL;
  klass->is_content_locked         = ligma_item_real_is_content_locked;
  klass->is_position_locked        = ligma_item_real_is_position_locked;
  klass->is_visibility_locked      = ligma_item_real_is_visibility_locked;
  klass->get_tree                  = NULL;
  klass->bounds                    = ligma_item_real_bounds;
  klass->duplicate                 = ligma_item_real_duplicate;
  klass->convert                   = ligma_item_real_convert;
  klass->rename                    = ligma_item_real_rename;
  klass->start_move                = NULL;
  klass->end_move                  = NULL;
  klass->start_transform           = ligma_item_real_start_transform;
  klass->end_transform             = ligma_item_real_end_transform;
  klass->translate                 = ligma_item_real_translate;
  klass->scale                     = ligma_item_real_scale;
  klass->resize                    = ligma_item_real_resize;
  klass->flip                      = NULL;
  klass->rotate                    = NULL;
  klass->transform                 = NULL;
  klass->get_clip                  = ligma_item_real_get_clip;
  klass->fill                      = NULL;
  klass->stroke                    = NULL;
  klass->to_selection              = NULL;

  klass->default_name              = NULL;
  klass->rename_desc               = NULL;
  klass->translate_desc            = NULL;
  klass->scale_desc                = NULL;
  klass->resize_desc               = NULL;
  klass->flip_desc                 = NULL;
  klass->rotate_desc               = NULL;
  klass->transform_desc            = NULL;
  klass->fill_desc                 = NULL;
  klass->stroke_desc               = NULL;

  ligma_item_props[PROP_IMAGE] = g_param_spec_object ("image", NULL, NULL,
                                                     LIGMA_TYPE_IMAGE,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT);
  ligma_item_props[PROP_ID] = g_param_spec_int ("id", NULL, NULL,
                                               0, G_MAXINT, 0,
                                               LIGMA_PARAM_READABLE);

  ligma_item_props[PROP_WIDTH] = g_param_spec_int ("width", NULL, NULL,
                                                  1, LIGMA_MAX_IMAGE_SIZE, 1,
                                                  LIGMA_PARAM_READABLE);

  ligma_item_props[PROP_HEIGHT] = g_param_spec_int ("height", NULL, NULL,
                                                   1, LIGMA_MAX_IMAGE_SIZE, 1,
                                                   LIGMA_PARAM_READABLE);

  ligma_item_props[PROP_OFFSET_X] = g_param_spec_int ("offset-x", NULL, NULL,
                                                     -LIGMA_MAX_IMAGE_SIZE,
                                                     LIGMA_MAX_IMAGE_SIZE, 0,
                                                     LIGMA_PARAM_READABLE);

  ligma_item_props[PROP_OFFSET_Y] = g_param_spec_int ("offset-y", NULL, NULL,
                                                     -LIGMA_MAX_IMAGE_SIZE,
                                                     LIGMA_MAX_IMAGE_SIZE, 0,
                                                     LIGMA_PARAM_READABLE);

  ligma_item_props[PROP_VISIBLE] = g_param_spec_boolean ("visible", NULL, NULL,
                                                        TRUE,
                                                        LIGMA_PARAM_READABLE);

  ligma_item_props[PROP_COLOR_TAG] = g_param_spec_enum ("color-tag", NULL, NULL,
                                                       LIGMA_TYPE_COLOR_TAG,
                                                       LIGMA_COLOR_TAG_NONE,
                                                       LIGMA_PARAM_READABLE);

  ligma_item_props[PROP_LOCK_CONTENT] = g_param_spec_boolean ("lock-content",
                                                             NULL, NULL,
                                                             FALSE,
                                                             LIGMA_PARAM_READABLE);

  ligma_item_props[PROP_LOCK_POSITION] = g_param_spec_boolean ("lock-position",
                                                              NULL, NULL,
                                                              FALSE,
                                                              LIGMA_PARAM_READABLE);

  ligma_item_props[PROP_LOCK_VISIBILITY] = g_param_spec_boolean ("lock-visibility",
                                                                NULL, NULL,
                                                                FALSE,
                                                                LIGMA_PARAM_READABLE);

  g_object_class_install_properties (object_class, N_PROPS, ligma_item_props);
}

static void
ligma_item_init (LigmaItem *item)
{
  LigmaItemPrivate *private = GET_PRIVATE (item);

  g_object_force_floating (G_OBJECT (item));

  private->parasites              = ligma_parasite_list_new ();
  private->visible                = TRUE;
  private->bind_visible_to_active = TRUE;
}

static void
ligma_item_constructed (GObject *object)
{
  LigmaItemPrivate *private = GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_IMAGE (private->image));
  ligma_assert (private->ID != 0);
}

static void
ligma_item_finalize (GObject *object)
{
  LigmaItemPrivate *private = GET_PRIVATE (object);

  if (private->offset_nodes)
    {
      g_list_free_full (private->offset_nodes,
                        (GDestroyNotify) g_object_unref);
      private->offset_nodes = NULL;
    }

  if (private->image && private->image->ligma)
    {
      ligma_id_table_remove (private->image->ligma->item_table, private->ID);
      private->image = NULL;
    }

  g_clear_object (&private->parasites);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_item_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  LigmaItem *item = LIGMA_ITEM (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      ligma_item_set_image (item, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_item_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  LigmaItemPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, private->image);
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
    case PROP_OFFSET_X:
      g_value_set_int (value, private->offset_x);
      break;
    case PROP_OFFSET_Y:
      g_value_set_int (value, private->offset_y);
      break;
    case PROP_VISIBLE:
      g_value_set_boolean (value, private->visible);
      break;
    case PROP_COLOR_TAG:
      g_value_set_enum (value, private->color_tag);
      break;
    case PROP_LOCK_CONTENT:
      g_value_set_boolean (value, private->lock_content);
      break;
    case PROP_LOCK_POSITION:
      g_value_set_boolean (value, private->lock_position);
      break;
    case PROP_LOCK_VISIBILITY:
      g_value_set_boolean (value, private->lock_visibility);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
ligma_item_get_memsize (LigmaObject *object,
                       gint64     *gui_size)
{
  LigmaItemPrivate *private = GET_PRIVATE (object);
  gint64           memsize = 0;

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (private->parasites),
                                      gui_size);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
ligma_item_real_is_content_locked (LigmaItem  *item,
                                  LigmaItem **locked_item)
{
  LigmaItem *parent = ligma_item_get_parent (item);

  if (GET_PRIVATE (item)->lock_content)
    {
      if (locked_item)
        *locked_item = item;
    }
  else if (parent && ligma_item_is_content_locked (parent, locked_item))
    {
      return TRUE;
    }

  return GET_PRIVATE (item)->lock_content;
}

static gboolean
ligma_item_real_is_position_locked (LigmaItem  *item,
                                   LigmaItem **locked_item,
                                   gboolean   check_children)
{
  LigmaItem *parent = ligma_item_get_parent (item);

  if (GET_PRIVATE (item)->lock_position)
    {
      if (locked_item)
        *locked_item = item;
    }
  else if (parent &&
           LIGMA_ITEM_GET_CLASS (item)->is_position_locked (parent, locked_item, FALSE))
    {
      return TRUE;
    }

  return GET_PRIVATE (item)->lock_position;
}

static gboolean
ligma_item_real_is_visibility_locked (LigmaItem  *item,
                                     LigmaItem **locked_item)
{
  /* Unlike other locks, the visibility lock does not propagate from
   * parents or children.
   */

  if (GET_PRIVATE (item)->lock_visibility && locked_item)
    *locked_item = item;

  return GET_PRIVATE (item)->lock_visibility;
}

static gboolean
ligma_item_real_bounds (LigmaItem *item,
                       gdouble  *x,
                       gdouble  *y,
                       gdouble  *width,
                       gdouble  *height)
{
  LigmaItemPrivate *private = GET_PRIVATE (item);

  *x      = 0;
  *y      = 0;
  *width  = private->width;
  *height = private->height;

  return TRUE;
}

static LigmaItem *
ligma_item_real_duplicate (LigmaItem *item,
                          GType     new_type)
{
  LigmaItemPrivate *private;
  LigmaItem        *new_item;
  gchar           *new_name;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);

  private = GET_PRIVATE (item);

  g_return_val_if_fail (LIGMA_IS_IMAGE (private->image), NULL);
  g_return_val_if_fail (g_type_is_a (new_type, LIGMA_TYPE_ITEM), NULL);

  /*  formulate the new name  */
  {
    const gchar *name;
    gint         len;

    name = ligma_object_get_name (item);

    g_return_val_if_fail (name != NULL, NULL);

    len = strlen (_("copy"));

    if ((strlen (name) >= len &&
         strcmp (&name[strlen (name) - len], _("copy")) == 0) ||
        g_regex_match_simple ("#([0-9]+)\\s*$", name, 0, 0))
      {
        /* don't have redundant "copy"s */
        new_name = g_strdup (name);
      }
    else
      {
        new_name = g_strdup_printf (_("%s copy"), name);
      }
  }

  new_item = ligma_item_new (new_type,
                            ligma_item_get_image (item), new_name,
                            private->offset_x, private->offset_y,
                            ligma_item_get_width  (item),
                            ligma_item_get_height (item));

  g_free (new_name);

  ligma_viewable_set_expanded (LIGMA_VIEWABLE (new_item),
                              ligma_viewable_get_expanded (LIGMA_VIEWABLE (item)));

  g_object_unref (GET_PRIVATE (new_item)->parasites);
  GET_PRIVATE (new_item)->parasites = ligma_parasite_list_copy (private->parasites);

  ligma_item_set_visible   (new_item, ligma_item_get_visible   (item), FALSE);
  ligma_item_set_color_tag (new_item, ligma_item_get_color_tag (item), FALSE);

  if (ligma_item_can_lock_content (new_item))
    ligma_item_set_lock_content (new_item, ligma_item_get_lock_content (item),
                                FALSE);

  if (ligma_item_can_lock_position (new_item))
    ligma_item_set_lock_position (new_item, ligma_item_get_lock_position (item),
                                 FALSE);

  if (ligma_item_can_lock_visibility (new_item))
    ligma_item_set_lock_visibility (new_item, ligma_item_get_lock_visibility (item),
                                   FALSE);

  return new_item;
}

static void
ligma_item_real_convert (LigmaItem  *item,
                        LigmaImage *dest_image,
                        GType      old_type)
{
  ligma_item_set_image (item, dest_image);
}

static gboolean
ligma_item_real_rename (LigmaItem     *item,
                       const gchar  *new_name,
                       const gchar  *undo_desc,
                       GError      **error)
{
  if (ligma_item_is_attached (item))
    ligma_item_tree_rename_item (ligma_item_get_tree (item), item,
                                new_name, TRUE, undo_desc);
  else
    ligma_object_set_name (LIGMA_OBJECT (item), new_name);

  return TRUE;
}

static void
ligma_item_real_translate (LigmaItem *item,
                          gdouble   offset_x,
                          gdouble   offset_y,
                          gboolean  push_undo)
{
  LigmaItemPrivate *private = GET_PRIVATE (item);

  ligma_item_set_offset (item,
                        private->offset_x + SIGNED_ROUND (offset_x),
                        private->offset_y + SIGNED_ROUND (offset_y));
}

static void
ligma_item_real_start_transform (LigmaItem *item,
                                gboolean  push_undo)
{
  ligma_item_start_move (item, push_undo);
}

static void
ligma_item_real_end_transform (LigmaItem *item,
                                gboolean  push_undo)
{
  ligma_item_end_move (item, push_undo);
}

static void
ligma_item_real_scale (LigmaItem              *item,
                      gint                   new_width,
                      gint                   new_height,
                      gint                   new_offset_x,
                      gint                   new_offset_y,
                      LigmaInterpolationType  interpolation,
                      LigmaProgress          *progress)
{
  LigmaItemPrivate *private = GET_PRIVATE (item);

  if (private->width != new_width)
    {
      private->width = new_width;
      g_object_notify_by_pspec (G_OBJECT (item), ligma_item_props[PROP_WIDTH]);
    }

  if (private->height != new_height)
    {
      private->height = new_height;
      g_object_notify_by_pspec (G_OBJECT (item), ligma_item_props[PROP_HEIGHT]);
    }

  ligma_item_set_offset (item, new_offset_x, new_offset_y);
}

static void
ligma_item_real_resize (LigmaItem     *item,
                       LigmaContext  *context,
                       LigmaFillType  fill_type,
                       gint          new_width,
                       gint          new_height,
                       gint          offset_x,
                       gint          offset_y)
{
  LigmaItemPrivate *private = GET_PRIVATE (item);

  if (private->width != new_width)
    {
      private->width = new_width;
      g_object_notify_by_pspec (G_OBJECT (item), ligma_item_props[PROP_WIDTH]);
    }

  if (private->height != new_height)
    {
      private->height = new_height;
      g_object_notify_by_pspec (G_OBJECT (item), ligma_item_props[PROP_HEIGHT]);
    }

  ligma_item_set_offset (item,
                        private->offset_x - offset_x,
                        private->offset_y - offset_y);
}

static LigmaTransformResize
ligma_item_real_get_clip (LigmaItem            *item,
                         LigmaTransformResize  clip_result)
{
  if (ligma_item_get_lock_position (item))
    return LIGMA_TRANSFORM_RESIZE_CLIP;
  else
    return clip_result;
}


/*  public functions  */

/**
 * ligma_item_new:
 * @type:     The new item's type.
 * @image:    The new item's #LigmaImage.
 * @name:     The name to assign the item.
 * @offset_x: The X offset to assign the item.
 * @offset_y: The Y offset to assign the item.
 * @width:    The width to assign the item.
 * @height:   The height to assign the item.
 *
 * Returns: The newly created item.
 */
LigmaItem *
ligma_item_new (GType        type,
               LigmaImage   *image,
               const gchar *name,
               gint         offset_x,
               gint         offset_y,
               gint         width,
               gint         height)
{
  LigmaItem        *item;
  LigmaItemPrivate *private;

  g_return_val_if_fail (g_type_is_a (type, LIGMA_TYPE_ITEM), NULL);
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  item = g_object_new (type,
                       "image", image,
                       NULL);

  private = GET_PRIVATE (item);

  private->width  = width;
  private->height = height;
  ligma_item_set_offset (item, offset_x, offset_y);

  if (name && strlen (name))
    ligma_object_set_name (LIGMA_OBJECT (item), name);
  else
    ligma_object_set_static_name (LIGMA_OBJECT (item),
                                 LIGMA_ITEM_GET_CLASS (item)->default_name);

  return item;
}

/**
 * ligma_item_remove:
 * @item: the #LigmaItem to remove.
 *
 * This function sets the 'removed' flag on @item to %TRUE, and emits
 * a 'removed' signal on the item.
 */
void
ligma_item_removed (LigmaItem *item)
{
  LigmaContainer *children;

  g_return_if_fail (LIGMA_IS_ITEM (item));

  GET_PRIVATE (item)->removed = TRUE;

  children = ligma_viewable_get_children (LIGMA_VIEWABLE (item));

  if (children)
    ligma_container_foreach (children, (GFunc) ligma_item_removed, NULL);

  g_signal_emit (item, ligma_item_signals[REMOVED], 0);
}

/**
 * ligma_item_is_removed:
 * @item: the #LigmaItem to check.
 *
 * Returns: %TRUE if the 'removed' flag is set for @item, %FALSE otherwise.
 */
gboolean
ligma_item_is_removed (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  return GET_PRIVATE (item)->removed;
}

/**
 * ligma_item_unset_removed:
 * @item: a #LigmaItem which was on the undo stack
 *
 * Unsets an item's "removed" state. This function is called when an
 * item was on the undo stack and is added back to its parent
 * container during and undo or redo. It must never be called from
 * anywhere else.
 **/
void
ligma_item_unset_removed (LigmaItem *item)
{
  LigmaContainer *children;

  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (ligma_item_is_removed (item));

  GET_PRIVATE (item)->removed = FALSE;

  children = ligma_viewable_get_children (LIGMA_VIEWABLE (item));

  if (children)
    ligma_container_foreach (children, (GFunc) ligma_item_unset_removed, NULL);

  if (LIGMA_ITEM_GET_CLASS (item)->unset_removed)
    LIGMA_ITEM_GET_CLASS (item)->unset_removed (item);
}

/**
 * ligma_item_is_attached:
 * @item: The #LigmaItem to check.
 *
 * Returns: %TRUE if the item is attached to an image, %FALSE otherwise.
 */
gboolean
ligma_item_is_attached (LigmaItem *item)
{
  LigmaImage *image;
  LigmaItem  *parent;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  image = ligma_item_get_image (item);
  if (image != NULL && ligma_image_is_hidden_item (image, item))
    return TRUE;

  parent = ligma_item_get_parent (item);

  if (parent)
    return ligma_item_is_attached (parent);

  return LIGMA_ITEM_GET_CLASS (item)->is_attached (item);
}

LigmaItem *
ligma_item_get_parent (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);

  return LIGMA_ITEM (ligma_viewable_get_parent (LIGMA_VIEWABLE (item)));
}

LigmaItemTree *
ligma_item_get_tree (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);

  if (LIGMA_ITEM_GET_CLASS (item)->get_tree)
    return LIGMA_ITEM_GET_CLASS (item)->get_tree (item);

  return NULL;
}

LigmaContainer *
ligma_item_get_container (LigmaItem *item)
{
  LigmaItem     *parent;
  LigmaItemTree *tree;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);

  parent = ligma_item_get_parent (item);

  if (parent)
    return ligma_viewable_get_children (LIGMA_VIEWABLE (parent));

  tree = ligma_item_get_tree (item);

  if (tree)
    return tree->container;

  return NULL;
}

GList *
ligma_item_get_container_iter (LigmaItem *item)
{
  LigmaContainer *container;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);

  container = ligma_item_get_container (item);

  if (container)
    return LIGMA_LIST (container)->queue->head;

  return NULL;
}

gint
ligma_item_get_index (LigmaItem *item)
{
  LigmaContainer *container;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), -1);

  container = ligma_item_get_container (item);

  if (container)
    return ligma_container_get_child_index (container, LIGMA_OBJECT (item));

  return -1;
}

GList *
ligma_item_get_path (LigmaItem *item)
{
  LigmaContainer *container;
  GList         *path = NULL;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);
  g_return_val_if_fail (ligma_item_is_attached (item), NULL);

  container = ligma_item_get_container (item);

  while (container)
    {
      guint32 index = ligma_container_get_child_index (container,
                                                      LIGMA_OBJECT (item));

      path = g_list_prepend (path, GUINT_TO_POINTER (index));

      item = ligma_item_get_parent (item);

      if (item)
        container = ligma_item_get_container (item);
      else
        container = NULL;
    }

  return path;
}

gboolean
ligma_item_bounds (LigmaItem *item,
                  gint     *x,
                  gint     *y,
                  gint     *width,
                  gint     *height)
{
  gdouble  tmp_x, tmp_y, tmp_width, tmp_height;
  gboolean retval;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  retval = LIGMA_ITEM_GET_CLASS (item)->bounds (item,
                                               &tmp_x, &tmp_y,
                                               &tmp_width, &tmp_height);

  if (x)      *x      = floor (tmp_x);
  if (y)      *y      = floor (tmp_y);
  if (width)  *width  = ceil (tmp_x + tmp_width)  - floor (tmp_x);
  if (height) *height = ceil (tmp_y + tmp_height) - floor (tmp_y);

  return retval;
}

gboolean
ligma_item_bounds_f (LigmaItem *item,
                    gdouble  *x,
                    gdouble  *y,
                    gdouble  *width,
                    gdouble  *height)
{
  gdouble  tmp_x, tmp_y, tmp_width, tmp_height;
  gboolean retval;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  retval = LIGMA_ITEM_GET_CLASS (item)->bounds (item,
                                               &tmp_x, &tmp_y,
                                               &tmp_width, &tmp_height);

  if (x)      *x      = tmp_x;
  if (y)      *y      = tmp_y;
  if (width)  *width  = tmp_width;
  if (height) *height = tmp_height;

  return retval;
}

/**
 * ligma_item_duplicate:
 * @item:     The #LigmaItem to duplicate.
 * @new_type: The type to make the new item.
 *
 * Returns: the newly created item.
 */
LigmaItem *
ligma_item_duplicate (LigmaItem *item,
                     GType     new_type)
{
  LigmaItemPrivate *private;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);

  private = GET_PRIVATE (item);

  g_return_val_if_fail (LIGMA_IS_IMAGE (private->image), NULL);
  g_return_val_if_fail (g_type_is_a (new_type, LIGMA_TYPE_ITEM), NULL);

  return LIGMA_ITEM_GET_CLASS (item)->duplicate (item, new_type);
}

/**
 * ligma_item_convert:
 * @item:       The #LigmaItem to convert.
 * @dest_image: The #LigmaImage in which to place the converted item.
 * @new_type:   The type to convert the item to.
 *
 * Returns: the new item that results from the conversion.
 */
LigmaItem *
ligma_item_convert (LigmaItem  *item,
                   LigmaImage *dest_image,
                   GType      new_type)
{
  LigmaItem *new_item;
  GType     old_type;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);
  g_return_val_if_fail (LIGMA_IS_IMAGE (GET_PRIVATE (item)->image), NULL);
  g_return_val_if_fail (LIGMA_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (g_type_is_a (new_type, LIGMA_TYPE_ITEM), NULL);

  old_type = G_TYPE_FROM_INSTANCE (item);

  new_item = ligma_item_duplicate (item, new_type);

  if (new_item)
    LIGMA_ITEM_GET_CLASS (new_item)->convert (new_item, dest_image, old_type);

  return new_item;
}

/**
 * ligma_item_rename:
 * @item:     The #LigmaItem to rename.
 * @new_name: The new name to give the item.
 * @error:    Return location for error message.
 *
 * This function assigns a new name to the item, if the desired name is
 * different from the name it already has, and pushes an entry onto the
 * undo stack for the item's image.  If @new_name is NULL or empty, the
 * default name for the item's class is used.  If the name is changed,
 * the LigmaObject::name-changed signal is emitted for the item.
 *
 * Returns: %TRUE if the @item could be renamed, %FALSE otherwise.
 */
gboolean
ligma_item_rename (LigmaItem     *item,
                  const gchar  *new_name,
                  GError      **error)
{
  LigmaItemClass *item_class;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  item_class = LIGMA_ITEM_GET_CLASS (item);

  if (! new_name || ! *new_name)
    new_name = item_class->default_name;

  if (strcmp (new_name, ligma_object_get_name (item)))
    return item_class->rename (item, new_name, item_class->rename_desc, error);

  return TRUE;
}

/**
 * ligma_item_get_width:
 * @item: The #LigmaItem to check.
 *
 * Returns: The width of the item.
 */
gint
ligma_item_get_width (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), -1);

  return GET_PRIVATE (item)->width;
}

/**
 * ligma_item_get_height:
 * @item: The #LigmaItem to check.
 *
 * Returns: The height of the item.
 */
gint
ligma_item_get_height (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), -1);

  return GET_PRIVATE (item)->height;
}

void
ligma_item_set_size (LigmaItem *item,
                    gint      width,
                    gint      height)
{
  LigmaItemPrivate *private;

  g_return_if_fail (LIGMA_IS_ITEM (item));

  private = GET_PRIVATE (item);

  if (private->width  != width ||
      private->height != height)
    {
      g_object_freeze_notify (G_OBJECT (item));

      if (private->width != width)
        {
          private->width = width;
          g_object_notify_by_pspec (G_OBJECT (item),
                                    ligma_item_props[PROP_WIDTH]);
        }

      if (private->height != height)
        {
          private->height = height;
          g_object_notify_by_pspec (G_OBJECT (item),
                                    ligma_item_props[PROP_HEIGHT]);
        }

      g_object_thaw_notify (G_OBJECT (item));

      ligma_viewable_size_changed (LIGMA_VIEWABLE (item));
    }
}

/**
 * ligma_item_get_offset:
 * @item:     The #LigmaItem to check.
 * @offset_x: (out) (optional): Return location for the item's X offset.
 * @offset_y: (out) (optional): Return location for the item's Y offset.
 *
 * Reveals the X and Y offsets of the item.
 */
void
ligma_item_get_offset (LigmaItem *item,
                      gint     *offset_x,
                      gint     *offset_y)
{
  LigmaItemPrivate *private;

  g_return_if_fail (LIGMA_IS_ITEM (item));

  private = GET_PRIVATE (item);

  if (offset_x) *offset_x = private->offset_x;
  if (offset_y) *offset_y = private->offset_y;
}

void
ligma_item_set_offset (LigmaItem *item,
                      gint      offset_x,
                      gint      offset_y)
{
  LigmaItemPrivate *private;
  GList           *list;

  g_return_if_fail (LIGMA_IS_ITEM (item));

  private = GET_PRIVATE (item);

  g_object_freeze_notify (G_OBJECT (item));

  if (private->offset_x != offset_x)
    {
      private->offset_x = offset_x;
      g_object_notify_by_pspec (G_OBJECT (item),
                                ligma_item_props[PROP_OFFSET_X]);
    }

  if (private->offset_y != offset_y)
    {
      private->offset_y = offset_y;
      g_object_notify_by_pspec (G_OBJECT (item),
                                ligma_item_props[PROP_OFFSET_Y]);
    }

  for (list = private->offset_nodes; list; list = g_list_next (list))
    {
      GeglNode *node = list->data;

      gegl_node_set (node,
                     "x", (gdouble) private->offset_x,
                     "y", (gdouble) private->offset_y,
                     NULL);
    }

  g_object_thaw_notify (G_OBJECT (item));
}

gint
ligma_item_get_offset_x (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), 0);

  return GET_PRIVATE (item)->offset_x;
}

gint
ligma_item_get_offset_y (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), 0);

  return GET_PRIVATE (item)->offset_y;
}

void
ligma_item_start_move (LigmaItem *item,
                      gboolean  push_undo)
{
  g_return_if_fail (LIGMA_IS_ITEM (item));

  if (LIGMA_ITEM_GET_CLASS (item)->start_move)
    LIGMA_ITEM_GET_CLASS (item)->start_move (item, push_undo);
}

void
ligma_item_end_move (LigmaItem *item,
                    gboolean  push_undo)
{
  g_return_if_fail (LIGMA_IS_ITEM (item));

  if (LIGMA_ITEM_GET_CLASS (item)->end_move)
    LIGMA_ITEM_GET_CLASS (item)->end_move (item, push_undo);
}

void
ligma_item_start_transform (LigmaItem *item,
                           gboolean  push_undo)
{
  g_return_if_fail (LIGMA_IS_ITEM (item));

  if (LIGMA_ITEM_GET_CLASS (item)->start_transform)
    LIGMA_ITEM_GET_CLASS (item)->start_transform (item, push_undo);
}

void
ligma_item_end_transform (LigmaItem *item,
                         gboolean  push_undo)
{
  g_return_if_fail (LIGMA_IS_ITEM (item));

  if (LIGMA_ITEM_GET_CLASS (item)->end_transform)
    LIGMA_ITEM_GET_CLASS (item)->end_transform (item, push_undo);
}

/**
 * ligma_item_translate:
 * @item:      The #LigmaItem to move.
 * @offset_x:  Increment to the X offset of the item.
 * @offset_y:  Increment to the Y offset of the item.
 * @push_undo: If %TRUE, create an entry in the image's undo stack
 *             for this action.
 *
 * Adds the specified increments to the X and Y offsets for the item.
 */
void
ligma_item_translate (LigmaItem *item,
                     gdouble   offset_x,
                     gdouble   offset_y,
                     gboolean  push_undo)
{
  LigmaItemClass *item_class;
  LigmaImage     *image;

  g_return_if_fail (LIGMA_IS_ITEM (item));

  item_class = LIGMA_ITEM_GET_CLASS (item);
  image = ligma_item_get_image (item);

  if (! ligma_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_ITEM_DISPLACE,
                                 item_class->translate_desc);

  ligma_item_start_transform (item, push_undo);

  item_class->translate (item, offset_x, offset_y, push_undo);

  ligma_item_end_transform (item, push_undo);

  if (push_undo)
    ligma_image_undo_group_end (image);
}

/**
 * ligma_item_check_scaling:
 * @item:       Item to check
 * @new_width:  proposed width of item, in pixels
 * @new_height: proposed height of item, in pixels
 *
 * Scales item dimensions, then snaps them to pixel centers
 *
 * Returns: %FALSE if any dimension reduces to zero as a result
 *          of this; otherwise, returns %TRUE.
 **/
gboolean
ligma_item_check_scaling (LigmaItem *item,
                         gint      new_width,
                         gint      new_height)
{
  LigmaItemPrivate *private;
  LigmaImage       *image;
  gdouble          img_scale_w;
  gdouble          img_scale_h;
  gint             new_item_offset_x;
  gint             new_item_offset_y;
  gint             new_item_width;
  gint             new_item_height;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  private = GET_PRIVATE (item);
  image   = ligma_item_get_image (item);

  img_scale_w       = ((gdouble) new_width /
                       (gdouble) ligma_image_get_width (image));
  img_scale_h       = ((gdouble) new_height /
                       (gdouble) ligma_image_get_height (image));
  new_item_offset_x = SIGNED_ROUND (img_scale_w * private->offset_x);
  new_item_offset_y = SIGNED_ROUND (img_scale_h * private->offset_y);
  new_item_width    = SIGNED_ROUND (img_scale_w * (private->offset_x +
                                                   ligma_item_get_width (item))) -
                      new_item_offset_x;
  new_item_height   = SIGNED_ROUND (img_scale_h * (private->offset_y +
                                                   ligma_item_get_height (item))) -
                      new_item_offset_y;

  return (new_item_width > 0 && new_item_height > 0);
}

void
ligma_item_scale (LigmaItem              *item,
                 gint                   new_width,
                 gint                   new_height,
                 gint                   new_offset_x,
                 gint                   new_offset_y,
                 LigmaInterpolationType  interpolation,
                 LigmaProgress          *progress)
{
  LigmaItemClass *item_class;
  LigmaImage     *image;
  gboolean       push_undo;

  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  if (new_width < 1 || new_height < 1)
    return;

  item_class = LIGMA_ITEM_GET_CLASS (item);
  image = ligma_item_get_image (item);

  push_undo = ligma_item_is_attached (item);

  if (push_undo)
    ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_ITEM_SCALE,
                                 item_class->scale_desc);

  ligma_item_start_transform (item, push_undo);

  g_object_freeze_notify (G_OBJECT (item));

  item_class->scale (item, new_width, new_height, new_offset_x, new_offset_y,
                     interpolation, progress);

  g_object_thaw_notify (G_OBJECT (item));

  ligma_item_end_transform (item, push_undo);

  if (push_undo)
    ligma_image_undo_group_end (image);
}

/**
 * ligma_item_scale_by_factors:
 * @item:     Item to be transformed by explicit width and height factors.
 * @w_factor: scale factor to apply to width and horizontal offset
 * @h_factor: scale factor to apply to height and vertical offset
 * @interpolation:
 * @progress:
 *
 * Scales item dimensions and offsets by uniform width and
 * height factors.
 *
 * Use ligma_item_scale_by_factors() in circumstances when the same
 * width and height scaling factors are to be uniformly applied to a
 * set of items. In this context, the item's dimensions and offsets
 * from the sides of the containing image all change by these
 * predetermined factors. By fiat, the fixed point of the transform is
 * the upper left hand corner of the image. Returns %FALSE if a
 * requested scale factor is zero or if a scaling zero's out a item
 * dimension; returns %TRUE otherwise.
 *
 * Use ligma_item_scale() in circumstances where new item width
 * and height dimensions are predetermined instead.
 *
 * Side effects: Undo set created for item. Old item imagery
 *               scaled & painted to new item tiles.
 *
 * Returns: %TRUE, if the scaled item has positive dimensions
 *          %FALSE if the scaled item has at least one zero dimension
 **/
gboolean
ligma_item_scale_by_factors (LigmaItem              *item,
                            gdouble                w_factor,
                            gdouble                h_factor,
                            LigmaInterpolationType  interpolation,
                            LigmaProgress          *progress)
{
  return ligma_item_scale_by_factors_with_origin (item,
                                                 w_factor, h_factor,
                                                 0, 0, 0, 0,
                                                 interpolation, progress);
}

/**
 * ligma_item_scale_by_factors:
 * @item:         Item to be transformed by explicit width and height factors.
 * @w_factor:     scale factor to apply to width and horizontal offset
 * @h_factor:     scale factor to apply to height and vertical offset
 * @origin_x:     x-coordinate of the transformation input origin
 * @origin_y:     y-coordinate of the transformation input origin
 * @new_origin_x: x-coordinate of the transformation output origin
 * @new_origin_y: y-coordinate of the transformation output origin
 * @interpolation:
 * @progress:
 *
 * Same as ligma_item_scale_by_factors(), but with the option to specify
 * custom input and output points of origin for the transformation.
 *
 * Returns: %TRUE, if the scaled item has positive dimensions
 *          %FALSE if the scaled item has at least one zero dimension
 **/
gboolean
ligma_item_scale_by_factors_with_origin (LigmaItem              *item,
                                        gdouble                w_factor,
                                        gdouble                h_factor,
                                        gint                   origin_x,
                                        gint                   origin_y,
                                        gint                   new_origin_x,
                                        gint                   new_origin_y,
                                        LigmaInterpolationType  interpolation,
                                        LigmaProgress          *progress)
{
  LigmaItemPrivate *private;
  LigmaContainer   *children;
  gint             new_width, new_height;
  gint             new_offset_x, new_offset_y;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), FALSE);

  private = GET_PRIVATE (item);

  if (w_factor <= 0.0 || h_factor <= 0.0)
    {
      g_warning ("%s: requested width or height scale is non-positive",
                 G_STRFUNC);
      return FALSE;
    }

  children = ligma_viewable_get_children (LIGMA_VIEWABLE (item));

  /* avoid discarding empty layer groups */
  if (children && ligma_container_is_empty (children))
    return TRUE;

  new_offset_x = SIGNED_ROUND (w_factor * (private->offset_x - origin_x));
  new_offset_y = SIGNED_ROUND (h_factor * (private->offset_y - origin_y));
  new_width    = SIGNED_ROUND (w_factor * (private->offset_x - origin_x +
                                           ligma_item_get_width (item))) -
                 new_offset_x;
  new_height   = SIGNED_ROUND (h_factor * (private->offset_y - origin_y +
                                           ligma_item_get_height (item))) -
                 new_offset_y;

  new_offset_x += new_origin_x;
  new_offset_y += new_origin_y;

  if (new_width > 0 && new_height > 0)
    {
      ligma_item_scale (item,
                       new_width, new_height,
                       new_offset_x, new_offset_y,
                       interpolation, progress);
      return TRUE;
    }

  return FALSE;
}

/**
 * ligma_item_scale_by_origin:
 * @item:         The item to be transformed by width & height scale factors
 * @new_width:    The width that item will acquire
 * @new_height:   The height that the item will acquire
 * @interpolation:
 * @progress:
 * @local_origin: sets fixed point of the scaling transform. See below.
 *
 * Sets item dimensions to new_width and
 * new_height. Derives vertical and horizontal scaling
 * transforms from new width and height. If local_origin is
 * %TRUE, the fixed point of the scaling transform coincides
 * with the item's center point.  Otherwise, the fixed
 * point is taken to be [-LigmaItem::offset_x, -LigmaItem::->offset_y].
 *
 * Since this function derives scale factors from new and
 * current item dimensions, these factors will vary from
 * item to item because of aliasing artifacts; factor
 * variations among items can be quite large where item
 * dimensions approach pixel dimensions. Use
 * ligma_item_scale_by_factors() where constant scales are to
 * be uniformly applied to a number of items.
 *
 * Side effects: undo set created for item.
 *               Old item imagery scaled
 *               & painted to new item tiles
 **/
void
ligma_item_scale_by_origin (LigmaItem              *item,
                           gint                   new_width,
                           gint                   new_height,
                           LigmaInterpolationType  interpolation,
                           LigmaProgress          *progress,
                           gboolean               local_origin)
{
  LigmaItemPrivate *private;
  gint             new_offset_x, new_offset_y;

  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  private = GET_PRIVATE (item);

  if (new_width == 0 || new_height == 0)
    {
      g_warning ("%s: requested width or height equals zero", G_STRFUNC);
      return;
    }

  if (local_origin)
    {
      new_offset_x = (private->offset_x +
                      ((ligma_item_get_width  (item) - new_width)  / 2.0));
      new_offset_y = (private->offset_y +
                      ((ligma_item_get_height (item) - new_height) / 2.0));
    }
  else
    {
      new_offset_x = (gint) (((gdouble) new_width *
                              (gdouble) private->offset_x /
                              (gdouble) ligma_item_get_width (item)));

      new_offset_y = (gint) (((gdouble) new_height *
                              (gdouble) private->offset_y /
                              (gdouble) ligma_item_get_height (item)));
    }

  ligma_item_scale (item,
                   new_width, new_height,
                   new_offset_x, new_offset_y,
                   interpolation, progress);
}

void
ligma_item_resize (LigmaItem     *item,
                  LigmaContext  *context,
                  LigmaFillType  fill_type,
                  gint          new_width,
                  gint          new_height,
                  gint          offset_x,
                  gint          offset_y)
{
  LigmaItemClass *item_class;
  LigmaImage     *image;
  gboolean       push_undo;

  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  if (new_width < 1 || new_height < 1)
    return;

  item_class = LIGMA_ITEM_GET_CLASS (item);
  image = ligma_item_get_image (item);

  push_undo = ligma_item_is_attached (item);

  if (push_undo)
    ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_ITEM_RESIZE,
                                 item_class->resize_desc);

  /* note that we call ligma_item_start_move(), and not
   * ligma_item_start_transform().  whether or not a resize operation should be
   * considered a transform operation, or a move operation, depends on the
   * intended use of these functions by subclasses.  atm, we only use
   * ligma_item_{start,end}_transform() to suspend mask resizing in group
   * layers, which should not happen when reisizing a group, hence the call to
   * ligma_item_start_move().
   *
   * see the comment in ligma_group_layer_resize() for more information.
   */
  ligma_item_start_move (item, push_undo);

  g_object_freeze_notify (G_OBJECT (item));

  item_class->resize (item, context, fill_type,
                      new_width, new_height, offset_x, offset_y);

  g_object_thaw_notify (G_OBJECT (item));

  ligma_item_end_move (item, push_undo);

  if (push_undo)
    ligma_image_undo_group_end (image);
}

void
ligma_item_flip (LigmaItem            *item,
                LigmaContext         *context,
                LigmaOrientationType  flip_type,
                gdouble              axis,
                gboolean             clip_result)
{
  LigmaItemClass *item_class;
  LigmaImage     *image;
  gboolean       push_undo;

  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (ligma_item_is_attached (item));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  item_class = LIGMA_ITEM_GET_CLASS (item);
  image = ligma_item_get_image (item);

  push_undo = ligma_item_is_attached (item);

  if (push_undo)
    ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_TRANSFORM,
                                 item_class->flip_desc);

  ligma_item_start_transform (item, push_undo);

  g_object_freeze_notify (G_OBJECT (item));

  item_class->flip (item, context, flip_type, axis, clip_result);

  g_object_thaw_notify (G_OBJECT (item));

  ligma_item_end_transform (item, push_undo);

  if (push_undo)
    ligma_image_undo_group_end (image);
}

void
ligma_item_rotate (LigmaItem         *item,
                  LigmaContext      *context,
                  LigmaRotationType  rotate_type,
                  gdouble           center_x,
                  gdouble           center_y,
                  gboolean          clip_result)
{
  LigmaItemClass *item_class;
  LigmaImage     *image;
  gboolean       push_undo;

  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (ligma_item_is_attached (item));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  item_class = LIGMA_ITEM_GET_CLASS (item);
  image = ligma_item_get_image (item);

  push_undo = ligma_item_is_attached (item);

  if (push_undo)
    ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_TRANSFORM,
                                 item_class->rotate_desc);

  ligma_item_start_transform (item, push_undo);

  g_object_freeze_notify (G_OBJECT (item));

  item_class->rotate (item, context, rotate_type, center_x, center_y,
                      clip_result);

  g_object_thaw_notify (G_OBJECT (item));

  ligma_item_end_transform (item, push_undo);

  if (push_undo)
    ligma_image_undo_group_end (image);
}

void
ligma_item_transform (LigmaItem               *item,
                     LigmaContext            *context,
                     const LigmaMatrix3      *matrix,
                     LigmaTransformDirection  direction,
                     LigmaInterpolationType   interpolation,
                     LigmaTransformResize     clip_result,
                     LigmaProgress           *progress)
{
  LigmaItemClass *item_class;
  LigmaImage     *image;
  gboolean       push_undo;

  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (ligma_item_is_attached (item));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (matrix != NULL);
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  item_class = LIGMA_ITEM_GET_CLASS (item);
  image = ligma_item_get_image (item);

  push_undo = ligma_item_is_attached (item);

  if (push_undo)
    ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_TRANSFORM,
                                 item_class->transform_desc);

  ligma_item_start_transform (item, push_undo);

  g_object_freeze_notify (G_OBJECT (item));

  item_class->transform (item, context, matrix, direction, interpolation,
                         clip_result, progress);

  g_object_thaw_notify (G_OBJECT (item));

  ligma_item_end_transform (item, push_undo);

  if (push_undo)
    ligma_image_undo_group_end (image);
}

LigmaTransformResize
ligma_item_get_clip (LigmaItem            *item,
                    LigmaTransformResize  clip_result)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), LIGMA_TRANSFORM_RESIZE_ADJUST);

  return LIGMA_ITEM_GET_CLASS (item)->get_clip (item, clip_result);
}

gboolean
ligma_item_fill (LigmaItem        *item,
                GList           *drawables,
                LigmaFillOptions *fill_options,
                gboolean         push_undo,
                LigmaProgress    *progress,
                GError         **error)
{
  LigmaItemClass *item_class;
  GList         *iter;
  gboolean       retval = FALSE;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (ligma_item_is_attached (item), FALSE);
  g_return_val_if_fail (LIGMA_IS_FILL_OPTIONS (fill_options), FALSE);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  item_class = LIGMA_ITEM_GET_CLASS (item);

  for (iter = drawables; iter; iter = iter->next)
    {
      g_return_val_if_fail (LIGMA_IS_DRAWABLE (iter->data), FALSE);
      g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (iter->data)), FALSE);
    }

  if (item_class->fill)
    {
      LigmaImage *image = ligma_item_get_image (item);

      if (push_undo)
        ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_PAINT,
                                     item_class->fill_desc);

      for (iter = drawables; iter; iter = iter->next)
        {
          retval = item_class->fill (item, iter->data, fill_options,
                                     push_undo, progress, error);
          if (! retval)
            break;
        }

      if (push_undo)
        ligma_image_undo_group_end (image);
    }

  return retval;
}

gboolean
ligma_item_stroke (LigmaItem          *item,
                  GList             *drawables,
                  LigmaContext       *context,
                  LigmaStrokeOptions *stroke_options,
                  LigmaPaintOptions  *paint_options,
                  gboolean           push_undo,
                  LigmaProgress      *progress,
                  GError           **error)
{
  LigmaItemClass *item_class;
  GList         *iter;
  gboolean       retval = FALSE;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (ligma_item_is_attached (item), FALSE);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (LIGMA_IS_STROKE_OPTIONS (stroke_options), FALSE);
  g_return_val_if_fail (paint_options == NULL ||
                        LIGMA_IS_PAINT_OPTIONS (paint_options), FALSE);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  item_class = LIGMA_ITEM_GET_CLASS (item);

  for (iter = drawables; iter; iter = iter->next)
    {
      g_return_val_if_fail (LIGMA_IS_DRAWABLE (iter->data), FALSE);
      g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (iter->data)), FALSE);
    }

  if (item_class->stroke)
    {
      LigmaImage *image = ligma_item_get_image (item);

      ligma_stroke_options_prepare (stroke_options, context, paint_options);

      if (push_undo)
        ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_PAINT,
                                     item_class->stroke_desc);

      for (iter = drawables; iter; iter = iter->next)
        {
          retval = item_class->stroke (item, iter->data, stroke_options, push_undo,
                                       progress, error);
          if (! retval)
            break;
        }

      if (push_undo)
        ligma_image_undo_group_end (image);

      ligma_stroke_options_finish (stroke_options);
    }

  return retval;
}

void
ligma_item_to_selection (LigmaItem       *item,
                        LigmaChannelOps  op,
                        gboolean        antialias,
                        gboolean        feather,
                        gdouble         feather_radius_x,
                        gdouble         feather_radius_y)
{
  LigmaItemClass *item_class;

  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (ligma_item_is_attached (item));

  item_class = LIGMA_ITEM_GET_CLASS (item);

  if (item_class->to_selection)
    item_class->to_selection (item, op, antialias,
                              feather, feather_radius_x, feather_radius_y);
}

void
ligma_item_add_offset_node (LigmaItem *item,
                           GeglNode *node)
{
  LigmaItemPrivate *private;

  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (GEGL_IS_NODE (node));

  private = GET_PRIVATE (item);

  g_return_if_fail (g_list_find (private->offset_nodes, node) == NULL);

  gegl_node_set (node,
                 "x", (gdouble) private->offset_x,
                 "y", (gdouble) private->offset_y,
                 NULL);

  private->offset_nodes = g_list_append (private->offset_nodes,
                                         g_object_ref (node));
}

void
ligma_item_remove_offset_node (LigmaItem *item,
                              GeglNode *node)
{
  LigmaItemPrivate *private;

  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (GEGL_IS_NODE (node));

  private = GET_PRIVATE (item);

  g_return_if_fail (g_list_find (private->offset_nodes, node) != NULL);

  private->offset_nodes = g_list_remove (private->offset_nodes, node);
  g_object_unref (node);
}

gint
ligma_item_get_id (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), -1);

  return GET_PRIVATE (item)->ID;
}

LigmaItem *
ligma_item_get_by_id (Ligma *ligma,
                     gint  item_id)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  if (ligma->item_table == NULL)
    return NULL;

  return (LigmaItem *) ligma_id_table_lookup (ligma->item_table, item_id);
}

LigmaTattoo
ligma_item_get_tattoo (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), 0);

  return GET_PRIVATE (item)->tattoo;
}

void
ligma_item_set_tattoo (LigmaItem   *item,
                      LigmaTattoo  tattoo)
{
  g_return_if_fail (LIGMA_IS_ITEM (item));

  GET_PRIVATE (item)->tattoo = tattoo;
}

LigmaImage *
ligma_item_get_image (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);

  return GET_PRIVATE (item)->image;
}

void
ligma_item_set_image (LigmaItem  *item,
                     LigmaImage *image)
{
  LigmaItemPrivate *private;

  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (! ligma_item_is_attached (item));
  g_return_if_fail (! ligma_item_is_removed (item));
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = GET_PRIVATE (item);

  if (image == private->image)
    return;

  g_object_freeze_notify (G_OBJECT (item));

  if (private->ID == 0)
    {
      private->ID = ligma_id_table_insert (image->ligma->item_table, item);

      g_object_notify_by_pspec (G_OBJECT (item), ligma_item_props[PROP_ID]);
    }

  if (private->tattoo == 0 || private->image != image)
    {
      private->tattoo = ligma_image_get_new_tattoo (image);
    }

  private->image = image;
  g_object_notify_by_pspec (G_OBJECT (item), ligma_item_props[PROP_IMAGE]);

  g_object_thaw_notify (G_OBJECT (item));
}

/**
 * ligma_item_replace_item:
 * @item: a newly allocated #LigmaItem
 * @replace: the #LigmaItem to be replaced by @item
 *
 * This function shouly only be called right after @item has been
 * newly allocated.
 *
 * Replaces @replace by @item, as far as possible within the #LigmaItem
 * class. The new @item takes over @replace's ID, tattoo, offset, size
 * etc. and all these properties are set to %NULL on @replace.
 *
 * This function *only* exists to allow subclasses to do evil hacks
 * like in XCF text layer loading. Don't ever use this function if you
 * are not sure.
 *
 * After this function returns, @replace has become completely
 * unusable, should only be used to steal everything it has (like its
 * drawable properties if it's a drawable), and then be destroyed.
 **/
void
ligma_item_replace_item (LigmaItem *item,
                        LigmaItem *replace)
{
  LigmaItemPrivate *private;
  gint             offset_x;
  gint             offset_y;

  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (! ligma_item_is_attached (item));
  g_return_if_fail (! ligma_item_is_removed (item));
  g_return_if_fail (LIGMA_IS_ITEM (replace));

  private = GET_PRIVATE (item);

  ligma_object_set_name (LIGMA_OBJECT (item), ligma_object_get_name (replace));

  if (private->ID)
    ligma_id_table_remove (ligma_item_get_image (item)->ligma->item_table,
                          ligma_item_get_id (item));

  private->ID = ligma_item_get_id (replace);
  ligma_id_table_replace (ligma_item_get_image (item)->ligma->item_table,
                         ligma_item_get_id (item),
                         item);

  /* Set image before tattoo so that the explicitly set tattoo overrides
   * the one implicitly set when setting the image
   */
  ligma_item_set_image (item, ligma_item_get_image (replace));
  GET_PRIVATE (replace)->image  = NULL;

  ligma_item_set_tattoo (item, ligma_item_get_tattoo (replace));
  ligma_item_set_tattoo (replace, 0);

  g_object_unref (private->parasites);
  private->parasites = GET_PRIVATE (replace)->parasites;
  GET_PRIVATE (replace)->parasites = NULL;

  ligma_item_get_offset (replace, &offset_x, &offset_y);
  ligma_item_set_offset (item, offset_x, offset_y);

  ligma_item_set_size (item,
                      ligma_item_get_width  (replace),
                      ligma_item_get_height (replace));

  ligma_item_set_visible       (item, ligma_item_get_visible (replace), FALSE);
  ligma_item_set_color_tag     (item, ligma_item_get_color_tag (replace), FALSE);
  ligma_item_set_lock_content  (item, ligma_item_get_lock_content (replace), FALSE);
  ligma_item_set_lock_position (item, ligma_item_get_lock_position (replace), FALSE);
  ligma_item_set_lock_visibility (item, ligma_item_get_lock_visibility (replace), FALSE);
}

/**
 * ligma_item_set_parasites:
 * @item: a #LigmaItem
 * @parasites: a #LigmaParasiteList
 *
 * Set an @item's #LigmaParasiteList. It's usually never needed to
 * fiddle with an item's parasite list directly. This function exists
 * for special purposes only, like when creating items from unusual
 * sources.
 **/
void
ligma_item_set_parasites (LigmaItem         *item,
                         LigmaParasiteList *parasites)
{
  LigmaItemPrivate *private;

  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (LIGMA_IS_PARASITE_LIST (parasites));

  private = GET_PRIVATE (item);

  g_set_object (&private->parasites, parasites);
}

/**
 * ligma_item_get_parasites:
 * @item: a #LigmaItem
 *
 * Get an @item's #LigmaParasiteList. It's usually never needed to
 * fiddle with an item's parasite list directly. This function exists
 * for special purposes only, like when saving an item to XCF.
 *
 * Returns: The @item's #LigmaParasiteList.
 **/
LigmaParasiteList *
ligma_item_get_parasites (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);

  return GET_PRIVATE (item)->parasites;
}

gboolean
ligma_item_parasite_validate (LigmaItem            *item,
                             const LigmaParasite  *parasite,
                             GError             **error)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (parasite != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return TRUE;
}

void
ligma_item_parasite_attach (LigmaItem           *item,
                           const LigmaParasite *parasite,
                           gboolean            push_undo)
{
  LigmaItemPrivate *private;
  LigmaParasite     copy;

  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (parasite != NULL);

  private = GET_PRIVATE (item);

  /*  make a temporary copy of the LigmaParasite struct because
   *  ligma_parasite_shift_parent() changes it
   */
  copy = *parasite;

  if (! ligma_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    {
      /*  only set the dirty bit manually if we can be saved and the new
       *  parasite differs from the current one and we aren't undoable
       */
      if (ligma_parasite_is_undoable (&copy))
        {
          /* do a group in case we have attach_parent set */
          ligma_image_undo_group_start (private->image,
                                       LIGMA_UNDO_GROUP_PARASITE_ATTACH,
                                       C_("undo-type", "Attach Parasite"));

          ligma_image_undo_push_item_parasite (private->image, NULL, item, &copy);
        }
      else if (ligma_parasite_is_persistent (&copy) &&
               ! ligma_parasite_compare (&copy,
                                        ligma_item_parasite_find
                                        (item, ligma_parasite_get_name (&copy))))
        {
          ligma_image_undo_push_cantundo (private->image,
                                         C_("undo-type", "Attach Parasite to Item"));
        }
    }

  ligma_parasite_list_add (private->parasites, &copy);

  if (ligma_parasite_has_flag (&copy, LIGMA_PARASITE_ATTACH_PARENT))
    {
      ligma_parasite_shift_parent (&copy);
      ligma_image_parasite_attach (private->image, &copy, TRUE);
    }
  else if (ligma_parasite_has_flag (&copy, LIGMA_PARASITE_ATTACH_GRANDPARENT))
    {
      ligma_parasite_shift_parent (&copy);
      ligma_parasite_shift_parent (&copy);
      ligma_parasite_attach (private->image->ligma, &copy);
    }

  if (ligma_item_is_attached (item) &&
      ligma_parasite_is_undoable (&copy))
    {
      ligma_image_undo_group_end (private->image);
    }
}

void
ligma_item_parasite_detach (LigmaItem    *item,
                           const gchar *name,
                           gboolean     push_undo)
{
  LigmaItemPrivate    *private;
  const LigmaParasite *parasite;

  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (name != NULL);

  private = GET_PRIVATE (item);

  parasite = ligma_parasite_list_find (private->parasites, name);

  if (! parasite)
    return;

  if (! ligma_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    {
      if (ligma_parasite_is_undoable (parasite))
        {
          ligma_image_undo_push_item_parasite_remove (private->image,
                                                     C_("undo-type", "Remove Parasite from Item"),
                                                     item,
                                                     ligma_parasite_get_name (parasite));
        }
      else if (ligma_parasite_is_persistent (parasite))
        {
          ligma_image_undo_push_cantundo (private->image,
                                         C_("undo-type", "Remove Parasite from Item"));
        }
    }

  ligma_parasite_list_remove (private->parasites, name);
}

const LigmaParasite *
ligma_item_parasite_find (LigmaItem    *item,
                         const gchar *name)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);

  return ligma_parasite_list_find (GET_PRIVATE (item)->parasites, name);
}

static void
ligma_item_parasite_list_foreach_func (gchar          *name,
                                      LigmaParasite   *parasite,
                                      gchar        ***cur)
{
  *(*cur)++ = (gchar *) g_strdup (name);
}

gchar **
ligma_item_parasite_list (LigmaItem *item)
{
  LigmaItemPrivate  *private;
  gint              count;
  gchar           **list;
  gchar           **cur;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);

  private = GET_PRIVATE (item);

  count = ligma_parasite_list_length (private->parasites);

  cur = list = g_new0 (gchar *, count + 1);

  ligma_parasite_list_foreach (private->parasites,
                              (GHFunc) ligma_item_parasite_list_foreach_func,
                              &cur);

  return list;
}

gboolean
ligma_item_set_visible (LigmaItem *item,
                       gboolean  visible,
                       gboolean  push_undo)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  visible = visible ? TRUE : FALSE;

  if (ligma_item_get_visible (item) != visible)
    {
      if (ligma_item_is_visibility_locked (item, NULL))
        {
          return FALSE;
        }

      if (push_undo && ligma_item_is_attached (item))
        {
          LigmaImage *image = ligma_item_get_image (item);

          if (image)
            ligma_image_undo_push_item_visibility (image, NULL, item);
        }

      GET_PRIVATE (item)->visible = visible;

      if (GET_PRIVATE (item)->bind_visible_to_active)
        ligma_filter_set_active (LIGMA_FILTER (item), visible);

      g_signal_emit (item, ligma_item_signals[VISIBILITY_CHANGED], 0);

      g_object_notify_by_pspec (G_OBJECT (item), ligma_item_props[PROP_VISIBLE]);
    }

  return TRUE;
}

gboolean
ligma_item_get_visible (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  return GET_PRIVATE (item)->visible;
}

gboolean
ligma_item_is_visible (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  if (ligma_item_get_visible (item))
    {
      LigmaItem *parent;

      parent = LIGMA_ITEM (ligma_viewable_get_parent (LIGMA_VIEWABLE (item)));

      if (parent)
        return ligma_item_is_visible (parent);

      return TRUE;
    }

  return FALSE;
}

void
ligma_item_bind_visible_to_active (LigmaItem *item,
                                  gboolean  bind)
{
  g_return_if_fail (LIGMA_IS_ITEM (item));

  GET_PRIVATE (item)->bind_visible_to_active = bind;

  if (bind)
    ligma_filter_set_active (LIGMA_FILTER (item), ligma_item_get_visible (item));
}

void
ligma_item_set_color_tag (LigmaItem     *item,
                         LigmaColorTag  color_tag,
                         gboolean      push_undo)
{
  g_return_if_fail (LIGMA_IS_ITEM (item));

  if (ligma_item_get_color_tag (item) != color_tag)
    {
      if (push_undo && ligma_item_is_attached (item))
        {
          LigmaImage *image = ligma_item_get_image (item);

          if (image)
            ligma_image_undo_push_item_color_tag (image, NULL, item);
        }

      GET_PRIVATE (item)->color_tag = color_tag;

      g_signal_emit (item, ligma_item_signals[COLOR_TAG_CHANGED], 0);

      g_object_notify_by_pspec (G_OBJECT (item),
                                ligma_item_props[PROP_COLOR_TAG]);
    }
}

LigmaColorTag
ligma_item_get_color_tag (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), LIGMA_COLOR_TAG_NONE);

  return GET_PRIVATE (item)->color_tag;
}

LigmaColorTag
ligma_item_get_merged_color_tag (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), LIGMA_COLOR_TAG_NONE);

  if (ligma_item_get_color_tag (item) == LIGMA_COLOR_TAG_NONE)
    {
      LigmaItem *parent;

      parent = LIGMA_ITEM (ligma_viewable_get_parent (LIGMA_VIEWABLE (item)));

      if (parent)
        return ligma_item_get_merged_color_tag (parent);
    }

  return ligma_item_get_color_tag (item);
}

void
ligma_item_set_lock_content (LigmaItem *item,
                            gboolean  lock_content,
                            gboolean  push_undo)
{
  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (ligma_item_can_lock_content (item));

  lock_content = lock_content ? TRUE : FALSE;

  if (ligma_item_get_lock_content (item) != lock_content)
    {
      if (push_undo && ligma_item_is_attached (item))
        {
          /* Right now I don't think this should be pushed. */
#if 0
          LigmaImage *image = ligma_item_get_image (item);

          ligma_image_undo_push_item_lock_content (image, NULL, item);
#endif
        }

      GET_PRIVATE (item)->lock_content = lock_content;

      g_signal_emit (item, ligma_item_signals[LOCK_CONTENT_CHANGED], 0);

      g_object_notify_by_pspec (G_OBJECT (item),
                                ligma_item_props[PROP_LOCK_CONTENT]);
    }
}

gboolean
ligma_item_get_lock_content (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  return GET_PRIVATE (item)->lock_content;
}

gboolean
ligma_item_can_lock_content (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  return TRUE;
}

gboolean
ligma_item_is_content_locked (LigmaItem  *item,
                             LigmaItem **locked_item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  return LIGMA_ITEM_GET_CLASS (item)->is_content_locked (item, locked_item);
}

void
ligma_item_set_lock_position (LigmaItem *item,
                             gboolean  lock_position,
                             gboolean  push_undo)
{
  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (ligma_item_can_lock_position (item));

  lock_position = lock_position ? TRUE : FALSE;

  if (ligma_item_get_lock_position (item) != lock_position)
    {
      if (push_undo && ligma_item_is_attached (item))
        {
          LigmaImage *image = ligma_item_get_image (item);

          ligma_image_undo_push_item_lock_position (image, NULL, item);
        }

      GET_PRIVATE (item)->lock_position = lock_position;

      g_signal_emit (item, ligma_item_signals[LOCK_POSITION_CHANGED], 0);

      g_object_notify_by_pspec (G_OBJECT (item),
                                ligma_item_props[PROP_LOCK_POSITION]);
    }
}

gboolean
ligma_item_get_lock_position (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  return GET_PRIVATE (item)->lock_position;
}

gboolean
ligma_item_can_lock_position (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  return TRUE;
}

gboolean
ligma_item_is_position_locked (LigmaItem  *item,
                              LigmaItem **locked_item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  return LIGMA_ITEM_GET_CLASS (item)->is_position_locked (item, locked_item, TRUE);
}

void
ligma_item_set_lock_visibility (LigmaItem *item,
                               gboolean  lock_visibility,
                               gboolean  push_undo)
{
  g_return_if_fail (LIGMA_IS_ITEM (item));
  g_return_if_fail (ligma_item_can_lock_visibility (item));

  lock_visibility = lock_visibility ? TRUE : FALSE;

  if (ligma_item_get_lock_visibility (item) != lock_visibility)
    {
      if (push_undo && ligma_item_is_attached (item))
        {
          LigmaImage *image = ligma_item_get_image (item);

          ligma_image_undo_push_item_lock_visibility (image, NULL, item);
        }

      GET_PRIVATE (item)->lock_visibility = lock_visibility;

      g_signal_emit (item, ligma_item_signals[LOCK_VISIBILITY_CHANGED], 0);

      g_object_notify (G_OBJECT (item), "lock-visibility");
    }
}

gboolean
ligma_item_get_lock_visibility (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  return GET_PRIVATE (item)->lock_visibility;
}

gboolean
ligma_item_can_lock_visibility (LigmaItem *item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  return TRUE;
}

gboolean
ligma_item_is_visibility_locked (LigmaItem  *item,
                                LigmaItem **locked_item)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  return LIGMA_ITEM_GET_CLASS (item)->is_visibility_locked (item, locked_item);
}

gboolean
ligma_item_mask_bounds (LigmaItem *item,
                       gint     *x1,
                       gint     *y1,
                       gint     *x2,
                       gint     *y2)
{
  LigmaImage   *image;
  LigmaChannel *selection;
  gint         x, y, width, height;
  gboolean     retval;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (ligma_item_is_attached (item), FALSE);

  image     = ligma_item_get_image (item);
  selection = ligma_image_get_mask (image);

  /* check for is_empty() before intersecting so we ignore the
   * selection if it is suspended (like when stroking)
   */
  if (LIGMA_ITEM (selection) != item       &&
      ! ligma_channel_is_empty (selection) &&
      ligma_item_bounds (LIGMA_ITEM (selection), &x, &y, &width, &height))
    {
      gint off_x, off_y;
      gint x2, y2;

      ligma_item_get_offset (item, &off_x, &off_y);

      x2 = x + width;
      y2 = y + height;

      x  = CLAMP (x  - off_x, 0, ligma_item_get_width  (item));
      y  = CLAMP (y  - off_y, 0, ligma_item_get_height (item));
      x2 = CLAMP (x2 - off_x, 0, ligma_item_get_width  (item));
      y2 = CLAMP (y2 - off_y, 0, ligma_item_get_height (item));

      width  = x2 - x;
      height = y2 - y;

      retval = TRUE;
    }
  else
    {
      x      = 0;
      y      = 0;
      width  = ligma_item_get_width  (item);
      height = ligma_item_get_height (item);

      retval = FALSE;
    }

  if (x1) *x1 = x;
  if (y1) *y1 = y;
  if (x2) *x2 = x + width;
  if (y2) *y2 = y + height;

  return retval;
}

/**
 * ligma_item_mask_intersect:
 * @item:   a #LigmaItem
 * @x: (out) (optional): return location for x
 * @y: (out) (optional): return location for y
 * @width: (out) (optional): return location for the width
 * @height: (out) (optional): return location for the height
 *
 * Intersect the area of the @item and its image's selection mask.
 * The computed area is the bounding box of the selection within the
 * item.
 */
gboolean
ligma_item_mask_intersect (LigmaItem *item,
                          gint     *x,
                          gint     *y,
                          gint     *width,
                          gint     *height)
{
  LigmaImage   *image;
  LigmaChannel *selection;
  gint         tmp_x, tmp_y;
  gint         tmp_width, tmp_height;
  gboolean     retval;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (ligma_item_is_attached (item), FALSE);

  image     = ligma_item_get_image (item);
  selection = ligma_image_get_mask (image);

  /* check for is_empty() before intersecting so we ignore the
   * selection if it is suspended (like when stroking)
   */
  if (LIGMA_ITEM (selection) != item       &&
      ! ligma_channel_is_empty (selection) &&
      ligma_item_bounds (LIGMA_ITEM (selection),
                        &tmp_x, &tmp_y, &tmp_width, &tmp_height))
    {
      gint off_x, off_y;

      ligma_item_get_offset (item, &off_x, &off_y);

      retval = ligma_rectangle_intersect (tmp_x - off_x, tmp_y - off_y,
                                         tmp_width, tmp_height,
                                         0, 0,
                                         ligma_item_get_width  (item),
                                         ligma_item_get_height (item),
                                         &tmp_x, &tmp_y,
                                         &tmp_width, &tmp_height);
    }
  else
    {
      tmp_x      = 0;
      tmp_y      = 0;
      tmp_width  = ligma_item_get_width  (item);
      tmp_height = ligma_item_get_height (item);

      retval = TRUE;
    }

  if (x)      *x      = tmp_x;
  if (y)      *y      = tmp_y;
  if (width)  *width  = tmp_width;
  if (height) *height = tmp_height;

  return retval;
}

gboolean
ligma_item_is_in_set (LigmaItem    *item,
                     LigmaItemSet  set)
{
  LigmaItemPrivate *private;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);

  private = GET_PRIVATE (item);

  switch (set)
    {
    case LIGMA_ITEM_SET_NONE:
      return FALSE;

    case LIGMA_ITEM_SET_ALL:
      return TRUE;

    case LIGMA_ITEM_SET_IMAGE_SIZED:
      return (ligma_item_get_width  (item) == ligma_image_get_width  (private->image) &&
              ligma_item_get_height (item) == ligma_image_get_height (private->image));

    case LIGMA_ITEM_SET_VISIBLE:
      return ligma_item_get_visible (item);
    }

  return FALSE;
}
