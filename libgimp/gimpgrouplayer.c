/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpgrouplayer.c
 * Copyright (C) 2022 Jehan
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


struct _GimpGroupLayer
{
  GimpLayer parent_instance;
};


G_DEFINE_TYPE (GimpGroupLayer, gimp_group_layer, GIMP_TYPE_LAYER)

#define parent_class gimp_group_layer_parent_class


static void
gimp_group_layer_class_init (GimpGroupLayerClass *klass)
{
}

static void
gimp_group_layer_init (GimpGroupLayer *layer)
{
}


/* Public API. */

/**
 * gimp_group_layer_get_by_id:
 * @layer_id: The layer id.
 *
 * Returns a #GimpGroupLayer representing @layer_id. This function calls
 * gimp_item_get_by_id() and returns the item if it is a group layer or
 * %NULL otherwise.
 *
 * Returns: (nullable) (transfer none): a #GimpGroupLayer for @layer_id or
 *          %NULL if @layer_id does not represent a valid group layer.
 *          The object belongs to libgimp and you must not modify or
 *          unref it.
 *
 * Since: 3.0
 **/
GimpGroupLayer *
gimp_group_layer_get_by_id (gint32 layer_id)
{
  GimpItem *item = gimp_item_get_by_id (layer_id);

  if (GIMP_IS_GROUP_LAYER (item))
    return (GimpGroupLayer *) item;

  return NULL;
}

/**
 * gimp_group_layer_new:
 * @image:            The image to which to add the layer.
 * @name: (nullable): The group layer name.
 *
 * Create a new group layer.
 *
 * This procedure creates a new group layer with a given @name. If @name is
 * %NULL, GIMP will choose a name using its default layer name algorithm.
 *
 * The new group layer still needs to be added to the image, as this is
 * not automatic. Add the new layer with the [method@Image.insert_layer]
 * method.
 *
 * Other attributes such as layer mask modes, and offsets should be set
 * with explicit procedure calls.
 *
 * Returns: (transfer none): The newly created group layer.
 *          The object belongs to libgimp and you should not free it.
 *
 * Since: 3.0
 */
GimpGroupLayer *
gimp_group_layer_new (GimpImage   *image,
                      const gchar *name)
{
  GimpGroupLayer *layer = _gimp_group_layer_new (image);

  if (name != NULL)
    gimp_item_set_name (GIMP_ITEM (layer), name);

  return layer;
}
