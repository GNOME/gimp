/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimppath.c
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


struct _GimpPath
{
  GimpItem parent_instance;
};

G_DEFINE_TYPE (GimpPath, gimp_path, GIMP_TYPE_ITEM)

#define parent_class gimp_path_parent_class


static void
gimp_path_class_init (GimpPathClass *klass)
{
}

static void
gimp_path_init (GimpPath *path)
{
}

/**
 * gimp_path_get_by_id:
 * @path_id: The path id.
 *
 * Returns a #GimpPath representing @path_id. This function
 * calls gimp_item_get_by_id() and returns the item if it is a path
 * or %NULL otherwise.
 *
 * Returns: (nullable) (transfer none): a #GimpPath for @path_id
 *          or %NULL if @path_id does not represent a valid
 *          path. The object belongs to libgimp and you must not
 *          modify or unref it.
 *
 * Since: 3.0
 **/
GimpPath *
gimp_path_get_by_id (gint32 path_id)
{
  GimpItem *item = gimp_item_get_by_id (path_id);

  if (GIMP_IS_PATH (item))
    return (GimpPath *) item;

  return NULL;
}
