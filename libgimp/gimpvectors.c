/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpvectors.c
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

#include <string.h>

#include "gimp.h"
#include "gimpvectors.h"


/**
 * gimp_vectors_is_valid:
 * @vectors_ID: The vectors object to check.
 *
 * Deprecated: Use gimp_item_is_valid() instead.
 *
 * Returns: Whether the vectors ID is valid.
 *
 * Since: 2.4
 */
gboolean
gimp_vectors_is_valid (gint32 vectors_ID)
{
  return gimp_item_is_valid (vectors_ID);
}

/**
 * gimp_vectors_get_image:
 * @vectors_ID: The vectors object.
 *
 * Deprecated: Use gimp_item_get_image() instead.
 *
 * Returns: The vectors image.
 *
 * Since: 2.4
 */
gint32
gimp_vectors_get_image (gint32 vectors_ID)
{
  return gimp_item_get_image (vectors_ID);
}

/**
 * gimp_vectors_get_name:
 * @vectors_ID: The vectors object.
 *
 * Deprecated: Use gimp_item_get_name() instead.
 *
 * Returns: The name of the vectors object.
 *
 * Since: 2.4
 */
gchar *
gimp_vectors_get_name (gint32 vectors_ID)
{
  return gimp_item_get_name (vectors_ID);
}

/**
 * gimp_vectors_set_name:
 * @vectors_ID: The vectors object.
 * @name: the new name of the path.
 *
 * Deprecated: Use gimp_item_set_name() instead.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.4
 */
gboolean
gimp_vectors_set_name (gint32       vectors_ID,
                       const gchar *name)
{
  return gimp_item_set_name (vectors_ID, name);
}

/**
 * gimp_vectors_get_visible:
 * @vectors_ID: The vectors object.
 *
 * Deprecated: Use gimp_item_get_visible() instead.
 *
 * Returns: TRUE if the path is visible, FALSE otherwise.
 *
 * Since: 2.4
 */
gboolean
gimp_vectors_get_visible (gint32 vectors_ID)
{
  return gimp_item_get_visible (vectors_ID);
}

/**
 * gimp_vectors_set_visible:
 * @vectors_ID: The vectors object.
 * @visible: Whether the path is visible.
 *
 * Deprecated: Use gimp_item_set_visible() instead.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.4
 */
gboolean
gimp_vectors_set_visible (gint32   vectors_ID,
                          gboolean visible)
{
  return gimp_item_set_visible (vectors_ID, visible);
}

/**
 * gimp_vectors_get_linked:
 * @vectors_ID: The vectors object.
 *
 * Deprecated: Use gimp_item_get_linked() instead.
 *
 * Returns: TRUE if the path is linked, FALSE otherwise.
 *
 * Since: 2.4
 */
gboolean
gimp_vectors_get_linked (gint32 vectors_ID)
{
  return gimp_item_get_linked (vectors_ID);
}

/**
 * gimp_vectors_set_linked:
 * @vectors_ID: The vectors object.
 * @linked: Whether the path is linked.
 *
 * Deprecated: Use gimp_item_set_linked() instead.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.4
 */
gboolean
gimp_vectors_set_linked (gint32   vectors_ID,
                         gboolean linked)
{
  return gimp_item_set_linked (vectors_ID, linked);
}

/**
 * gimp_vectors_get_tattoo:
 * @vectors_ID: The vectors object.
 *
 * Deprecated: Use gimp_item_get_tattoo() instead.
 *
 * Returns: The vectors tattoo.
 *
 * Since: 2.4
 */
gint
gimp_vectors_get_tattoo (gint32 vectors_ID)
{
  return gimp_item_get_tattoo (vectors_ID);
}

/**
 * gimp_vectors_set_tattoo:
 * @vectors_ID: The vectors object.
 * @tattoo: the new tattoo.
 *
 * Deprecated: Use gimp_item_set_tattoo() instead.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.4
 */
gboolean
gimp_vectors_set_tattoo (gint32 vectors_ID,
                         gint   tattoo)
{
  return gimp_item_set_tattoo (vectors_ID, tattoo);
}

/**
 * gimp_vectors_parasite_find:
 * @vectors_ID: The vectors object.
 * @name: The name of the parasite to find.
 *
 * Deprecated: Use gimp_item_get_parasite() instead.
 *
 * Returns: The found parasite.
 *
 * Since: 2.4
 **/
GimpParasite *
gimp_vectors_parasite_find (gint32       vectors_ID,
                            const gchar *name)
{
  return gimp_item_get_parasite (vectors_ID, name);
}

/**
 * gimp_vectors_parasite_attach:
 * @vectors_ID: The vectors object.
 * @parasite: The parasite to attach to a vectors object.
 *
 * Deprecated: Use gimp_item_attach_parasite() instead.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.4
 **/
gboolean
gimp_vectors_parasite_attach (gint32              vectors_ID,
                              const GimpParasite *parasite)
{
  return gimp_item_attach_parasite (vectors_ID, parasite);
}

/**
 * gimp_vectors_parasite_detach:
 * @vectors_ID: The vectors object.
 * @name: The name of the parasite to detach from a vectors object.
 *
 * Deprecated: Use gimp_item_detach_parasite() instead.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.4
 **/
gboolean
gimp_vectors_parasite_detach (gint32       vectors_ID,
                              const gchar *name)
{
  return gimp_item_detach_parasite (vectors_ID, name);
}

/**
 * gimp_vectors_parasite_list:
 * @vectors_ID: The vectors object.
 * @num_parasites: The number of attached parasites.
 * @parasites: The names of currently attached parasites.
 *
 * Deprecated: Use gimp_item_get_parasite_list() instead.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.4
 **/
gboolean
gimp_vectors_parasite_list (gint32    vectors_ID,
                            gint     *num_parasites,
                            gchar  ***parasites)
{
  *parasites = gimp_item_get_parasite_list (vectors_ID, num_parasites);

  return *parasites != NULL;
}
