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

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimpcontainer.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpsessioninfo.h"

#include "display/gimpdisplay.h"

#include "dialogs/dialogs.h"

#include "windows-commands.h"


/*  public functions  */

void
windows_show_toolbox_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  windows_show_toolbox ();
}

void
windows_show_display_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpDisplay *display = g_object_get_data (G_OBJECT (action), "display");

  gtk_window_present (GTK_WINDOW (display->shell));
}

void
windows_show_dock_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  GtkWindow *dock = g_object_get_data (G_OBJECT (action), "dock");

  gtk_window_present (dock);
}

void
windows_open_recent_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpSessionInfo *info = g_object_get_data (G_OBJECT (action), "info");

  g_object_ref (info);
  gimp_container_remove (global_recent_docks, GIMP_OBJECT (info));

  global_dock_factory->session_infos =
    g_list_append (global_dock_factory->session_infos, info);

  gimp_session_info_restore (info, global_dock_factory);
  gimp_session_info_clear_info (info);
}

void
windows_show_toolbox (void)
{
  if (! global_toolbox_factory->open_dialogs)
    {
      GtkWidget *toolbox;

      toolbox = gimp_dialog_factory_dock_new (global_toolbox_factory,
                                              gdk_screen_get_default ());

      gtk_widget_show (toolbox);
    }
  else
    {
      gimp_dialog_factory_show_toolbox (global_toolbox_factory);
    }
}
