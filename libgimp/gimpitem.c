/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpitem.c
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
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"
#include "gimpitem.h"


/**
 * gimp_item_get_metadata:
 * @item_ID: The item.
 *
 * Returns the item's metadata.
 *
 * Returns metadata from the item.
 *
 * Returns: The metadata, or %NULL if there is none.
 *
 * Since: GIMP 2.10
 **/
GimpMetadata *
gimp_item_get_metadata (gint32 item_ID)
{
  GimpMetadata *metadata = NULL;
  gchar          *metadata_string;

  metadata_string = _gimp_item_get_metadata (item_ID);
  if (metadata_string)
    {
      metadata = gimp_metadata_deserialize (metadata_string);
      g_free (metadata_string);
    }

  return metadata;
}

/**
 * gimp_item_set_metadata:
 * @item_ID: The item.
 * @metadata: The GimpMetadata object.
 *
 * Set the item's metadata.
 *
 * Sets metadata on the item, or deletes it if
 * @metadata is %NULL.
 *
 * Returns: TRUE on success.
 *
 * Since: GIMP 2.10
 **/
gboolean
gimp_item_set_metadata (gint32          item_ID,
                        GimpMetadata   *metadata)
{
  gchar    *metadata_string = NULL;
  gboolean  success;

  if (metadata)
    metadata_string = gimp_metadata_serialize (metadata);

  success = _gimp_item_set_metadata (item_ID, metadata_string);

  if (metadata_string)
    g_free (metadata_string);

  return success;
}

