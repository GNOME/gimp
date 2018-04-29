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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpdisplayconfig.h"
#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpsessioninfo.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "dialogs-actions.h"
#include "windows-commands.h"

#include "gimp-intl.h"


void
windows_hide_docks_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  Gimp     *gimp;
  gboolean  active;
  return_if_no_gimp (gimp, data);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (active != GIMP_GUI_CONFIG (gimp->config)->hide_docks)
    g_object_set (gimp->config,
                  "hide-docks", active,
                  NULL);
}

void
windows_set_tabs_position_cmd_callback (GtkAction *action,
                                        GtkAction *current,
                                        gpointer   data)
{
  Gimp         *gimp;
  GimpPosition  value;
  return_if_no_gimp (gimp, data);

  value = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  if (value != GIMP_GUI_CONFIG (gimp->config)->tabs_position)
    g_object_set (gimp->config,
                  "tabs-position", value,
                  NULL);
}

void
windows_use_single_window_mode_cmd_callback (GtkAction *action,
                                             gpointer   data)
{
  Gimp     *gimp;
  gboolean  active;
  return_if_no_gimp (gimp, data);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (active != GIMP_GUI_CONFIG (gimp->config)->single_window_mode)
    g_object_set (gimp->config,
                  "single-window-mode", active,
                  NULL);
}

void
windows_show_display_next_cmd_callback (GtkAction *action,
                                        gpointer   data)
{
  GimpDisplay *display;
  Gimp        *gimp;
  gint         index;
  return_if_no_display (display, data);
  return_if_no_gimp (gimp, data);

  index = gimp_container_get_child_index (gimp->displays,
                                          GIMP_OBJECT (display));
  index++;

  if (index >= gimp_container_get_n_children (gimp->displays))
    index = 0;

  display = GIMP_DISPLAY (gimp_container_get_child_by_index (gimp->displays,
                                                             index));
  gimp_display_shell_present (gimp_display_get_shell (display));
}

void
windows_show_display_previous_cmd_callback (GtkAction *action,
                                            gpointer   data)
{
  GimpDisplay *display;
  Gimp        *gimp;
  gint         index;
  return_if_no_display (display, data);
  return_if_no_gimp (gimp, data);

  index = gimp_container_get_child_index (gimp->displays,
                                          GIMP_OBJECT (display));
  index--;

  if (index < 0)
    index = gimp_container_get_n_children (gimp->displays) - 1;

  display = GIMP_DISPLAY (gimp_container_get_child_by_index (gimp->displays,
                                                             index));
  gimp_display_shell_present (gimp_display_get_shell (display));
}

void
windows_show_display_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpDisplay *display = g_object_get_data (G_OBJECT (action), "display");

  gimp_display_shell_present (gimp_display_get_shell (display));
}

void
windows_show_dock_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  GtkWindow *dock_window = g_object_get_data (G_OBJECT (action), "dock-window");

  gtk_window_present (dock_window);
}

void
windows_open_recent_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpSessionInfo        *info;
  GimpDialogFactoryEntry *entry;
  Gimp                   *gimp;
  GtkWidget              *widget;
  return_if_no_gimp (gimp, data);
  return_if_no_widget (widget, data);

  info  = g_object_get_data (G_OBJECT (action), "info");
  entry = gimp_session_info_get_factory_entry (info);

  if (entry && strcmp ("gimp-toolbox-window", entry->identifier) == 0 &&
      dialogs_actions_toolbox_exists (gimp))
    {
      gimp_message (gimp,
                    G_OBJECT (action_data_get_widget (data)),
                    GIMP_MESSAGE_WARNING,
                    _("The chosen recent dock contains a toolbox. Please "
                      "close the currently open toolbox and try again."));
      return;
    }

  g_object_ref (info);

  gimp_container_remove (global_recent_docks, GIMP_OBJECT (info));

  gimp_dialog_factory_add_session_info (gimp_dialog_factory_get_singleton (),
                                        info);

  gimp_session_info_restore (info, gimp_dialog_factory_get_singleton (),
                             gimp_widget_get_monitor (widget));

  g_object_unref (info);
}
