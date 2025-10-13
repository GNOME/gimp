/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpvectorlayer.c
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


struct _GimpVectorLayer
{
  GimpLayer parent_instance;
};


G_DEFINE_TYPE_WITH_CODE (GimpVectorLayer, gimp_vector_layer, GIMP_TYPE_LAYER,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_RASTERIZABLE, NULL))

#define parent_class gimp_vector_layer_parent_class


static void
gimp_vector_layer_class_init (GimpVectorLayerClass *klass)
{
}

static void
gimp_vector_layer_init (GimpVectorLayer *layer)
{
}


/* Public API. */

/**
 * gimp_vector_layer_get_by_id:
 * @layer_id: The layer id.
 *
 * Returns a #GimpVectorLayer representing @layer_id. This function calls
 * gimp_item_get_by_id() and returns the item if it is layer or %NULL
 * otherwise.
 *
 * Returns: (nullable) (transfer none): a #GimpVectorLayer for @layer_id or
 *          %NULL if @layer_id does not represent a valid layer. The
 *          object belongs to libgimp and you must not modify or unref
 *          it.
 *
 * Since: 3.0
 **/
GimpVectorLayer *
gimp_vector_layer_get_by_id (gint32 layer_id)
{
  GimpItem *item = gimp_item_get_by_id (layer_id);

  if (GIMP_IS_VECTOR_LAYER (item))
    return (GimpVectorLayer *) item;

  return NULL;
}
