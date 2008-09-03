/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2002 Maurits Rijk  lpeek.mrijk@consunet.nl
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

#ifndef _IMAP_TABLE_H
#define _IMAP_TABLE_H

GtkWidget *create_spin_button_in_table(GtkWidget *table, GtkWidget *label,
				       int row, int col,
				       int value, int min, int max);
GtkWidget *create_check_button_in_table(GtkWidget *table, int row, int col,
					const char *text);
GtkWidget *create_radio_button_in_table(GtkWidget *table, GSList *group,
					int row, int col, const char *text);
GtkWidget *create_label_in_table(GtkWidget *table, int row, int col,
				 const char *text);
GtkWidget *create_entry_in_table(GtkWidget *table, GtkWidget *label, int row,
				 int col);

#endif /* _IMAP_TABLE_H */

