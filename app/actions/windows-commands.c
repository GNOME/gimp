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

#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "config/ligmadisplayconfig.h"
#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmasessioninfo.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "dialogs-actions.h"
#include "windows-commands.h"

#include "ligma-intl.h"


void
windows_hide_docks_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  Ligma     *ligma;
  gboolean  active;
  return_if_no_ligma (ligma, data);

  active = g_variant_get_boolean (value);

  if (active != LIGMA_GUI_CONFIG (ligma->config)->hide_docks)
    g_object_set (ligma->config,
                  "hide-docks", active,
                  NULL);
}

void
windows_use_single_window_mode_cmd_callback (LigmaAction *action,
                                             GVariant   *value,
                                             gpointer    data)
{
  Ligma     *ligma;
  gboolean  active;
  return_if_no_ligma (ligma, data);

  active = g_variant_get_boolean (value);

  if (active != LIGMA_GUI_CONFIG (ligma->config)->single_window_mode)
    g_object_set (ligma->config,
                  "single-window-mode", active,
                  NULL);
}

void
windows_show_tabs_cmd_callback (LigmaAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  Ligma     *ligma;
  gboolean  active;
  return_if_no_ligma (ligma, data);

  active = g_variant_get_boolean (value);

  if (active != LIGMA_GUI_CONFIG (ligma->config)->show_tabs)
    g_object_set (ligma->config,
                  "show-tabs", active,
                  NULL);
}


void
windows_set_tabs_position_cmd_callback (LigmaAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  Ligma         *ligma;
  LigmaPosition  position;
  return_if_no_ligma (ligma, data);

  position = (LigmaPosition) g_variant_get_int32 (value);

  if (position != LIGMA_GUI_CONFIG (ligma->config)->tabs_position)
    g_object_set (ligma->config,
                  "tabs-position", position,
                  NULL);
}

void
windows_show_display_next_cmd_callback (LigmaAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  LigmaDisplay *display;
  Ligma        *ligma;
  gint         index;
  return_if_no_display (display, data);
  return_if_no_ligma (ligma, data);

  index = ligma_container_get_child_index (ligma->displays,
                                          LIGMA_OBJECT (display));
  index++;

  if (index >= ligma_container_get_n_children (ligma->displays))
    index = 0;

  display = LIGMA_DISPLAY (ligma_container_get_child_by_index (ligma->displays,
                                                             index));
  ligma_display_shell_present (ligma_display_get_shell (display));
}

void
windows_show_display_previous_cmd_callback (LigmaAction *action,
                                            GVariant   *value,
                                            gpointer    data)
{
  LigmaDisplay *display;
  Ligma        *ligma;
  gint         index;
  return_if_no_display (display, data);
  return_if_no_ligma (ligma, data);

  index = ligma_container_get_child_index (ligma->displays,
                                          LIGMA_OBJECT (display));
  index--;

  if (index < 0)
    index = ligma_container_get_n_children (ligma->displays) - 1;

  display = LIGMA_DISPLAY (ligma_container_get_child_by_index (ligma->displays,
                                                             index));
  ligma_display_shell_present (ligma_display_get_shell (display));
}

void
windows_show_display_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaDisplay *display = g_object_get_data (G_OBJECT (action), "display");

  ligma_display_shell_present (ligma_display_get_shell (display));
}

void
windows_show_dock_cmd_callback (LigmaAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GtkWindow *dock_window = g_object_get_data (G_OBJECT (action), "dock-window");

  gtk_window_present (dock_window);
}

void
windows_open_recent_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaSessionInfo        *info;
  LigmaDialogFactoryEntry *entry;
  Ligma                   *ligma;
  GtkWidget              *widget;
  return_if_no_ligma (ligma, data);
  return_if_no_widget (widget, data);

  info  = g_object_get_data (G_OBJECT (action), "info");
  entry = ligma_session_info_get_factory_entry (info);

  if (entry && strcmp ("ligma-toolbox-window", entry->identifier) == 0 &&
      dialogs_actions_toolbox_exists (ligma))
    {
      ligma_message (ligma,
                    G_OBJECT (action_data_get_widget (data)),
                    LIGMA_MESSAGE_WARNING,
                    _("The chosen recent dock contains a toolbox. Please "
                      "close the currently open toolbox and try again."));
      return;
    }

  g_object_ref (info);

  ligma_container_remove (global_recent_docks, LIGMA_OBJECT (info));

  ligma_dialog_factory_add_session_info (ligma_dialog_factory_get_singleton (),
                                        info);

  ligma_session_info_restore (info, ligma_dialog_factory_get_singleton (),
                             ligma_widget_get_monitor (widget));

  g_object_unref (info);
}
