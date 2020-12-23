/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpcontainer-filter.c
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "core-types.h"

#include "gimpcontainer.h"
#include "gimpcontainer-filter.h"
#include "gimplist.h"


typedef struct
{
  GimpObjectFilterFunc   filter;
  GimpContainer         *container;
  gpointer               user_data;
} GimpContainerFilterContext;


static void
gimp_container_filter_foreach_func (GimpObject                 *object,
                                    GimpContainerFilterContext *context)
{
  if (context->filter (object, context->user_data))
    gimp_container_add (context->container, object);
}

/**
 * gimp_container_filter:
 * @container: a #GimpContainer to filter
 * @filter: a #GimpObjectFilterFunc
 * @user_data: a pointer passed to @filter
 *
 * Calls the supplied @filter function on each object in @container.
 * A return value of %TRUE is interpreted as a match.
 *
 * Returns: a weak #GimpContainer filled with matching objects.
 **/
GimpContainer *
gimp_container_filter (GimpContainer        *container,
                       GimpObjectFilterFunc  filter,
                       gpointer              user_data)
{
  GimpContainer              *result;
  GimpContainerFilterContext  context;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (filter != NULL, NULL);

  result =
    g_object_new (G_TYPE_FROM_INSTANCE (container),
                  "children-type", gimp_container_get_children_type (container),
                  "policy",        GIMP_CONTAINER_POLICY_WEAK,
                  NULL);

  context.filter    = filter;
  context.container = result;
  context.user_data = user_data;

  gimp_container_foreach (container,
                          (GFunc) gimp_container_filter_foreach_func,
                          &context);

  /*  This is somewhat ugly, but it keeps lists in the same order.  */
  if (GIMP_IS_LIST (result))
    gimp_list_reverse (GIMP_LIST (result));


  return result;
}


static gboolean
gimp_object_filter_by_name (GimpObject   *object,
                            const GRegex *regex)
{
  return g_regex_match (regex, gimp_object_get_name (object), 0, NULL);
}

/**
 * gimp_container_filter_by_name:
 * @container: a #GimpContainer to filter
 * @regexp: a regular expression (as a %NULL-terminated string)
 * @error: error location to report errors or %NULL
 *
 * This function performs a case-insensitive regular expression search
 * on the names of the GimpObjects in @container.
 *
 * Returns: a weak #GimpContainer filled with matching objects.
 **/
GimpContainer *
gimp_container_filter_by_name (GimpContainer  *container,
                               const gchar    *regexp,
                               GError        **error)
{
  GimpContainer *result;
  GRegex        *regex;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (regexp != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  regex = g_regex_new (regexp, G_REGEX_CASELESS | G_REGEX_OPTIMIZE, 0,
                       error);

  if (! regex)
    return NULL;

  result =
    gimp_container_filter (container,
                           (GimpObjectFilterFunc) gimp_object_filter_by_name,
                           regex);

  g_regex_unref (regex);

  return result;
}


gchar **
gimp_container_get_filtered_name_array (GimpContainer *container,
                                        const gchar   *regexp)
{
  GimpContainer *weak;
  GError        *error = NULL;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);

  if (regexp == NULL || strlen (regexp) == 0)
    return (gimp_container_get_name_array (container));

  weak = gimp_container_filter_by_name (container, regexp, &error);

  if (weak)
    {
      gchar **retval = gimp_container_get_name_array (weak);

      g_object_unref (weak);

      return retval;
    }
  else
    {
      g_warning ("%s", error->message);
      g_error_free (error);

      return NULL;
    }
}
