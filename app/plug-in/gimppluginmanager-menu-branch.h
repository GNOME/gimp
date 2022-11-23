/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapluginmanager-menu-branch.h
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

#ifndef __LIGMA_PLUG_IN_MANAGER_MENU_BRANCH_H__
#define __LIGMA_PLUG_IN_MANAGER_MENU_BRANCH_H__


struct _LigmaPlugInMenuBranch
{
  GFile *file;
  gchar *menu_path;
  gchar *menu_label;
};


void     ligma_plug_in_manager_menu_branch_exit  (LigmaPlugInManager *manager);

/* Add a menu branch */
void     ligma_plug_in_manager_add_menu_branch   (LigmaPlugInManager *manager,
                                                 GFile             *file,
                                                 const gchar       *menu_path,
                                                 const gchar       *menu_label);
GSList * ligma_plug_in_manager_get_menu_branches (LigmaPlugInManager *manager);


#endif /* __LIGMA_PLUG_IN_MANAGER_MENU_BRANCH_H__ */
