/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmacontainer-filter.c
 * Copyright (C) 2003  Sven Neumann <sven@ligma.org>
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

#include "ligmacontainer.h"
#include "ligmacontainer-filter.h"
#include "ligmalist.h"


typedef struct
{
  LigmaObjectFilterFunc   filter;
  LigmaContainer         *container;
  gpointer               user_data;
} LigmaContainerFilterContext;


static void
ligma_container_filter_foreach_func (LigmaObject                 *object,
                                    LigmaContainerFilterContext *context)
{
  if (context->filter (object, context->user_data))
    ligma_container_add (context->container, object);
}

/**
 * ligma_container_filter:
 * @container: a #LigmaContainer to filter
 * @filter: a #LigmaObjectFilterFunc
 * @user_data: a pointer passed to @filter
 *
 * Calls the supplied @filter function on each object in @container.
 * A return value of %TRUE is interpreted as a match.
 *
 * Returns: a weak #LigmaContainer filled with matching objects.
 **/
LigmaContainer *
ligma_container_filter (LigmaContainer        *container,
                       LigmaObjectFilterFunc  filter,
                       gpointer              user_data)
{
  LigmaContainer              *result;
  LigmaContainerFilterContext  context;

  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (filter != NULL, NULL);

  result =
    g_object_new (G_TYPE_FROM_INSTANCE (container),
                  "children-type", ligma_container_get_children_type (container),
                  "policy",        LIGMA_CONTAINER_POLICY_WEAK,
                  NULL);

  context.filter    = filter;
  context.container = result;
  context.user_data = user_data;

  ligma_container_foreach (container,
                          (GFunc) ligma_container_filter_foreach_func,
                          &context);

  /*  This is somewhat ugly, but it keeps lists in the same order.  */
  if (LIGMA_IS_LIST (result))
    ligma_list_reverse (LIGMA_LIST (result));


  return result;
}


static gboolean
ligma_object_filter_by_name (LigmaObject   *object,
                            const GRegex *regex)
{
  return g_regex_match (regex, ligma_object_get_name (object), 0, NULL);
}

/**
 * ligma_container_filter_by_name:
 * @container: a #LigmaContainer to filter
 * @regexp: a regular expression (as a %NULL-terminated string)
 * @error: error location to report errors or %NULL
 *
 * This function performs a case-insensitive regular expression search
 * on the names of the LigmaObjects in @container.
 *
 * Returns: a weak #LigmaContainer filled with matching objects.
 **/
LigmaContainer *
ligma_container_filter_by_name (LigmaContainer  *container,
                               const gchar    *regexp,
                               GError        **error)
{
  LigmaContainer *result;
  GRegex        *regex;

  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (regexp != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  regex = g_regex_new (regexp, G_REGEX_CASELESS | G_REGEX_OPTIMIZE, 0,
                       error);

  if (! regex)
    return NULL;

  result =
    ligma_container_filter (container,
                           (LigmaObjectFilterFunc) ligma_object_filter_by_name,
                           regex);

  g_regex_unref (regex);

  return result;
}


gchar **
ligma_container_get_filtered_name_array (LigmaContainer *container,
                                        const gchar   *regexp)
{
  LigmaContainer *weak;
  GError        *error = NULL;

  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);

  if (regexp == NULL || strlen (regexp) == 0)
    return (ligma_container_get_name_array (container));

  weak = ligma_container_filter_by_name (container, regexp, &error);

  if (weak)
    {
      gchar **retval = ligma_container_get_name_array (weak);

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
