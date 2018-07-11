/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginmanager-menu-branch.c
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

#include <gio/gio.h>

#include "plug-in-types.h"

#include "gimppluginmanager.h"
#include "gimppluginmanager-menu-branch.h"
#include "plug-in-menu-path.h"


/*  public functions  */

void
gimp_plug_in_manager_menu_branch_exit (GimpPlugInManager *manager)
{
  GSList *list;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));

  for (list = manager->menu_branches; list; list = list->next)
    {
      GimpPlugInMenuBranch *branch = list->data;

      g_object_unref (branch->file);
      g_free (branch->menu_path);
      g_free (branch->menu_label);
      g_slice_free (GimpPlugInMenuBranch, branch);
    }

  g_slist_free (manager->menu_branches);
  manager->menu_branches = NULL;
}

void
gimp_plug_in_manager_add_menu_branch (GimpPlugInManager *manager,
                                      GFile             *file,
                                      const gchar       *menu_path,
                                      const gchar       *menu_label)
{
  GimpPlugInMenuBranch *branch;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (G_IS_FILE (file));
  g_return_if_fail (menu_path != NULL);
  g_return_if_fail (menu_label != NULL);

  branch = g_slice_new (GimpPlugInMenuBranch);

  branch->file       = g_object_ref (file);
  branch->menu_path  = plug_in_menu_path_map (menu_path, menu_label);
  branch->menu_label = g_strdup (menu_label);

  manager->menu_branches = g_slist_append (manager->menu_branches, branch);

  g_signal_emit_by_name (manager, "menu-branch-added",
                         branch->file,
                         branch->menu_path,
                         branch->menu_label);

#ifdef VERBOSE
  g_print ("added menu branch \"%s\" at path \"%s\"\n",
           branch->menu_label, branch->menu_path);
#endif
}

GSList *
gimp_plug_in_manager_get_menu_branches (GimpPlugInManager *manager)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);

  return manager->menu_branches;
}
