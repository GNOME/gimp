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
#include <gegl-plugin.h>
#include <gtk/gtk.h>

#include "menus-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "core/gimp.h"
#include "core/gimp-filter-history.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpstringaction.h"
#include "widgets/gimpuimanager.h"

#include "filters-menu.h"


/*  public functions  */

void
filters_menu_setup (GimpUIManager *manager,
                    const gchar   *ui_path)
{
  GimpActionGroup  *group;
  gchar           **gegl_actions;
  gint              i;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);

  for (i = 0; i < gimp_filter_history_size (manager->gimp); i++)
    {
      gchar *action_name;

      action_name = g_strdup_printf ("filters-recent-%02d", i + 1);

      gimp_ui_manager_add_ui (manager, "/Filters/Recently Used/[Filters]",
                              action_name, FALSE);

      g_free (action_name);
    }

  group        = gimp_ui_manager_get_action_group (manager, "filters");
  gegl_actions = g_object_get_data (G_OBJECT (group), "filters-group-generated-gegl-actions");

  g_return_if_fail (gegl_actions != NULL);

  for (i = 0; i < g_strv_length (gegl_actions); i++)
    {
      GimpAction  *action;
      gchar       *path;
      gchar       *root;
      const gchar *op_name;

      action  = gimp_action_group_get_action (group, gegl_actions[i]);
      op_name = (const gchar *) GIMP_STRING_ACTION (action)->value;
      path    = (gchar *) gegl_operation_get_key (op_name, "gimp:menu-path");

      if (path == NULL)
        continue;

      path = g_strdup (path);
      root = strstr (path, "/");

      if (root == NULL || root == path)
        {
          g_printerr ("GEGL operation \"%s\" attempted to register a menu item "
                      "with an invalid value for key \"gimp:menu-path\": \"%s\"\n"
                      "Expected format is \"<MenuName>/menu/submenu.\n",
                      gegl_actions[i], path);
        }
      else
        {
          GList *managers;

          *root    = '\0';
          managers = gimp_ui_managers_from_name (path);

          if (managers == NULL)
            {
              g_printerr ("GEGL operation \"%s\" attempted to register an item in "
                          "the invalid menu \"%s\": use either \"<Image>\", "
                          "\"<Layers>\", \"<Channels>\", \"<Paths>\", "
                          "\"<Colormap>\", \"<Brushes>\", \"<Dynamics>\", "
                          "\"<MyPaintBrushes>\", \"<Gradients>\", \"<Palettes>\", "
                          "\"<Patterns>\", \"<ToolPresets>\", \"<Fonts>\" "
                          "\"<Buffers>\" or \"<QuickMask>\".\n",
                          gegl_actions[i], path);
            }
          else
            {
              *root = '/';

              for (GList *m = managers; m; m = m->next)
                gimp_ui_manager_add_ui (m->data, root, gegl_actions[i], FALSE);
            }
        }
      g_free (path);
    }
}
