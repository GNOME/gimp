/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimplinklayer.c
 * Copyright (C) 2025 Jehan
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


struct _GimpLinkLayer
{
  GimpLayer parent_instance;
};


G_DEFINE_TYPE_WITH_CODE (GimpLinkLayer, gimp_link_layer, GIMP_TYPE_LAYER,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_RASTERIZABLE, NULL))

#define parent_class gimp_link_layer_parent_class


static void
gimp_link_layer_class_init (GimpLinkLayerClass *klass)
{
}

static void
gimp_link_layer_init (GimpLinkLayer *layer)
{
}


/* Public API. */

/**
 * gimp_link_layer_get_by_id:
 * @layer_id: The layer id.
 *
 * Returns a #GimpLinkLayer representing @layer_id. This function calls
 * [func@Gimp.Item.get_by_id] and returns the item if it is a link
 * layer or %NULL otherwise.
 *
 * Returns: (nullable) (transfer none): a #GimpLinkLayer for @layer_id or
 *          %NULL if @layer_id does not represent a valid link layer. The
 *          object belongs to libgimp and you must not modify or unref
 *          it.
 *
 * Since: 3.2
 **/
GimpLinkLayer *
gimp_link_layer_get_by_id (gint32 layer_id)
{
  GimpItem *item = gimp_item_get_by_id (layer_id);

  if (GIMP_IS_LINK_LAYER (item))
    return (GimpLinkLayer *) item;

  return NULL;
}
