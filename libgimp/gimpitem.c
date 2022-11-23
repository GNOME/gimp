/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * ligmaitem.c
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

#include "ligma.h"

#include "libligmabase/ligmawire.h" /* FIXME kill this include */

#include "ligmaplugin-private.h"
#include "ligmaprocedure-private.h"


enum
{
  PROP_0,
  PROP_ID,
  N_PROPS
};


typedef struct _LigmaItemPrivate
{
  gint id;
} LigmaItemPrivate;


static void   ligma_item_set_property  (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec);
static void   ligma_item_get_property  (GObject      *object,
                                       guint         property_id,
                                       GValue       *value,
                                       GParamSpec   *pspec);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (LigmaItem, ligma_item, G_TYPE_OBJECT)

#define parent_class ligma_item_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
ligma_item_class_init (LigmaItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_item_set_property;
  object_class->get_property = ligma_item_get_property;

  props[PROP_ID] =
    g_param_spec_int ("id",
                      "The item id",
                      "The item id for internal use",
                      0, G_MAXINT32, 0,
                      LIGMA_PARAM_READWRITE |
                      G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
ligma_item_init (LigmaItem *item)
{
}

static void
ligma_item_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  LigmaItem        *item = LIGMA_ITEM (object);
  LigmaItemPrivate *priv = ligma_item_get_instance_private (item);

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
ligma_item_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  LigmaItem        *item = LIGMA_ITEM (object);
  LigmaItemPrivate *priv = ligma_item_get_instance_private (item);

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
 * ligma_item_get_id:
 * @item: The item.
 *
 * Returns: the item ID.
 *
 * Since: 3.0
 **/
gint32
ligma_item_get_id (LigmaItem *item)
{
  if (item)
    {
      LigmaItemPrivate *priv = ligma_item_get_instance_private (item);

      return priv->id;
    }
  else
    {
      return -1;
    }
}

/**
 * ligma_item_get_by_id:
 * @item_id: The item id.
 *
 * Returns a #LigmaItem representing @item_id. Since #LigmaItem is an
 * abstract class, the real object type will actually be the proper
 * subclass.
 *
 * Returns: (nullable) (transfer none): a #LigmaItem for @item_id or
 *          %NULL if @item_id does not represent a valid item.
 *          The object belongs to libligma and you must not modify
 *          or unref it.
 *
 * Since: 3.0
 **/
LigmaItem *
ligma_item_get_by_id (gint32 item_id)
{
  if (item_id > 0)
    {
      LigmaPlugIn    *plug_in   = ligma_get_plug_in ();
      LigmaProcedure *procedure = _ligma_plug_in_get_procedure (plug_in);

      return _ligma_procedure_get_item (procedure, item_id);
    }

  return NULL;
}

/**
 * ligma_item_is_valid:
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
ligma_item_is_valid (LigmaItem *item)
{
  return ligma_item_id_is_valid (ligma_item_get_id (item));
}

/**
 * ligma_item_is_drawable:
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
ligma_item_is_drawable (LigmaItem *item)
{
  return ligma_item_id_is_drawable (ligma_item_get_id (item));
}

/**
 * ligma_item_is_layer:
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
ligma_item_is_layer (LigmaItem *item)
{
  return ligma_item_id_is_layer (ligma_item_get_id (item));
}

/**
 * ligma_item_is_text_layer:
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
ligma_item_is_text_layer (LigmaItem *item)
{
  return ligma_item_id_is_text_layer (ligma_item_get_id (item));
}

/**
 * ligma_item_is_channel:
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
ligma_item_is_channel (LigmaItem *item)
{
  return ligma_item_id_is_channel (ligma_item_get_id (item));
}

/**
 * ligma_item_is_layer_mask:
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
ligma_item_is_layer_mask (LigmaItem *item)
{
  return ligma_item_id_is_layer_mask (ligma_item_get_id (item));
}

/**
 * ligma_item_is_selection:
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
ligma_item_is_selection (LigmaItem *item)
{
  return ligma_item_id_is_selection (ligma_item_get_id (item));
}

/**
 * ligma_item_is_vectors:
 * @item: The item.
 *
 * Returns whether the item is a vectors.
 *
 * This procedure returns TRUE if the specified item is a vectors.
 *
 * Returns: TRUE if the item is a vectors, FALSE otherwise.
 *
 * Since: 2.8
 **/
gboolean
ligma_item_is_vectors (LigmaItem *item)
{
  return ligma_item_id_is_vectors (ligma_item_get_id (item));
}

/**
 * ligma_item_list_children:
 * @item: The item.
 *
 * Returns the item's list of children.
 *
 * This procedure returns the list of items which are children of the
 * specified item. The order is topmost to bottommost.
 *
 * Returns: (element-type LigmaItem) (transfer container):
 *          The item's list of children.
 *          The returned list must be freed with g_list_free(). Item
 *          elements belong to libligma and must not be unrefed.
 *
 * Since: 3.0
 **/
GList *
ligma_item_list_children (LigmaItem *item)
{
  LigmaItem **children;
  gint       num_children;
  GList     *list = NULL;
  gint       i;

  children = ligma_item_get_children (item, &num_children);

  for (i = 0; i < num_children; i++)
    list = g_list_prepend (list, children[i]);

  g_free (children);

  return g_list_reverse (list);
}
