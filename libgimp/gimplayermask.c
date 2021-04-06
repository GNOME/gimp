/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimplayermask.c
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
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"


struct _GimpLayerMask
{
  GimpChannel parent_instance;
};


G_DEFINE_TYPE (GimpLayerMask, gimp_layer_mask, GIMP_TYPE_CHANNEL)

#define parent_class gimp_layer_mask_parent_class


static void
gimp_layer_mask_class_init (GimpLayerMaskClass *klass)
{
}

static void
gimp_layer_mask_init (GimpLayerMask *layer_mask)
{
}

/**
 * gimp_layer_mask_get_by_id:
 * @layer_mask_id: The layer_mask id.
 *
 * Returns a #GimpLayerMask representing @layer_mask_id. This function
 * calls gimp_item_get_by_id() and returns the item if it is
 * layer_mask or %NULL otherwise.
 *
 * Returns: (nullable) (transfer none): a #GimpLayerMask for
 *          @layer_mask_id or %NULL if @layer_mask_id does not
 *          represent a valid layer_mask. The object belongs to
 *          libgimp and you must not modify or unref it.
 *
 * Since: 3.0
 **/
GimpLayerMask *
gimp_layer_mask_get_by_id (gint32 layer_mask_id)
{
  GimpItem *item = gimp_item_get_by_id (layer_mask_id);

  if (GIMP_IS_LAYER_MASK (item))
    return (GimpLayerMask *) item;

  return NULL;
}
