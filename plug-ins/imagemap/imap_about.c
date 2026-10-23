/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
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
 *
 */

#include "config.h"

#include <gtk/gtk.h>

#include "imap_about.h"
#include "imap_main.h"

#include "libgimp/stdplugins-intl.h"

void
do_about_dialog (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
   static GtkWidget *dialog;
   if (!dialog)
     {
       const gchar* authors[] = {"Maurits Rijk (m.rijk@chello.nl)", NULL};

       dialog = g_object_new (GTK_TYPE_ABOUT_DIALOG,
                              "transient-for", get_dialog(),
                              "program-name",  _("Image Map Plug-in"),
                              "version", "2.3",
                              "authors", authors,
                              "copyright",
                              _("Copyright © 1999-2005 by Maurits Rijk"),
                              "license",
                              _("Released under the GNU General Public License"),
                              NULL);

       g_signal_connect (dialog, "response",
                         G_CALLBACK (gtk_widget_destroy),
                         dialog);

       g_signal_connect (dialog, "destroy",
                         G_CALLBACK (gtk_widget_destroyed),
                         &dialog);

#ifdef PLATFORM_OSX
       /* since `dialog` is not created with gimp_dialog_new, drop minimize manually */
       gtk_widget_realize (dialog);
       gdk_window_set_functions (gtk_widget_get_window (dialog),
                                 GDK_FUNC_RESIZE |
                                 GDK_FUNC_MOVE   |
                                 GDK_FUNC_CLOSE  |
                                 GDK_FUNC_MAXIMIZE);
#endif
     }

   gtk_window_present (GTK_WINDOW (dialog));
}
