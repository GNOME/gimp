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

#include "config/gimpguiconfig.h"

#include "core/gimp.h"

#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "window-menu.h"


/*  private functions  */

static void   window_menu_display_opened (GdkDisplayManager *disp_manager,
                                          GdkDisplay        *display,
                                          GimpUIManager     *manager);
static void   window_menu_display_closed (GdkDisplay        *display,
                                          gboolean           is_error,
                                          GimpUIManager     *manager);


/*  public functions  */

void
window_menu_setup (GimpUIManager *manager,
                   const gchar   *group_name,
                   const gchar   *ui_path)
{
  GdkDisplayManager *disp_manager = gdk_display_manager_get ();
  GSList            *displays;
  GSList            *list;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);

  g_object_set_data_full (G_OBJECT (manager), "move-to-screen-group-name",
                          g_strdup (group_name),
                          (GDestroyNotify) g_free);
  g_object_set_data_full (G_OBJECT (manager), "move-to-screen-ui-path",
                          g_strdup (ui_path),
                          (GDestroyNotify) g_free);

  displays = gdk_display_manager_list_displays (disp_manager);

  /*  present displays in the order in which they were opened  */
  displays = g_slist_reverse (displays);

  for (list = displays; list; list = g_slist_next (list))
    {
      window_menu_display_opened (disp_manager, list->data, manager);
    }

  g_slist_free (displays);

  g_signal_connect_object (disp_manager, "display-opened",
                           G_CALLBACK (window_menu_display_opened),
                           G_OBJECT (manager), 0);
}


/*  private functions  */

static void
window_menu_display_opened (GdkDisplayManager *disp_manager,
                            GdkDisplay        *display,
                            GimpUIManager     *manager)
{
  const gchar *group_name;
  const gchar *ui_path;
  const gchar *display_name;
  gchar       *action_name;

  group_name = g_object_get_data (G_OBJECT (manager),
                                  "move-to-screen-group-name");
  ui_path    = g_object_get_data (G_OBJECT (manager),
                                  "move-to-screen-ui-path");

  display_name = gdk_display_get_name (display);
  if (! display_name)
    display_name = "eek";

  action_name = g_strdup_printf ("%s-move-to-screen-%s", group_name, display_name);
  gimp_make_valid_action_name (action_name);

  /* TODO GMenu: there is also a case with the dockable menu, which is not yet
   * using our new GAction/GMenu-based code. In such case, the ui_path is
   * "/dockable-popup/Move to Screen".
   */
  if (g_str_has_prefix (ui_path, "/image-menubar/"))
    gimp_ui_manager_add_ui (manager, "/View/Move to Screen", action_name, FALSE);

  g_free (action_name);

  g_signal_connect_object (display, "closed",
                           G_CALLBACK (window_menu_display_closed),
                           G_OBJECT (manager), 0);
}

static void
window_menu_display_closed (GdkDisplay    *display,
                            gboolean       is_error,
                            GimpUIManager *manager)
{
  const gchar *group_name;
  const gchar *display_name;
  gchar       *action_name;

  group_name = g_object_get_data (G_OBJECT (manager), "move-to-screen-group-name");
  display_name = gdk_display_get_name (display);
  if (! display_name)
    display_name = "eek";

  action_name = g_strdup_printf ("%s-move-to-screen-%s", group_name, display_name);
  gimp_ui_manager_remove_ui (manager, action_name);

  g_free (action_name);
}
