/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpimage.c
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
#include "gimpimage.h"

/**
 * gimp_viewable_get_attributes:
 * @object_ID: The viewable object.
 *
 * Returns the viewable's attributes.
 *
 * Returns attributes from the viewable.
 *
 * Returns: The attributes, or %NULL if there is none.
 *
 * Since: GIMP 2.10
 **/
GimpAttributes *
gimp_viewable_get_attributes (gint32 object_ID)
{
  GimpAttributes *attributes = NULL;
  gchar          *attributes_string;

  attributes_string = _gimp_viewable_get_attributes (object_ID);
  if (attributes_string)
    {
      attributes = gimp_attributes_deserialize (NULL, attributes_string);
      g_free (attributes_string);
    }

  return attributes;
}

/**
 * gimp_viewable_set_attributes:
 * @object_ID: The viewable object.
 * @attributes: The GimpAttributes object.
 *
 * Set the viewable's attributes.
 *
 * Sets attributes on the viewable, or deletes it if
 * @attributes is %NULL.
 *
 * Returns: TRUE on success.
 *
 * Since: GIMP 2.10
 **/
gboolean
gimp_viewable_set_attributes (gint32          object_ID,
                              GimpAttributes *attributes)
{
  gchar    *attributes_string = NULL;
  gboolean  success;

  if (attributes)
    attributes_string = gimp_attributes_serialize (attributes);

  success = _gimp_viewable_set_attributes (object_ID, attributes_string);

  if (attributes_string)
    g_free (attributes_string);

  return success;
}
