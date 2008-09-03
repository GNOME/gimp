/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
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
 *
 */

#ifndef _IMAP_MENU_FUNCS_H
#define _IMAP_MENU_FUNCS_H

typedef void (*MenuCallback)(GtkWidget *widget, gpointer data);

void init_accel_group(GtkWidget *window);
GtkWidget *insert_item_with_label(GtkWidget *parent, gint position,
				  gchar *label, MenuCallback activate,
				  gpointer data);

void menu_command(GtkWidget *widget, gpointer data);

void add_accelerator(GtkWidget *widget, guint accelerator_key,
		     guint8 accelerator_mods);


#endif /* _IMAP_MENU_FUNCS_H */
