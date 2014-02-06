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

#include "gimp.h"
#include "gimp-parasites.h"
#include "gimpchannel.h"
#include "gimpidtable.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimpitem.h"
#include "gimpitem-preview.h"
#include "gimpitemtree.h"
#include "gimplist.h"
#include "gimpmarshal.h"
#include "gimpparasitelist.h"
#include "gimpprogress.h"
#include "gimpstrokeoptions.h"

#include "paint/gimppaintoptions.h"

#include "gimp-intl.h"


enum
{
  REMOVED,
  VISIBILITY_CHANGED,
  LINKED_CHANGED,
  LOCK_CONTENT_CHANGED,
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
  PROP_LINKED,
  PROP_LOCK_CONTENT
};


typedef struct _GimpItemPrivate GimpItemPrivate;

struct _GimpItemPrivate
{
  gint              ID;                 /*  provides a unique ID     */
  guint32           tattoo;             /*  provides a permanent ID  */

  GimpImage        *image;              /*  item owner               */

  GimpParasiteList *parasites;          /*  Plug-in parasite data    */

  gint              width, height;      /*  size in pixels           */
  gint              offset_x, offset_y; /*  pixel offset in image    */

  guint             visible      : 1;   /*  control visibility       */
  guint             linked       : 1;   /*  control linkage          */
  guint             lock_content : 1;   /*  content editability      */

  guint             removed : 1;        /*  removed from the image?  */

  GeglNode         *node;               /*  the GEGL node to plug
                                            into the graph           */
  GeglNode         *offset_node;        /*  the offset as a node     */
};

#define GET_PRIVATE(item) G_TYPE_INSTANCE_GET_PRIVATE (item, \
                                                       GIMP_TYPE_ITEM, \
                                                       GimpItemPrivate)


/*  local function prototypes  */

static void       gimp_item_constructed             (GObject        *object);
static void       gimp_item_finalize                (GObject        *object);
static void       gimp_item_set_property            (GObject        *object,
                                                     guint           property_id,
                                                     const GValue   *value,
                                                     GParamSpec     *pspec);
static void       gimp_item_get_property            (GObject        *object,
                                                     guint           property_id,
                                                     GValue         *value,
                                                     GParamSpec     *pspec);

static gint64     gimp_item_get_memsize             (GimpObject     *object,
                                                     gint64         *gui_size);

static void       gimp_item_real_visibility_changed (GimpItem       *item);

static gboolean   gimp_item_real_is_content_locked  (const GimpItem *item);
static GimpItem * gimp_item_real_duplicate          (GimpItem       *item,
                                                     GType           new_type);
static void       gimp_item_real_convert            (GimpItem       *item,
                                                     GimpImage      *dest_image);
static gboolean   gimp_item_real_rename             (GimpItem       *item,
                                                     const gchar    *new_name,
                                                     const gchar    *undo_desc,
                                                     GError        **error);
static void       gimp_item_real_translate          (GimpItem       *item,
                                                     gint            offset_x,
                                                     gint            offset_y,
                                                     gboolean        push_undo);
static void       gimp_item_real_scale              (GimpItem       *item,
                                                     gint            new_width,
                                                     gint            new_height,
                                                     gint            new_offset_x,
                                                     gint            new_offset_y,
                                                     GimpInterpolationType interpolation,
                                                     GimpProgress   *progress);
static void       gimp_item_real_resize             (GimpItem       *item,
                                                     GimpContext    *context,
                                                     gint            new_width,
                                                     gint            new_height,
                                                     gint            offset_x,
                                                     gint            offset_y);
static GeglNode * gimp_item_real_get_node           (GimpItem       *item);
static void       gimp_item_sync_offset_node        (GimpItem       *item);


G_DEFINE_TYPE (GimpItem, gimp_item, GIMP_TYPE_VIEWABLE)

#define parent_class gimp_item_parent_class

static guint gimp_item_signals[LAST_SIGNAL] = { 0 };


static void
gimp_item_class_init (GimpItemClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);

  gimp_item_signals[REMOVED] =
    g_signal_new ("removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpItemClass, removed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_item_signals[VISIBILITY_CHANGED] =
    g_signal_new ("visibility-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpItemClass, visibility_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_item_signals[LINKED_CHANGED] =
    g_signal_new ("linked-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpItemClass, linked_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_item_signals[LOCK_CONTENT_CHANGED] =
    g_signal_new ("lock-content-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpItemClass, lock_content_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->constructed        = gimp_item_constructed;
  object_class->finalize           = gimp_item_finalize;
  object_class->set_property       = gimp_item_set_property;
  object_class->get_property       = gimp_item_get_property;

  gimp_object_class->get_memsize   = gimp_item_get_memsize;

  viewable_class->get_preview_size = gimp_item_get_preview_size;
  viewable_class->get_popup_size   = gimp_item_get_popup_size;

  klass->removed                   = NULL;
  klass->visibility_changed        = gimp_item_real_visibility_changed;
  klass->linked_changed            = NULL;
  klass->lock_content_changed      = NULL;

  klass->unset_removed             = NULL;
  klass->is_attached               = NULL;
  klass->is_content_locked         = gimp_item_real_is_content_locked;
  klass->get_tree                  = NULL;
  klass->duplicate                 = gimp_item_real_duplicate;
  klass->convert                   = gimp_item_real_convert;
  klass->rename                    = gimp_item_real_rename;
  klass->translate                 = gimp_item_real_translate;
  klass->scale                     = gimp_item_real_scale;
  klass->resize                    = gimp_item_real_resize;
  klass->flip                      = NULL;
  klass->rotate                    = NULL;
  klass->transform                 = NULL;
  klass->stroke                    = NULL;
  klass->to_selection              = NULL;
  klass->get_node                  = gimp_item_real_get_node;

  klass->default_name              = NULL;
  klass->rename_desc               = NULL;
  klass->translate_desc            = NULL;
  klass->scale_desc                = NULL;
  klass->resize_desc               = NULL;
  klass->flip_desc                 = NULL;
  klass->rotate_desc               = NULL;
  klass->transform_desc            = NULL;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image", NULL, NULL,
                                                        GIMP_TYPE_IMAGE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_int ("id", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_WIDTH,
                                   g_param_spec_int ("width", NULL, NULL,
                                                     1, GIMP_MAX_IMAGE_SIZE, 1,
                                                     GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", NULL, NULL,
                                                     1, GIMP_MAX_IMAGE_SIZE, 1,
                                                     GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_OFFSET_X,
                                   g_param_spec_int ("offset-x", NULL, NULL,
                                                     -GIMP_MAX_IMAGE_SIZE,
                                                     GIMP_MAX_IMAGE_SIZE, 0,
                                                     GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_OFFSET_Y,
                                   g_param_spec_int ("offset-y", NULL, NULL,
                                                     -GIMP_MAX_IMAGE_SIZE,
                                                     GIMP_MAX_IMAGE_SIZE, 0,
                                                     GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_VISIBLE,
                                   g_param_spec_boolean ("visible", NULL, NULL,
                                                         TRUE,
                                                         GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_LINKED,
                                   g_param_spec_boolean ("linked", NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_LOCK_CONTENT,
                                   g_param_spec_boolean ("lock-content",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READABLE));

  g_type_class_add_private (klass, sizeof (GimpItemPrivate));
}

static void
gimp_item_init (GimpItem *item)
{
  GimpItemPrivate *private = GET_PRIVATE (item);

  g_object_force_floating (G_OBJECT (item));

  private->ID           = 0;
  private->tattoo       = 0;
  private->image        = NULL;
  private->parasites    = gimp_parasite_list_new ();
  private->width        = 0;
  private->height       = 0;
  private->offset_x     = 0;
  private->offset_y     = 0;
  private->visible      = TRUE;
  private->linked       = FALSE;
  private->lock_content = FALSE;
  private->removed      = FALSE;
  private->node         = NULL;
  private->offset_node  = NULL;
}

static void
gimp_item_constructed (GObject *object)
{
  GimpItemPrivate *private = GET_PRIVATE (object);

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_IMAGE (private->image));
  g_assert (private->ID != 0);
}

static void
gimp_item_finalize (GObject *object)
{
  GimpItemPrivate *private = GET_PRIVATE (object);

  if (private->node)
    {
      g_object_unref (private->node);
      private->node = NULL;
    }

  if (private->image && private->image->gimp)
    {
      gimp_id_table_remove (private->image->gimp->item_table, private->ID);
      private->image = NULL;
    }

  if (private->parasites)
    {
      g_object_unref (private->parasites);
      private->parasites = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_item_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GimpItem *item = GIMP_ITEM (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      gimp_item_set_image (item, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_item_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GimpItemPrivate *private = GET_PRIVATE (object);

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
    case PROP_LINKED:
      g_value_set_boolean (value, private->linked);
      break;
    case PROP_LOCK_CONTENT:
      g_value_set_boolean (value, private->lock_content);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_item_get_memsize (GimpObject *object,
                       gint64     *gui_size)
{
  GimpItemPrivate *private = GET_PRIVATE (object);
  gint64           memsize = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (private->parasites),
                                      gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_item_real_visibility_changed (GimpItem *item)
{
  GimpItemPrivate *private = GET_PRIVATE (item);

  if (! private->node)
    return;

  if (gimp_item_get_visible (item))
    {
      /* Leave this up to subclasses */
    }
  else
    {
      GeglNode *input  = gegl_node_get_input_proxy  (private->node, "input");
      GeglNode *output = gegl_node_get_output_proxy (private->node, "output");

      gegl_node_connect_to (input,  "output",
                            output, "input");
    }
}

static gboolean
gimp_item_real_is_content_locked (const GimpItem *item)
{
  GimpItem *parent = gimp_item_get_parent (item);

  if (parent && gimp_item_is_content_locked (parent))
    return TRUE;

  return GET_PRIVATE (item)->lock_content;
}

static GimpItem *
gimp_item_real_duplicate (GimpItem *item,
                          GType     new_type)
{
  GimpItemPrivate *private;
  GimpItem        *new_item;
  gchar           *new_name;

  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);

  private = GET_PRIVATE (item);

  g_return_val_if_fail (GIMP_IS_IMAGE (private->image), NULL);
  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_ITEM), NULL);

  /*  formulate the new name  */
  {
    const gchar *name;
    gchar       *ext;
    gint         number;
    gint         len;

    name = gimp_object_get_name (item);

    g_return_val_if_fail (name != NULL, NULL);


    ext = strrchr (name, '#');
    len = strlen (_("copy"));

    if ((strlen (name) >= len &&
         strcmp (&name[strlen (name) - len], _("copy")) == 0) ||
        (ext && (number = atoi (ext + 1)) > 0 &&
         ((int)(log10 (number) + 1)) == strlen (ext + 1)))
      {
        /* don't have redundant "copy"s */
        new_name = g_strdup (name);
      }
    else
      {
        new_name = g_strdup_printf (_("%s copy"), name);
      }
  }

  new_item = gimp_item_new (new_type,
                            gimp_item_get_image (item), new_name,
                            private->offset_x, private->offset_y,
                            gimp_item_get_width  (item),
                            gimp_item_get_height (item));

  g_free (new_name);

  g_object_unref (GET_PRIVATE (new_item)->parasites);
  GET_PRIVATE (new_item)->parasites = gimp_parasite_list_copy (private->parasites);

  gimp_item_set_visible (new_item, gimp_item_get_visible (item), FALSE);
  gimp_item_set_linked  (new_item, gimp_item_get_linked (item),  FALSE);

  if (gimp_item_can_lock_content (new_item))
    gimp_item_set_lock_content (new_item, gimp_item_get_lock_content (item),
                                FALSE);

  return new_item;
}

static void
gimp_item_real_convert (GimpItem  *item,
                        GimpImage *dest_image)
{
  gimp_item_set_image (item, dest_image);
}

static gboolean
gimp_item_real_rename (GimpItem     *item,
                       const gchar  *new_name,
                       const gchar  *undo_desc,
                       GError      **error)
{
  if (gimp_item_is_attached (item))
    gimp_item_tree_rename_item (gimp_item_get_tree (item), item,
                                new_name, TRUE, undo_desc);
  else
    gimp_object_set_name (GIMP_OBJECT (item), new_name);

  return TRUE;
}

static void
gimp_item_real_translate (GimpItem *item,
                          gint      offset_x,
                          gint      offset_y,
                          gboolean  push_undo)
{
  GimpItemPrivate *private = GET_PRIVATE (item);

  gimp_item_set_offset (item,
                        private->offset_x + offset_x,
                        private->offset_y + offset_y);
}

static void
gimp_item_real_scale (GimpItem              *item,
                      gint                   new_width,
                      gint                   new_height,
                      gint                   new_offset_x,
                      gint                   new_offset_y,
                      GimpInterpolationType  interpolation,
                      GimpProgress          *progress)
{
  GimpItemPrivate *private = GET_PRIVATE (item);

  if (private->width != new_width)
    {
      private->width = new_width;
      g_object_notify (G_OBJECT (item), "width");
    }

  if (private->height != new_height)
    {
      private->height = new_height;
      g_object_notify (G_OBJECT (item), "height");
    }

  gimp_item_set_offset (item, new_offset_x, new_offset_y);
}

static void
gimp_item_real_resize (GimpItem    *item,
                       GimpContext *context,
                       gint         new_width,
                       gint         new_height,
                       gint         offset_x,
                       gint         offset_y)
{
  GimpItemPrivate *private = GET_PRIVATE (item);

  if (private->width != new_width)
    {
      private->width = new_width;
      g_object_notify (G_OBJECT (item), "width");
    }

  if (private->height != new_height)
    {
      private->height = new_height;
      g_object_notify (G_OBJECT (item), "height");
    }

  gimp_item_set_offset (item,
                        private->offset_x - offset_x,
                        private->offset_y - offset_y);
}

static GeglNode *
gimp_item_real_get_node (GimpItem *item)
{
  GimpItemPrivate *private = GET_PRIVATE (item);

  private->node = gegl_node_new ();

  return private->node;
}

static void
gimp_item_sync_offset_node (GimpItem *item)
{
  GimpItemPrivate *private = GET_PRIVATE (item);

  if (private->offset_node)
    gegl_node_set (private->offset_node,
                   "x", (gdouble) private->offset_x,
                   "y", (gdouble) private->offset_y,
                   NULL);
}


/*  public functions  */

/**
 * gimp_item_new:
 * @type:     The new item's type.
 * @image:    The new item's #GimpImage.
 * @name:     The name to assign the item.
 * @offset_x: The X offset to assign the item.
 * @offset_y: The Y offset to assign the item.
 * @width:    The width to assign the item.
 * @height:   The height to assign the item.
 *
 * Return value: The newly created item.
 */
GimpItem *
gimp_item_new (GType        type,
               GimpImage   *image,
               const gchar *name,
               gint         offset_x,
               gint         offset_y,
               gint         width,
               gint         height)
{
  GimpItem        *item;
  GimpItemPrivate *private;

  g_return_val_if_fail (g_type_is_a (type, GIMP_TYPE_ITEM), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  item = g_object_new (type,
                       "image", image,
                       NULL);

  private = GET_PRIVATE (item);

  private->width  = width;
  private->height = height;
  gimp_item_set_offset (item, offset_x, offset_y);

  if (name && strlen (name))
    gimp_object_set_name (GIMP_OBJECT (item), name);
  else
    gimp_object_set_static_name (GIMP_OBJECT (item),
                                 GIMP_ITEM_GET_CLASS (item)->default_name);

  return item;
}

/**
 * gimp_item_remove:
 * @item: the #GimpItem to remove.
 *
 * This function sets the 'removed' flag on @item to #TRUE, and emits
 * a 'removed' signal on the item.
 */
void
gimp_item_removed (GimpItem *item)
{
  GimpContainer *children;

  g_return_if_fail (GIMP_IS_ITEM (item));

  GET_PRIVATE (item)->removed = TRUE;

  children = gimp_viewable_get_children (GIMP_VIEWABLE (item));

  if (children)
    gimp_container_foreach (children, (GFunc) gimp_item_removed, NULL);

  g_signal_emit (item, gimp_item_signals[REMOVED], 0);
}

/**
 * gimp_item_is_removed:
 * @item: the #GimpItem to check.
 *
 * Returns: %TRUE if the 'removed' flag is set for @item, %FALSE otherwise.
 */
gboolean
gimp_item_is_removed (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  return GET_PRIVATE (item)->removed;
}

/**
 * gimp_item_unset_removed:
 * @item: a #GimpItem which was on the undo stack
 *
 * Unsets an item's "removed" state. This function is called when an
 * item was on the undo stack and is added back to its parent
 * container during and undo or redo. It must never be called from
 * anywhere else.
 **/
void
gimp_item_unset_removed (GimpItem *item)
{
  GimpContainer *children;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_is_removed (item));

  GET_PRIVATE (item)->removed = FALSE;

  children = gimp_viewable_get_children (GIMP_VIEWABLE (item));

  if (children)
    gimp_container_foreach (children, (GFunc) gimp_item_unset_removed, NULL);

  if (GIMP_ITEM_GET_CLASS (item)->unset_removed)
    GIMP_ITEM_GET_CLASS (item)->unset_removed (item);
}

/**
 * gimp_item_is_attached:
 * @item: The #GimpItem to check.
 *
 * Returns: %TRUE if the item is attached to an image, %FALSE otherwise.
 */
gboolean
gimp_item_is_attached (const GimpItem *item)
{
  GimpItem *parent;

  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  parent = gimp_item_get_parent (item);

  if (parent)
    return gimp_item_is_attached (parent);

  return GIMP_ITEM_GET_CLASS (item)->is_attached (item);
}

GimpItem *
gimp_item_get_parent (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);

  return GIMP_ITEM (gimp_viewable_get_parent (GIMP_VIEWABLE (item)));
}

GimpItemTree *
gimp_item_get_tree (GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);

  if (GIMP_ITEM_GET_CLASS (item)->get_tree)
    return GIMP_ITEM_GET_CLASS (item)->get_tree (item);

  return NULL;
}

GimpContainer *
gimp_item_get_container (GimpItem *item)
{
  GimpItem     *parent;
  GimpItemTree *tree;

  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);

  parent = gimp_item_get_parent (item);

  if (parent)
    return gimp_viewable_get_children (GIMP_VIEWABLE (parent));

  tree = gimp_item_get_tree (item);

  if (tree)
    return tree->container;

  return NULL;
}

GList *
gimp_item_get_container_iter (GimpItem *item)
{
  GimpContainer *container;

  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);

  container = gimp_item_get_container (item);

  if (container)
    return GIMP_LIST (container)->list;

  return NULL;
}

gint
gimp_item_get_index (GimpItem *item)
{
  GimpContainer *container;

  g_return_val_if_fail (GIMP_IS_ITEM (item), -1);

  container = gimp_item_get_container (item);

  if (container)
    return gimp_container_get_child_index (container, GIMP_OBJECT (item));

  return -1;
}

GList *
gimp_item_get_path (GimpItem *item)
{
  GimpContainer *container;
  GList         *path = NULL;

  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (gimp_item_is_attached (item), NULL);

  container = gimp_item_get_container (item);

  while (container)
    {
      guint32 index = gimp_container_get_child_index (container,
                                                      GIMP_OBJECT (item));

      path = g_list_prepend (path, GUINT_TO_POINTER (index));

      item = gimp_item_get_parent (item);

      if (item)
        container = gimp_item_get_container (item);
      else
        container = NULL;
    }

  return path;
}

/**
 * gimp_item_duplicate:
 * @item:     The #GimpItem to duplicate.
 * @new_type: The type to make the new item.
 *
 * Returns: the newly created item.
 */
GimpItem *
gimp_item_duplicate (GimpItem *item,
                     GType     new_type)
{
  GimpItemPrivate *private;

  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);

  private = GET_PRIVATE (item);

  g_return_val_if_fail (GIMP_IS_IMAGE (private->image), NULL);
  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_ITEM), NULL);

  return GIMP_ITEM_GET_CLASS (item)->duplicate (item, new_type);
}

/**
 * gimp_item_convert:
 * @item:       The #GimpItem to convert.
 * @dest_image: The #GimpImage in which to place the converted item.
 * @new_type:   The type to convert the item to.
 *
 * Returns: the new item that results from the conversion.
 */
GimpItem *
gimp_item_convert (GimpItem  *item,
                   GimpImage *dest_image,
                   GType      new_type)
{
  GimpItem *new_item;

  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (GET_PRIVATE (item)->image), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_ITEM), NULL);

  new_item = gimp_item_duplicate (item, new_type);

  if (new_item)
    GIMP_ITEM_GET_CLASS (new_item)->convert (new_item, dest_image);

  return new_item;
}

/**
 * gimp_item_rename:
 * @item:     The #GimpItem to rename.
 * @new_name: The new name to give the item.
 * @error:    Return location for error message.
 *
 * This function assigns a new name to the item, if the desired name is
 * different from the name it already has, and pushes an entry onto the
 * undo stack for the item's image.  If @new_name is NULL or empty, the
 * default name for the item's class is used.  If the name is changed,
 * the GimpObject::name-changed signal is emitted for the item.
 *
 * Returns: %TRUE if the @item could be renamed, %FALSE otherwise.
 */
gboolean
gimp_item_rename (GimpItem     *item,
                  const gchar  *new_name,
                  GError      **error)
{
  GimpItemClass *item_class;

  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  item_class = GIMP_ITEM_GET_CLASS (item);

  if (! new_name || ! *new_name)
    new_name = item_class->default_name;

  if (strcmp (new_name, gimp_object_get_name (item)))
    return item_class->rename (item, new_name, item_class->rename_desc, error);

  return TRUE;
}

/**
 * gimp_item_get_width:
 * @item: The #GimpItem to check.
 *
 * Returns: The width of the item.
 */
gint
gimp_item_get_width (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), -1);

  return GET_PRIVATE (item)->width;
}

/**
 * gimp_item_get_height:
 * @item: The #GimpItem to check.
 *
 * Returns: The height of the item.
 */
gint
gimp_item_get_height (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), -1);

  return GET_PRIVATE (item)->height;
}

void
gimp_item_set_size (GimpItem *item,
                    gint      width,
                    gint      height)
{
  GimpItemPrivate *private;

  g_return_if_fail (GIMP_IS_ITEM (item));

  private = GET_PRIVATE (item);

  if (private->width  != width ||
      private->height != height)
    {
      g_object_freeze_notify (G_OBJECT (item));

      if (private->width != width)
        {
          private->width = width;
          g_object_notify (G_OBJECT (item), "width");
        }

      if (private->height != height)
        {
          private->height = height;
          g_object_notify (G_OBJECT (item), "height");
        }

      g_object_thaw_notify (G_OBJECT (item));

      gimp_viewable_size_changed (GIMP_VIEWABLE (item));
    }
}

/**
 * gimp_item_get_offset:
 * @item:     The #GimpItem to check.
 * @offset_x: Return location for the item's X offset.
 * @offset_y: Return location for the item's Y offset.
 *
 * Reveals the X and Y offsets of the item.
 */
void
gimp_item_get_offset (const GimpItem *item,
                      gint           *offset_x,
                      gint           *offset_y)
{
  GimpItemPrivate *private;

  g_return_if_fail (GIMP_IS_ITEM (item));

  private = GET_PRIVATE (item);

  if (offset_x) *offset_x = private->offset_x;
  if (offset_y) *offset_y = private->offset_y;
}

void
gimp_item_set_offset (GimpItem *item,
                      gint      offset_x,
                      gint      offset_y)
{
  GimpItemPrivate *private;

  g_return_if_fail (GIMP_IS_ITEM (item));

  private = GET_PRIVATE (item);

  g_object_freeze_notify (G_OBJECT (item));

  if (private->offset_x != offset_x)
    {
      private->offset_x = offset_x;
      g_object_notify (G_OBJECT (item), "offset-x");
    }

  if (private->offset_y != offset_y)
    {
      private->offset_y = offset_y;
      g_object_notify (G_OBJECT (item), "offset-y");
    }

  gimp_item_sync_offset_node (item);

  g_object_thaw_notify (G_OBJECT (item));
}

gint
gimp_item_get_offset_x (GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), 0);

  return GET_PRIVATE (item)->offset_x;
}

gint
gimp_item_get_offset_y (GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), 0);

  return GET_PRIVATE (item)->offset_y;
}

/**
 * gimp_item_translate:
 * @item:      The #GimpItem to move.
 * @offset_x:  Increment to the X offset of the item.
 * @offset_y:  Increment to the Y offset of the item.
 * @push_undo: If #TRUE, create an entry in the image's undo stack
 *             for this action.
 *
 * Adds the specified increments to the X and Y offsets for the item.
 */
void
gimp_item_translate (GimpItem *item,
                     gint      offset_x,
                     gint      offset_y,
                     gboolean  push_undo)
{
  GimpItemClass *item_class;
  GimpImage     *image;

  g_return_if_fail (GIMP_IS_ITEM (item));

  item_class = GIMP_ITEM_GET_CLASS (item);
  image = gimp_item_get_image (item);

  if (! gimp_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_DISPLACE,
                                 item_class->translate_desc);

  item_class->translate (item, offset_x, offset_y, push_undo);

  if (push_undo)
    gimp_image_undo_group_end (image);
}

/**
 * gimp_item_check_scaling:
 * @item:       Item to check
 * @new_width:  proposed width of item, in pixels
 * @new_height: proposed height of item, in pixels
 *
 * Scales item dimensions, then snaps them to pixel centers
 *
 * Returns: #FALSE if any dimension reduces to zero as a result
 *          of this; otherwise, returns #TRUE.
 **/
gboolean
gimp_item_check_scaling (const GimpItem *item,
                         gint            new_width,
                         gint            new_height)
{
  GimpImage *image;
  gdouble    img_scale_w;
  gdouble    img_scale_h;
  gint       new_item_width;
  gint       new_item_height;

  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  image = gimp_item_get_image (item);

  img_scale_w     = ((gdouble) new_width /
                     (gdouble) gimp_image_get_width (image));
  img_scale_h     = ((gdouble) new_height /
                     (gdouble) gimp_image_get_height (image));
  new_item_width  = ROUND (img_scale_w * (gdouble) gimp_item_get_width  (item));
  new_item_height = ROUND (img_scale_h * (gdouble) gimp_item_get_height (item));

  return (new_item_width > 0 && new_item_height > 0);
}

void
gimp_item_scale (GimpItem              *item,
                 gint                   new_width,
                 gint                   new_height,
                 gint                   new_offset_x,
                 gint                   new_offset_y,
                 GimpInterpolationType  interpolation,
                 GimpProgress          *progress)
{
  GimpItemClass *item_class;
  GimpImage     *image;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  if (new_width < 1 || new_height < 1)
    return;

  item_class = GIMP_ITEM_GET_CLASS (item);
  image = gimp_item_get_image (item);

  if (gimp_item_is_attached (item))
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_SCALE,
                                 item_class->scale_desc);

  g_object_freeze_notify (G_OBJECT (item));

  item_class->scale (item, new_width, new_height, new_offset_x, new_offset_y,
                     interpolation, progress);

  g_object_thaw_notify (G_OBJECT (item));

  if (gimp_item_is_attached (item))
    gimp_image_undo_group_end (image);
}

/**
 * gimp_item_scale_by_factors:
 * @item:     Item to be transformed by explicit width and height factors.
 * @w_factor: scale factor to apply to width and horizontal offset
 * @h_factor: scale factor to apply to height and vertical offset
 * @interpolation:
 * @progress:
 *
 * Scales item dimensions and offsets by uniform width and
 * height factors.
 *
 * Use gimp_item_scale_by_factors() in circumstances when the same
 * width and height scaling factors are to be uniformly applied to a
 * set of items. In this context, the item's dimensions and offsets
 * from the sides of the containing image all change by these
 * predetermined factors. By fiat, the fixed point of the transform is
 * the upper left hand corner of the image. Returns #FALSE if a
 * requested scale factor is zero or if a scaling zero's out a item
 * dimension; returns #TRUE otherwise.
 *
 * Use gimp_item_scale() in circumstances where new item width
 * and height dimensions are predetermined instead.
 *
 * Side effects: Undo set created for item. Old item imagery
 *               scaled & painted to new item tiles.
 *
 * Returns: #TRUE, if the scaled item has positive dimensions
 *          #FALSE if the scaled item has at least one zero dimension
 **/
gboolean
gimp_item_scale_by_factors (GimpItem              *item,
                            gdouble                w_factor,
                            gdouble                h_factor,
                            GimpInterpolationType  interpolation,
                            GimpProgress          *progress)
{
  GimpItemPrivate *private;
  gint             new_width, new_height;
  gint             new_offset_x, new_offset_y;

  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);

  private = GET_PRIVATE (item);

  if (w_factor == 0.0 || h_factor == 0.0)
    {
      g_warning ("%s: requested width or height scale equals zero", G_STRFUNC);
      return FALSE;
    }

  new_offset_x = SIGNED_ROUND (w_factor * (gdouble) private->offset_x);
  new_offset_y = SIGNED_ROUND (h_factor * (gdouble) private->offset_y);
  new_width    = ROUND (w_factor * (gdouble) gimp_item_get_width  (item));
  new_height   = ROUND (h_factor * (gdouble) gimp_item_get_height (item));

  if (new_width != 0 && new_height != 0)
    {
      gimp_item_scale (item,
                       new_width, new_height,
                       new_offset_x, new_offset_y,
                       interpolation, progress);
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_item_scale_by_origin:
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
 * #TRUE, the fixed point of the scaling transform coincides
 * with the item's center point.  Otherwise, the fixed
 * point is taken to be [-GimpItem::offset_x, -GimpItem::->offset_y].
 *
 * Since this function derives scale factors from new and
 * current item dimensions, these factors will vary from
 * item to item because of aliasing artifacts; factor
 * variations among items can be quite large where item
 * dimensions approach pixel dimensions. Use
 * gimp_item_scale_by_factors() where constant scales are to
 * be uniformly applied to a number of items.
 *
 * Side effects: undo set created for item.
 *               Old item imagery scaled
 *               & painted to new item tiles
 **/
void
gimp_item_scale_by_origin (GimpItem              *item,
                           gint                   new_width,
                           gint                   new_height,
                           GimpInterpolationType  interpolation,
                           GimpProgress          *progress,
                           gboolean               local_origin)
{
  GimpItemPrivate *private;
  gint             new_offset_x, new_offset_y;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  private = GET_PRIVATE (item);

  if (new_width == 0 || new_height == 0)
    {
      g_warning ("%s: requested width or height equals zero", G_STRFUNC);
      return;
    }

  if (local_origin)
    {
      new_offset_x = (private->offset_x +
                      ((gimp_item_get_width  (item) - new_width)  / 2.0));
      new_offset_y = (private->offset_y +
                      ((gimp_item_get_height (item) - new_height) / 2.0));
    }
  else
    {
      new_offset_x = (gint) (((gdouble) new_width *
                              (gdouble) private->offset_x /
                              (gdouble) gimp_item_get_width (item)));

      new_offset_y = (gint) (((gdouble) new_height *
                              (gdouble) private->offset_y /
                              (gdouble) gimp_item_get_height (item)));
    }

  gimp_item_scale (item,
                   new_width, new_height,
                   new_offset_x, new_offset_y,
                   interpolation, progress);
}

void
gimp_item_resize (GimpItem    *item,
                  GimpContext *context,
                  gint         new_width,
                  gint         new_height,
                  gint         offset_x,
                  gint         offset_y)
{
  GimpItemClass *item_class;
  GimpImage     *image;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  if (new_width < 1 || new_height < 1)
    return;

  item_class = GIMP_ITEM_GET_CLASS (item);
  image = gimp_item_get_image (item);

  if (gimp_item_is_attached (item))
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_RESIZE,
                                 item_class->resize_desc);

  g_object_freeze_notify (G_OBJECT (item));

  item_class->resize (item, context, new_width, new_height, offset_x, offset_y);

  g_object_thaw_notify (G_OBJECT (item));

  if (gimp_item_is_attached (item))
    gimp_image_undo_group_end (image);
}

void
gimp_item_flip (GimpItem            *item,
                GimpContext         *context,
                GimpOrientationType  flip_type,
                gdouble              axis,
                gboolean             clip_result)
{
  GimpItemClass *item_class;
  GimpImage     *image;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_is_attached (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  item_class = GIMP_ITEM_GET_CLASS (item);
  image = gimp_item_get_image (item);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TRANSFORM,
                               item_class->flip_desc);

  g_object_freeze_notify (G_OBJECT (item));

  item_class->flip (item, context, flip_type, axis, clip_result);

  g_object_thaw_notify (G_OBJECT (item));

  gimp_image_undo_group_end (image);
}

void
gimp_item_rotate (GimpItem         *item,
                  GimpContext      *context,
                  GimpRotationType  rotate_type,
                  gdouble           center_x,
                  gdouble           center_y,
                  gboolean          clip_result)
{
  GimpItemClass *item_class;
  GimpImage     *image;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_is_attached (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  item_class = GIMP_ITEM_GET_CLASS (item);
  image = gimp_item_get_image (item);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TRANSFORM,
                               item_class->rotate_desc);

  g_object_freeze_notify (G_OBJECT (item));

  item_class->rotate (item, context, rotate_type, center_x, center_y,
                      clip_result);

  g_object_thaw_notify (G_OBJECT (item));

  gimp_image_undo_group_end (image);
}

void
gimp_item_transform (GimpItem               *item,
                     GimpContext            *context,
                     const GimpMatrix3      *matrix,
                     GimpTransformDirection  direction,
                     GimpInterpolationType   interpolation,
                     gint                    recursion_level,
                     GimpTransformResize     clip_result,
                     GimpProgress           *progress)
{
  GimpItemClass *item_class;
  GimpImage     *image;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_is_attached (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (matrix != NULL);
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  item_class = GIMP_ITEM_GET_CLASS (item);
  image = gimp_item_get_image (item);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TRANSFORM,
                               item_class->transform_desc);

  g_object_freeze_notify (G_OBJECT (item));

  item_class->transform (item, context, matrix, direction, interpolation,
                         recursion_level, clip_result, progress);

  g_object_thaw_notify (G_OBJECT (item));

  gimp_image_undo_group_end (image);
}

gboolean
gimp_item_stroke (GimpItem          *item,
                  GimpDrawable      *drawable,
                  GimpContext       *context,
                  GimpStrokeOptions *stroke_options,
                  GimpPaintOptions  *paint_options,
                  gboolean           push_undo,
                  GimpProgress      *progress,
                  GError           **error)
{
  GimpItemClass *item_class;
  gboolean       retval = FALSE;

  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (item), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (GIMP_IS_STROKE_OPTIONS (stroke_options), FALSE);
  g_return_val_if_fail (paint_options == NULL ||
                        GIMP_IS_PAINT_OPTIONS (paint_options), FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  item_class = GIMP_ITEM_GET_CLASS (item);

  if (item_class->stroke)
    {
      GimpImage *image = gimp_item_get_image (item);

      gimp_stroke_options_prepare (stroke_options, context, paint_options);

      if (push_undo)
        gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_PAINT,
                                     item_class->stroke_desc);

      retval = item_class->stroke (item, drawable, stroke_options, push_undo,
                                   progress, error);

      if (push_undo)
        gimp_image_undo_group_end (image);

      gimp_stroke_options_finish (stroke_options);
    }

  return retval;
}

void
gimp_item_to_selection (GimpItem       *item,
                        GimpChannelOps  op,
                        gboolean        antialias,
                        gboolean        feather,
                        gdouble         feather_radius_x,
                        gdouble         feather_radius_y)
{
  GimpItemClass *item_class;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_is_attached (item));

  item_class = GIMP_ITEM_GET_CLASS (item);

  if (item_class->to_selection)
    item_class->to_selection (item, op, antialias,
                              feather, feather_radius_x, feather_radius_y);
}

GeglNode *
gimp_item_get_node (GimpItem *item)
{
  GimpItemPrivate *private;

  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);

  private = GET_PRIVATE (item);

  if (private->node)
    return private->node;

  return GIMP_ITEM_GET_CLASS (item)->get_node (item);
}

GeglNode *
gimp_item_peek_node (GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);

  return GET_PRIVATE (item)->node;
}

GeglNode *
gimp_item_get_offset_node (GimpItem *item)
{
  GimpItemPrivate *private;

  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);

  private = GET_PRIVATE (item);

  if (! private->offset_node)
    {
      GeglNode *node = gimp_item_get_node (item);

      private->offset_node =
        gegl_node_new_child (node,
                             "operation", "gegl:translate",
                             "x",         (gdouble) private->offset_x,
                             "y",         (gdouble) private->offset_y,
                             NULL);
    }

  return private->offset_node;
}

gint
gimp_item_get_ID (GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), -1);

  return GET_PRIVATE (item)->ID;
}

GimpItem *
gimp_item_get_by_ID (Gimp *gimp,
                     gint  item_id)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (gimp->item_table == NULL)
    return NULL;

  return (GimpItem *) gimp_id_table_lookup (gimp->item_table, item_id);
}

GimpTattoo
gimp_item_get_tattoo (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), 0);

  return GET_PRIVATE (item)->tattoo;
}

void
gimp_item_set_tattoo (GimpItem   *item,
                      GimpTattoo  tattoo)
{
  g_return_if_fail (GIMP_IS_ITEM (item));

  GET_PRIVATE (item)->tattoo = tattoo;
}

GimpImage *
gimp_item_get_image (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);

  return GET_PRIVATE (item)->image;
}

void
gimp_item_set_image (GimpItem  *item,
                     GimpImage *image)
{
  GimpItemPrivate *private;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (! gimp_item_is_attached (item));
  g_return_if_fail (! gimp_item_is_removed (item));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GET_PRIVATE (item);

  if (image == private->image)
    return;

  g_object_freeze_notify (G_OBJECT (item));

  if (private->ID == 0)
    {
      private->ID = gimp_id_table_insert (image->gimp->item_table, item);

      g_object_notify (G_OBJECT (item), "id");
    }

  if (private->tattoo == 0 || private->image != image)
    {
      private->tattoo = gimp_image_get_new_tattoo (image);
    }

  private->image = image;
  g_object_notify (G_OBJECT (item), "image");

  g_object_thaw_notify (G_OBJECT (item));
}

/**
 * gimp_item_replace_item:
 * @item: a newly allocated #GimpItem
 * @replace: the #GimpItem to be replaced by @item
 *
 * This function shouly only be called right after @item has been
 * newly allocated.
 *
 * Replaces @replace by @item, as far as possible within the #GimpItem
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
gimp_item_replace_item (GimpItem *item,
                        GimpItem *replace)
{
  GimpItemPrivate *private;
  gint             offset_x;
  gint             offset_y;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (! gimp_item_is_attached (item));
  g_return_if_fail (! gimp_item_is_removed (item));
  g_return_if_fail (GIMP_IS_ITEM (replace));

  private = GET_PRIVATE (item);

  gimp_object_set_name (GIMP_OBJECT (item), gimp_object_get_name (replace));

  if (private->ID)
    gimp_id_table_remove (gimp_item_get_image (item)->gimp->item_table,
                          gimp_item_get_ID (item));

  private->ID = gimp_item_get_ID (replace);
  gimp_id_table_replace (gimp_item_get_image (item)->gimp->item_table,
                         gimp_item_get_ID (item),
                         item);

  /* Set image before tatoo so that the explicitly set tatoo overrides
   * the one implicitly set when setting the image
   */
  gimp_item_set_image (item, gimp_item_get_image (replace));
  GET_PRIVATE (replace)->image  = NULL;

  gimp_item_set_tattoo (item, gimp_item_get_tattoo (replace));
  gimp_item_set_tattoo (replace, 0);

  g_object_unref (private->parasites);
  private->parasites = GET_PRIVATE (replace)->parasites;
  GET_PRIVATE (replace)->parasites = NULL;

  gimp_item_get_offset (replace, &offset_x, &offset_y);
  gimp_item_set_offset (item, offset_x, offset_y);

  gimp_item_set_size (item,
                      gimp_item_get_width  (replace),
                      gimp_item_get_height (replace));

  gimp_item_set_visible      (item, gimp_item_get_visible (replace), FALSE);
  gimp_item_set_linked       (item, gimp_item_get_linked (replace), FALSE);
  gimp_item_set_lock_content (item, gimp_item_get_lock_content (replace), FALSE);
}

/**
 * gimp_item_set_parasites:
 * @item: a #GimpItem
 * @parasites: a #GimpParasiteList
 *
 * Set an @item's #GimpParasiteList. It's usually never needed to
 * fiddle with an item's parasite list directly. This function exists
 * for special purposes only, like when creating items from unusual
 * sources.
 **/
void
gimp_item_set_parasites (GimpItem         *item,
                         GimpParasiteList *parasites)
{
  GimpItemPrivate *private;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (GIMP_IS_PARASITE_LIST (parasites));

  private = GET_PRIVATE (item);

  if (parasites != private->parasites)
    {
      g_object_unref (private->parasites);
      private->parasites = g_object_ref (parasites);
    }
}

/**
 * gimp_item_get_parasites:
 * @item: a #GimpItem
 *
 * Get an @item's #GimpParasiteList. It's usually never needed to
 * fiddle with an item's parasite list directly. This function exists
 * for special purposes only, like when saving an item to XCF.
 *
 * Return value: The @item's #GimpParasiteList.
 **/
GimpParasiteList *
gimp_item_get_parasites (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);

  return GET_PRIVATE (item)->parasites;
}

void
gimp_item_parasite_attach (GimpItem           *item,
                           const GimpParasite *parasite,
                           gboolean            push_undo)
{
  GimpItemPrivate *private;
  GimpParasite     copy;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (parasite != NULL);

  private = GET_PRIVATE (item);

  /*  make a temporary copy of the GimpParasite struct because
   *  gimp_parasite_shift_parent() changes it
   */
  copy = *parasite;

  if (! gimp_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    {
      /*  only set the dirty bit manually if we can be saved and the new
       *  parasite differs from the current one and we aren't undoable
       */
      if (gimp_parasite_is_undoable (&copy))
        {
          /* do a group in case we have attach_parent set */
          gimp_image_undo_group_start (private->image,
                                       GIMP_UNDO_GROUP_PARASITE_ATTACH,
                                       C_("undo-type", "Attach Parasite"));

          gimp_image_undo_push_item_parasite (private->image, NULL, item, &copy);
        }
      else if (gimp_parasite_is_persistent (&copy) &&
               ! gimp_parasite_compare (&copy,
                                        gimp_item_parasite_find
                                        (item, gimp_parasite_name (&copy))))
        {
          gimp_image_undo_push_cantundo (private->image,
                                         C_("undo-type", "Attach Parasite to Item"));
        }
    }

  gimp_parasite_list_add (private->parasites, &copy);

  if (gimp_parasite_has_flag (&copy, GIMP_PARASITE_ATTACH_PARENT))
    {
      gimp_parasite_shift_parent (&copy);
      gimp_image_parasite_attach (private->image, &copy);
    }
  else if (gimp_parasite_has_flag (&copy, GIMP_PARASITE_ATTACH_GRANDPARENT))
    {
      gimp_parasite_shift_parent (&copy);
      gimp_parasite_shift_parent (&copy);
      gimp_parasite_attach (private->image->gimp, &copy);
    }

  if (gimp_item_is_attached (item) &&
      gimp_parasite_is_undoable (&copy))
    {
      gimp_image_undo_group_end (private->image);
    }
}

void
gimp_item_parasite_detach (GimpItem    *item,
                           const gchar *name,
                           gboolean     push_undo)
{
  GimpItemPrivate    *private;
  const GimpParasite *parasite;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (name != NULL);

  private = GET_PRIVATE (item);

  parasite = gimp_parasite_list_find (private->parasites, name);

  if (! parasite)
    return;

  if (! gimp_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    {
      if (gimp_parasite_is_undoable (parasite))
        {
          gimp_image_undo_push_item_parasite_remove (private->image,
                                                     C_("undo-type", "Remove Parasite from Item"),
                                                     item,
                                                     gimp_parasite_name (parasite));
        }
      else if (gimp_parasite_is_persistent (parasite))
        {
          gimp_image_undo_push_cantundo (private->image,
                                         C_("undo-type", "Remove Parasite from Item"));
        }
    }

  gimp_parasite_list_remove (private->parasites, name);
}

const GimpParasite *
gimp_item_parasite_find (const GimpItem *item,
                         const gchar    *name)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);

  return gimp_parasite_list_find (GET_PRIVATE (item)->parasites, name);
}

static void
gimp_item_parasite_list_foreach_func (gchar          *name,
                                      GimpParasite   *parasite,
                                      gchar        ***cur)
{
  *(*cur)++ = (gchar *) g_strdup (name);
}

gchar **
gimp_item_parasite_list (const GimpItem *item,
                         gint           *count)
{
  GimpItemPrivate  *private;
  gchar           **list;
  gchar           **cur;

  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (count != NULL, NULL);

  private = GET_PRIVATE (item);

  *count = gimp_parasite_list_length (private->parasites);

  cur = list = g_new (gchar *, *count);

  gimp_parasite_list_foreach (private->parasites,
                              (GHFunc) gimp_item_parasite_list_foreach_func,
                              &cur);

  return list;
}

void
gimp_item_set_visible (GimpItem *item,
                       gboolean  visible,
                       gboolean  push_undo)
{
  g_return_if_fail (GIMP_IS_ITEM (item));

  visible = visible ? TRUE : FALSE;

  if (gimp_item_get_visible (item) != visible)
    {
      if (push_undo && gimp_item_is_attached (item))
        {
          GimpImage *image = gimp_item_get_image (item);

          if (image)
            gimp_image_undo_push_item_visibility (image, NULL, item);
        }

      GET_PRIVATE (item)->visible = visible;

      g_signal_emit (item, gimp_item_signals[VISIBILITY_CHANGED], 0);

      g_object_notify (G_OBJECT (item), "visible");
    }
}

gboolean
gimp_item_get_visible (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  return GET_PRIVATE (item)->visible;
}

gboolean
gimp_item_is_visible (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  if (gimp_item_get_visible (item))
    {
      GimpItem *parent = gimp_item_get_parent (item);

      if (parent)
        return gimp_item_is_visible (parent);

      return TRUE;
    }

  return FALSE;
}

void
gimp_item_set_linked (GimpItem *item,
                      gboolean  linked,
                      gboolean  push_undo)
{
  g_return_if_fail (GIMP_IS_ITEM (item));

  linked = linked ? TRUE : FALSE;

  if (gimp_item_get_linked (item) != linked)
    {
      if (push_undo && gimp_item_is_attached (item))
        {
          GimpImage *image = gimp_item_get_image (item);

          if (image)
            gimp_image_undo_push_item_linked (image, NULL, item);
        }

      GET_PRIVATE (item)->linked = linked;

      g_signal_emit (item, gimp_item_signals[LINKED_CHANGED], 0);

      g_object_notify (G_OBJECT (item), "linked");
    }
}

gboolean
gimp_item_get_linked (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  return GET_PRIVATE (item)->linked;
}

void
gimp_item_set_lock_content (GimpItem *item,
                            gboolean  lock_content,
                            gboolean  push_undo)
{
  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_can_lock_content (item));

  lock_content = lock_content ? TRUE : FALSE;

  if (gimp_item_get_lock_content (item) != lock_content)
    {
      if (push_undo && gimp_item_is_attached (item))
        {
          /* Right now I don't think this should be pushed. */
#if 0
          GimpImage *image = gimp_item_get_image (item);

          gimp_image_undo_push_item_lock_content (image, NULL, item);
#endif
        }

      GET_PRIVATE (item)->lock_content = lock_content;

      g_signal_emit (item, gimp_item_signals[LOCK_CONTENT_CHANGED], 0);

      g_object_notify (G_OBJECT (item), "lock-content");
    }
}

gboolean
gimp_item_get_lock_content (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  return GET_PRIVATE (item)->lock_content;
}

gboolean
gimp_item_can_lock_content (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  return TRUE;
}

gboolean
gimp_item_is_content_locked (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  return GIMP_ITEM_GET_CLASS (item)->is_content_locked (item);
}

gboolean
gimp_item_mask_bounds (GimpItem *item,
                       gint     *x1,
                       gint     *y1,
                       gint     *x2,
                       gint     *y2)
{
  GimpImage   *image;
  GimpChannel *selection;
  gint         tmp_x1, tmp_y1;
  gint         tmp_x2, tmp_y2;
  gboolean     retval;

  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (item), FALSE);

  image     = gimp_item_get_image (item);
  selection = gimp_image_get_mask (image);

  if (GIMP_ITEM (selection) != item &&
      gimp_channel_bounds (selection, &tmp_x1, &tmp_y1, &tmp_x2, &tmp_y2))
    {
      gint off_x, off_y;

      gimp_item_get_offset (item, &off_x, &off_y);

      tmp_x1 = CLAMP (tmp_x1 - off_x, 0, gimp_item_get_width  (item));
      tmp_y1 = CLAMP (tmp_y1 - off_y, 0, gimp_item_get_height (item));
      tmp_x2 = CLAMP (tmp_x2 - off_x, 0, gimp_item_get_width  (item));
      tmp_y2 = CLAMP (tmp_y2 - off_y, 0, gimp_item_get_height (item));

      retval = TRUE;
    }
  else
    {
      tmp_x1 = 0;
      tmp_y1 = 0;
      tmp_x2 = gimp_item_get_width  (item);
      tmp_y2 = gimp_item_get_height (item);

      retval = FALSE;
    }

  if (x1) *x1 = tmp_x1;
  if (y1) *y1 = tmp_y1;
  if (x2) *x2 = tmp_x2;
  if (y2) *y2 = tmp_y2;

  return retval;;
}

gboolean
gimp_item_mask_intersect (GimpItem *item,
                          gint     *x,
                          gint     *y,
                          gint     *width,
                          gint     *height)
{
  GimpImage   *image;
  GimpChannel *selection;
  gint         tmp_x, tmp_y;
  gint         tmp_width, tmp_height;
  gboolean     retval;

  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (item), FALSE);

  image     = gimp_item_get_image (item);
  selection = gimp_image_get_mask (image);

  if (GIMP_ITEM (selection) != item &&
      gimp_channel_bounds (selection, &tmp_x, &tmp_y, &tmp_width, &tmp_height))
    {
      gint off_x, off_y;

      gimp_item_get_offset (item, &off_x, &off_y);

      tmp_width  -= tmp_x;
      tmp_height -= tmp_y;

      retval = gimp_rectangle_intersect (tmp_x - off_x, tmp_y - off_y,
                                         tmp_width, tmp_height,
                                         0, 0,
                                         gimp_item_get_width  (item),
                                         gimp_item_get_height (item),
                                         &tmp_x, &tmp_y,
                                         &tmp_width, &tmp_height);
    }
  else
    {
      tmp_x      = 0;
      tmp_y      = 0;
      tmp_width  = gimp_item_get_width  (item);
      tmp_height = gimp_item_get_height (item);

      retval = TRUE;
    }

  if (x)      *x      = tmp_x;
  if (y)      *y      = tmp_y;
  if (width)  *width  = tmp_width;
  if (height) *height = tmp_height;

  return retval;
}

gboolean
gimp_item_is_in_set (GimpItem    *item,
                     GimpItemSet  set)
{
  GimpItemPrivate *private;

  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  private = GET_PRIVATE (item);

  switch (set)
    {
    case GIMP_ITEM_SET_NONE:
      return FALSE;

    case GIMP_ITEM_SET_ALL:
      return TRUE;

    case GIMP_ITEM_SET_IMAGE_SIZED:
      return (gimp_item_get_width  (item) == gimp_image_get_width  (private->image) &&
              gimp_item_get_height (item) == gimp_image_get_height (private->image));

    case GIMP_ITEM_SET_VISIBLE:
      return gimp_item_get_visible (item);

    case GIMP_ITEM_SET_LINKED:
      return gimp_item_get_linked (item);
    }

  return FALSE;
}
