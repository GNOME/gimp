/* The GIMP -- an image manipulation program
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
#ifndef __INTERFACE_H__
#define __INTERFACE_H__

/* typedefs */
typedef void (*QueryFunc) (GtkWidget *, gpointer, gpointer);

/* externed variables  */
extern GtkWidget *tool_widgets[];
extern GtkWidget *popup_shell;
extern GtkTooltips *tool_tips;

/* function declarations */
GtkWidget *  create_pixmap_widget (GdkWindow *, char **, int, int);
void         create_toolbox (void);
void	     toolbox_free (void);
void         toolbox_raise_callback (GtkWidget *, gpointer);
void         create_display_shell (int, int, int, char *, int);
void         position_dialog (GtkWidget *, gpointer, gpointer);
void         center_dialog (GtkWidget *, gpointer, gpointer);
GtkWidget *  query_string_box (char *, char *, char *, QueryFunc, gpointer);
GtkWidget *  message_box (char *, GtkCallback, gpointer);

void tools_push_label (char *label);
void tools_pop_label (void);

void progress_start (void);
void progress_update (float);
void progress_step (void);
void progress_end (void);

#endif /* INTERFACE_H */
