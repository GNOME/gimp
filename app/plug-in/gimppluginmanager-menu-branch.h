/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginmanager-menu-branch.h
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

#pragma once


struct _GimpPlugInMenuBranch
{
  GFile *file;
  gchar *menu_path;
  gchar *menu_label;
};


void     gimp_plug_in_manager_menu_branch_exit  (GimpPlugInManager *manager);

/* Add a menu branch */
void     gimp_plug_in_manager_add_menu_branch   (GimpPlugInManager *manager,
                                                 GFile             *file,
                                                 const gchar       *menu_path,
                                                 const gchar       *menu_label);
GSList * gimp_plug_in_manager_get_menu_branches (GimpPlugInManager *manager);
