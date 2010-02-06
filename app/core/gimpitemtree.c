/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpitemtree.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>

#include "core-types.h"

#include "gimpimage.h"
#include "gimpitem.h"
#include "gimpitemstack.h"
#include "gimpitemtree.h"


enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_CONTAINER_TYPE,
  PROP_ITEM_TYPE
};


typedef struct _GimpItemTreePrivate GimpItemTreePrivate;

struct _GimpItemTreePrivate
{
  GimpImage *image;
  GType      container_type;
  GType      item_type;
};

#define GIMP_ITEM_TREE_GET_PRIVATE(object) \
        G_TYPE_INSTANCE_GET_PRIVATE (object, \
                                     GIMP_TYPE_ITEM_TREE, \
                                     GimpItemTreePrivate)


/*  local function prototypes  */

static GObject * gimp_item_tree_constructor  (GType                  type,
                                              guint                  n_params,
                                              GObjectConstructParam *params);
static void      gimp_item_tree_finalize     (GObject               *object);
static void      gimp_item_tree_set_property (GObject               *object,
                                              guint                  property_id,
                                              const GValue          *value,
                                              GParamSpec            *pspec);
static void      gimp_item_tree_get_property (GObject               *object,
                                              guint                  property_id,
                                              GValue                *value,
                                              GParamSpec            *pspec);

static gint64    gimp_item_tree_get_memsize  (GimpObject            *object,
                                              gint64                *gui_size);


G_DEFINE_TYPE (GimpItemTree, gimp_item_tree, GIMP_TYPE_OBJECT)

#define parent_class gimp_item_tree_parent_class


static void
gimp_item_tree_class_init (GimpItemTreeClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  object_class->constructor      = gimp_item_tree_constructor;
  object_class->finalize         = gimp_item_tree_finalize;
  object_class->set_property     = gimp_item_tree_set_property;
  object_class->get_property     = gimp_item_tree_get_property;

  gimp_object_class->get_memsize = gimp_item_tree_get_memsize;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        NULL, NULL,
                                                        GIMP_TYPE_IMAGE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONTAINER_TYPE,
                                   g_param_spec_gtype ("container-type",
                                                       NULL, NULL,
                                                       GIMP_TYPE_ITEM_STACK,
                                                       GIMP_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ITEM_TYPE,
                                   g_param_spec_gtype ("item-type",
                                                       NULL, NULL,
                                                       GIMP_TYPE_ITEM,
                                                       GIMP_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (GimpItemTreePrivate));
}

static void
gimp_item_tree_init (GimpItemTree *tree)
{
}

static GObject *
gimp_item_tree_constructor (GType                  type,
                            guint                  n_params,
                            GObjectConstructParam *params)
{
  GObject             *object;
  GimpItemTree        *tree;
  GimpItemTreePrivate *private;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  tree    = GIMP_ITEM_TREE (object);
  private = GIMP_ITEM_TREE_GET_PRIVATE (tree);

  g_assert (GIMP_IS_IMAGE (private->image));
  g_assert (g_type_is_a (private->container_type, GIMP_TYPE_ITEM_STACK));
  g_assert (g_type_is_a (private->item_type,      GIMP_TYPE_ITEM));
  g_assert (private->item_type != GIMP_TYPE_ITEM);

  tree->container = g_object_new (private->container_type,
                                  "name",          g_type_name (private->item_type),
                                  "children-type", private->item_type,
                                  "policy",        GIMP_CONTAINER_POLICY_STRONG,
                                  "unique-names",  TRUE,
                                  NULL);

  return object;
}

static void
gimp_item_tree_finalize (GObject *object)
{
  GimpItemTree *tree = GIMP_ITEM_TREE (object);

  if (tree->container)
    {
      g_object_unref (tree->container);
      tree->container = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_item_tree_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GimpItemTreePrivate *private = GIMP_ITEM_TREE_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      private->image = g_value_get_object (value); /* don't ref */
      break;
    case PROP_CONTAINER_TYPE:
      private->container_type = g_value_get_gtype (value);
      break;
    case PROP_ITEM_TYPE:
      private->item_type = g_value_get_gtype (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_item_tree_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GimpItemTreePrivate *private = GIMP_ITEM_TREE_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, private->image);
      break;
    case PROP_CONTAINER_TYPE:
      g_value_set_gtype (value, private->container_type);
      break;
    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, private->item_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_item_tree_get_memsize (GimpObject *object,
                            gint64     *gui_size)
{
  GimpItemTree *tree    = GIMP_ITEM_TREE (object);
  gint64        memsize = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (tree->container), gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}


/*  public functions  */

GimpItemTree *
gimp_item_tree_new (GimpImage *image,
                    GType      container_type,
                    GType      item_type)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (g_type_is_a (container_type, GIMP_TYPE_ITEM_STACK), NULL);
  g_return_val_if_fail (g_type_is_a (item_type, GIMP_TYPE_ITEM), NULL);

  return g_object_new (GIMP_TYPE_ITEM_TREE,
                       "image",          image,
                       "container-type", container_type,
                       "item-type",      item_type,
                       NULL);
}

gboolean
gimp_item_tree_reorder_item (GimpItemTree            *tree,
                             GimpItem                *item,
                             GimpItem                *new_parent,
                             gint                     new_index,
                             GimpItemReorderUndoFunc  undo_func,
                             const gchar             *undo_desc)
{
  GimpItemTreePrivate *private;
  GimpContainer       *container;
  GimpContainer       *new_container;
  gint                 n_items;

  g_return_val_if_fail (GIMP_IS_ITEM_TREE (tree), FALSE);

  private = GIMP_ITEM_TREE_GET_PRIVATE (tree);

  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (item, private->item_type),
                        FALSE);
  g_return_val_if_fail (gimp_item_is_attached (item), FALSE);
  g_return_val_if_fail (gimp_item_get_image (item) == private->image, FALSE);
  g_return_val_if_fail (new_parent == NULL ||
                        G_TYPE_CHECK_INSTANCE_TYPE (new_parent,
                                                    private->item_type),
                        FALSE);
  g_return_val_if_fail (new_parent == NULL ||
                        gimp_item_is_attached (new_parent), FALSE);
  g_return_val_if_fail (new_parent == NULL ||
                        gimp_item_get_image (new_parent) == private->image,
                        FALSE);
  g_return_val_if_fail (new_parent == NULL ||
                        gimp_viewable_get_children (GIMP_VIEWABLE (new_parent)),
                        FALSE);

  container = gimp_item_get_container (item);

  if (new_parent)
    new_container = gimp_viewable_get_children (GIMP_VIEWABLE (new_parent));
  else
    new_container = tree->container;

  n_items = gimp_container_get_n_children (new_container);

  if (new_container == container)
    n_items--;

  new_index = CLAMP (new_index, 0, n_items);

  if (new_container != container ||
      new_index     != gimp_item_get_index (item))
    {
      if (undo_func)
        undo_func (private->image, undo_desc, item);

      if (new_container != container)
        {
          g_object_ref (item);

          gimp_container_remove (container, GIMP_OBJECT (item));

          gimp_viewable_set_parent (GIMP_VIEWABLE (item),
                                    GIMP_VIEWABLE (new_parent));

          gimp_container_insert (new_container, GIMP_OBJECT (item), new_index);

          g_object_unref (item);
        }
      else
        {
          gimp_container_reorder (container, GIMP_OBJECT (item), new_index);
        }
    }

  return TRUE;
}
