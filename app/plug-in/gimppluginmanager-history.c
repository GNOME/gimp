/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
 *
 * gimppluginmanager-history.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "plug-in-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"

#include "gimppluginmanager.h"
#include "gimppluginmanager-history.h"
#include "gimppluginprocedure.h"


guint
gimp_plug_in_manager_history_size (GimpPlugInManager *manager)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), 0);

  return MAX (1, manager->gimp->config->plug_in_history_size);
}

guint
gimp_plug_in_manager_history_length (GimpPlugInManager *manager)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), 0);

  return g_slist_length (manager->history);
}

GimpPlugInProcedure *
gimp_plug_in_manager_history_nth (GimpPlugInManager *manager,
                                      guint              n)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);

  return g_slist_nth_data (manager->history, n);
}

void
gimp_plug_in_manager_history_add (GimpPlugInManager   *manager,
                                  GimpPlugInProcedure *procedure)
{
  GSList *list;
  gint    history_size;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (procedure));

  /* return early if the procedure is already at the top */
  if (manager->history && manager->history->data == procedure)
    return;

  history_size = gimp_plug_in_manager_history_size (manager);

  manager->history = g_slist_remove (manager->history, procedure);
  manager->history = g_slist_prepend (manager->history, procedure);

  list = g_slist_nth (manager->history, history_size);

  if (list)
    {
      manager->history = g_slist_remove_link (manager->history, list);
      g_slist_free (list);
    }

  gimp_plug_in_manager_history_changed (manager);
}

void
gimp_plug_in_manager_history_remove (GimpPlugInManager   *manager,
                                     GimpPlugInProcedure *procedure)
{
  GSList *link;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (procedure));

  link = g_slist_find (manager->history, procedure);

  if (link)
    {
      manager->history = g_slist_delete_link (manager->history, link);

      gimp_plug_in_manager_history_changed (manager);
    }
}

void
gimp_plug_in_manager_history_clear (GimpPlugInManager   *manager)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));

  if (manager->history)
    {
      g_slist_free (manager->history);
      manager->history = NULL;

      gimp_plug_in_manager_history_changed (manager);
    }
}


