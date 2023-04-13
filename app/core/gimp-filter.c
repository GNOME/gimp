/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
 *
 * gimp-filter.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimp-filter.h"

#include "pdb/gimpprocedure.h"


/*  local function prototypes  */

static gint   gimp_filter_history_compare (GimpProcedure *proc1,
                                           GimpProcedure *proc2);


/*  public functions  */

gint
gimp_filter_history_size (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), 0);

  return MAX (1, gimp->config->filter_history_size);
}

gint
gimp_filter_history_length (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), 0);

  return g_list_length (gimp->filter_history);
}

GimpProcedure *
gimp_filter_history_nth (Gimp  *gimp,
                         gint   n)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_list_nth_data (gimp->filter_history, n);
}

void
gimp_filter_history_add (Gimp          *gimp,
                         GimpProcedure *procedure)
{
  GList *link;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  /* return early if the procedure is already at the top */
  if (gimp->filter_history &&
      gimp_filter_history_compare (gimp->filter_history->data, procedure) == 0)
    return;

  /* ref new first then unref old, they might be the same */
  g_object_ref (procedure);

  link = g_list_find_custom (gimp->filter_history, procedure,
                             (GCompareFunc) gimp_filter_history_compare);

  if (link)
    {
      g_object_unref (link->data);
      gimp->filter_history = g_list_delete_link (gimp->filter_history, link);
    }

  gimp->filter_history = g_list_prepend (gimp->filter_history, procedure);

  link = g_list_nth (gimp->filter_history, gimp_filter_history_size (gimp));

  if (link)
    {
      g_object_unref (link->data);
      gimp->filter_history = g_list_delete_link (gimp->filter_history, link);
    }

  gimp_filter_history_changed (gimp);
}

void
gimp_filter_history_remove (Gimp          *gimp,
                            GimpProcedure *procedure)
{
  GList *link;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  link = g_list_find_custom (gimp->filter_history, procedure,
                             (GCompareFunc) gimp_filter_history_compare);

  if (link)
    {
      g_object_unref (link->data);
      gimp->filter_history = g_list_delete_link (gimp->filter_history, link);

      gimp_filter_history_changed (gimp);
    }
}

void
gimp_filter_history_clear (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->filter_history)
    {
      g_list_free_full (gimp->filter_history, (GDestroyNotify) g_object_unref);
      gimp->filter_history = NULL;

      gimp_filter_history_changed (gimp);
    }
}

void
gimp_filter_gegl_ops_add (Gimp        *gimp,
                          const gchar *action_name,
                          const gchar *op_name)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (action_name != NULL);
  g_return_if_fail (op_name != NULL);

  g_hash_table_replace (gimp->filter_gegl_ops, g_strdup (action_name), g_strdup (op_name));
}

const gchar *
gimp_filter_gegl_ops_get (Gimp        *gimp,
                          const gchar *action_name)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (action_name != NULL, NULL);

  return g_hash_table_lookup (gimp->filter_gegl_ops, action_name);
}

GList *
gimp_filter_gegl_ops_list (Gimp *gimp)
{
  GList *actions;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  actions = g_hash_table_get_keys (gimp->filter_gegl_ops);

  return g_list_sort (actions, (GCompareFunc) g_strcmp0);
}


/*  private functions  */

static gint
gimp_filter_history_compare (GimpProcedure *proc1,
                             GimpProcedure *proc2)
{
  /*  the procedures can have the same name, but could still be two
   *  different filters using the same operation, so also compare
   *  their menu labels
   */
  return (gimp_procedure_name_compare (proc1, proc2) ||
          strcmp (gimp_procedure_get_menu_label (proc1),
                  gimp_procedure_get_menu_label (proc2)));
}
