/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
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

#ifndef _IMAP_TOOLS_H
#define _IMAP_TOOLS_H

#include "imap_command.h"

typedef struct {
   GtkWidget *container;
   GtkWidget *arrow;
   GtkWidget *rectangle;
   GtkWidget *circle;
   GtkWidget *polygon;
   GtkWidget *edit;
   GtkWidget *delete;

   CommandFactory_t cmd_delete;
   CommandFactory_t cmd_edit;
} Tools_t;

Tools_t *make_tools(GtkWidget *window);
void tools_select_arrow(void);
void tools_select_rectangle(void);
void tools_select_circle(void);
void tools_select_polygon(void);
void tools_set_sensitive(gboolean sensitive);

void arrow_on_button_press(GtkWidget *widget, GdkEventButton *event, 
			   gpointer data);

#define tools_set_delete_command(tools, command) \
	((tools)->cmd_delete = (command))
#define tools_set_edit_command(tools, command) \
	((tools)->cmd_edit = (command))

#endif /* _IMAP_TOOLS_H */
