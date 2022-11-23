/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmathumb/ligmathumb.h"

#include "menus-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmaimage.h"
#include "core/ligmalist.h"
#include "core/ligmaviewable.h"

#include "widgets/ligmaaction.h"
#include "widgets/ligmaactionimpl.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadock.h"
#include "widgets/ligmadockwindow.h"
#include "widgets/ligmasessioninfo.h"
#include "widgets/ligmauimanager.h"

#include "display/ligmadisplay.h"

#include "dialogs/dialogs.h"

#include "actions/windows-actions.h"

#include "windows-menu.h"


static void      windows_menu_display_add                (LigmaContainer     *container,
                                                          LigmaDisplay       *display,
                                                          LigmaUIManager     *manager);
static void      windows_menu_display_remove             (LigmaContainer     *container,
                                                          LigmaDisplay       *display,
                                                          LigmaUIManager     *manager);
static void      windows_menu_display_reorder            (LigmaContainer     *container,
                                                          LigmaDisplay       *display,
                                                          gint               new_index,
                                                          LigmaUIManager     *manager);
static void      windows_menu_image_notify               (LigmaDisplay       *display,
                                                          const GParamSpec  *unused,
                                                          LigmaUIManager     *manager);
static void      windows_menu_dock_window_added          (LigmaDialogFactory *factory,
                                                          LigmaDockWindow    *dock_window,
                                                          LigmaUIManager     *manager);
static void      windows_menu_dock_window_removed        (LigmaDialogFactory *factory,
                                                          LigmaDockWindow    *dock_window,
                                                          LigmaUIManager     *manager);
static gchar   * windows_menu_dock_window_to_merge_id    (LigmaDockWindow    *dock_window);
static void      windows_menu_recent_add                 (LigmaContainer     *container,
                                                          LigmaSessionInfo   *info,
                                                          LigmaUIManager     *manager);
static void      windows_menu_recent_remove              (LigmaContainer     *container,
                                                          LigmaSessionInfo   *info,
                                                          LigmaUIManager     *manager);
static gboolean  windows_menu_display_query_tooltip      (GtkWidget         *widget,
                                                          gint               x,
                                                          gint               y,
                                                          gboolean           keyboard_mode,
                                                          GtkTooltip        *tooltip,
                                                          LigmaAction        *action);


void
windows_menu_setup (LigmaUIManager *manager,
                    const gchar   *ui_path)
{
  GList *list;

  g_return_if_fail (LIGMA_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);

  g_object_set_data (G_OBJECT (manager), "image-menu-ui-path",
                     (gpointer) ui_path);

  g_signal_connect_object (manager->ligma->displays, "add",
                           G_CALLBACK (windows_menu_display_add),
                           manager, 0);
  g_signal_connect_object (manager->ligma->displays, "remove",
                           G_CALLBACK (windows_menu_display_remove),
                           manager, 0);
  g_signal_connect_object (manager->ligma->displays, "reorder",
                           G_CALLBACK (windows_menu_display_reorder),
                           manager, 0);

  for (list = ligma_get_display_iter (manager->ligma);
       list;
       list = g_list_next (list))
    {
      LigmaDisplay *display = list->data;

      windows_menu_display_add (manager->ligma->displays, display, manager);
    }

  g_signal_connect_object (ligma_dialog_factory_get_singleton (), "dock-window-added",
                           G_CALLBACK (windows_menu_dock_window_added),
                           manager, 0);
  g_signal_connect_object (ligma_dialog_factory_get_singleton (), "dock-window-removed",
                           G_CALLBACK (windows_menu_dock_window_removed),
                           manager, 0);

  for (list = ligma_dialog_factory_get_open_dialogs (ligma_dialog_factory_get_singleton ());
       list;
       list = g_list_next (list))
    {
      LigmaDockWindow *dock_window = list->data;

      if (LIGMA_IS_DOCK_WINDOW (dock_window))
        windows_menu_dock_window_added (ligma_dialog_factory_get_singleton (),
                                        dock_window,
                                        manager);
    }

  g_signal_connect_object (global_recent_docks, "add",
                           G_CALLBACK (windows_menu_recent_add),
                           manager, 0);
  g_signal_connect_object (global_recent_docks, "remove",
                           G_CALLBACK (windows_menu_recent_remove),
                           manager, 0);

  for (list = g_list_last (LIGMA_LIST (global_recent_docks)->queue->head);
       list;
       list = g_list_previous (list))
    {
      LigmaSessionInfo *info = list->data;

      windows_menu_recent_add (global_recent_docks, info, manager);
    }
}


/*  private functions  */

static void
windows_menu_display_add (LigmaContainer *container,
                          LigmaDisplay   *display,
                          LigmaUIManager *manager)
{
  g_signal_connect_object (display, "notify::image",
                           G_CALLBACK (windows_menu_image_notify),
                           manager, 0);

  if (ligma_display_get_image (display))
    windows_menu_image_notify (display, NULL, manager);
}

static void
windows_menu_display_remove (LigmaContainer *container,
                             LigmaDisplay   *display,
                             LigmaUIManager *manager)
{
  gchar *merge_key = g_strdup_printf ("windows-display-%04d-merge-id",
                                      ligma_display_get_id (display));
  guint  merge_id;

  merge_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (manager),
                                                  merge_key));

  if (merge_id)
    ligma_ui_manager_remove_ui (manager, merge_id);

  g_object_set_data (G_OBJECT (manager), merge_key, NULL);

  g_free (merge_key);
}

static void
windows_menu_display_reorder (LigmaContainer *container,
                              LigmaDisplay   *display,
                              gint           new_index,
                              LigmaUIManager *manager)
{
  gint n_display = ligma_container_get_n_children (container);
  gint i;

  for (i = new_index; i < n_display; i++)
    {
      LigmaObject *d = ligma_container_get_child_by_index (container, i);

      windows_menu_display_remove (container, LIGMA_DISPLAY (d), manager);
    }

  /* If I don't ensure the menu items are effectively removed, adding
   * the same ones may simply cancel the effect of the removal, hence
   * losing the menu reordering.
   */
  ligma_ui_manager_ensure_update (manager);

  for (i = new_index; i < n_display; i++)
    {
      LigmaObject *d = ligma_container_get_child_by_index (container, i);

      windows_menu_display_add (container, LIGMA_DISPLAY (d), manager);
    }
}

static void
windows_menu_image_notify (LigmaDisplay      *display,
                           const GParamSpec *unused,
                           LigmaUIManager    *manager)
{
  if (ligma_display_get_image (display))
    {
      gchar *merge_key = g_strdup_printf ("windows-display-%04d-merge-id",
                                          ligma_display_get_id (display));
      guint  merge_id;

      merge_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (manager),
                                                      merge_key));

      if (! merge_id)
        {
          GtkWidget   *widget;
          const gchar *ui_path;
          gchar       *action_name;
          gchar       *action_path;
          gchar       *full_path;

          ui_path = g_object_get_data (G_OBJECT (manager),
                                       "image-menu-ui-path");

          action_name = ligma_display_get_action_name (display);
          action_path = g_strdup_printf ("%s/Windows/Images", ui_path);

          merge_id = ligma_ui_manager_new_merge_id (manager);

          g_object_set_data (G_OBJECT (manager), merge_key,
                             GUINT_TO_POINTER (merge_id));

          ligma_ui_manager_add_ui (manager, merge_id,
                                  action_path, action_name, action_name,
                                  GTK_UI_MANAGER_MENUITEM,
                                  FALSE);

          full_path = g_strconcat (action_path, "/", action_name, NULL);

          widget = ligma_ui_manager_get_widget (manager, full_path);

          if (widget)
            {
              LigmaAction *action;

              action = ligma_ui_manager_find_action (manager,
                                                    "windows", action_name);

              g_signal_connect_object (widget, "query-tooltip",
                                       G_CALLBACK (windows_menu_display_query_tooltip),
                                       action, 0);
            }

          g_free (action_name);
          g_free (action_path);
          g_free (full_path);
        }

      g_free (merge_key);
    }
  else
    {
      windows_menu_display_remove (manager->ligma->displays, display, manager);
    }
}

static void
windows_menu_dock_window_added (LigmaDialogFactory *factory,
                                LigmaDockWindow    *dock_window,
                                LigmaUIManager     *manager)
{
  const gchar *ui_path;
  gchar       *action_name;
  gchar       *action_path;
  gchar       *merge_key;
  guint        merge_id;

  ui_path = g_object_get_data (G_OBJECT (manager), "image-menu-ui-path");

  action_name = windows_actions_dock_window_to_action_name (dock_window);
  action_path = g_strdup_printf ("%s/Windows/Docks",
                                 ui_path);

  merge_key = windows_menu_dock_window_to_merge_id (dock_window);
  merge_id  = ligma_ui_manager_new_merge_id (manager);

  g_object_set_data (G_OBJECT (manager), merge_key,
                     GUINT_TO_POINTER (merge_id));

  ligma_ui_manager_add_ui (manager, merge_id,
                          action_path, action_name, action_name,
                          GTK_UI_MANAGER_MENUITEM,
                          FALSE);

  g_free (merge_key);
  g_free (action_path);
  g_free (action_name);
}

static void
windows_menu_dock_window_removed (LigmaDialogFactory *factory,
                                  LigmaDockWindow    *dock_window,
                                  LigmaUIManager     *manager)
{
  gchar *merge_key = windows_menu_dock_window_to_merge_id (dock_window);
  guint  merge_id  = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (manager),
                                                          merge_key));
  if (merge_id)
    ligma_ui_manager_remove_ui (manager, merge_id);

  g_object_set_data (G_OBJECT (manager), merge_key, NULL);

  g_free (merge_key);
}

static gchar *
windows_menu_dock_window_to_merge_id (LigmaDockWindow *dock_window)
{
  return g_strdup_printf ("windows-dock-%04d-merge-id",
                          ligma_dock_window_get_id (dock_window));
}

static void
windows_menu_recent_add (LigmaContainer   *container,
                         LigmaSessionInfo *info,
                         LigmaUIManager   *manager)
{
  const gchar *ui_path;
  gchar       *action_name;
  gchar       *action_path;
  gint         info_id;
  gchar       *merge_key;
  guint        merge_id;

  ui_path = g_object_get_data (G_OBJECT (manager), "image-menu-ui-path");

  info_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (info),
                                                "recent-action-id"));

  action_name = g_strdup_printf ("windows-recent-%04d", info_id);
  action_path = g_strdup_printf ("%s/Windows/Recently Closed Docks", ui_path);

  merge_key = g_strdup_printf ("windows-recent-%04d-merge-id", info_id);
  merge_id = ligma_ui_manager_new_merge_id (manager);

  g_object_set_data (G_OBJECT (manager), merge_key,
                     GUINT_TO_POINTER (merge_id));

  ligma_ui_manager_add_ui (manager, merge_id,
                          action_path, action_name, action_name,
                          GTK_UI_MANAGER_MENUITEM,
                          TRUE);

  g_free (merge_key);
  g_free (action_path);
  g_free (action_name);
}

static void
windows_menu_recent_remove (LigmaContainer   *container,
                            LigmaSessionInfo *info,
                            LigmaUIManager   *manager)
{
  gint   info_id;
  gchar *merge_key;
  guint  merge_id;

  info_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (info),
                                                "recent-action-id"));

  merge_key = g_strdup_printf ("windows-recent-%04d-merge-id", info_id);

  merge_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (manager),
                                                  merge_key));

  if (merge_id)
    ligma_ui_manager_remove_ui (manager, merge_id);

  g_object_set_data (G_OBJECT (manager), merge_key, NULL);

  g_free (merge_key);
}

static gboolean
windows_menu_display_query_tooltip (GtkWidget  *widget,
                                    gint        x,
                                    gint        y,
                                    gboolean    keyboard_mode,
                                    GtkTooltip *tooltip,
                                    LigmaAction *action)
{
  LigmaActionImpl *impl  = LIGMA_ACTION_IMPL (action);
  LigmaImage      *image = LIGMA_IMAGE (impl->viewable);
  gchar          *text;
  gdouble         xres;
  gdouble         yres;
  gint            width;
  gint            height;

  if (! image)
    return FALSE;

  text = gtk_widget_get_tooltip_text (widget);
  gtk_tooltip_set_text (tooltip, text);
  g_free (text);

  ligma_image_get_resolution (image, &xres, &yres);

  ligma_viewable_calc_preview_size (ligma_image_get_width  (image),
                                   ligma_image_get_height (image),
                                   LIGMA_VIEW_SIZE_HUGE, LIGMA_VIEW_SIZE_HUGE,
                                   FALSE, xres, yres,
                                   &width, &height, NULL);

  gtk_tooltip_set_icon (tooltip,
                        ligma_viewable_get_pixbuf (impl->viewable,
                                                  impl->context,
                                                  width, height));

  return TRUE;
}
