/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gtk/gtk.h>

#include "menus-types.h"

#include "core/gimp.h"

#include "plug-in/plug-in-proc.h"

#include "widgets/gimpuimanager.h"

#include "file-save-menu.h"


void
file_save_menu_setup (GimpUIManager *manager,
                      const gchar   *ui_path)
{
  GSList *list;
  guint   merge_id;

  merge_id = gtk_ui_manager_new_merge_id (GTK_UI_MANAGER (manager));

  for (list = manager->gimp->save_procs; list; list = g_slist_next (list))
    {
      PlugInProcDef *file_proc = list->data;
      gchar         *path;

      if (! strcmp (file_proc->db_info.name, "gimp_xcf_save"))
        path = g_strdup_printf ("%s/%s", ui_path, "Internal");
      else
        path = g_strdup (ui_path);

      gtk_ui_manager_add_ui (GTK_UI_MANAGER (manager), merge_id,
                             path,
                             file_proc->db_info.name,
                             file_proc->db_info.name,
                             GTK_UI_MANAGER_MENUITEM,
                             FALSE);

      g_free (path);
    }
}
