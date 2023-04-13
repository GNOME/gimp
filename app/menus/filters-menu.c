/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "menus-types.h"

#include "core/gimp.h"
#include "core/gimp-filter.h"

#include "widgets/gimpuimanager.h"

#include "filters-menu.h"


/*  public functions  */

void
filters_menu_setup (GimpUIManager *manager,
                    const gchar   *ui_path)
{
  GList *actions;
  gint   i;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);

  for (i = 0; i < gimp_filter_history_size (manager->gimp); i++)
    {
      gchar *action_name;

      action_name = g_strdup_printf ("filters-recent-%02d", i + 1);

      gimp_ui_manager_add_ui (manager, "/Filters/Recently Used",
                              action_name, "Filters", TRUE);

      g_free (action_name);
    }

  actions = gimp_filter_gegl_ops_list (manager->gimp);
  for (GList *iter = actions; iter; iter = iter->next)
    gimp_ui_manager_add_ui (manager, "/Filters/Generic/GEGL Operations",
                            iter->data, NULL, FALSE);
  g_list_free (actions);
}
