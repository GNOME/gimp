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

/*  externed variables  */
extern GtkWidget   * tool_widgets[];
extern GtkTooltips * tool_tips;

/*  function declarations  */
GtkWidget *  create_pixmap_widget   (GdkWindow    *parent,
				     gchar       **data,
				     gint          width,
				     gint          height);

GdkPixmap *  create_tool_pixmap     (GtkWidget    *parent,
				     ToolType      type);

void         create_toolbox         (void);
void	     toolbox_free           (void);

void         toolbox_raise_callback (GtkWidget    *widget,
				     gpointer      client_data);

void         create_display_shell   (GDisplay     *gdisp,
				     gint          width,
				     gint          height,
				     gchar        *title,
				     gint          type);

#endif /* __INTERFACE_H__ */
