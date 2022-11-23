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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmathumb/ligmathumb.h"

#include "menus-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmaviewable.h"

#include "widgets/ligmaaction.h"
#include "widgets/ligmaactionimpl.h"
#include "widgets/ligmauimanager.h"

#include "file-menu.h"


static gboolean file_menu_open_recent_query_tooltip (GtkWidget  *widget,
                                                     gint        x,
                                                     gint        y,
                                                     gboolean    keyboard_mode,
                                                     GtkTooltip *tooltip,
                                                     LigmaAction *action);


void
file_menu_setup (LigmaUIManager *manager,
                 const gchar   *ui_path)
{
  gint  n_entries;
  guint merge_id;
  gint  i;

  g_return_if_fail (LIGMA_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);

  n_entries = LIGMA_GUI_CONFIG (manager->ligma->config)->last_opened_size;

  merge_id = ligma_ui_manager_new_merge_id (manager);

  for (i = 0; i < n_entries; i++)
    {
      GtkWidget *widget;
      gchar     *action_name;
      gchar     *action_path;
      gchar     *full_path;

      action_name = g_strdup_printf ("file-open-recent-%02d", i + 1);
      action_path = g_strdup_printf ("%s/File/Open Recent/Files", ui_path);

      ligma_ui_manager_add_ui (manager, merge_id,
                              action_path, action_name, action_name,
                              GTK_UI_MANAGER_MENUITEM,
                              FALSE);

      full_path = g_strconcat (action_path, "/", action_name, NULL);

      widget = ligma_ui_manager_get_widget (manager, full_path);

      if (widget)
        {
          LigmaAction *action;

          action = ligma_ui_manager_find_action (manager, "file", action_name);

          g_signal_connect_object (widget, "query-tooltip",
                                   G_CALLBACK (file_menu_open_recent_query_tooltip),
                                   action, 0);
        }

      g_free (action_name);
      g_free (action_path);
      g_free (full_path);
    }
}

static gboolean
file_menu_open_recent_query_tooltip (GtkWidget  *widget,
                                     gint        x,
                                     gint        y,
                                     gboolean    keyboard_mode,
                                     GtkTooltip *tooltip,
                                     LigmaAction *action)
{
  LigmaActionImpl *impl = LIGMA_ACTION_IMPL (action);
  gchar          *text;

  text = gtk_widget_get_tooltip_text (widget);
  gtk_tooltip_set_text (tooltip, text);
  g_free (text);

  gtk_tooltip_set_icon (tooltip,
                        ligma_viewable_get_pixbuf (impl->viewable,
                                                  impl->context,
                                                  LIGMA_THUMB_SIZE_NORMAL,
                                                  LIGMA_THUMB_SIZE_NORMAL));

  return TRUE;
}
