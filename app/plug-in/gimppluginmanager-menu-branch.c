/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginmanager-menu-branch.c
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

#include "plug-in-types.h"

#include "gimppluginmanager.h"
#include "gimppluginmanager-menu-branch.h"


/*  public functions  */

void
gimp_plug_in_manager_menu_branch_exit (GimpPlugInManager *manager)
{
  GSList *list;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));

  for (list = manager->menu_branches; list; list = list->next)
    {
      GimpPlugInMenuBranch *branch = list->data;

      g_free (branch->prog_name);
      g_free (branch->menu_path);
      g_free (branch->menu_label);
      g_slice_free (GimpPlugInMenuBranch, branch);
    }

  g_slist_free (manager->menu_branches);
  manager->menu_branches = NULL;
}

void
gimp_plug_in_manager_add_menu_branch (GimpPlugInManager *manager,
                                      const gchar       *prog_name,
                                      const gchar       *menu_path,
                                      const gchar       *menu_label)
{
  GimpPlugInMenuBranch *branch;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (prog_name != NULL);
  g_return_if_fail (menu_path != NULL);
  g_return_if_fail (menu_label != NULL);

  branch = g_slice_new (GimpPlugInMenuBranch);

  branch->prog_name  = g_strdup (prog_name);
  branch->menu_path  = g_strdup (menu_path);
  branch->menu_label = g_strdup (menu_label);

  manager->menu_branches = g_slist_append (manager->menu_branches, branch);

  g_signal_emit_by_name (manager, "menu-branch-added",
                         prog_name, menu_path, menu_label);

#ifdef VERBOSE
  g_print ("added menu branch \"%s\" at path \"%s\"\n",
           branch->menu_label, branch->menu_path);
#endif
}
