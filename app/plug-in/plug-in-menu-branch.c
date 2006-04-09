/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-menu-branch.c
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

#include "core/gimp.h"

#include "plug-in-menu-branch.h"


/*  public functions  */

void
plug_in_menu_branch_exit (Gimp *gimp)
{
  GSList *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  for (list = gimp->plug_in_menu_branches; list; list = list->next)
    {
      PlugInMenuBranch *branch = list->data;

      g_free (branch->prog_name);
      g_free (branch->menu_path);
      g_free (branch->menu_label);
      g_free (branch);
    }

  g_slist_free (gimp->plug_in_menu_branches);
  gimp->plug_in_menu_branches = NULL;
}

void
plug_in_menu_branch_add (Gimp        *gimp,
                         const gchar *prog_name,
                         const gchar *menu_path,
                         const gchar *menu_label)
{
  PlugInMenuBranch *branch;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (prog_name != NULL);
  g_return_if_fail (menu_path != NULL);
  g_return_if_fail (menu_label != NULL);

  if (! gimp->no_interface)
    gimp_menus_create_branch (gimp, prog_name, menu_path, menu_label);

  branch = g_new (PlugInMenuBranch, 1);

  branch->prog_name  = g_strdup (prog_name);
  branch->menu_path  = g_strdup (menu_path);
  branch->menu_label = g_strdup (menu_label);

  gimp->plug_in_menu_branches = g_slist_append (gimp->plug_in_menu_branches,
                                                branch);

#ifdef VERBOSE
  g_print ("added menu branch \"%s\" at path \"%s\"\n",
           branch->menu_label, branch->menu_path);
#endif
}
