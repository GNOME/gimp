/* GIMP - The GNU Image Manipulation Program
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

#include <gtk/gtk.h>

#include "menus-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"

#include "widgets/gimpuimanager.h"

#include "file-menu.h"


void
file_menu_setup (GimpUIManager *manager,
                 const gchar   *ui_path)
{
  GtkUIManager *ui_manager;
  gint          n_entries;
  guint         merge_id;
  gint          i;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);

  ui_manager = GTK_UI_MANAGER (manager);

  n_entries = GIMP_GUI_CONFIG (manager->gimp->config)->last_opened_size;

  merge_id = gtk_ui_manager_new_merge_id (ui_manager);

  for (i = 0; i < n_entries; i++)
    {
      gchar *action_name;
      gchar *action_path;

      action_name = g_strdup_printf ("file-open-recent-%02d", i + 1);
      action_path = g_strdup_printf ("%s/File/Open Recent/Files", ui_path);

      gtk_ui_manager_add_ui (ui_manager, merge_id,
                             action_path, action_name, action_name,
                             GTK_UI_MANAGER_MENUITEM,
                             FALSE);

      g_free (action_name);
      g_free (action_path);
    }
}
