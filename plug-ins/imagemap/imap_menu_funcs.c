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

#include "imap_command.h"
#include "imap_menu_funcs.h"

static GtkAccelGroup *accelerator_group;

void
init_accel_group(GtkWidget *window)
{
   accelerator_group = gtk_accel_group_new();
   gtk_window_add_accel_group(GTK_WINDOW(window), accelerator_group);
}

void
add_accelerator(GtkWidget *widget, guint accelerator_key,
                guint8 accelerator_mods)
{
   gtk_widget_add_accelerator(widget, "activate", accelerator_group,
                              accelerator_key, accelerator_mods,
                              GTK_ACCEL_VISIBLE);
}

GtkWidget*
insert_item_with_label (GtkWidget   *parent,
                        gint         position,
                        gchar       *label,
                        MenuCallback activate,
                        gpointer     data)
{
   GtkWidget *item = gtk_menu_item_new_with_mnemonic (label);
   gtk_menu_shell_insert (GTK_MENU_SHELL (parent), item, position);
   g_signal_connect (item, "activate", G_CALLBACK (activate), data);
   gtk_widget_show (item);

   return item;
}
