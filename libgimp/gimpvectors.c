/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpvectors.c
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


struct _GimpVectorsPrivate
{
  gpointer unused;
};


G_DEFINE_TYPE_WITH_PRIVATE (GimpVectors, gimp_vectors, GIMP_TYPE_ITEM)

#define parent_class gimp_vectors_parent_class


static void
gimp_vectors_class_init (GimpVectorsClass *klass)
{
}

static void
gimp_vectors_init (GimpVectors *vectors)
{
  vectors->priv = gimp_vectors_get_instance_private (vectors);
}

/**
 * gimp_vectors_get_by_id:
 * @vectors_id: The vectors id.
 *
 * Returns a #GimpVectors representing @vectors_id. This function
 * calls gimp_item_get_by_id() and returns the item if it is vectors
 * or %NULL otherwise.
 *
 * Returns: (nullable) (transfer none): a #GimpVectors for @vectors_id
 *          or %NULL if @vectors_id does not represent a valid
 *          vectors. The object belongs to libgimp and you must not
 *          modify or unref it.
 *
 * Since: 3.0
 **/
GimpVectors *
gimp_vectors_get_by_id (gint32 vectors_id)
{
  GimpItem *item = gimp_item_get_by_id (vectors_id);

  if (GIMP_IS_VECTORS (item))
    return (GimpVectors *) item;

  return NULL;
}
