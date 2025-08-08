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

#include <string.h>
#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpthumb/gimpthumb.h"

#include "menus-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"
#include "core/gimpviewable.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpactionimpl.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockwindow.h"
#include "widgets/gimpsessioninfo.h"
#include "widgets/gimpuimanager.h"

#include "display/gimpdisplay.h"

#include "dialogs/dialogs.h"

#include "actions/windows-actions.h"

#include "windows-menu.h"


static void      windows_menu_display_add                (GimpContainer     *container,
                                                          GimpDisplay       *display,
                                                          GimpUIManager     *manager);
static void      windows_menu_display_remove             (GimpContainer     *container,
                                                          GimpDisplay       *display,
                                                          GimpUIManager     *manager);
static void      windows_menu_display_reorder            (GimpContainer     *container,
                                                          GimpDisplay       *display,
                                                          gint               old_index,
                                                          gint               new_index,
                                                          GimpUIManager     *manager);
static void      windows_menu_image_notify               (GimpDisplay       *display,
                                                          const GParamSpec  *unused,
                                                          GimpUIManager     *manager);
static void      windows_menu_dock_window_added          (GimpDialogFactory *factory,
                                                          GimpDockWindow    *dock_window,
                                                          GimpUIManager     *manager);
static void      windows_menu_dock_window_removed        (GimpDialogFactory *factory,
                                                          GimpDockWindow    *dock_window,
                                                          GimpUIManager     *manager);
static void      windows_menu_recent_add                 (GimpContainer     *container,
                                                          GimpSessionInfo   *info,
                                                          GimpUIManager     *manager);
static void      windows_menu_recent_remove              (GimpContainer     *container,
                                                          GimpSessionInfo   *info,
                                                          GimpUIManager     *manager);


void
windows_menu_setup (GimpUIManager *manager,
                    const gchar   *ui_path)
{
  GList *list;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);

  g_object_set_data (G_OBJECT (manager), "image-menu-ui-path",
                     (gpointer) ui_path);

  g_signal_connect_object (manager->gimp->displays, "add",
                           G_CALLBACK (windows_menu_display_add),
                           manager, 0);
  g_signal_connect_object (manager->gimp->displays, "remove",
                           G_CALLBACK (windows_menu_display_remove),
                           manager, 0);
  g_signal_connect_object (manager->gimp->displays, "reorder",
                           G_CALLBACK (windows_menu_display_reorder),
                           manager, 0);

  for (list = gimp_get_display_iter (manager->gimp);
       list;
       list = g_list_next (list))
    {
      GimpDisplay *display = list->data;

      windows_menu_display_add (manager->gimp->displays, display, manager);
    }

  g_signal_connect_object (gimp_dialog_factory_get_singleton (), "dock-window-added",
                           G_CALLBACK (windows_menu_dock_window_added),
                           manager, 0);
  g_signal_connect_object (gimp_dialog_factory_get_singleton (), "dock-window-removed",
                           G_CALLBACK (windows_menu_dock_window_removed),
                           manager, 0);

  for (list = gimp_dialog_factory_get_open_dialogs (gimp_dialog_factory_get_singleton ());
       list;
       list = g_list_next (list))
    {
      GimpDockWindow *dock_window = list->data;

      if (GIMP_IS_DOCK_WINDOW (dock_window))
        windows_menu_dock_window_added (gimp_dialog_factory_get_singleton (),
                                        dock_window,
                                        manager);
    }

  g_signal_connect_object (global_recent_docks, "add",
                           G_CALLBACK (windows_menu_recent_add),
                           manager, 0);
  g_signal_connect_object (global_recent_docks, "remove",
                           G_CALLBACK (windows_menu_recent_remove),
                           manager, 0);

  for (list = g_list_last (GIMP_LIST (global_recent_docks)->queue->head);
       list;
       list = g_list_previous (list))
    {
      GimpSessionInfo *info = list->data;

      windows_menu_recent_add (global_recent_docks, info, manager);
    }
}


/*  private functions  */

static void
windows_menu_display_add (GimpContainer *container,
                          GimpDisplay   *display,
                          GimpUIManager *manager)
{
  g_signal_connect_object (display, "notify::image",
                           G_CALLBACK (windows_menu_image_notify),
                           manager, 0);

  if (gimp_display_get_image (display))
    windows_menu_image_notify (display, NULL, manager);
}

static void
windows_menu_display_remove (GimpContainer *container,
                             GimpDisplay   *display,
                             GimpUIManager *manager)
{
  gchar *action_name;

  action_name = gimp_display_get_action_name (display);
  gimp_ui_manager_remove_ui (manager, action_name);

  g_free (action_name);
}

static void
windows_menu_display_reorder (GimpContainer *container,
                              GimpDisplay   *display,
                              gint           old_index,
                              gint           new_index,
                              GimpUIManager *manager)
{
  gint n_display = gimp_container_get_n_children (container);
  gint i;

  for (i = new_index; i < n_display; i++)
    {
      GimpObject *d = gimp_container_get_child_by_index (container, i);

      windows_menu_display_remove (container, GIMP_DISPLAY (d), manager);
    }

  for (i = new_index; i < n_display; i++)
    {
      GimpObject *d = gimp_container_get_child_by_index (container, i);

      windows_menu_display_add (container, GIMP_DISPLAY (d), manager);
    }
}

static void
windows_menu_image_notify (GimpDisplay      *display,
                           const GParamSpec *unused,
                           GimpUIManager    *manager)
{
  windows_menu_display_remove (manager->gimp->displays, display, manager);

  if (gimp_display_get_image (display))
    {
      gchar *action_name;

      action_name = gimp_display_get_action_name (display);

      gimp_ui_manager_add_ui (manager, "/Windows/[Images]", action_name, FALSE);

      g_free (action_name);
    }
}

static void
windows_menu_dock_window_added (GimpDialogFactory *factory,
                                GimpDockWindow    *dock_window,
                                GimpUIManager     *manager)
{
  gchar *action_name;

  action_name = windows_actions_dock_window_to_action_name (dock_window);

  /* TODO GMenu: doesn't look like it's working, neither will old or new API. */
  gimp_ui_manager_add_ui (manager, "/Windows/[Docks]", action_name, FALSE);

  g_free (action_name);
}

static void
windows_menu_dock_window_removed (GimpDialogFactory *factory,
                                  GimpDockWindow    *dock_window,
                                  GimpUIManager     *manager)
{
  gchar *action_name;

  action_name = windows_actions_dock_window_to_action_name (dock_window);
  gimp_ui_manager_remove_ui (manager, action_name);

  g_free (action_name);
}

static void
windows_menu_recent_add (GimpContainer   *container,
                         GimpSessionInfo *info,
                         GimpUIManager   *manager)
{
  gchar *action_name;
  gint   info_id;

  info_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (info), "recent-action-id"));

  action_name = g_strdup_printf ("windows-recent-%04d", info_id);

  gimp_ui_manager_add_ui (manager, "/Windows/Recently Closed Docks", action_name, TRUE);

  g_free (action_name);
}

static void
windows_menu_recent_remove (GimpContainer   *container,
                            GimpSessionInfo *info,
                            GimpUIManager   *manager)
{
  gchar *action_name;
  gint   info_id;

  info_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (info), "recent-action-id"));

  action_name = g_strdup_printf ("windows-recent-%04d", info_id);

  gimp_ui_manager_remove_uis (manager, action_name);

  g_free (action_name);
}
