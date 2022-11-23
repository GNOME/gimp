/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * ligmavectors.c
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


struct _LigmaVectors
{
  LigmaItem parent_instance;
};

G_DEFINE_TYPE (LigmaVectors, ligma_vectors, LIGMA_TYPE_ITEM)

#define parent_class ligma_vectors_parent_class


static void
ligma_vectors_class_init (LigmaVectorsClass *klass)
{
}

static void
ligma_vectors_init (LigmaVectors *vectors)
{
}

/**
 * ligma_vectors_get_by_id:
 * @vectors_id: The vectors id.
 *
 * Returns a #LigmaVectors representing @vectors_id. This function
 * calls ligma_item_get_by_id() and returns the item if it is vectors
 * or %NULL otherwise.
 *
 * Returns: (nullable) (transfer none): a #LigmaVectors for @vectors_id
 *          or %NULL if @vectors_id does not represent a valid
 *          vectors. The object belongs to libligma and you must not
 *          modify or unref it.
 *
 * Since: 3.0
 **/
LigmaVectors *
ligma_vectors_get_by_id (gint32 vectors_id)
{
  LigmaItem *item = ligma_item_get_by_id (vectors_id);

  if (LIGMA_IS_VECTORS (item))
    return (LigmaVectors *) item;

  return NULL;
}
