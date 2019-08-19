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

#include "gimppixbuf.h"

enum
{
  PROP_0,
  PROP_ID,
  N_PROPS
};

struct _GimpItemPrivate
{
  gint id;
};

static void       gimp_item_set_property  (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void       gimp_item_get_property  (GObject      *object,
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
  item->priv = gimp_item_get_instance_private (item);
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
    case PROP_ID:
      item->priv->id = g_value_get_int (value);
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
  GimpItem *item = GIMP_ITEM (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, item->priv->id);
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
 * Returns: the item ID.
 *
 * Since: 3.0
 **/
gint32
gimp_item_get_id (GimpItem *item)
{
  return item ? item->priv->id : -1;
}

/**
 * gimp_item_new_by_id:
 * @item_id: The item id.
 *
 * Creates a #GimpItem representing @item_id. Since #GimpItem is an
 * abstract class, the object real type will actually be the proper
 * subclass.
 *
 * Returns: (nullable) (transfer full): a #GimpItem for @item_id or
 *          %NULL if @item_id does not represent a valid item.
 *
 * Since: 3.0
 **/
GimpItem *
gimp_item_new_by_id (gint32 item_id)
{
  GimpItem *item = NULL;

  if (_gimp_item_is_valid (item_id))
    {
      if (_gimp_item_is_layer (item_id))
        item = g_object_new (GIMP_TYPE_LAYER,
                             "id", item_id,
                             NULL);
    }

  return item;
}

/**
 * gimp_item_get_children:
 * @item: The item.
 *
 * Returns the item's list of children.
 *
 * This procedure returns the list of items which are children of the
 * specified item. The order is topmost to bottommost.
 *
 * Returns: (element-type GimpItem) (transfer full):
 *          The item's list of children.
 *          The returned value must be freed with:
 *          g_list_free_full(list, g_object_unref);
 *
 * Since: 3.0
 **/
GList *
gimp_item_get_children (GimpItem *item)
{
  GList *children = NULL;
  gint  *ids;
  gint   num_items;
  gint   i;

  ids = _gimp_item_get_children (item, &num_items);

  for (i = 0; i < num_items; i++)
    children = g_list_prepend (children, gimp_item_new_by_id (ids[i]));

  children = g_list_reverse (children);
  g_free (ids);

  return children;
}

/**
 * gimp_item_get_children_deprecated: (skip)
 * @item_id: The item.
 * @num_children: (out): The item's number of children.
 *
 * Returns the item's list of children.
 *
 * This procedure returns the list of items which are children of the
 * specified item. The order is topmost to bottommost.
 *
 * Returns: (array length=num_children) (element-type gint32) (transfer full):
 *          The item's list of children.
 *          The returned value must be freed with g_free().
 *
 * Since: 2.8
 **/
gint *
gimp_item_get_children_deprecated (gint32    item_id,
                                   gint     *num_children)
{
  GimpItem *item;
  gint     *children;

  item     = gimp_item_new_by_id (item_id);
  children = _gimp_item_get_children (item, num_children);
  g_object_unref (item);

  return children;
}
