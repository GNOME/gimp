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


struct _GimpLayerMaskPrivate
{
  gpointer unused;
};


G_DEFINE_TYPE_WITH_PRIVATE (GimpLayerMask, gimp_layer_mask, GIMP_TYPE_CHANNEL)

#define parent_class gimp_layer_mask_parent_class


static void
gimp_layer_mask_class_init (GimpLayerMaskClass *klass)
{
}

static void
gimp_layer_mask_init (GimpLayerMask *layer_mask)
{
  layer_mask->priv = gimp_layer_mask_get_instance_private (layer_mask);
}
