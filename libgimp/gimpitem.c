/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpitem.c
 * Copyright (C) Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"

#include "libgimpbase/gimpwire.h" /* FIXME kill this include */

#include "gimpplugin-private.h"


enum
{
  PROP_0,
  PROP_ID,
  N_PROPS
};


typedef struct _GimpItemPrivate
{
  gint id;
} GimpItemPrivate;


static void   gimp_item_set_property  (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec);
static void   gimp_item_get_property  (GObject      *object,
                                       guint         property_id,
                                       GValue       *value,
                                       GParamSpec   *pspec);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (GimpItem, gimp_item, G_TYPE_OBJECT)

#define parent_class gimp_item_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
gimp_item_class_init (GimpItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_item_set_property;
  object_class->get_property = gimp_item_get_property;

  props[PROP_ID] =
    g_param_spec_int ("id",
                      "The item id",
                      "The item id for internal use",
                      0, G_MAXINT32, 0,
                      GIMP_PARAM_READWRITE |
                      G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
gimp_item_init (GimpItem *item)
{
}

static void
gimp_item_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GimpItem        *item = GIMP_ITEM (object);
  GimpItemPrivate *priv = gimp_item_get_instance_private (item);

  switch (property_id)
    {
    case PROP_ID:
      priv->id = g_value_get_int (value);
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
  GimpItem        *item = GIMP_ITEM (object);
  GimpItemPrivate *priv = gimp_item_get_instance_private (item);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, priv->id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/* Public API. */


/**
 * gimp_item_get_id:
 * @item: The item.
 *
 * Note: in most use cases, you should not need an item's ID which is
 * mostly internal data and not reusable across sessions.
 *
 * Returns: the item ID.
 *
 * Since: 3.0
 **/
gint32
gimp_item_get_id (GimpItem *item)
{
  if (item)
    {
      GimpItemPrivate *priv = gimp_item_get_instance_private (item);

      return priv->id;
    }
  else
    {
      return -1;
    }
}

/**
 * gimp_item_get_by_id:
 * @item_id: The item id.
 *
 * Returns a #GimpItem representing @item_id. Since #GimpItem is an
 * abstract class, the real object type will actually be the proper
 * subclass.
 *
 * Note: in most use cases, you should not need to retrieve a #GimpItem
 * by its ID, which is mostly internal data and not reusable across
 * sessions. Use the appropriate functions for your use case instead.
 *
 * Returns: (nullable) (transfer none): a #GimpItem for @item_id or
 *          %NULL if @item_id does not represent a valid item.
 *          The object belongs to libgimp and you must not modify
 *          or unref it.
 *
 * Since: 3.0
 **/
GimpItem *
gimp_item_get_by_id (gint32 item_id)
{
  if (item_id > 0)
    {
      GimpPlugIn *plug_in = gimp_get_plug_in ();

      return _gimp_plug_in_get_item (plug_in, item_id);
    }

  return NULL;
}

/**
 * gimp_item_is_valid:
 * @item: The item to check.
 *
 * Returns TRUE if the item is valid.
 *
 * This procedure checks if the given item is valid and refers to an
 * existing item.
 *
 * Returns: Whether the item is valid.
 *
 * Since: 2.8
 **/
gboolean
gimp_item_is_valid (GimpItem *item)
{
  return gimp_item_id_is_valid (gimp_item_get_id (item));
}

/**
 * gimp_item_is_drawable:
 * @item: The item.
 *
 * Returns whether the item is a drawable.
 *
 * This procedure returns TRUE if the specified item is a drawable.
 *
 * Returns: TRUE if the item is a drawable, FALSE otherwise.
 *
 * Since: 2.8
 **/
gboolean
gimp_item_is_drawable (GimpItem *item)
{
  return gimp_item_id_is_drawable (gimp_item_get_id (item));
}

/**
 * gimp_item_is_layer:
 * @item: The item.
 *
 * Returns whether the item is a layer.
 *
 * This procedure returns TRUE if the specified item is a layer.
 *
 * Returns: TRUE if the item is a layer, FALSE otherwise.
 *
 * Since: 2.8
 **/
gboolean
gimp_item_is_layer (GimpItem *item)
{
  return gimp_item_id_is_layer (gimp_item_get_id (item));
}

/**
 * gimp_item_is_text_layer:
 * @item: The item.
 *
 * Returns whether the item is a text layer.
 *
 * This procedure returns TRUE if the specified item is a text
 * layer.
 *
 * Returns: TRUE if the item is a text layer, FALSE otherwise.
 *
 * Since: 2.8
 **/
gboolean
gimp_item_is_text_layer (GimpItem *item)
{
  return gimp_item_id_is_text_layer (gimp_item_get_id (item));
}

/**
 * gimp_item_is_vector_layer:
 * @item: The item.
 *
 * Returns whether the item is a vector layer.
 *
 * This procedure returns TRUE if the specified item is a vector
 * layer.
 *
 * Returns: TRUE if the item is a vector layer, FALSE otherwise.
 *
 * Since: 3.2
 **/
gboolean
gimp_item_is_vector_layer (GimpItem *item)
{
  return gimp_item_id_is_vector_layer (gimp_item_get_id (item));
}

/**
 * gimp_item_is_link_layer:
 * @item: The item.
 *
 * Returns whether the item is a link layer.
 *
 * This procedure returns TRUE if the specified item is a link
 * layer.
 *
 * Returns: TRUE if the item is a link layer, FALSE otherwise.
 *
 * Since: 3.2
 **/
gboolean
gimp_item_is_link_layer (GimpItem *item)
{
  return gimp_item_id_is_link_layer (gimp_item_get_id (item));
}

/**
 * gimp_item_is_group_layer:
 * @item: The item.
 *
 * Returns whether the item is a group layer.
 *
 * This procedure returns TRUE if the specified item is a group
 * layer.
 *
 * Returns: TRUE if the item is a group layer, FALSE otherwise.
 *
 * Since: 3.0
 **/
gboolean
gimp_item_is_group_layer (GimpItem *item)
{
  return gimp_item_id_is_group_layer (gimp_item_get_id (item));
}

/**
 * gimp_item_is_channel:
 * @item: The item.
 *
 * Returns whether the item is a channel.
 *
 * This procedure returns TRUE if the specified item is a channel.
 *
 * Returns: TRUE if the item is a channel, FALSE otherwise.
 *
 * Since: 2.8
 **/
gboolean
gimp_item_is_channel (GimpItem *item)
{
  return gimp_item_id_is_channel (gimp_item_get_id (item));
}

/**
 * gimp_item_is_layer_mask:
 * @item: The item.
 *
 * Returns whether the item is a layer mask.
 *
 * This procedure returns TRUE if the specified item is a layer
 * mask.
 *
 * Returns: TRUE if the item is a layer mask, FALSE otherwise.
 *
 * Since: 2.8
 **/
gboolean
gimp_item_is_layer_mask (GimpItem *item)
{
  return gimp_item_id_is_layer_mask (gimp_item_get_id (item));
}

/**
 * gimp_item_is_selection:
 * @item: The item.
 *
 * Returns whether the item is a selection.
 *
 * This procedure returns TRUE if the specified item is a selection.
 *
 * Returns: TRUE if the item is a selection, FALSE otherwise.
 *
 * Since: 2.8
 **/
gboolean
gimp_item_is_selection (GimpItem *item)
{
  return gimp_item_id_is_selection (gimp_item_get_id (item));
}

/**
 * gimp_item_is_path:
 * @item: The item.
 *
 * Returns whether the item is a path.
 *
 * This procedure returns TRUE if the specified item is a path.
 *
 * Returns: TRUE if the item is a path, FALSE otherwise.
 *
 * Since: 2.8
 **/
gboolean
gimp_item_is_path (GimpItem *item)
{
  return gimp_item_id_is_path (gimp_item_get_id (item));
}

/**
 * gimp_item_list_children: (skip)
 * @item: The item.
 *
 * Returns the item's list of children.
 *
 * This procedure returns the list of items which are children of the
 * specified item. The order is topmost to bottommost.
 *
 * Returns: (element-type GimpItem) (transfer container):
 *          The item's list of children.
 *          The returned list must be freed with g_list_free(). Item
 *          elements belong to libgimp and must not be unrefed.
 *
 * Since: 3.0
 **/
GList *
gimp_item_list_children (GimpItem *item)
{
  GimpItem **children;
  GList     *list = NULL;
  gint       i;

  children = gimp_item_get_children (item);

  if (children)
    for (i = 0; children[i] != NULL; i++)
      list = g_list_prepend (list, children[i]);

  g_free (children);

  return g_list_reverse (list);
}
