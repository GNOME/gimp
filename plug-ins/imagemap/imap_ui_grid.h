/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2002 Maurits Rijk  lpeek.mrijk@consunet.nl
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
 *
 */

#ifndef _IMAP_UI_GRID_H
#define _IMAP_UI_GRID_H

GtkWidget *   create_spin_button_in_grid  (GtkWidget  *grid,
                                           GtkWidget  *label,
                                           gint        row,
                                           gint        col,
                                           gint        value,
                                           gint        min,
                                           gint        max);
GtkWidget *   create_check_button_in_grid (GtkWidget  *grid,
                                           gint        row,
                                           gint        col,
                                           const char *text);
GtkWidget *   create_radio_button_in_grid (GtkWidget  *grid,
                                           GSList     *group,
                                           gint        row,
                                           gint        col,
                                           const char *text);
GtkWidget *   create_label_in_grid        (GtkWidget  *grid,
                                           gint        row,
                                           gint        col,
                                           const char *text);
GtkWidget *   create_entry_in_grid        (GtkWidget  *grid,
                                           GtkWidget  *label,
                                           gint        row,
                                           gint        col);

#endif /* _IMAP_UI_GRID_H */

