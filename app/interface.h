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

#include "toolsF.h"
#include "gdisplayF.h"
#include "libgimp/gimpunit.h"

/* typedefs */
typedef void (*QueryFunc) (GtkWidget *, gpointer, gpointer);

/* externed variables  */
extern GtkWidget *tool_widgets[];
extern GtkWidget *popup_shell;
extern GtkTooltips *tool_tips;

/* function declarations */
GtkWidget *  create_pixmap_widget   (GdkWindow   *parent,
				     char       **data,
				     int          width,
				     int          height);

GdkPixmap *  create_tool_pixmap     (GtkWidget   *parent,
				     ToolType     type);

void         create_toolbox         (void);
void	     toolbox_free           (void);

void         toolbox_raise_callback (GtkWidget   *widget,
				     gpointer     client_data);

void         create_display_shell   (GDisplay    *gdisp,
				     int          width,
				     int          height,
				     char        *title,
				     int          type);

/* commented out because these functions are not in interface.c
 * is this a bug or did I miss something?? -- michael
 * void         position_dialog        (GtkWidget *, gpointer, gpointer);
 * void         center_dialog          (GtkWidget *, gpointer, gpointer);
 */

/* some simple query dialogs
 * if object != NULL then the query boxes will connect to the "destroy"
 * signal of this object
 */
GtkWidget *  query_string_box       (char        *title,
				     char        *message,
				     char        *initial,
				     GtkObject   *object,
				     QueryFunc    callback,
				     gpointer     data);
GtkWidget *  query_int_box          (char        *title,
				     char        *message,
				     int          initial,
				     int          lower,
				     int          upper,
				     GtkObject   *object,
				     QueryFunc    callback,
				     gpointer     data);
GtkWidget *  query_float_box        (char        *title,
				     char        *message,
				     float        initial,
				     float        lower,
				     float        upper,
				     int          digits,
				     GtkObject   *object,
				     QueryFunc    callback,
				     gpointer     data);
GtkWidget *  query_size_box         (char        *title,
				     char        *message,
				     float        initial,
				     float        lower,
				     float        upper,
				     int          digits,
				     GUnit        unit,
				     float        resolution,
				     GtkObject   *object,
				     QueryFunc    callback,
				     gpointer     data);

/* a simple message box */
GtkWidget *  message_box            (char        *message,
				     GtkCallback  callback,
				     gpointer     data);

void tools_push_label               (char *label);
void tools_pop_label                (void);

#endif /* INTERFACE_H */
